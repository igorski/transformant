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
#include "formantfilter.h"
#include "calc.h"
#include <cmath>

namespace Igorski {

/* constructor / destructor */

/**
 * @param aVowel {float} interpolated value within
 *                       the range of the amount specified in the coeffs Array
 */
FormantFilter::FormantFilter( float aVowel )
{
    memset( _currentCoeffs, 0.0, COEFF_AMOUNT );
    memset( _memory,        0.0, MEMORY_SIZE );

    setVowel( aVowel );

    lfo = new LFO();
    hasLFO = false;
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

    cacheVowel();

    if ( hasLFO ) {
        cacheLFO();
    }
}

void FormantFilter::setLFO( float LFORatePercentage, float LFODepth )
{
    bool wasEnabled = hasLFO;
    bool enabled    = LFORatePercentage > 0.f;

    hasLFO = enabled;

    bool hadChange = ( wasEnabled != enabled ) || _lfoDepth != LFODepth;

    if ( enabled )
        lfo->setRate(
            VST::MIN_LFO_RATE() + (
                LFORatePercentage * ( VST::MAX_LFO_RATE() - VST::MIN_LFO_RATE() )
            )
        );

    // turning LFO off
    if ( !hasLFO && wasEnabled ) {
        _tempVowel = _vowel;
        cacheVowel();
    }

    if ( hadChange ) {
        _lfoDepth = LFODepth;
        cacheLFO();
    }
}

void FormantFilter::process( float* inBuffer, int bufferSize )
{
    size_t j, k;
    double res;
    float out;

    for ( size_t i = 0; i < bufferSize; ++i )
    {
        res = _currentCoeffs[ 0 ] * inBuffer[ i ];

        for ( j = 1, k = 0; j < COEFF_AMOUNT; ++j, ++k ) {
            res += _currentCoeffs[ j ] * _memory[ k ];
        }

        j = MEMORY_SIZE;
        while ( j-- > 1 ) {
            _memory[ j ] = _memory[ j - 1 ];
        }

        //res = ((fabs(res) > 1.0e-10 && res < 1.0) || res < -1.0e-10 && res > -1.0) && !isinf(res) ? res : 0.0f;
        undenormaliseDouble( res );

        _memory[ 0 ] = res;

        // write output

        inBuffer[ i ] = ( float ) _memory[ 0 ];


        // if LFO is active, keep it moving

        if ( hasLFO ) {
            float lfoValue = lfo->peek() * .5f  + .5f; // make waveform unipolar
            _tempVowel = std::min( _lfoMax, _lfoMin + _lfoRange * lfoValue );
            cacheVowel();
        }
    }
}

/* private methods */

void FormantFilter::cacheVowel()
{
    _tempVowel;

    // vowels are defined in 0 - 4 range (see COEFFICIENTS)

    double scaledValue = Calc::scale( _tempVowel, 1.f, 4.f );

    // interpolate the value between vowels

    int roundVowel = ( int )( scaledValue );
    double fracpart = INTERPOLATE ? roundVowel - scaledValue : 1.0;

    for ( int i = 0; i < COEFF_AMOUNT; i++ )
    {
        _currentCoeffs[ i ] = fracpart * COEFFICIENTS[ roundVowel ][ i ] +
                              // add next vowel (note the overflow check when roundVowel is 4)
                              ( 1.0 - fracpart ) * COEFFICIENTS[ roundVowel + ( roundVowel < 4 )][ i ];
    }
}

void FormantFilter::cacheLFO()
{
    _lfoRange = _vowel * _lfoDepth;
    _lfoMax   = std::min( 1., _vowel + _lfoRange / 2. );
    _lfoMin   = std::max( 0., _vowel - _lfoRange / 2. );
}

}