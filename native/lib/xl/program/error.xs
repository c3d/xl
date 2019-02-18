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

module ERROR with
// ----------------------------------------------------------------------------
//    Interface for handling errors
// ----------------------------------------------------------------------------

    // Type used to report an error
    type error with
        Message as text

    // Conversion to text
    text Error:error is Error.Message

    // Generate an error, possibly capturing some arguments
    error Message:text                  as error
    error Format:text, Args:list        as error

    // The opposite of an error is called success
    success                             is not error

    // Try evaluating an expression that may return an error
    // This is NOT exception handling, but type matching
    try Error  :error   catch Body      is Body
    try Success:success catch Body      is Success
