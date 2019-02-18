// ****************************************************************************
//  list.xs                                         XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for lists
//
//     In XL, lists are comma-separated sequences of values
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

LIST[item:type] has
// ----------------------------------------------------------------------------
//    Interface for a list of items
// ----------------------------------------------------------------------------

    // Structure of a list
    list is either
        Head:item, Rest:list
        Item:item


LIST has
// ----------------------------------------------------------------------------
//    Interface for making it easy to create list types
// ----------------------------------------------------------------------------

    list[item:type]             is SUPER.LIST[item].list
    list of item:type           is list[item]
    list                        is list[anything]
