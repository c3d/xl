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
#include "native.h"
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
static Tree_p      received        = nullptr;
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

struct PartialCloneMode
// ----------------------------------------------------------------------------
//   Clone mode where we stop cloning at a specific cutpoint
// ----------------------------------------------------------------------------
{
    Tree_p cutpoint;

    template<typename CloneClass>
    Tree *Clone(Tree *t, CloneClass *clone)
    {
        if (t == cutpoint)
            return new Scope();
        return t->Do(clone);
    }

    template<typename CloneClass>
    Tree *Adjust(Tree * /* from */, Tree *to, CloneClass * /* clone */)
    {
        return to;
    }
};
typedef TreeCloneTemplate<PartialCloneMode> PartialClone;


static Tree_p xl_attach_context(Scope *symbols, Tree *code)
// ----------------------------------------------------------------------------
//   Attach the scope for the given code, stopping at the global context
// ----------------------------------------------------------------------------
{
    // Do a clone of the symbol table up to the main global context
    PartialClone partialClone;
    partialClone.cutpoint = MAIN->context.Symbols();
    Tree_p symbolsToSend = partialClone.Clone(symbols);
    record(remote, "Sending context %t", symbolsToSend);
    return new Prefix(symbolsToSend, code);
}


static Tree *xl_restore_context(Tree *tree)
// ----------------------------------------------------------------------------
//   Restore the special context classes in a symbol table
// ----------------------------------------------------------------------------
//   The serializer does not know about things like Rewrite, Scope, etc.
//   So we reconstruct them after receiving them from the remote side.
{
    switch(tree->Kind())
    {
    case INFIX:
    {
        Infix *infix = (Infix *) tree;
        if (infix->name == "is" || infix->name == ":=")
            return new Rewrite(infix);
        if (infix->name == "\n")
        {
            Tree *left  = xl_restore_context(infix->left);
            Tree *right = xl_restore_context(infix->right);
            Rewrite *payload = left->As<Rewrite>();
            if (!payload)
                record(remote_error, "No payload for Rewrites %t", infix);
            Rewrite *rewrite = right->As<Rewrite>();
            if (rewrite)
                return new Rewrites(payload, rewrite);
            Rewrites *rewrites = right->As<Rewrites>();
            if (rewrites)
                return new Rewrites(payload, rewrites);
        }
        record(remote_error, "Context contains unexpected infix %t", infix);
        return infix;
    }
    case PREFIX:
    {
        Prefix *prefix = (Prefix *) tree;
        Tree *left = xl_restore_context(prefix->left);
        Scope *enclosing = left->As<Scope>();
        if (enclosing)
        {
            if (Prefix *import = prefix->right->AsPrefix())
                return new Scopes(enclosing, import);
            Tree *right = xl_restore_context(prefix->right);
            if (Scope *inner = right->As<Scope>())
                return new Scopes(enclosing, inner);
        }
        record(remote_error, "Context contains unexpected prefix %t", prefix);
        return prefix;
    }
    case BLOCK:
    {
        Block *block = (Block *) tree;
        if (block->IsBraces())
        {
            Tree *child = xl_restore_context(block->child);
            return new Scope(child);
        }
        record(remote_error, "Context contains unexpected block %t", block);
        return block;
    }
    default:
        record(remote_error, "Context contains unexpected tree %t", tree);
        break;
    }

    return tree;
}


static Tree_p xl_merge_context(Scope *environment, Tree_p code)
// ----------------------------------------------------------------------------
//    Merge code we received into the current running context
// ----------------------------------------------------------------------------
//    The serializer is going to lower context-specific classes such as
//    Scope, Rewrite, etc into their base class, but the evaluation should
//    be able to deal with that and simply run through ProcessDeclarations
//    again (e.g. through ProcessScope).
{
    if (code)
    {
        // We receive the symbol table as a prefix, not a closure
        if (Prefix *prefix = code->AsPrefix())
        {
            if (Block *block = prefix->left->AsBlock())
            {
                Tree_p restored = xl_restore_context(block);
                Scope *scope = restored->As<Scope>();
                if (!scope)
                {
                    record(remote_error, "Context %t is invalid", restored);
                    return code;
                }

                // Find the top scope, and re-attach it to the current context
                Scope_p where = scope;
                while (Scope *enclosing = where->Enclosing())
                {
                    if (enclosing->IsEmpty())
                        break;
                    else
                        where = enclosing;
                }
                where->Reparent(environment);

                // Return a closure with that reconstructed scope
                code = new Closure(scope, prefix->right);
            }
        }
    }
    return code;
}



