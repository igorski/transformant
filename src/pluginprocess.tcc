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
namespace Igorski
{
template <typename SampleType>
void PluginProcess::process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
                             int bufferSize, uint32 sampleFramesSize ) {

    ScopedNoDenormals noDenormals;

    bool mixDry = _dryWetMix < 1.f;

    SampleType dryMix = static_cast<SampleType>( 1.f - _dryWetMix );
    SampleType wetMix = static_cast<SampleType>( _dryWetMix );

    // prepare the mix buffers and clone the incoming buffer contents into the pre-mix buffer

    prepareMixBuffers( inBuffer, numInChannels, bufferSize );

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        auto channelMixBuffer        = _mixBuffer->getBufferForChannel( c );

        std::memcpy( _preBuffer, channelMixBuffer, bufferSize * sizeof( float ));
        
        // pre formant filter distortion processing

        if ( !distortionPostMix ) {
            if ( distortionTypeCrusher ) {
                bitCrusher.process( channelMixBuffer, bufferSize );
            } else {
                waveShaper.process( channelMixBuffer, bufferSize );
            }
        }

        // formant filter

        if ( c % 2 == 0 ) {
            formantFilterL.process( channelMixBuffer, bufferSize );
        } else {
           formantFilterR.process( channelMixBuffer, bufferSize );
        }

        // post formant filter distortion processing

        if ( distortionPostMix ) {
            if ( distortionTypeCrusher ) {
                bitCrusher.process( channelMixBuffer, bufferSize );
            } else {
                waveShaper.process( channelMixBuffer, bufferSize );
            }
        }

        float maxBoost = bitCrusher.isActive() && bitCrusher.getBits() <= 2 ? 0.5f : 4.0f;
        float inSample;

        // apply make-up gain to keep volume balanced between non-bit processed scratch buffer pre mix buffer
        _makeUpGainProcessors[ c ].apply( _preBuffer, channelMixBuffer, bufferSize, maxBoost );

        // write the effected mix buffers into the output buffer

        for ( size_t i = 0; i < bufferSize; ++i ) {

            // before writing to the out buffer we take a snapshot of the current in sample
            // value as VST2 in Ableton Live supplies the same buffer for in and out!
            
            inSample = channelInBuffer[ i ];

            // wet mix (e.g. the effected signal)
            channelOutBuffer[ i ] = static_cast<SampleType>( channelMixBuffer[ i ] ) * wetMix;

            // dry mix (e.g. mix in the input signal)

            if ( mixDry ) {
                channelOutBuffer[ i ] += ( inSample * dryMix );
            }
        }
    }
    // limit the output signal as it can get quite hot
    limiter.process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

template <typename SampleType>
void PluginProcess::prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize )
{
    // clone the in buffer contents

    for ( int c = 0; c < numInChannels; ++c ) {

        SampleType* inChannelBuffer = inBuffer[ c ];
        auto channelMixBuffer = _mixBuffer->getBufferForChannel( c );

        for ( int i = 0; i < bufferSize; ++i ) {
            // clone into the pre mix buffer for pre-processing
            channelMixBuffer[ i ] = static_cast<float>( inChannelBuffer[ i ] );
        }
    }
}

}
