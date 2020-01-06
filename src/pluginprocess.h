/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2020 Igor Zinken - https://www.igorski.nl
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

#include "global.h"
#include "audiobuffer.h"
#include "bitcrusher.h"
#include "formantfilter.h"
#include "limiter.h"
#include <vector>

using namespace Steinberg;

namespace Igorski {
class PluginProcess {

    public:
        PluginProcess( int amountOfChannels );
        ~PluginProcess();

        // apply effect to incoming sampleBuffer contents

        template <typename SampleType>
        void process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
            int bufferSize, uint32 sampleFramesSize
        );

        BitCrusher* bitCrusher;
        Limiter* limiter;
        FormantFilter* formantFilterL;
        FormantFilter* formantFilterR;

        // whether effects are applied onto the input delay signal or onto
        // the delayed signal itself (false = on input, true = on delay)

        bool bitCrusherPostMix;

    private:
        AudioBuffer* _mixBuffer;  // buffer used for the sample process mixing

        int  _amountOfChannels;

        // ensures the pre- and post mix buffers match the appropriate amount of channels
        // and buffer size. this also clones the contents of given in buffer into the pre-mix buffer
        // the buffers are pooled so this can be called upon each process cycle without allocation overhead

        template <typename SampleType>
        void prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize );

};
}

#include "pluginprocess.tcc"

#endif
