#ifndef REMOTE_H
#define REMOTE_H
// *****************************************************************************
// remote.h                                                           XL project
// *****************************************************************************
//
// File description:
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
// *****************************************************************************
// This software is licensed under the GNU General Public License v3
// (C) 2015-2019, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can r redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
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
