// ****************************************************************************
//  xl.scanner.position.xs          (C) 1992-2003 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
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
// ****************************************************************************
// This document is released under the GNU Genral Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

module XL.SCANNER.POSITION with
// ----------------------------------------------------------------------------
//    Interface for scanner positions
// ----------------------------------------------------------------------------

    // Reset the module (new compilation)
    procedure Reset

    // Start reading a new file, return first position
    procedure OpenFile (name : text)
    procedure CloseFile (pos : integer)

    // Return file, start and end for a global position
    procedure PositionToFile (pos : integer; out file : text; out offset : integer)

    // Return file, line and column for a global position
    procedure PositionToLine (pos : integer; out file : text; out line : integer; out column : integer; out linetext : text)


    
