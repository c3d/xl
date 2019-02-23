// ****************************************************************************
//  clone.xs                                        XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Clone (deep copy) operation
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

type clonable[type source is clonable] with
// ----------------------------------------------------------------------------
//    A type that can be cloned
// ----------------------------------------------------------------------------
//    A clone is a deep copy of the source

    Clone Source:source                 as clonable

    // Indicate if cloning  can be made by simply copying the input
    CLONE_IS_COPY                       as boolean is true
