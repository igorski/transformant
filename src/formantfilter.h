/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Igor Zinken - https://www.igorski.nl
 *
 * Adapted from public source code by alex@smartelectronix.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __FORMANTFILTER_H_INCLUDED__
#define __FORMANTFILTER_H_INCLUDED__

#include "lfo.h"
#include <math.h>

namespace Igorski {
class FormantFilter
{
    static const int MEMORY_SIZE  = 10;
    static const int COEFF_AMOUNT = 11;

    static const bool INTERPOLATE = false; // whether to interpolate formants between vowels

    public:
        FormantFilter( float aVowel );
        ~FormantFilter();

        void setVowel( float aVowel );
        float getVowel();
        void setLFO( float LFORatePercentage, float LFODepth );
        void process( float* inBuffer, int bufferSize );

        LFO* lfo;
        bool hasLFO;

    private:

        double _vowel;
        double _tempVowel;
        float  _lfoDepth;
        double _lfoRange;
        double _lfoMax;
        double _lfoMin;

        double _currentCoeffs[ COEFF_AMOUNT ];
        double _memory[ MEMORY_SIZE ];

        void cacheVowel();
        void cacheLFO();

        double COEFFICIENTS[ 5 ][ COEFF_AMOUNT ] = {

            // vowel "A"

            { 8.11044e-06, 8.943665402, -36.83889529, 92.01697887, -154.337906, 181.6233289, -151.8651235, 89.09614114, -35.10298511, 8.388101016, -0.923313471 },

            // vowel "E"

            { 4.36215e-06, 8.90438318, -36.55179099, 91.05750846, -152.422234, 179.1170248, -149.6496211, 87.78352223, -34.60687431, 8.282228154, -0.914150747 },

            // vowel "I"

            { 3.33819e-06, 8.893102966, -36.49532826, 90.96543286, -152.4545478, 179.4835618, -150.315433, 88.43409371, -34.98612086, 8.407803364, -0.932568035 },

            // vowel "O"

            { 1.13572e-06, 8.994734087, -37.2084849, 93.22900521, -156.6929844, 184.596544, -154.3755513, 90.49663749, -35.58964535, 8.478996281, -0.929252233 },

            // vowel "U"

            { 4.09431e-07, 8.997322763, -37.20218544, 93.11385476, -156.2530937, 183.7080141, -153.2631681, 89.59539726, -35.12454591, 8.338655623, -0.910251753 }
        };

};
}

#endif
