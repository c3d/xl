// ****************************************************************************
//  xl.pointer.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Machine low-level pointers
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

POINTER[item:type] has
// ----------------------------------------------------------------------------
//   Interface for machine-level pointers
// ----------------------------------------------------------------------------

    use XL.ALIAS[value]

    type pointer
    type offset
    type size

    *P:pointer                          as alias[value]

    in out P:pointer++                  as pointer
    ++in out P:pointer                  as pointer
    in out P:pointer--                  as pointer
    --in out P:pointer                  as pointer

    P:pointer + O:offset                as pointer
    P:pointer - O:offset                as pointer
    P:pointer - Q:pointer               as offset

    out x:pointer := y:pointer          as pointer

    in out x:pointer += y:offset        as pointer
    in out x:pointer -= y:offset        as pointer

    P:pointer[O:offset]                 as value

    P:pointer.(Name:name)               is (*P).Name

    bit_size  pointer                   as size
    bit_align pointer                   as size
    bit_size  X:pointer                 as size
    bit_align X:pointer                 as size


POINTER has
// ----------------------------------------------------------------------------
//   Interface to easily create pointer types
// ----------------------------------------------------------------------------

    pointer[item:type]                  is POINTER[item].pointer
    pointer to item:type                is pointer[item]
