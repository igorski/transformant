/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Igor Zinken - https://www.igorski.nl
 *
 * Adaptation of source provided in the JUCE library:
 * Copyright (c) 2020 - Raw Material Software Limited
 *
 * JUCE is an open source library subject to commercial or open-source
 * licensing.
 *
 * The code included in this file is provided under the terms of the ISC license
 * http://www.isc.org/downloads/software-support-policy/isc-license. Permission
 * To use, copy, modify, and/or distribute this software for any purpose with or
 * without fee is hereby granted provided that the above copyright notice and
 * this permission notice appear in all copies.
 *
 * JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
 * EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
 * DISCLAIMED.
 */
#ifndef __SND_H_INCLUDED__
#define __SND_H_INCLUDED__

#ifdef __SSE__
    #define USE_SSE_INTRINSICS
#endif
#ifdef __SSE2__
    #define USE_SSE_INTRINSICS
#endif

#ifdef USE_SSE_INTRINSICS
#include <xmmintrin.h>
#endif

//==============================================================================
/**
Helper class providing an RAII-based mechanism for temporarily disabling
denormals on your CPU.
*/
class ScopedNoDenormals
{
    public:
        inline ScopedNoDenormals() noexcept
        {
        #ifdef USE_SSE_INTRINSICS
            mxcsr = _mm_getcsr();
            _mm_setcsr (mxcsr | 0x8040); // add the DAZ and FZ bits
        #endif
        }


        inline ~ScopedNoDenormals() noexcept
        {
        #ifdef USE_SSE_INTRINSICS
            _mm_setcsr (mxcsr);
        #endif
        }

    private:
        #ifdef USE_SSE_INTRINSICS
        unsigned int mxcsr;
        #endif
};

#endif
