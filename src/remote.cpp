// *****************************************************************************
// remote.cpp                                                         XL project
// *****************************************************************************
//
// File description:
//
//     Implementation of a simple socket-based transport for XL programs
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2015-2020, Christophe de Dinechin <christophe@dinechin.org>
// (C) 0,
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

#include "remote.h"
#include "runtime.h"
#include "serializer.h"
#include "main.h"
#include "save.h"
#include "tree-clone.h"
#include "fdstream.hpp"

#include <sys/types.h>
#ifndef HAVE_SYS_SOCKET_H
#include "winsock2.h"
#undef Context
#else // HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif // HAVE_SYS_SOCKET_H
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <string>
#include <sstream>


RECORDER(remote,        64, "Remote context information");
RECORDER(remote_tell,   32, "Evaluating the 'tell' command in remote package");
RECORDER(remote_ask,    32, "Evaluating the 'ask' command in remote package");
RECORDER(remote_invoke, 32, "Evaluating 'invoke' in remote package");
RECORDER(remote_listen, 32, "Evaluating 'listen' in remote package");
RECORDER(remote_reply,  32, "Evaluating 'reply' in remote package");
RECORDER(remote_error,  64, "Errors from the remote package");

XL_BEGIN


// ============================================================================
//
//    Global state (per thread?)
//
// ============================================================================

static uint        active_children = 0;
static int         reply_socket    = 0;
static Tree_p      received        = xl_nil;
static Tree_p      hook            = xl_true;
static bool        listening       = true;



// ============================================================================
//
//   Utilities for the code below
//
// ============================================================================

static Tree *xl_read_tree(int sock)
// ----------------------------------------------------------------------------
//   Read a tree directly from the socket
// ----------------------------------------------------------------------------
{
    int fd = dup(sock);         // stdio_filebuf closes its fd in dtor
    boost::fdistream is(fd);
    return Deserializer::Read(is);
}


static void xl_write_tree(int sock, Tree *tree)
// ----------------------------------------------------------------------------
//   Write a tree directly into the socket
// ----------------------------------------------------------------------------
{
    int fd = dup(sock);         // stdio_filebuf closes its fd in dtor
    boost::fdostream os(fd);
    Serializer::Write(os, tree);
}



// ============================================================================
//
//    Clone the symbol tables that go with a tree
//
// ============================================================================

struct StopAtGlobalsCloneMode
// ----------------------------------------------------------------------------
//   Clone mode where all the children nodes are copied (default)
// ----------------------------------------------------------------------------
{
    Tree_p cutpoint;

    template<typename CloneClass>
    Tree *Clone(Tree *t, CloneClass *clone)
    {
        if (t == cutpoint)
            return xl_nil;
        return t->Do(clone);
    }

    template<typename CloneClass>
    Tree *Adjust(Tree * /* from */, Tree *to, CloneClass * /* clone */)
    {
        return to;
    }
};
typedef TreeCloneTemplate<StopAtGlobalsCloneMode> StopAtGlobalsClone;


static Tree_p xl_attach_context(Context &context, Tree *code)
// ----------------------------------------------------------------------------
//   Attach the scope for the given code
// ----------------------------------------------------------------------------
{
    // Find first enclosing scope containing a "module_path"
    Scope_p globals;
    Rewrite_p rewrite;
    Name *module_path = new Name("module_path", code->Position());
    Tree *found = context.Bound(module_path, true, &rewrite, &globals);

    // Do a clone of the symbol table up to that point
    StopAtGlobalsClone partialClone;
    if (found)
        partialClone.cutpoint = EnclosingScope(globals);
    Scope_p symbols = context.Symbols();
    Tree_p symbolsToSend = partialClone.Clone(symbols);

    record(remote, "Sending context %t", symbolsToSend.Pointer());

    return new Prefix(symbolsToSend, code, code->Position());
}


static Tree *xl_restore_nil(Tree *tree)
// ----------------------------------------------------------------------------
//   Restore 'nil' names in the symbol tables
// ----------------------------------------------------------------------------
{
    if (Name *name = tree->AsName())
    {
        if (name->value == "nil")
            return xl_nil;
    }
    else if (Infix *infix = tree->AsInfix())
    {
        infix->left  = xl_restore_nil(infix->left);
        infix->right = xl_restore_nil(infix->right);
    }
    else if (Prefix *prefix = tree->AsPrefix())
    {
        prefix->left  = xl_restore_nil(prefix->left);
        prefix->right = xl_restore_nil(prefix->right);
    }
    else if (Postfix *postfix = tree->AsPostfix())
    {
        postfix->left  = xl_restore_nil(postfix->left);
        postfix->right = xl_restore_nil(postfix->right);
    }
    else if (Block *block = tree->AsBlock())
    {
        block->child = xl_restore_nil(block->child);
    }
    return tree;
}


