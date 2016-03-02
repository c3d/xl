#ifndef REMOTE_H
#define REMOTE_H
// ****************************************************************************
//  remote.h                                                     ELFE project
// ****************************************************************************
//
//   File Description:
//
//    The interface for a simple socket-based transport of ELFE programs
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

ELFE_BEGIN

const uint ELFE_DEFAULT_PORT = 1205;

int     elfe_tell(Context *, text host, Tree *body);
Tree_p  elfe_ask(Context *, text host, Tree *body);
Tree_p  elfe_invoke(Context *, text host, Tree *body);
int     elfe_reply(Context *, Tree *body);
Tree_p  elfe_listen_received();
Tree_p  elfe_listen_hook(Tree *body);
int     elfe_listen(Context *, uint forking, uint port = ELFE_DEFAULT_PORT);

ELFE_END

#endif // REMOTE_H
