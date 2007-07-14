// ****************************************************************************
//   (C) Copyright 2000-2007 Hewlett-Packard Development Company, L.P.
//   HPVM: $RCSfile$ $Revision$ - HP CONFIDENTIAL
// ****************************************************************************
// 
//   File Description:
// 
//    Implementation of basic C-style pointers
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

module XL.POINTER with

    generic [type T]
    type pointer written pointer to T is XL.BYTECODE.pointer_type

    function Dereference (P : pointer) return pointer.T written *P                      is XL.BYTECODE.deref_ptr
    function Pointer() return pointer                                                   is XL.BYTECODE.zero_ptr
    function Pointer(in out item : pointer.T) return pointer written &item              is XL.BYTECODE.address_ptr
    to Copy(out Tgt : pointer; Src : pointer) written Tgt := Src                        is XL.BYTECODE.copy_ptr
    to Zero(out Tgt : pointer) written Tgt := nil                                       is XL.BYTECODE.zero_ptr
