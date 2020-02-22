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
    double coeff = 2.0 / ( FORMANT_TABLE_SIZE - 1 );

    for ( size_t i = 0; i < MAX_FORMANT_WIDTH; i++ )
    {
        for ( size_t j = 0; j < FORMANT_TABLE_SIZE; j++ ) {
            FORMANT_TABLE[ j + i * FORMANT_TABLE_SIZE ] = generateFormant( -1 + j * coeff, double( i ));
        }
    }

    _sampleRate         = sampleRate;
    _halfSampleRateFrac = 1.f / ( _sampleRate * 0.5f );

    setVowel( aVowel );
    recalculate();

    // note: LFO is always "on" as its used by the formant synthesis
    // when we want the audible oscillation of vowels to stop, the LFO
    // depth is merely at 0

    lfo = new LFO( _sampleRate );
    setLFO( 0.f, 0.f );
}

FormantFilter::~FormantFilter()
{
    delete lfo;
}

/* public methods */

float FormantFilter::getVowel()
{
    return ( float ) _vowel;
}

void FormantFilter::setVowel( float aVowel )
{
    _vowel = ( double ) aVowel;

    double tempRatio = _tempVowel / std::max( 0.000000001, _vowel );

    // in case FormantFilter is attached to oscillator, keep relative offset
    // of currently moving vowel in place
    _tempVowel = ( hasLFO ) ? _vowel * tempRatio : _vowel;

    cacheCoeffOffset();
    cacheLFO();
}

void FormantFilter::setLFO( float LFORatePercentage, float LFODepth )
{
    bool isLFOenabled = LFORatePercentage > 0.f;
    bool wasChanged   = hasLFO != isLFOenabled || _lfoDepth != LFODepth;

    hasLFO = isLFOenabled;

    lfo->setRate(
        VST::MIN_LFO_RATE() + (
            LFORatePercentage * ( VST::MAX_LFO_RATE() - VST::MIN_LFO_RATE() )
        )
    );

    if ( wasChanged ) {
        _lfoDepth = LFODepth;
        cacheLFO();
    }
}

