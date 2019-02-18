// ****************************************************************************
//  versioning.xs                                   XL - An extensible language
// ****************************************************************************
//
//   File Description:
//
//     Interface for semantic versioning
//
//     Reference: https://semver.org
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

interface VERSIONING is
// ----------------------------------------------------------------------------
//    Interface for semantic versioning
// ----------------------------------------------------------------------------

    // The 'version' type contains values like 0.3.3
    type version is major:integer.minor:integer.patch:integer

    // Versions can be ordered
    use COMPARISONS[version]
