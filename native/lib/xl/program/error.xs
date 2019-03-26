// ****************************************************************************
//  error.xs                                        XL - An extensible language
// ****************************************************************************
//
//   File Description:
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
// ****************************************************************************
//  (C) 2019 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

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
