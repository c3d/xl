// ****************************************************************************
//  remote.cpp                                                  ELIOT project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of a simple socket-based transport for ELIOT programs
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2015 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2015 Taodyne SAS
// ****************************************************************************

#include "remote.h"
#include "runtime.h"
#include "serializer.h"
#include "main.h"
#include "save.h"
#include "tree-clone.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <string>
#include <sstream>
#include <ext/stdio_filebuf.h>


ELIOT_BEGIN


// ============================================================================
//
//    Global state (per thread?)
//
// ============================================================================

static uint        active_children = 0;
static int         reply_socket    = 0;
static Tree_p      received        = eliot_nil;
static Tree_p      hook            = eliot_true;
static bool        listening       = true;



// ============================================================================
//
//   Utilities for the code below
//
// ============================================================================

static Tree *eliot_read_tree(int sock)
// ----------------------------------------------------------------------------
//   Read a tree directly from the socket
// ----------------------------------------------------------------------------
{
    int fd = dup(sock);         // stdio_filebuf closes its fd in dtor
    __gnu_cxx::stdio_filebuf<char> filebuf(fd, std::ios::in);
    std::istream is(&filebuf);
    return Deserializer::Read(is);
}


static void eliot_write_tree(int sock, Tree *tree)
// ----------------------------------------------------------------------------
//   Write a tree directly into the socket
// ----------------------------------------------------------------------------
{
    int fd = dup(sock);         // stdio_filebuf closes its fd in dtor
    __gnu_cxx::stdio_filebuf<char> filebuf(fd, std::ios::out);
    std::ostream os(&filebuf);
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
            return eliot_nil;
        return t->Do(clone);
    }

    template<typename CloneClass>
    Tree *Adjust(Tree * /* from */, Tree *to, CloneClass */* clone */)
    {
        return to;
    }
};
typedef TreeCloneTemplate<StopAtGlobalsCloneMode> StopAtGlobalsClone;


static Tree_p eliot_attach_context(Context *context, Tree *code)
// ----------------------------------------------------------------------------
//   Attach the context for the given code
// ----------------------------------------------------------------------------
{
    // Find first enclosing scope containing a "module_path"
    Scope_p globals;
    Rewrite_p rewrite;
    Name *module_path = new Name("module_path", code->Position());
    Tree *found = context->Bound(module_path, true, &rewrite, &globals);

    // Do a clone of the symbol table up to that point
    StopAtGlobalsClone partialClone;
    if (found)
        partialClone.cutpoint = ScopeParent(globals);
    Scope_p symbols = context->CurrentScope();
    Tree_p symbolsToSend = partialClone.Clone(symbols);

    IFTRACE(remote)
        std::cerr << "Sending context:\n"
                  << new Context(symbolsToSend->As<Scope>()) << "\n";

    return new Prefix(symbolsToSend, code, code->Position());
}


static Tree *eliot_restore_nil(Tree *tree)
// ----------------------------------------------------------------------------
//   Restore 'nil' names in the symbol tables
// ----------------------------------------------------------------------------
{
    if (Name *name = tree->AsName())
    {
        if (name->value == "nil")
            return eliot_nil;
    }
    else if (Infix *infix = tree->AsInfix())
    {
        infix->left  = eliot_restore_nil(infix->left);
        infix->right = eliot_restore_nil(infix->right);
    }
    else if (Prefix *prefix = tree->AsPrefix())
    {
        prefix->left  = eliot_restore_nil(prefix->left);
        prefix->right = eliot_restore_nil(prefix->right);
    }
    else if (Postfix *postfix = tree->AsPostfix())
    {
        postfix->left  = eliot_restore_nil(postfix->left);
        postfix->right = eliot_restore_nil(postfix->right);
    }
    else if (Block *block = tree->AsBlock())
    {
        block->child = eliot_restore_nil(block->child);
    }
    return tree;
}


