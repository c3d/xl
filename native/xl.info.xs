// ****************************************************************************
//  xl.info.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Infomration that can be attached to source trees
//
//
//
//
//
//
//
//
// ****************************************************************************
//  (C) 2018 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the GNU General Public License v3
//   See LICENSE file for details.
// ****************************************************************************

module XL.Info with

    type info with
        next : info
        if DEBUG then
            owner : info


    X:info := Y:info is
        compile_error "Cannot copy info data"
