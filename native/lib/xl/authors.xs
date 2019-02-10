// ****************************************************************************
//  authors.xs                                      XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//    Default specification for authors of the XL standard modules
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

use TEXT
use SEMANTIC_VERSIONING
use DATE


AUTHORS has
// ----------------------------------------------------------------------------
//   Default module information use for XL modules
// ----------------------------------------------------------------------------
//   The organization below assumes that for XL modules, there is
//   (at the moment) a single author

    VERSION             is 0.0.1
    DATE                is February 10, 2019
    URL                 is "http://github.com/c3d/xl"
    LICENSE             is "GPL v3"
    CREATOR             is "Christophe de Dinechin"
    AUTHORS             is CREATOR
    EMAIL               is "christophe@dinechin.org"
    COPYRIGHT           is "(C) " & Year(DATE) & Join(AUTHORS, ", ")
