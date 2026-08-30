/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2026 Igor Zinken - https://www.igorski.nl
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

#include "calc.h"
#include "global.h"
#include "lfo.h"
#include <math.h>

namespace Igorski {
class FormantFilter
{
    static const int VOWEL_AMOUNT       = 4;
    static const int COEFF_AMOUNT       = 9;
    static const int FORMANT_TABLE_SIZE = ( 256 + 1 ); // The last entry of the table equals the first (to avoid a modulo)
    static const int MAX_FORMANT_WIDTH  = 64;
    static constexpr float ATTENUATOR  = 0.0005f;

    // hard coded values for dynamics processing, in -1 to +1 range

    static constexpr float DYNAMICS_THRESHOLD                  = 0.10f;
    static constexpr float DYNAMICS_RATIO                      = 0.50f;
    static constexpr float DYNAMICS_LEVEL                      = 0.65f;
    static constexpr float DYNAMICS_ATTACK                     = 0.18f;
    static constexpr float DYNAMICS_RELEASE                    = 0.55f;
    static constexpr float DYNAMICS_LIMITER_DYNAMICS_THRESHOLD = 0.99f;
    static constexpr float DYNAMICS_GATE_DYNAMICS_THRESHOLD    = 0.02f;
    static constexpr float DYNAMICS_GATE_DYNAMICS_ATTACK       = 0.10f;
    static constexpr float DYNAMICS_GATE_DECAY                 = 0.50f;
    static constexpr float DYNAMICS_MIX                        = 1.00f;

    // whether to apply the formant synthesis to the signal
    // otherwise the input is applied to the carrier directly

    static const bool APPLY_SYNTHESIS_SIGNAL = false;

    public:
        FormantFilter( float aVowel = 0.f, float sampleRate = VST::DEFAULT_SAMPLE_RATE );
        ~FormantFilter();

        void setSampleRate( float sampleRate );
        void setVowel( float aVowel );
        float getVowel();
        void setLFO( float LFORatePercentage, float LFODepth );
        void process( float* inBuffer, int bufferSize );

        LFO lfo;
        bool hasLFO;

    private:

        float _sampleRate;
        float _halfSampleRateFrac;
        float _vowel;
        float _tempVowel;
        int   _coeffOffset;
        float _lfoDepth;
        float _lfoRange;
        float _lfoMax;
        float _lfoMin;

        void cacheLFO();
        inline void cacheCoeffOffset()
        {
            _coeffOffset = static_cast<int>( Calc::scale( _tempVowel, 1.f, static_cast<float>( COEFF_AMOUNT ) - 1 ));
        }

        // vowel definitions

        struct Formant {
            float value;
            float coeffs[ COEFF_AMOUNT ];
        };

        float FORMANT_WIDTH_SCALE[ VOWEL_AMOUNT ] = { 100.f, 120.f, 150.f, 300.f };

        Formant A_COEFFICIENTS[ VOWEL_AMOUNT ] = {
            { 0.f, { 1.f, 0.5f, 1.f, 1.f, 0.7f, 1.f, 1.f, 0.3f, 1.f } },
            { 0.f, { 2.f, 0.5f, 0.7f, 0.7f, 0.35f, 0.3f, 0.5f, 1.f, 0.7f } },
            { 0.f, { 0.3f, 0.15f, 0.2f, 0.4f, 0.1f, 0.3f, 0.7f, 0.2f, 0.2f } },
            { 0.f, { 0.2f, 0.1f, 0.2f, 0.3f, 0.1f, 0.1f, 0.3f, 0.2f, 0.3f } }
        };

        Formant F_COEFFICIENTS[ VOWEL_AMOUNT ] = {
            { 100.f, {  730.f,  200.f,  400.f,  250.f,  190.f,  350.f,  550.f,  550.f,  450.f } },
            { 100.f, { 1090.f, 2100.f,  900.f, 1700.f,  800.f, 1900.f, 1600.f,  850.f, 1100.f } },
            { 100.f, { 2440.f, 3100.f, 2300.f, 2100.f, 2000.f, 2500.f, 2250.f, 1900.f, 1500.f } },
            { 100.f, { 3400.f, 4700.f, 3000.f, 3300.f, 3400.f, 3700.f, 3200.f, 3000.f, 3000.f } }
        };

        // the below are used for the formant synthesis

        float FORMANT_TABLE[ FORMANT_TABLE_SIZE * MAX_FORMANT_WIDTH ];
        float _phase = 0.f;

        float generateFormant( float phase, const float width );
        float getFormant( float phase, float width );
        float getCarrier( const float position, const float phase );

        // Fast approximation of cos( pi * x ) for x in -1 to +1 range

        inline float fast_cos( const float x )
        {
            float x2 = x * x;
            return 1.f + x2 * ( -4.f + 2.f * x2 );
        }

        // dynamics processing (compression and limiting to keep vowel level constant)

        inline float compress( float sample )
        {
            float a, b, i, j, g, out;
            float e   = _dEnv;
            float e2  = _dEnv2;
            float ge  = _dGainEnv;
            float re  = ( 1.f - _dRelease );
            float lth = _dLimThreshold;

            if ( _fullDynamicsProcessing ) {

                // apply compression, gating and limiting

                if ( lth == 0.f ) {
                    lth = 1000.f;
                }
                a = sample;
                i = ( a < 0.f ) ? -a : a;

                e  = ( i > e ) ? e + _dAttack * ( i - e ) : e * re;
                e2 = ( i > e ) ? i : e2 * re; // ir;

                g = ( e > _dThreshold ) ? _dTrim / ( 1.f + _dRatio * (( e / _dThreshold ) - 1.f )) : _dTrim;

                if ( g < 0.f ) {
                    g = 0.f;
                }
                if ( g * e2 > lth ) {
                    g = lth / e2; // limiting
                }
                ge  = ( e > _dExpThreshold ) ? ge + _dGateAttack - _dGateAttack * ge : ge * _dExpRatio; // gating
                out = a * ( g * ge + _dDry );
            }
            else {
                // compression only
                a = sample;
                i = ( a < 0.f ) ? -a : a;

                e = ( i > e )  ? e + _dAttack * ( i - e ) : e * re; // envelope
                g = ( e > _dThreshold ) ? _dTrim / ( 1.f + _dRatio * (( e / _dThreshold ) - 1.f )) : _dTrim; // gain

                out = a * ( g + _dDry ); // VCA
            }

            // catch denormals

            _dEnv     = ( e  < 1.0e-10 ) ? 0.f : e;
            _dEnv2    = ( e2 < 1.0e-10 ) ? 0.f : e2;
            _dGainEnv = ( ge < 1.0e-10 ) ? 0.f : ge;

            return out;
        }

        void cacheDynamicsProcessing();

        float _dThreshold;
        float _dRatio;
        float _dAttack;
        float _dRelease;
        float _dTrim;
        float _dLimThreshold;
        float _dExpThreshold;
        float _dExpRatio;
        float _dDry;
        float _dEnv;
        float _dEnv2;
        float _dGainEnv;
        float _dGateAttack;
        bool _fullDynamicsProcessing;

};
}

#endif
