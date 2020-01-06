/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Igor Zinken - https://www.igorski.nl
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
#include "waveshaper.h"
#include <cmath>

namespace MWEngine {

// constructor

WaveShaper::WaveShaper( float amount, float level )
{
    setAmount( amount );
    setLevel ( level );
}

/* public methods */

void WaveShaper::process( float* inBuffer, int bufferSize )
{
    for ( int j = 0; j < bufferSize; ++j )
    {
        float input = inBuffer[ j ];

        inBuffer[ j ] = input * ( std::abs( input ) + _amount ) / ( input^2 + ( _amount - 1.f ) * std::abs( _input ) + 1.f );
    }
}

/* getters / setters */

float WaveShaper::getAmount()
{
    return _amount;
}

void WaveShaper::setAmount( float value )
{
    _amount = value;
}

float WaveShaper::getLevel()
{
    return _level;
}

void WaveShaper::setLevel( float value )
{
    _level = value;
}

} // E.O namespace MWEngine