static Tree_p xl_merge_context(Context &context, Tree *code)
// ----------------------------------------------------------------------------
//    Merge the code into the current running context
// ----------------------------------------------------------------------------
{
    if (code)
    {
        if (Prefix *prefix = code->AsPrefix())
        {
            Scope *scope = prefix->left->As<Scope>();
            code = prefix->right;

            // Walk up the chain for incoming symbols, stop at end
            Context *codeCtx = context.Pointer();
            if (scope)
            {
                scope = xl_restore_nil(scope)->As<Scope>();
                codeCtx = new Context(scope);
                while (Scope *parent = EnclosingScope(scope))
                    scope = parent;

                // Reattach that end to current scope
                scope->left = context.Symbols();
            }

            // And make the resulting code a closure at that location
            code = Interpreter::MakeClosure(codeCtx, code);
        }
    }

    return code;
}



// ============================================================================
//
//    Simple program exchange over TCP/IP
//
// ============================================================================

static int xl_send(Context &context, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send the text for the given body to the target host, return open fd
// ----------------------------------------------------------------------------
{
    // Compute port number
    int port = XL_DEFAULT_PORT;
    size_t found = host.rfind(':');
    if (found != std::string::npos)
    {
        text portText= host.substr(found+1);
        port = atoi(portText.c_str());
        if (!port)
        {
            record(remote_error, "Port %s is invalid, using %d",
                   portText.c_str(), XL_DEFAULT_PORT);
            port = XL_DEFAULT_PORT;
        }
        host = host.substr(0, found);
    }

    // Open socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        record(remote_error, "Error opening socket: %s (%d)",
               strerror(errno), errno);
        return -1;
    }

    // Resolve server name
    struct hostent *server = gethostbyname(host.c_str());
    if (!server)
    {
        record(remote_error, "Error resolving server %s: %s (%d)",
               host.c_str(), strerror(errno), errno);
        return -1;
    }

    // Initialize socket
    sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    memcpy((char *) &address.sin_addr.s_addr,
           (char *) server->h_addr,
           server->h_length);
    address.sin_port = htons(port);

    // Connect
    if (connect(sock, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        record(remote_error, "Error connecting to %s port %d: %s (%d)",
               host.c_str(), port, strerror(errno), errno);
        return -1;
    }

    // Attach the running context, i.e. all symbols we might need
    code = xl_attach_context(context, code);

    // Write program to socket
    xl_write_tree(sock, code);

    return sock;
}


