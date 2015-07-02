#ifndef REMOTE_H
#define REMOTE_H
// ****************************************************************************
//  remote.h                                                       Tao project
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

int     xl_tell(text host, Tree *body);
int     xl_listen(Context *context, uint port = XL_DEFAULT_PORT);

XL_END

#endif // REMOTE_H
