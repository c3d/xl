#ifndef REMOTE_H
#define REMOTE_H
// ****************************************************************************
//  remote.h                                                     ELIOT project
// ****************************************************************************
//
//   File Description:
//
//    The interface for a simple socket-based transport of ELIOT programs
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

ELIOT_BEGIN

const uint ELIOT_DEFAULT_PORT = 1205;

int     eliot_tell(text host, Tree *body);
Tree_p  eliot_ask(text host, Tree *body);
Tree_p  eliot_invoke(Context *, text host, Tree *body);
int     eliot_reply(Context *, Tree *body);
Tree_p  eliot_listen_received();
Tree_p  eliot_listen_hook(Tree *body);
int     eliot_listen(Context *, uint forking, uint port = ELIOT_DEFAULT_PORT);

ELIOT_END

#endif // REMOTE_H