static Tree_p eliot_merge_context(Context *context, Tree *code)
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
            Context *codeCtx = context;
            if (scope)
            {
                scope = eliot_restore_nil(scope)->As<Scope>();
                codeCtx = new Context(scope);
                while (Scope *parent = ScopeParent(scope))
                    scope = parent;
                
                // Reattach that end to current scope
                scope->left = context->CurrentScope();
            }
                
            // And make the resulting code a closure at that location
            code = MakeClosure(codeCtx, code);
        }
    }

    return code;
}



// ============================================================================
//
//    Simple program exchange over TCP/IP
//
// ============================================================================

static int eliot_send(Context *context, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send the text for the given body to the target host, return open fd
// ----------------------------------------------------------------------------
{
    // Compute port number
    int port = ELIOT_DEFAULT_PORT;
    size_t found = host.rfind(':');
    if (found != std::string::npos)
    {
        text portText= host.substr(found+1);
        port = atoi(portText.c_str());
        if (!port)
        {
            std::cerr << "eliot_tell: Port '" << portText << " is invalid, "
                      << "using " << ELIOT_DEFAULT_PORT << "\n";
            port = ELIOT_DEFAULT_PORT;
        }
        host = host.substr(0, found);
    }

    // Open socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "eliot_tell: Error opening socket: "
                  << strerror(errno) << "\n";
        return -1;
    }

    // Resolve server name
    struct hostent *server = gethostbyname(host.c_str());
    if (!server)
    {
        std::cerr << "eliot_tell: Error resolving server '" << host << "': "
                  << strerror(errno) << "\n";
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
        std::cerr << "eliot_tell: Error connecting to '"
                  << host << "' port " << port << ": "
                  << strerror(errno) << "\n";
        return -1;
    }

    // Attach the running context, i.e. all symbols we might need
    code = eliot_attach_context(context, code);

    // Write program to socket
    eliot_write_tree(sock, code);

    return sock;
}


