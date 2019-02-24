// ****************************************************************************
//  conversions.xs                                  XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for standard conversions
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

use BOOLEAN
use INTEGER
use INTEGER
use REAL
use DECIMAL
use TEXT
use SYMBOL


with
    type itype          like integer
    type isource        like integer
    type rtype          like real
    type rsource        like real
    type dtype          like decimal
    type dsource        like decimal
    type etype          like enumerated
    type esource        like enumerated
    type ctype          like character
    type csource        like character

    IntToIntImplicit    is ( isource.BitSize < itype.BitSize or
                             (isource.BitSize = itype.BitSize and
                              isource.Signed  = itype.Signed))

    IntToRealImplicit   is (isource.BitSize < rtype.MantissaBits)

// Explicit numerical conversions
itype Value:isource     as itype
itype Value:rsource     as itype
itype Value:dsource     as itype

rtype Value:isource     as itype
rtype Value:rsource     as itype
rtype Value:dsource     as itype

dtype Value:isource     as itype
dtype Value:rsource     as itype
dtype Value:dsource     as itype

// Explicit enumerated conversions
itype   Value:boolean   as itype
boolean Value:isource   as boolean
itype   Value:esource   as itype
etype   Value:isource   as etype
itype   Value:csource   as itype
ctype   Value:isource   as ctype

// Implicit numerical conversions
Value:isource           as itype        when IntToIntImplicit
Value:isource           as rtype        when IntToRealImplicit
