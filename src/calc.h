/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018-2020 Igor Zinken - https://www.igorski.nl
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
#ifndef __CALC_HEADER__
#define __CALC_HEADER__

#include <cmath>
#include <algorithm>
#include "global.h"

#define undenormaliseFloat(sample) ((((*(uint32 *)&(sample))&0x7f800000)==0)&&((sample)!=0.f))
#define undenormaliseDouble(sample) ((((((uint32 *)&(sample))[1])&0x7fe00000)==0)&&((sample)!=0.))

/**
 * convenience utilities to process values
 * common to the VST plugin context
 */
namespace Igorski {
namespace Calc {

    /**
     * convert given value in seconds to the appropriate
     * value in samples (for the current sampling rate)
     */
    inline int secondsToBuffer( float seconds, float sampleRate )
    {
        return static_cast<int>( seconds * sampleRate );
    }

    /**
     * convert given value in milliseconds to the appropriate
     * value in samples (for the current sampling rate)
     */
    inline int millisecondsToBuffer( float milliseconds, float sampleRate )
    {
        return secondsToBuffer( milliseconds / 1000.f, sampleRate );
    }

    inline float millisecondsToCoeff( float milliseconds, float sampleRate )
    {
        float timeInSeconds = milliseconds / 1000.f;
        return std::exp( -1.0f / ( timeInSeconds * sampleRate ));
    }

    // convenience method to ensure given value is within the 0.f - +1.f range

    inline float cap( float value )
    {
        return std::min( 1.f, std::max( 0.f, value ));
    }

    // convenience method to ensure a sample is in the valid -1.f - +1.f range

    inline float capSample( float value )
    {
        return std::min( 1.f, std::max( -1.f, value ));
    }

    // convenience method to round given number value to the nearest
    // multiple of valueToRoundTo
    // e.g. roundTo( 236.32, 10 ) == 240 and roundTo( 236.32, 5 ) == 235

    inline float roundTo( float value, float valueToRoundTo )
    {
        float resto = fmod( value, valueToRoundTo );

        if ( resto <= ( valueToRoundTo / 2.f )) {
            return value - resto;
        }
        return value + valueToRoundTo - resto;
    }

    // inverts a 0 - 1 normalized min-to-max value to have 0 be the max and 1 the min

    inline float inverseNormalize( float value )
    {
        return ( 1.f - value ) / 1.f;
    }

    // convenience method to scale given value and its expected maxValue against
    // an arbitrary range (defined by maxCompareValue in relation to maxValue)

    inline float scale( float value, float maxValue, float maxCompareValue )
    {
        float ratio = maxCompareValue / maxValue;
        return std::min( maxValue, value ) * ratio;
    }

    inline float constrain( float lowerLimit, float upperLimit, float valueToConstrain )
    {
        if ( valueToConstrain < lowerLimit ) {
            return lowerLimit;
        }
        return upperLimit < valueToConstrain ? upperLimit : valueToConstrain;
    }

    // cast a floating point value to a boolean true/false

    inline bool toBool( float value )
    {
        return value >= .5;
    }
}
}

#endif