int eliot_tell(Context *context, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send the text for the given body to the target host
// ----------------------------------------------------------------------------
{
    IFTRACE(remote)
        std::cerr << "eliot_tell: Telling " << host << ":\n"
                  << code << "\n";
    int sock = eliot_send(context, host, code);
    if (sock < 0)
        return sock;
    close(sock);
    return 0;
}


Tree_p eliot_ask(Context *context, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send code to the target, wait for reply
// ----------------------------------------------------------------------------
{
    IFTRACE(remote)
        std::cerr << "eliot_ask: Asking " << host << ":\n"
                  << code << "\n";
    int sock = eliot_send(context, host, code);
    if (sock < 0)
        return eliot_nil;

    Tree *result = eliot_read_tree(sock);
    result = eliot_merge_context(context, result);
    IFTRACE(remote)
        std::cerr << "eliot_ask: Response from " << host << " was:\n"
                  << result << "\n";
    
    close(sock);

    return result;
}


Tree_p eliot_invoke(Context *context, text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send code to the target, wait for multiple replies
// ----------------------------------------------------------------------------
{
    IFTRACE(remote)
        std::cerr << "eliot_invoke: Invoking " << host << ":\n"
                  << code << "\n";
    int sock = eliot_send(context, host, code);
    if (sock < 0)
        return eliot_nil;

    Tree_p result = eliot_nil;
    while (true)
    {
        Tree *response = eliot_read_tree(sock);
        if (response == NULL)
            break;
        
        IFTRACE(remote)
            std::cerr << "eliot_invoke: Response from " << host << " was:\n"
                      << response << "\n";
        response = eliot_merge_context(context, response);
        result = context->Evaluate(response);
        if (result == eliot_nil)
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
        IFTRACE(remote)
            std::cerr << "eliot_listen: Child PID " << childPID << " died "
                      << (flag ? "nowait" : "wait") << "\n";
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
    while (child_wait(WNOHANG) > 0)
        /* Loop */;
}



Tree_p  eliot_listen_received()
// ----------------------------------------------------------------------------
//    Return the incoming message before evaluation
// ----------------------------------------------------------------------------
{
    return received;
}


Tree_p eliot_listen_hook(Tree *newHook)
// ----------------------------------------------------------------------------
//   Set the listen hook, return the previous one
// ----------------------------------------------------------------------------
{
    Tree_p result = hook;
    if (newHook != eliot_nil)
        hook = newHook;
    return result;
}


int eliot_listen(Context *context, uint forking, uint port)
// ----------------------------------------------------------------------------
//    Listen on the given port for sockets, evaluate programs when received
// ----------------------------------------------------------------------------
{
    // Open the socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "eliot_listen: Error opening socket: "
                  << strerror(errno) << "\n";
        return -1;
    }

    int option = 1;
    if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&option, sizeof (option)) < 0)
        std::cerr << "eliot_listen: Error setting SO_REUSEADDR: "
                  << strerror(errno) << "\n";

    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        std::cerr << "eliot_listen: Error binding to port " << port << ": "
                  << strerror(errno) << "\n";
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
            IFTRACE(remote)
                std::cerr << "eliot_listen: Too many children, waiting\n";
            int childPID = child_wait(0);
            if (childPID > 0)
                IFTRACE(remote)
                    std::cerr << "eliot_listen: Child " << childPID
                              << " died, resuming\n";
        }

        // Accept input
        IFTRACE(remote)
            std::cerr << "eliot_listen: Accepting input\n";
        sockaddr_in client = { 0 };
        socklen_t length = sizeof(client);
        int insock = accept(sock, (struct sockaddr *) &client, &length);
        if (insock < 0)
        {
            std::cerr << "eliot_listen: Error accepting port " << port << ": "
                      << strerror(errno) << "\n";
            continue;
        }
        IFTRACE(remote)
            std::cerr << "eliot_listen: Got incoming connexion\n";

        // Fork child for incoming connexion
        int pid = forking ? fork() : 0;
        if (pid == -1)
        {
            std::cerr << "eliot_listen: Error forking child\n";
        }
        else if (pid)
        {
            IFTRACE(remote)
                std::cerr << "eliot_listen: Forked pid " << pid << "\n";
            close(insock);
            active_children++;
        }
        else
        {
            // Read data from client
            Tree *code = eliot_read_tree(insock);

            // Evaluate resulting code
            if (code)
            {
                IFTRACE(remote)
                    std::cerr << "eliot_listen: Received code: " << code << "\n";
                received = code;
                Tree_p hookResult = context->Evaluate(hook);
                if (hookResult != eliot_nil)
                {
                    Save<int> saveReply(reply_socket, insock);
                    code = eliot_merge_context(context, code);
                    Tree_p result = context->Evaluate(code);
                    IFTRACE(remote)
                        std::cerr << "eliot_listen: Evaluated as: "
                                  << result << "\n";
                    eliot_write_tree(insock, result);
                    IFTRACE(remote)
                        std::cerr << "eliot_listen: Response sent\n";
                }
                if (hookResult == eliot_false || hookResult == eliot_nil)
                {
                    listening = false;
                }
            }
            close(insock);

            if (forking)
            {
                IFTRACE(remote)
                    std::cerr << "eliot_listen: Exiting PID "
                              << getpid() << "\n";
                exit(listening ? 0 : 42);
            }
        }
    }

    close(sock);
    return 0;
}


int eliot_reply(Context *context, Tree *code)
// ----------------------------------------------------------------------------
//   Send code back to whoever invoked us
// ----------------------------------------------------------------------------
{
    if (!reply_socket)
    {
        std::cerr << "eliot_reply: Not replying to anybody\n";
        return -1;
    }

    IFTRACE(remote)
        std::cerr << "eliot_reply: Replying:\n" << code << "\n";
    code = eliot_attach_context(context, code);
    IFTRACE(remote)
        std::cerr << "eliot_reply: After replacement:\n" << code << "\n";
    eliot_write_tree(reply_socket, code);
    return 0;
}


ELIOT_END