int xl_tell(Scope *scope, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send the text for the given body to the target host
// ----------------------------------------------------------------------------
{
    Context context(scope);
    record(remote_tell, "Telling %s: %t", host.c_str(), code);
    int sock = xl_send(context, host, code);
    if (sock < 0)
        return sock;
    close(sock);
    return 0;
}


Tree_p xl_ask(Scope *scope, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send code to the target, wait for reply
// ----------------------------------------------------------------------------
{
    Context context(scope);
    record(remote_ask, "Asking %s: %t", host.c_str(), code);
    int sock = xl_send(context, host, code);
    if (sock < 0)
        return xl_nil;

    Tree_p result = xl_read_tree(sock);
    result = xl_merge_context(context, result);
    record(remote_ask, "Response from %s was %t", host.c_str(), result);

    close(sock);

    return result;
}


Tree_p xl_invoke(Scope *scope, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send code to the target, wait for multiple replies
// ----------------------------------------------------------------------------
{
    Context context(scope);
    record(remote_invoke, "Invoking %s: %t", host.c_str(), code);
    int sock = xl_send(context, host, code);
    if (sock < 0)
        return xl_nil;

    Tree_p result = xl_nil;
    while (true)
    {
        Tree_p response = xl_read_tree(sock);
        if (response == nullptr)
            break;

        record(remote_invoke, "Response from %s was %t",
               host.c_str(), response);
        response = xl_merge_context(context, response);
        record(remote_invoke, "After merge, response was %t", response);
        result = xl_evaluate(context.Symbols(), response);
        record(remote_invoke, "After eval, was %t", result);
        if (result == xl_nil)
            break;
    }
    close(sock);

    return result;
}



// ============================================================================
//
//   Listening side
//
// ============================================================================

#ifndef HAVE_SYS_SOCKET_H
#define waitpid(a,b,c)        0
#define WIFEXITED(status)     0
#define WEXITSTATUS(status)   0
#define WNOHANG               0
#define SIGCHLD               0
#define fork()                0
typedef int socklen_t;
#endif // HAVE_SYS_SOCKET_H

static int child_wait(int flag)
// ----------------------------------------------------------------------------
//   Wait for a child to die
// ----------------------------------------------------------------------------
{
    int status = 0;
    int childPID = waitpid(-1, &status, flag);
    if (childPID < 0)
        return childPID;

    if (childPID > 0)
    {
        record(remote_listen, "Child PID %d died %+s status %d",
               childPID, flag ? "nowait" : "wait", status);
        active_children--;
        if (!flag && WIFEXITED(status))
        {
            int rc = WEXITSTATUS(status);
            if (rc == 42)
                listening = false;
        }
    }
    return childPID;
}


static void child_died(int)
// ----------------------------------------------------------------------------
//    When a child dies, get its exit status
// ----------------------------------------------------------------------------
{
    record(remote, "Child died, waiting");
    while (child_wait(WNOHANG) > 0)
        /* Loop */;
    record(remote, "Child died, end of wait");
}



Tree_p  xl_listen_received()
// ----------------------------------------------------------------------------
//    Return the incoming message before evaluation
// ----------------------------------------------------------------------------
{
    return received;
}


Tree_p xl_listen_hook(Tree *newHook)
// ----------------------------------------------------------------------------
//   Set the listen hook, return the previous one
// ----------------------------------------------------------------------------
{
    Tree_p result = hook;
    if (newHook != xl_nil)
        hook = newHook;
    return result;
}


int xl_listen(Scope *scope, uint forking, uint port)
// ----------------------------------------------------------------------------
//    Listen on the given port for sockets, evaluate programs when received
// ----------------------------------------------------------------------------
{
    // Open the socket
    Context context(scope);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "xl_listen: Error opening socket: "
                  << strerror(errno) << "\n";
        return -1;
    }

    int option = 1;
    if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&option, sizeof (option)) < 0)
        std::cerr << "xl_listen: Error setting SO_REUSEADDR: "
                  << strerror(errno) << "\n";

    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        record(remote_error, "Error binding to port %d: %s (%d)",
               port, strerror(errno), errno);
        return -1;
    }

    // Listen to socket
    listen(sock, 5);

    // Make sure we get notified when a child dies
    signal(SIGCHLD, child_died);

    // Accept client
    listening = true;
    while (listening)
    {
        // Block until we can accept more connexions (avoid fork bombs)
        while (forking && active_children >= forking)
        {
            record(remote, "xl_listen: Too many children, waiting");
            int childPID = child_wait(0);
            if (childPID > 0)
                record(remote, "xl_listen: Child %d died, resuming", childPID);
        }

        // Accept input
        record(remote, "xl_listen: Accepting input");
        sockaddr_in client = { 0 };
        socklen_t length = sizeof(client);
        int insock = accept(sock, (struct sockaddr *) &client, &length);
        if (insock < 0)
        {
            std::cerr << "xl_listen: Error accepting port " << port << ": "
                      << strerror(errno) << "\n";
            continue;
        }
        record(remote_listen, "Got incoming connexion");

        // Fork child for incoming connexion
        int pid = forking ? fork() : 0;
        if (pid == -1)
        {
            std::cerr << "xl_listen: Error forking child\n";
        }
        else if (pid)
        {
            record(remote_listen, "Forked pid %d", pid);
            close(insock);
            active_children++;
        }
        else
        {
            // Read data from client
            Tree_p code = xl_read_tree(insock);

            // Evaluate resulting code
            if (code)
            {
                record(remote_listen, "Received code: %t", code);
                received = code;
                Tree_p hookResult = xl_evaluate(scope, hook);
                if (hookResult != xl_nil)
                {
                    Save<int> saveReply(reply_socket, insock);
                    code = xl_merge_context(context, code);
                    Tree_p result = xl_evaluate(scope, code);
                    record(remote_listen, "Evaluated as %t", result);
                    xl_write_tree(insock, result);
                    record(remote_listen, "Response sent");
                }
                if (hookResult == xl_false || hookResult == xl_nil)
                {
                    listening = false;
                }
            }
            close(insock);

            if (forking)
            {
                record(remote_listen, "Exiting PID %d", getpid());
                exit(listening ? 0 : 42);
            }
        }
    }

    close(sock);
    return 0;
}


int xl_reply(Scope *scope, Tree *code)
// ----------------------------------------------------------------------------
//   Send code back to whoever invoked us
// ----------------------------------------------------------------------------
{
    Context context(scope);
    if (!reply_socket)
    {
        record(remote_reply, "Not replying to anybody");
        return -1;
    }

    record(remote_reply, "Replying: %t", code);
    code = xl_attach_context(context, code);
    record(remote_reply, "After replacement: %t", code);
    xl_write_tree(reply_socket, code);
    return 0;
}


XL_END
