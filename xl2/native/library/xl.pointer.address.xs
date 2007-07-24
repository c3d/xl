// ****************************************************************************
//   (C) Copyright 2000-2007 Hewlett-Packard Development Company, L.P.
//   HPVM: $RCSfile$ $Revision$ - HP CONFIDENTIAL
// ****************************************************************************
// 
//   File Description:
// 
//    Taking the address of variables and objects
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

    function Address(in out item : pointer.item) return pointer written &item           is XL.BYTECODE.address_ptr

