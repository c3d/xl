// ****************************************************************************
//  remote.cpp                                                     Tao project
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2015 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2015 Taodyne SAS
// ****************************************************************************

#include "remote.h"
#include "runtime.h"
#include "serializer.h"
#include "main.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <string>
#include <sstream>


XL_BEGIN

// ============================================================================
//
//    Simple program exchange over TCP/IP
//
// ============================================================================

int xl_tell(text host, Tree *code)
// ----------------------------------------------------------------------------
//   Send the text for the given body to the target host
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
            std::cerr << "xl_tell: Port '" << portText << " is invalid, "
                      << "using " << XL_DEFAULT_PORT << "\n";
            port = XL_DEFAULT_PORT;
        }
        host = host.substr(0, found);
    }

    // Open socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "xl_tell: Error opening socket: "
                  << strerror(errno) << "\n";
        return -1;
    }

    // Resolve server name
    struct hostent *server = gethostbyname(host.c_str());
    if (!server)
    {
        std::cerr << "xl_tell: Error resolving server '" << host << "': "
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
        std::cerr << "xl_tell: Error connecting to '"
                  << host << "' port " << port << ": "
                  << strerror(errno) << "\n";
        return -1;
    }

    // Get program in serialized form
    std::ostringstream out;
    Serializer serialize(out);
    code->Do(serialize);
    text payload = out.str();

    // Write the payload
    uint sent = write(sock, payload.data(), payload.length());
    if (sent < payload.length())
    {
        std::cerr << "xl_tell: Error writing data: "
                  << strerror(errno) << "\n";
        return -1;
    }

    // Success
    close(sock);
    return 0;
}


static int active_children = 0;
static void child_died(int)
// ----------------------------------------------------------------------------
//    When a child dies, get its exit status
// ----------------------------------------------------------------------------
{
    while (int childPID = waitpid(-1, NULL, WNOHANG))
    {
        if (childPID < 0)
            break;
        IFTRACE(remote)
            std::cerr << "xl_listen: Child PID " << childPID << " died\n";
        active_children--;
    }
}


int xl_listen(Context *context, uint port)
// ----------------------------------------------------------------------------
//    Listen on the given port for sockets, evaluate programs when received
// ----------------------------------------------------------------------------
{
    // Open the socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "xl_listen: Error opening socket: "
                  << strerror(errno) << "\n";
        return -1;
    }
    
    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        std::cerr << "xl_listen: Error binding to port " << port << ": "
                  << strerror(errno) << "\n";
        return -1;
    }

    // Listen to socket
    listen(sock, 5);

    // Make sure we get notified when a child dies
    signal(SIGCHLD, child_died);

    // Accept client
    int forking = MAIN->options.listen_forks;
    while (true)
    {
        // Block until we can accept more connexions (avoid fork bombs)
        while (forking && active_children >= forking)
        {
            IFTRACE(remote)
                std::cerr << "xl_listen: Too many children, waiting\n";
            if (int childPID = waitpid(-1, NULL, 0))
            {
                if (childPID > 0)
                {
                    IFTRACE(remote)
                        std::cerr << "xl_listen: Child " << childPID
                                  << " died, resuming\n";
                    active_children--;
                }
            }
        }

        // Accept input
        struct sockaddr_in client;
        socklen_t length = sizeof(client);
        int insock = accept(sock, (struct sockaddr *) &client, &length);
        if (insock < 0)
        {
            std::cerr << "xl_listen: Error accepting port " << port << ": "
                      << strerror(errno) << "\n";
            continue;
        }

        // Fork child for incoming connexion
        int pid = forking ? fork() : 0;
        if (pid == -1)
        {
            std::cerr << "xl_listen: Error forking child\n";
        }
        else if (pid)
        {
            IFTRACE(remote)
                std::cerr << "xl_listen: Forked pid " << pid << "\n";
            close(insock);
            active_children++;
        }
        else
        {
            // Read data from client
            char buffer[256];
            text payload = "";
            while(true)
            {
                int rd = read(insock, buffer, sizeof(buffer)-1);
                if (rd <= 0)
                    break;
                payload += text(buffer, rd);
            }
            close(insock);

            // Deserialize data
            std::istringstream input(payload);
            Deserializer deserializer(input);
            Tree *code = deserializer.ReadTree();
            
            // Evaluate resullting code (todo: fork)
            if (code)
            {
                IFTRACE(remote)
                {
                    std::cerr << "xl_listen: Received code: " << code << "\n";
                    std::cerr << "xl_listen; Evaluating\n";
                }
                context->Evaluate(code);
            }
            if (forking)
            {
                IFTRACE(remote)
                    std::cerr << "xl_listen: Exiting PID " << getpid() << "\n";
                exit(0);
            }
        }
    }
        
}

XL_END
