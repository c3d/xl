// ****************************************************************************
//   (C) Copyright 2000-2007 Hewlett-Packard Development Company, L.P.
//   HPVM: $RCSfile$ $Revision$ - HP CONFIDENTIAL
// ****************************************************************************
// 
//   File Description:
// 
//    Implementation of basic pointer functionality
// 
// 
// 
// 
// 
// 
// 
// 
// ****************************************************************************
// This document is property of Hewlett-Packard DC. It must not be
// copied, reproduced or disclosed outside Hewlett-Packard without prior
// written approval.
// ****************************************************************************

use XL.POINTER

module XL.POINTER.ADDRESS with

    type address is XL.BYTECODE.xlptr

    function Address(value : integer) return address written value                      is XL.BYTECODE.int_to_ptr
    function Address(value : unsigned) return address                                   is XL.BYTECODE.int_to_ptr

    function Integer(addr : integer) return integer                                     is XL.BYTECODE.ptr_to_int
    function Unsigned(addr : unsigned) return unsigned                                  is XL.BYTECODE.ptr_to_int

    function Address(in out item : pointer.T) return pointer written &item              is XL.BYTECODE.address_ptr

    function Pointer(addr : integer) return pointer                                    is XL.BYTECODE.unsigned_to_ptr
    function Unsigned(ptr : pointer) return integer                                    is XL.BYTECODE.ptr_to_unsigned

