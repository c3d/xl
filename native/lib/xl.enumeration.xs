// ****************************************************************************
//  xl.enumeration.xs                               XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Definition of enumerations
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

module XL.ENUMERATION with

    enumeration [type reptype] Body with
        type value

        text    Value:value     as text
        value   Text:text       as value
        reptype Value:value     as reptype
        value   Rep:reptype     as value

        enumeration_body Body,
            source First:name, Rep:reptype is
                source_code
                    [[First]] : value is reptype [[Rep]]

        enumeration_body Body, Code is
            Index : reptype := 0
            body Body
            body First, Rest is
                FirstSource is body First
                RestSource  is body Rest
                source_code
                    [[FirstSource]]
                    [[RestSource]]
            body First:name = Rep:reptype is
                Index := Rep
                body First
            body {First:name is Rep:reptype} is
                body First = Rep
            body First:name is
                Rep is Index++
                Code.source First, Rep

    use XL.UNSIGNED
    enumeration Body is enumeration[unsigned] Body
