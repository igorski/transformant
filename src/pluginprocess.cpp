/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2026 Igor Zinken - https://www.igorski.nl
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
#include "pluginprocess.h"
#include "calc.h"
#include <math.h>

namespace Igorski {

PluginProcess::PluginProcess( int amountOfChannels, float sampleRate, int maxBufferSize ) {
    _amountOfChannels = amountOfChannels;
    _makeUpGainProcessors.resize( amountOfChannels );

    _mixBuffer = nullptr;
    _preBuffer = nullptr;

    setHostProperties( sampleRate, maxBufferSize );
}

PluginProcess::~PluginProcess() {
    delete _mixBuffer;
    delete[] _preBuffer;
}

/* public methods */

void PluginProcess::setHostProperties( float sampleRate, int maxBufferSize ) {
    bool hadSampleRateChange = _sampleRate != sampleRate;

    _sampleRate = sampleRate;

    if ( hadSampleRateChange ) {
        for ( int c = 0; c < _amountOfChannels; ++c ) {
            _makeUpGainProcessors.at( c ).prepare( _sampleRate );
        }
        formantFilterL.setSampleRate( _sampleRate );
        formantFilterR.setSampleRate( _sampleRate );
    }

    if ( _hostBufferSize < maxBufferSize ) {
        _hostBufferSize = maxBufferSize;

        if ( _mixBuffer != nullptr ) {
            delete _mixBuffer;
        }
        _mixBuffer = new AudioBuffer( _amountOfChannels, _hostBufferSize );

        delete[] _preBuffer;
        _preBuffer = new float[ _hostBufferSize ];
    }
}

void PluginProcess::setDryWetMix( float value )
{
    _dryWetMix = value;
}

}
