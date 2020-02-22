/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Igor Zinken - https://www.igorski.nl
 *
 * Adapted from public source code by Paul Sernine, based on work by Thierry Rochebois
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
#include "calc.h"
#include <math.h>

#define FORMANT_TABLE_SIZE (256+1) // The last entry of the table equals the first (to avoid a modulo)
#define MAX_FORMANT_WIDTH 64

namespace Igorski {
class FormantFilter
{
    static const int VOWEL_AMOUNT = 4;
    static const int COEFF_AMOUNT = 9;
    static constexpr double SCALE = 0.001;

    // whether to apply the formant synthesis to the signal
    // otherwise the input is applied to the carrier directly

    static const bool APPLY_SYNTHESIS_SIGNAL = false;

    public:
        FormantFilter( float aVowel, float sampleRate );
        ~FormantFilter();

        void setVowel( float aVowel );
        float getVowel();
        void setLFO( float LFORatePercentage, float LFODepth );
        void process( double* inBuffer, int bufferSize );

        LFO* lfo;
        bool hasLFO;

    private:

        float  _sampleRate;
        float  _halfSampleRateFrac;
        double _vowel;
        double _tempVowel;
        int    _coeffOffset;
        float  _lfoDepth;
        double _lfoRange;
        double _lfoMax;
        double _lfoMin;

        void cacheLFO();
        inline void cacheCoeffOffset()
        {
            _coeffOffset = ( int ) Calc::scale( _tempVowel, 1.f, ( float ) COEFF_AMOUNT - 1 );
        }

        // vowel definitions

        struct Formant {
            double value;
            double coeffs[ COEFF_AMOUNT ];
        };

        double FORMANT_WIDTH_SCALE[ VOWEL_AMOUNT ] = { 100, 120, 150, 300 };

        Formant A_COEFFICIENTS[ VOWEL_AMOUNT ] = {
            { 0.0, { 1.0, 0.5, 1.0, 1.0, 0.7, 1.0, 1.0, 0.3, 1.0 } },
            { 0.0, { 2.0, 0.5, 0.7, 0.7,0.35, 0.3, 0.5, 1.0, 0.7 } },
            { 0.0, { 0.3,0.15, 0.2, 0.4, 0.1, 0.3, 0.7, 0.2, 0.2 } },
            { 0.0, { 0.2, 0.1, 0.2, 0.3, 0.1, 0.1, 0.3, 0.2, 0.3 } }
        };

        Formant F_COEFFICIENTS[ VOWEL_AMOUNT ] = {
            { 100.0, {  730,  200,  400,  250,  190,  350,  550,  550,  450 } },
            { 100.0, { 1090, 2100,  900, 1700,  800, 1900, 1600,  850, 1100 } },
            { 100.0, { 2440, 3100, 2300, 2100, 2000, 2500, 2250, 1900, 1500 } },
            { 100.0, { 3400, 4700, 3000, 3300, 3400, 3700, 3200, 3000, 3000 } }
        };

        // the below are used for the formant synthesis

        double FORMANT_TABLE[ FORMANT_TABLE_SIZE * MAX_FORMANT_WIDTH ];

        double _formantPhase   = 0.0;
        double _unFormantPhase = 0.0;
        double _phaseAcc       = 0.0;
        double _phase          = 0.0;

        double generateFormant( double phase, const double width );
        double getFormant( double phase, double width );
        double getCarrier( const double position, const double phase );

        // Fast approximation of cos( pi * x ) for x in -1 to +1 range

        inline double fast_cos( const double x )
        {
            double x2 = x * x;
            return 1 + x2 * ( -4 + 2 * x2 );
        }

};
}

#endif
