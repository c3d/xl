// *****************************************************************************
// xl.errors.xs                                                       XL project
// *****************************************************************************
//
// File description:
//
//     Errors for the XL compiler
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
// (C) 2003-2004,2006-2008,2015, Christophe de Dinechin <christophe@dinechin.org>
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

import PT = XL.PARSER.TREE

module XL.ERRORS with

    // Saving and restoring errors
    procedure PushErrorContext()
    function PopErrorContext() return boolean
    procedure DisplayLastErrors()
    function LastErrorsText() return text
    function PendingErrors() return integer
    procedure PutLastErrorFirst()
    function ErrorTree(previous : PT.tree := nil) return PT.tree
    procedure ReplayErrors(token : PT.tree)

    // Signaling an error
    procedure Error (E : text; pos : integer; args : string of text)

    // With 'text' parameters
    procedure Error (E : text; pos : integer)
    procedure Error (E : text; pos : integer; arg : text)
    procedure Error (E : text; pos : integer; arg : text; arg2 : text)
    procedure Error (E : text; pos : integer;
                     arg : text; arg2 : text; arg3 : text)

    // With 'tree' parameters (pos deduced from arg1)
    procedure Error (E : text; arg : PT.tree)
    procedure Error (E : text; arg : PT.tree; arg2 : text)
    procedure Error (E : text; arg : PT.tree; arg2 : PT.tree)
    procedure Error (E : text; arg : PT.tree; arg2 : text; arg3 : text)
    procedure Error (E : text; arg : PT.tree; arg2 : text; arg3 : PT.tree)
    procedure Error (E : text; arg : PT.tree; arg2 : PT.tree; arg3 : text)
    procedure Error (E : text; arg : PT.tree; arg2 : PT.tree; arg3 : PT.tree)

    error_count : integer := 0
    max_errors  : integer := 10
