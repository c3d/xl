// *****************************************************************************
// xl.program.error.xs                                                XL project
// *****************************************************************************
//
// File description:
//
//    Interface for error handling
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
// (C) 2019-2020, Christophe de Dinechin <christophe@dinechin.org>
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

use TEXT, LIST

type error with
// ----------------------------------------------------------------------------
//    A type representing an error
// ----------------------------------------------------------------------------

    // The message associated to the error
    Message                             as text

    // Generate an error, possibly capturing some arguments
    error Message:text                  as error
    error Format:text, Args:list        as error

    // Conversion to text
    text Error:error                    as text         is Error.Message


// The opposite of an error is called success
type success                            is not error

// Try evaluating an expression that may return an error
// This is NOT exception handling, but type matching
try Error  :error   catch Body      is Body
try Success:success catch Body      is Success
