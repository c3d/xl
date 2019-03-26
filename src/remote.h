#ifndef REMOTE_H
#define REMOTE_H
// ****************************************************************************
//  remote.h                                                     XL project
// ****************************************************************************
//
//   File Description:
//
//    The interface for a simple socket-based transport of XL programs
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

#include "base.h"
#include "tree.h"
#include "context.h"

XL_BEGIN

const uint XL_DEFAULT_PORT = 1205;

int     xl_tell(Scope *, text host, Tree *body);
Tree_p  xl_ask(Scope *, text host, Tree *body);
Tree_p  xl_invoke(Scope *, text host, Tree *body);
int     xl_reply(Scope *, Tree *body);
Tree_p  xl_listen_received();
Tree_p  xl_listen_hook(Tree *body);
int     xl_listen(Scope *, uint forking, uint port = XL_DEFAULT_PORT);

XL_END

#endif // REMOTE_H
