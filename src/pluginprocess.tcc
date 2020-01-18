/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Igor Zinken - https://www.igorski.nl
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
namespace Igorski
{
template <typename SampleType>
void PluginProcess::process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
                             int bufferSize, uint32 sampleFramesSize ) {

    // prepare the mix buffers and clone the incoming buffer contents into the pre-mix buffer

    prepareMixBuffers( inBuffer, numInChannels, bufferSize );

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        auto channelMixBuffer        = _mixBuffer->getBufferForChannel( c );

        // pre formant filter bit crusher processing

        if ( !distortionPostMix ) {
            if ( distortionTypeCrusher ) {
                bitCrusher->process( channelMixBuffer, bufferSize );
            } else {
                waveShaper->process( channelMixBuffer, bufferSize );
            }
        }

        // formant filter

        if ( c % 2 == 0 ) {
            formantFilterL->process( channelMixBuffer, bufferSize );
        } else {
           formantFilterR->process( channelMixBuffer, bufferSize );
        }

        // post formant filter bit crusher processing

        if ( distortionPostMix ) {
            if ( distortionTypeCrusher ) {
                bitCrusher->process( channelMixBuffer, bufferSize );
            } else {
                waveShaper->process( channelMixBuffer, bufferSize );
            }
        }

        // write the effected mix buffers into the output buffer
        // note here we convert the double values to whatever SampleType is

        for ( size_t i = 0; i < bufferSize; ++i ) {

            // before writing to the out buffer we take a snapshot of the current in sample
            // value as VST2 in Ableton Live supplies the same buffer for in and out!
            // in case we want to offer a wet/dry balance
            //inSample = channelInBuffer[ i ];

            // wet mix (e.g. the effected signal)
            channelOutBuffer[ i ] = ( SampleType ) channelMixBuffer[ i ];
        }
    }
    // limit the output signal as it can get quite hot
    limiter->process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

template <typename SampleType>
void PluginProcess::prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize )
{
    // if the pre mix buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    if ( _mixBuffer == nullptr || _mixBuffer->bufferSize != bufferSize ) {
        delete _mixBuffer;
        _mixBuffer = new AudioBuffer( numInChannels, bufferSize );
    }

    // clone the in buffer contents
    // note the clone is always cast to double as it is
    // used for internal processing (see PluginProcess::process)

    for ( int c = 0; c < numInChannels; ++c ) {

        SampleType* inChannelBuffer = ( SampleType* ) inBuffer[ c ];
        auto channelMixBuffer       = ( double* ) _mixBuffer->getBufferForChannel( c );

        for ( int i = 0; i < bufferSize; ++i ) {
            // clone into the pre mix buffer for pre-processing
            channelMixBuffer[ i ] = ( double ) inChannelBuffer[ i ];
        }
    }
}

}
