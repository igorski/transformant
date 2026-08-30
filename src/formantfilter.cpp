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
 * the Software and to permit persons to whom the Software is furnished to do so,
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
#include "formantfilter.h"
#include <cmath>

namespace Igorski {

/* constructor / destructor */

FormantFilter::FormantFilter( float aVowel, float sampleRate )
{
    float coeff = 2.f / ( FORMANT_TABLE_SIZE - 1.f );

    for ( size_t i = 0; i < MAX_FORMANT_WIDTH; i++ )
    {
        for ( size_t j = 0; j < FORMANT_TABLE_SIZE; j++ ) {
            FORMANT_TABLE[ j + i * FORMANT_TABLE_SIZE ] = generateFormant( -1 + j * coeff, static_cast<float>( i ));
        }
    }

    setSampleRate( sampleRate );

    setVowel( aVowel );
    cacheDynamicsProcessing();

    // note: LFO is always "on" as its used by the formant synthesis
    // when we want the audible oscillation of vowels to stop, the LFO
    // depth is merely at 0

    setLFO( 0.f, 0.f );
}

FormantFilter::~FormantFilter()
{
    // nowt...
}

/* public methods */

void FormantFilter::setSampleRate( float sampleRate )
{
    _sampleRate = sampleRate;
    _halfSampleRateFrac = 1.f / ( _sampleRate * 0.5f );

    lfo.setSampleRate( _sampleRate );
}

float FormantFilter::getVowel()
{
    return _vowel;
}

void FormantFilter::setVowel( float aVowel )
{
    _vowel = aVowel;

    float tempRatio = _tempVowel / std::max( 0.000000001f, _vowel );

    // in case FormantFilter is attached to oscillator, keep relative offset
    // of currently moving vowel in place
    _tempVowel = hasLFO ? _vowel * tempRatio : _vowel;

    cacheCoeffOffset();
    cacheLFO();
}

void FormantFilter::setLFO( float LFORatePercentage, float LFODepth )
{
    bool isLFOenabled = LFORatePercentage > 0.f;
    bool wasChanged   = hasLFO != isLFOenabled || _lfoDepth != LFODepth;

    hasLFO = isLFOenabled;

    lfo.setRate(
        VST::MIN_LFO_RATE() + (
            LFORatePercentage * ( VST::MAX_LFO_RATE() - VST::MIN_LFO_RATE() )
        )
    );

    if ( wasChanged ) {
        _lfoDepth = LFODepth;
        cacheLFO();
    }
}

void FormantFilter::process( float* inBuffer, int bufferSize )
{
    float lfoValue;
    float in, out, fp, ufp, phaseAcc, formant, carrier;

    for ( size_t i = 0; i < bufferSize; ++i )
    {
        in  = inBuffer[ i ];
        out = 0.0;

        // sweep the LFO

        lfoValue   = lfo.peek() * .5f  + .5f; // make waveform unipolar
        _tempVowel = std::min( _lfoMax, _lfoMin + _lfoRange * lfoValue ); // relative to LFO depth

        cacheCoeffOffset(); // ensure the appropriate coeff is used for the new _tempVowel value

        // calculate the phase for the formant synthesis and carrier

        fp  = 12 * powf( 2.0, 4 - 4 * _tempVowel );   // sweep
        // fp *= ( 1.0 + 0.01 * sinf( tmp * 0.0015 )); // optional vibrato (sinf value determines speed)
        ufp = 1.0 / fp;

        phaseAcc = fp * _halfSampleRateFrac;
        _phase  += phaseAcc;
        _phase  -= 2.f * ( _phase > 1.f );

        // calculate the coefficients

        for ( size_t j = 0; j < VOWEL_AMOUNT; ++j )
        {
            auto a = &A_COEFFICIENTS[ j ];
            auto f = &F_COEFFICIENTS[ j ];

            a->value += ATTENUATOR * ( a->coeffs[ _coeffOffset ] - a->value );
            f->value += ATTENUATOR * ( f->coeffs[ _coeffOffset ] - f->value );

            // apply formant onto the input signal

            float formant = APPLY_SYNTHESIS_SIGNAL ? getFormant( _phase, FORMANT_WIDTH_SCALE[ j ] * ufp ) : 1.0;
            float carrier = getCarrier( f->value * ufp, _phase );

            // the fp/fn coefficients stand for a -3dB/oct spectral envelope
            out += a->value * ( fp / f->value ) * in * formant * carrier;
        }

        // catch denormals

        undenormaliseFloat( out );

        // compress signal and write to output

        inBuffer[ i ] = compress( out );
    }
}

/* private methods */

void FormantFilter::cacheLFO()
{
    // when LFO is "off" we mock a depth of 0. In reality we keep
    // the LFO moving to feed the carrier signal. The LFO won't
    // change the active vowel coefficient in this mode.

    _lfoRange = _vowel * ( hasLFO ? _lfoDepth : 0.f );
    _lfoMax   = std::min( 1.f, _vowel + _lfoRange / 2.f );
    _lfoMin   = std::max( 0.f, _vowel - _lfoRange / 2.f );
}

float FormantFilter::generateFormant( float phase, const float width )
{
    int hmax   = static_cast<int>( 10 * width ) > FORMANT_TABLE_SIZE / 2 ? FORMANT_TABLE_SIZE / 2 : static_cast<int>( 10 * width );
    float jupe = 0.15f;

    float a = 0.5f;
    float phi = 0.0f;
    float hann, gaussian, harmonic;

    for ( size_t h = 1; h < hmax; h++ ) {
        phi     += VST::PI * phase;
        hann     = 0.5f + 0.5f * fast_cos( h * ( 1.f / hmax ));
        gaussian = 0.85f * exp( -h * h / ( width * width ));
        harmonic = cosf( phi );
        a += hann * ( gaussian + jupe ) * harmonic;
    }
    return a;
}

float FormantFilter::getFormant( float phase, float width )
{
    width = ( width < 0 ) ? 0 : width > MAX_FORMANT_WIDTH - 2 ? MAX_FORMANT_WIDTH - 2 : width;
    float P = ( FORMANT_TABLE_SIZE - 1 ) * ( phase + 1 ) * 0.5f; // normalize phase

    // calculate the integer and fractional parts of the phase and width

    int phaseI    = static_cast<int>( P );
    float phaseF = P - phaseI;

    int widthI    = static_cast<int>( width );
    float widthF = width - widthI;

    int i00 = phaseI + FORMANT_TABLE_SIZE * widthI;
    int i10 = i00 + FORMANT_TABLE_SIZE;

    // bilinear interpolation of formant values
    return ( 1.f - widthF ) *
           ( FORMANT_TABLE[ i00 ] + phaseF * ( FORMANT_TABLE[ i00 + 1 ] - FORMANT_TABLE[ i00 ])) +
             widthF * ( FORMANT_TABLE[ i10 ] + phaseF * ( FORMANT_TABLE[ i10 + 1 ] - FORMANT_TABLE[ i10 ]));
}

float FormantFilter::getCarrier( const float position, const float phase )
{
    float harmI = floor( position ); // integer and
    float harmF = position - harmI;  // fractional part of harmonic number

    // keep within -1 to +1 range
    float phi1 = fmodf( phase *  harmI        + 1 + 1000, 2.f ) - 1.f;
    float phi2 = fmodf( phase * ( harmI + 1 ) + 1 + 1000, 2.f ) - 1.f;

    // calculate the two carriers
    float carrier1 = fast_cos( phi1 );
    float carrier2 = fast_cos( phi2 );

    // return interpolation between the two carriers
    return carrier1 + harmF * ( carrier2 - carrier1 );
}

void FormantFilter::cacheDynamicsProcessing()
{
    _fullDynamicsProcessing = false;

    _dThreshold = pow( 10.f, ( 2.f * DYNAMICS_THRESHOLD - 2.f ));
    _dRatio     = 2.5f * DYNAMICS_RATIO - 0.5f;

    if ( _dRatio > 1.f ) {
        _dRatio = 1.f + 16.f * ( _dRatio - 1.f ) * ( _dRatio - 1.f );
        _fullDynamicsProcessing = true;
    }
    if ( _dRatio < 0.f ) {
        _dRatio = 0.6f * _dRatio;
        _fullDynamicsProcessing = true;
    }
    _dTrim    = pow( 10.f, ( 2.f * DYNAMICS_LEVEL ));
    _dAttack  = pow( 10.f, ( -0.002f - 2.f * DYNAMICS_ATTACK ));
    _dRelease = pow( 10.f, ( -2.f - 3.f * DYNAMICS_RELEASE ));
    
    // limiter
    
    if ( DYNAMICS_LIMITER_DYNAMICS_THRESHOLD > 0.98f ) {
        _dLimThreshold = 0.f;
    }
    else {
        _dLimThreshold = 0.99f * pow( 10.f, static_cast<int>( 30.f * DYNAMICS_LIMITER_DYNAMICS_THRESHOLD - 20.f ) / 20.f );
        _fullDynamicsProcessing = true;
    }
    
    // expander
    
    if ( DYNAMICS_GATE_DYNAMICS_THRESHOLD < 0.02f ) {
        _dExpThreshold = 0.f;
    }
    else {
        _dExpThreshold = pow( 10.f, ( 3.f * DYNAMICS_GATE_DYNAMICS_THRESHOLD - 3.f ));
        _fullDynamicsProcessing = true;
    }
    _dExpRatio   = 1.f - pow( 10.f, ( -2.f - 3.3f * DYNAMICS_GATE_DECAY ));
    _dGateAttack = pow( 10.f, ( -0.002f - 3.f * DYNAMICS_GATE_DYNAMICS_ATTACK ));
    
    if ( _dRatio < 0.0f && _dThreshold < 0.1f ) {
        _dRatio *= _dThreshold * 15.f;
    }
    _dDry   = 1.0f - DYNAMICS_MIX;
    _dTrim *= DYNAMICS_MIX;
}

}