void FormantFilter::process( double* inBuffer, int bufferSize )
{
    float lfoValue;
    double in, out, fp, ufp, phaseAcc, formant, carrier;

    for ( size_t i = 0; i < bufferSize; ++i )
    {
        in  = inBuffer[ i ];
        out = 0.0;

        // sweep the LFO

        lfoValue   = lfo->peek() * .5f  + .5f; // make waveform unipolar
        _tempVowel = std::min( _lfoMax, _lfoMin + _lfoRange * lfoValue ); // relative to LFO depth

        cacheCoeffOffset(); // ensure the appropriate coeff is used for the new _tempVowel value

        // calculate the phase for the formant synthesis and carrier

        fp  = 12 * powf( 2.0, 4 - 4 * _tempVowel );   // sweep
        // fp *= ( 1.0 + 0.01 * sinf( tmp * 0.0015 )); // optional vibrato (sinf value determines speed)
        ufp = 1.0 / fp;

        phaseAcc = fp * _halfSampleRateFrac;
        _phase  += phaseAcc;
        _phase  -= 2 * ( _phase > 1 );

        // calculate the coefficients

        for ( size_t j = 0; j < VOWEL_AMOUNT; ++j )
        {
            auto a = &A_COEFFICIENTS[ j ];
            auto f = &F_COEFFICIENTS[ j ];

            a->value += ATTENUATOR * ( a->coeffs[ _coeffOffset ] - a->value );
            f->value += ATTENUATOR * ( f->coeffs[ _coeffOffset ] - f->value );

            // apply formant onto the input signal

            double formant = APPLY_SYNTHESIS_SIGNAL ? getFormant( _phase, FORMANT_WIDTH_SCALE[ j ] * ufp ) : 1.0;
            double carrier = getCarrier( f->value * ufp, _phase );

            // the fp/fn coefficients stand for a -3dB/oct spectral envelope
            out += a->value * ( fp / f->value ) * in * formant * carrier;
        }

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

    _lfoRange = _vowel * ( hasLFO ? _lfoDepth : 0 );
    _lfoMax   = std::min( 1., _vowel + _lfoRange / 2. );
    _lfoMin   = std::max( 0., _vowel - _lfoRange / 2. );
}

double FormantFilter::generateFormant( double phase, const double width )
{
    int hmax    = int( 10 * width ) > FORMANT_TABLE_SIZE / 2 ? FORMANT_TABLE_SIZE / 2 : int( 10 * width );
    double jupe = 0.15f;

    double a = 0.5f;
    double phi = 0.0f;
    double hann, gaussian, harmonic;

    for ( size_t h = 1; h < hmax; h++ ) {
        phi     += VST::PI * phase;
        hann     = 0.5f + 0.5f * fast_cos( h * ( 1.0 / hmax ));
        gaussian = 0.85f * exp( -h * h / ( width * width ));
        harmonic = cosf( phi );
        a += hann * ( gaussian + jupe ) * harmonic;
    }
    return a;
}

double FormantFilter::getFormant( double phase, double width )
{
    width = ( width < 0 ) ? 0 : width > MAX_FORMANT_WIDTH - 2 ? MAX_FORMANT_WIDTH - 2 : width;
    double P = ( FORMANT_TABLE_SIZE - 1 ) * ( phase + 1 ) * 0.5f; // normalize phase

    // calculate the integer and fractional parts of the phase and width

    int phaseI    = ( int ) P;
    double phaseF = P - phaseI;

    int widthI    = ( int ) width;
    double widthF = width - widthI;

    int i00 = phaseI + FORMANT_TABLE_SIZE * widthI;
    int i10 = i00 + FORMANT_TABLE_SIZE;

    // bilinear interpolation of formant values
    return ( 1 - widthF ) *
           ( FORMANT_TABLE[ i00 ] + phaseF * ( FORMANT_TABLE[ i00 + 1 ] - FORMANT_TABLE[ i00 ])) +
             widthF * ( FORMANT_TABLE[ i10 ] + phaseF * ( FORMANT_TABLE[ i10 + 1 ] - FORMANT_TABLE[ i10 ]));
}

double FormantFilter::getCarrier( const double position, const double phase )
{
    double harmI = floor( position ); // integer and
    double harmF = position - harmI;  // fractional part of harmonic number

    // keep within -1 to +1 range
    double phi1 = fmodf( phase *  harmI        + 1 + 1000, 2.0 ) - 1.0;
    double phi2 = fmodf( phase * ( harmI + 1 ) + 1 + 1000, 2.0 ) - 1.0;

    // calculate the two carriers
    double carrier1 = fast_cos( phi1 );
    double carrier2 = fast_cos( phi2 );

    // return interpolation between the two carriers
    return carrier1 + harmF * ( carrier2 - carrier1 );
}

double FormantFilter::compress(double sample)
{
    double a, b, i, j, g, e=env, e2=env2, ra=rat, re= (1.f-rel), at=att, ga=gatt;
    double tr=trim, th=thr, lth=lthr, xth=xthr, ge=genv, y =dry;
    double out;

    if (mode) //comp/gate/lim
    {
        if (lth==0.f) lth=1000.f;
        a = sample;
        i = (a<0.f)? -a : a;

        e = (i>e)? e + at * (i - e) : e * re;
        e2 = (i>e)? i : e2 * re; //ir;

        g = (e>th)? tr / (1.f + ra * ((e / th) - 1.f)) : tr;

        if (g<0.f) g=0.f;
        if (g*e2>lth) g = lth/e2; //limit

        ge = (e>xth)? ge + ga - ga * ge : ge * xrat; //gate

        out = a * (g * ge + y);
    }
    else //compressor only
    {
        a = sample;
        i = (a<0.f)? -a : a;

        e = (i>e)? e + at * (i - e) : e * re; //envelope
        g = (e>th)? tr / (1.f + ra * ((e / th) - 1.f)) : tr; //gain

        out = a * (g + y); //vca
    }
    if (e <1.0e-10) env =0.f; else env =e;
    if (e2<1.0e-10) env2=0.f; else env2=e2;
    if (ge<1.0e-10) genv=0.f; else genv=ge;

    return out;
}

void FormantFilter::recalculate ()
{
    // make static
    double threshold = 0.10;
    double ratio = 0.50;
    double level = 0.65;
    double attack = 0.18; 
    double release = 0.55;
    double limiterThreshold = 0.99;
    double gateThreshold = 0.02;
    double gateAttack = 0.10;
    double gateDecay = 0.50;
    double fxMix = 1.00;
    
    mode = 0;
    thr  = pow( 10.0, ( 2.0 * threshold - 2.0 ));
    rat  = 2.5 * ratio - 0.5;
    if ( rat > 1.0 ) {
        rat = 1.f + 16.f*(rat-1.f) * (rat - 1.f);
        mode = 1;
    }
    if ( rat < 0.0 ) {
        rat = 0.6f * rat;
        mode = 1;
    }
    trim = pow( 10.0,( 2.0 * level ));
    att  = pow( 10.0,( -0.002 - 2.0 * attack ));
    rel  = pow( 10.0,( -2.0 - 3.0 * release ));
    
    // limiter
    
    if ( limiterThreshold > 0.98 ) {
        lthr = 0.f;
    }
    else {
        lthr = 0.99 * pow( 10.0, int( 30.0 * limiterThreshold - 20.0 ) / 20.f );
        mode = 1;
    }
    
    // expander
    
    if ( gateThreshold < 0.02 ) {
        xthr = 0.f;
    }
    else {
        xthr = pow( 10.f, ( 3.0 * gateThreshold - 3.0 ));
        mode = 1;
    }
    xrat = 1.0 - pow( 10.f, ( -2.0 - 3.3 * gateDecay ));
    irel = pow( 10.0, -2.0 / _sampleRate );
    gatt = pow( 10.0, (-0.002 - 3.0 * gateAttack ));
    
    if ( rat < 0.0f && thr < 0.1f ) {
        rat *= thr * 15.f;
    }
    dry   = 1.0f - fxMix; 
    trim *= fxMix;
}

}
