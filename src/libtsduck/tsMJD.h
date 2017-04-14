//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Modified Julian Date (MJD) utilities
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsTime.h"

namespace ts {

    // Size in bytes of an encoded complete Modified Julian Date (MJD)
    const size_t MJD_SIZE = 5;

    // Minimal size in bytes of an encoded Modified Julian Date (MJD), ie. date only.
    const size_t MJD_MIN_SIZE = 2;

    // This routine converts a Modified Julian Date (MJD) into a ts::Time.
    // The mjd must point to a 2-to-5 bytes area, in the format specified by a TDT.
    // Return false in case of error.
    TSDUCKDLL bool DecodeMJD (const uint8_t* mjd, size_t mjd_size, Time& time);

    // This routine converts a base::Time into a Modified Julian Date (MJD).
    // The mjd must point to a writeable 2-to-5 bytes area.
    // Return false in case of error.
    TSDUCKDLL bool EncodeMJD (const Time& time, uint8_t* mjd, size_t mjd_size);
}