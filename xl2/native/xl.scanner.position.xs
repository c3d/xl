// *****************************************************************************
// xl.scanner.position.xs                                             XL project
// *****************************************************************************
//
// File description:
//
//     Information about scanner positions
//
//     Positions are recorded as number of characters since the beginning
//     of a compilation. As new files are being read, this modules keep track
//     of what file correspond to which character range
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2015,2020, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

module XL.SCANNER.POSITION with
// ----------------------------------------------------------------------------
//    Interface for scanner positions
// ----------------------------------------------------------------------------

    // Reset the module (new compilation)
    procedure Reset

    // Start reading a new file, return first position
    function OpenFile (name : text) return integer
    procedure CloseFile (pos : integer)

    // Return file, start and end for a global position
    procedure PositionToFile (pos : integer;
                              out file : text;
                              out offset : integer)

    // Return file, line and column for a global position
    procedure PositionToLine (pos : integer;
                              out file : text;
                              out line : integer;
                              out column : integer;
                              out linetext : text)