// ============================================================================
//
//    Simple program exchange over TCP/IP
//
// ============================================================================

static int xl_send(Scope *scope, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send the text for the given body to the target host, return open fd
// ----------------------------------------------------------------------------
{
    // Compute port number
    int port = Opt::remotePort;
    size_t found = host.rfind(':');
    if (found != std::string::npos)
    {
        text portText= host.substr(found+1);
        port = atoi(portText.c_str());
        if (!port)
        {
            record(remote_error, "Port %s is invalid, using %d",
                   portText.c_str(), Opt::remotePort);
            port = Opt::remotePort;
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
    code = xl_attach_context(scope, code);

    // Write program to socket
    xl_write_tree(sock, code);

    return sock;
}


int xl_tell(Scope *scope, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send the text for the given body to the target host
// ----------------------------------------------------------------------------
{
    record(remote_tell, "Telling %s: %t", host.c_str(), code);
    int sock = xl_send(scope, host, code);
    if (sock < 0)
        return sock;
    close(sock);
    return 0;
}
NATIVE(xl_tell);


Tree_p xl_ask(Scope *scope, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send code to the target, wait for reply
// ----------------------------------------------------------------------------
{
    record(remote_ask, "Asking %s: %t", host.c_str(), code);
    int sock = xl_send(scope, host, code);
    if (sock < 0)
        return nullptr;

    Tree_p result = xl_read_tree(sock);
    result = xl_merge_context(scope, result);
    record(remote_ask, "Response from %s was %t", host.c_str(), result);

    close(sock);

    return result;
}
NATIVE(xl_ask);


Tree_p xl_invoke(Scope *scope, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send code to the target, wait for multiple replies
// ----------------------------------------------------------------------------
{
    record(remote_invoke, "Invoking %s: %t", host.c_str(), code);
    int sock = xl_send(scope, host, code);
    if (sock < 0)
        return nullptr;

    Tree_p result;
    while (true)
    {
        Tree_p response = xl_read_tree(sock);
        if (response == nullptr)
            break;

        record(remote_invoke, "Response from %s was %t",
               host.c_str(), response);
        response = xl_merge_context(scope, response);
        record(remote_invoke, "After merge, response was %t", response);
        result = xl_evaluate(scope, response);
        record(remote_invoke, "After eval, was %t", result);
    }
    close(sock);

    return result;
}
NATIVE(xl_invoke);



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
NATIVE(xl_listen_received);


Tree_p xl_listen_hook(Tree *newHook)
// ----------------------------------------------------------------------------
//   Set the listen hook, return the previous one
// ----------------------------------------------------------------------------
{
    Tree_p result = hook;
    hook = newHook;
    return result;
}
NATIVE(xl_listen_hook);


Tree_p xl_listen(Scope *scope, uint forking, uint port)
// ----------------------------------------------------------------------------
//    Listen on the given port for sockets, evaluate programs when received
// ----------------------------------------------------------------------------
{
    // Open the socket
    Context context(scope);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return Ooops("Error opening socket: $1").Arg(strerror(errno));

    int option = 1;
    if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&option, sizeof (option)) < 0)
        return Ooops("Error setting SO_REUSEADDR: $1").Arg(strerror(errno));

    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0)
        return Ooops("Error binding to port $1: $2")
            .Arg(port)
            .Arg(strerror(errno));

    // Listen to socket
    listen(sock, 5);

    // Make sure we get notified when a child dies
    signal(SIGCHLD, child_died);

    // Accept client
    listening = true;
    Tree_p hookResult;
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
                hookResult = xl_evaluate(scope, hook);
                if (hookResult)
                {
                    Save<int> saveReply(reply_socket, insock);
                    code = xl_merge_context(scope, code);
                    Tree_p result = xl_evaluate(scope, code);
                    record(remote_listen, "Evaluated as %t", result);
                    if (!result)
                    {
                        result = LastErrorAsErrorTree();
                        if (!result)
                            result = Ooops("Evaluation of $1 failed", code);
                    }
                    xl_write_tree(insock, result);
                    record(remote_listen, "Response sent");
                }
                if (hookResult == xl_false || hookResult == nullptr)
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
    return hookResult;
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
    code = xl_attach_context(scope, code);
    record(remote_reply, "After replacement: %t", code);
    xl_write_tree(reply_socket, code);
    return 0;
}
NATIVE(xl_reply);


XL_END
