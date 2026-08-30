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
#ifndef __PLUGIN_PROCESS__H_INCLUDED__
#define __PLUGIN_PROCESS__H_INCLUDED__

#include "audiobuffer.h"
#include "automakeupgain.h"
#include "bitcrusher.h"
#include "formantfilter.h"
#include "global.h"
#include "limiter.h"
#include "snd.h"
#include "waveshaper.h"
#include <vector>

using namespace Steinberg;

namespace Igorski {
class PluginProcess {

    public:
        PluginProcess( int amountOfChannels, float sampleRate, int maxBufferSize );
        ~PluginProcess();

        void setHostProperties( float sampleRate, int maxBufferSize );

        // apply effect to incoming sampleBuffer contents

        template <typename SampleType>
        void process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
            int bufferSize, uint32 sampleFramesSize
        );

        // for a speed improvement we don't actually iterate over all channels, but assume
        // that if the first channel is empty, all are.

        inline bool isBufferSilent( float** buffer, int numChannels, int bufferSize ) {
            auto channelBuffer = buffer[ 0 ];
            for ( int32 i = 0; i < bufferSize; ++i ) {
                if ( channelBuffer[ i ] != 0.f ) {
                    return false;
                }
            }
            return true;
        };

        inline bool isBufferSilent( double** buffer, int numChannels, int bufferSize ) {
            auto channelBuffer = buffer[ 0 ];
            for ( int32 i = 0; i < bufferSize; ++i ) {
                if ( channelBuffer[ i ] != 0.0 ) {
                    return false;
                }
            }
            return true;
        };

        BitCrusher bitCrusher;
        WaveShaper waveShaper;
        Limiter limiter;
        FormantFilter formantFilterL;
        FormantFilter formantFilterR;

        // whether effects are applied onto the input delay signal or onto
        // the delayed signal itself (false = on input, true = on delay)

        bool distortionPostMix     = false;
        bool distortionTypeCrusher = false;

        inline bool hasLFO() {
            return formantFilterL.hasLFO || formantFilterR.hasLFO;
        }

    private:
        std::vector<AutoMakeUpGain> _makeUpGainProcessors;
        AudioBuffer* _mixBuffer;  // buffer used for the sample process mixing
        float* _scratchBuffer = nullptr; // used for make-up gain processing (reused per channel)
        
        int _amountOfChannels = 0;
        int _hostBufferSize = 0;
        float _sampleRate;

        // ensures the pre- and post mix buffers match the appropriate amount of channels
        // and buffer size. this also clones the contents of given in buffer into the pre-mix buffer
        // the buffers are pooled so this can be called upon each process cycle without allocation overhead

        template <typename SampleType>
        void prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize );

};
}

#include "pluginprocess.tcc"

#endif
