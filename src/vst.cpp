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
#include "global.h"
#include "vst.h"
#include "paramids.h"
#include "calc.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include <stdio.h>

namespace Igorski {

//------------------------------------------------------------------------
// Transformant Implementation
//------------------------------------------------------------------------
Transformant::Transformant()
: fVowelL( 0.f )
, fVowelR( 0.f )
, fVowelSync( 1.f )
, fLFOVowelL( 0.f )
, fLFOVowelR( 0.f )
, fLFOVowelLDepth( 0.5f )
, fLFOVowelRDepth( 0.5f )
, fDistortionType( 0.f )
, fDrive( 0.f )
, fDistortionChain( 0.f )
, pluginProcess( nullptr )
, outputGainOld( 0.f )
, currentProcessMode( -1 ) // -1 means not initialized
{
    // register its editor class (the same as used in vstentry.cpp)
    setControllerClass( VST::ControllerUID );

    // should be created on setupProcessing, this however doesn't fire for Audio Unit using auval?
    pluginProcess = new PluginProcess( 2 );
}

//------------------------------------------------------------------------
Transformant::~Transformant()
{
    // free all allocated resources
    delete pluginProcess;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::initialize( FUnknown* context )
{
    //---always initialize the parent-------
    tresult result = AudioEffect::initialize( context );
    // if everything Ok, continue
    if ( result != kResultOk )
        return result;

    //---create Audio In/Out buses------
    addAudioInput ( STR16( "Stereo In" ),  SpeakerArr::kStereo );
    addAudioOutput( STR16( "Stereo Out" ), SpeakerArr::kStereo );

    //---create Event In/Out buses (1 bus with only 1 channel)------
    addEventInput( STR16( "Event In" ), 1 );

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::terminate()
{
    // nothing to do here yet...except calling our parent terminate
    return AudioEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::setActive (TBool state)
{
    if (state)
        sendTextMessage( "Transformant::setActive (true)" );
    else
        sendTextMessage( "Transformant::setActive (false)" );

    // reset output level meter
    outputGainOld = 0.f;

    // call our parent setActive
    return AudioEffect::setActive( state );
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::process( ProcessData& data )
{
    // In this example there are 4 steps:
    // 1) Read inputs parameters coming from host (in order to adapt our model values)
    // 2) Read inputs events coming from host (note on/off events)
    // 3) Apply the effect using the input buffer into the output buffer

    //---1) Read input parameter changes-----------
    IParameterChanges* paramChanges = data.inputParameterChanges;
    if ( paramChanges )
    {
        int32 numParamsChanged = paramChanges->getParameterCount();
        // for each parameter which are some changes in this audio block:
        for ( int32 i = 0; i < numParamsChanged; i++ )
        {
            IParamValueQueue* paramQueue = paramChanges->getParameterData( i );
            if ( paramQueue )
            {
                ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();
                switch ( paramQueue->getParameterId())
                {
                    // we use in this example only the last point of the queue.
                    // in some wanted case for specific kind of parameter it makes sense to retrieve all points
                    // and process the whole audio block in small blocks.

                    case kVowelLId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fVowelL = ( float ) value;
                        break;

                    case kVowelRId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fVowelR = ( float ) value;
                        break;

                    case kVowelSyncId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fVowelSync = ( float ) value;
                        break;

                    case kLFOVowelLId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fLFOVowelL = ( float ) value;
                        break;

                    case kLFOVowelRId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fLFOVowelR = ( float ) value;
                        break;

                    case kLFOVowelLDepthId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fLFOVowelLDepth = ( float ) value;
                        break;

                    case kLFOVowelRDepthId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fLFOVowelRDepth = ( float ) value;
                        break;

                    case kDistortionTypeId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fDistortionType = ( float ) value;
                        break;

                    case kDriveId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fDrive = ( float ) value;
                        break;

                    case kDistortionChainId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fDistortionChain = ( float ) value;
                        break;
                }
                syncModel();
            }
        }
    }

    //---2) Read input events-------------
//    IEventList* eventList = data.inputEvents;

    //-------------------------------------
    //---3) Process Audio---------------------
    //-------------------------------------

    if ( data.numInputs == 0 || data.numOutputs == 0 )
    {
        // nothing to do
        return kResultOk;
    }

    int32 numInChannels  = data.inputs[ 0 ].numChannels;
    int32 numOutChannels = data.outputs[ 0 ].numChannels;

    // --- get audio buffers----------------
    uint32 sampleFramesSize = getSampleFramesSizeInBytes( processSetup, data.numSamples );
    void** in  = getChannelBuffersPointer( processSetup, data.inputs [ 0 ] );
    void** out = getChannelBuffersPointer( processSetup, data.outputs[ 0 ] );

    // process the incoming sound!

    bool isDoublePrecision = ( data.symbolicSampleSize == kSample64 );

    if ( isDoublePrecision ) {
        // 64-bit samples, e.g. Reaper64
        pluginProcess->process<double>(
            ( double** ) in, ( double** ) out, numInChannels, numOutChannels,
            data.numSamples, sampleFramesSize
        );
    }
    else {
        // 32-bit samples, e.g. Ableton Live, Bitwig Studio... (oddly enough also when 64-bit?)
        pluginProcess->process<float>(
            ( float** ) in, ( float** ) out, numInChannels, numOutChannels,
            data.numSamples, sampleFramesSize
        );
    }

    // output flags

    data.outputs[ 0 ].silenceFlags = false; // there should always be output
    float outputGain = pluginProcess->limiter->getLinearGR();

    //---4) Write output parameter changes-----------
    IParameterChanges* outParamChanges = data.outputParameterChanges;
    // a new value of VuMeter will be sent to the host
    // (the host will send it back in sync to our controller for updating our editor)
    if ( !isDoublePrecision && outParamChanges && outputGainOld != outputGain ) {
        int32 index = 0;
        IParamValueQueue* paramQueue = outParamChanges->addParameterData( kVuPPMId, index );
        if ( paramQueue )
            paramQueue->addPoint( 0, outputGain, index );
    }
    outputGainOld = outputGain;

    return kResultOk;
}

//------------------------------------------------------------------------
tresult Transformant::receiveText( const char* text )
{
    // received from Controller
    fprintf( stderr, "[Transformant] received: " );
    fprintf( stderr, "%s", text );
    fprintf( stderr, "\n" );

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::setState( IBStream* state )
{
    // called when we load a preset, the model has to be reloaded

    float savedVowelL = 0.f;
    if ( state->read( &savedVowelL, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedVowelR = 0.f;
    if ( state->read( &savedVowelR, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedVowelSync = 0.f;
    if ( state->read( &savedVowelSync, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedLFOVowelL = 0.f;
    if ( state->read( &savedLFOVowelL, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedLFOVowelR = 0.f;
    if ( state->read( &savedLFOVowelR, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedLFOVowelLDepth = 0.f;
    if ( state->read( &savedLFOVowelLDepth, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedLFOVowelRDepth = 0.f;
    if ( state->read( &savedLFOVowelRDepth, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedDistortionType = 0.f;
    if ( state->read( &savedDistortionType, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedDrive = 0.f;
    if ( state->read( &savedDrive, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedDistortionChain = 0.f;
    if ( state->read( &savedDistortionChain, sizeof ( float )) != kResultOk )
        return kResultFalse;

#if BYTEORDER == kBigEndian
    SWAP32( savedVowelL )
    SWAP32( savedVowelR )
    SWAP32( savedVowelSync )
    SWAP32( savedLFOVowelL )
    SWAP32( savedLFOVowelR )
    SWAP32( savedLFOVowelLDepth )
    SWAP32( savedLFOVowelRDepth )
    SWAP32( savedDistortionType )
    SWAP32( savedDrive )
    SWAP32( savedDistortionChain )
#endif

    fVowelL          = savedVowelL;
    fVowelR          = savedVowelR;
    fVowelSync       = savedVowelSync;
    fLFOVowelL       = savedLFOVowelL;
    fLFOVowelR       = savedLFOVowelR;
    fLFOVowelLDepth  = savedLFOVowelLDepth;
    fLFOVowelRDepth  = savedLFOVowelRDepth;
    fDistortionType  = savedDistortionType;
    fDrive           = savedDrive;
    fDistortionChain = savedDistortionChain;

    syncModel();

    // Example of using the IStreamAttributes interface
    FUnknownPtr<IStreamAttributes> stream (state);
    if ( stream )
    {
        IAttributeList* list = stream->getAttributes ();
        if ( list )
        {
            // get the current type (project/Default..) of this state
            String128 string = {0};
            if ( list->getString( PresetAttributes::kStateType, string, 128 * sizeof( TChar )) == kResultTrue )
            {
                UString128 tmp( string );
                char ascii[128];
                tmp.toAscii( ascii, 128 );
                if ( !strncmp( ascii, StateType::kProject, strlen( StateType::kProject )))
                {
                    // we are in project loading context...
                }
            }

            // get the full file path of this state
            TChar fullPath[1024];
            memset( fullPath, 0, 1024 * sizeof( TChar ));
            if ( list->getString( PresetAttributes::kFilePathStringType,
                 fullPath, 1024 * sizeof( TChar )) == kResultTrue )
            {
                // here we have the full path ...
            }
        }
    }
    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::getState( IBStream* state )
{
    // here we need to save the model

    float toSaveVowelL          = fVowelL;
    float toSaveVowelR          = fVowelR;
    float toSaveVowelSync       = fVowelSync;
    float toSaveLFOVowelL       = fLFOVowelL;
    float toSaveLFOVowelR       = fLFOVowelR;
    float toSaveLFOVowelLDepth  = fLFOVowelLDepth;
    float toSaveLFOVowelRDepth  = fLFOVowelRDepth;
    float toSaveDistortionType  = fDistortionType;
    float toSaveDrive           = fDrive;
    float toSaveDistortionChain = fDistortionChain;

#if BYTEORDER == kBigEndian
    SWAP32( toSaveVowelL );
    SWAP32( toSaveVowelR );
    SWAP32( toSaveVowelSync );
    SWAP32( toSaveLFOVowelL );
    SWAP32( toSaveLFOVowelR );
    SWAP32( toSaveLFOVowelLDepth );
    SWAP32( toSaveLFOVowelRDepth );
    SWAP32( toSaveDistortionType );
    SWAP32( toSaveDrive );
    SWAP32( toSaveDriveDepth );
#endif

    state->write( &toSaveVowelL         , sizeof( float ));
    state->write( &toSaveVowelR         , sizeof( float ));
    state->write( &toSaveVowelSync      , sizeof( float ));
    state->write( &toSaveLFOVowelL      , sizeof( float ));
    state->write( &toSaveLFOVowelR      , sizeof( float ));
    state->write( &toSaveLFOVowelLDepth , sizeof( float ));
    state->write( &toSaveLFOVowelRDepth , sizeof( float ));
    state->write( &toSaveDistortionType , sizeof( float ));
    state->write( &toSaveDrive          , sizeof( float ));
    state->write( &toSaveDistortionChain, sizeof( float ));

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::setupProcessing( ProcessSetup& newSetup )
{
    // called before the process call, always in a disabled state (not active)

    // here we keep a trace of the processing mode (offline,...) for example.
    currentProcessMode = newSetup.processMode;

    VST::SAMPLE_RATE = newSetup.sampleRate;

    // spotted to fire multiple times...

    if ( pluginProcess != nullptr )
        delete pluginProcess;

    // TODO: creating a bunch of extra channels for no apparent reason?
    // get the correct channel amount and don't allocate more than necessary...
    pluginProcess = new PluginProcess( 6 );

    syncModel();

    return AudioEffect::setupProcessing( newSetup );
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::setBusArrangements( SpeakerArrangement* inputs,  int32 numIns,
                                               SpeakerArrangement* outputs, int32 numOuts )
{
    if ( numIns == 1 && numOuts == 1 )
    {
        // the host wants Mono => Mono (or 1 channel -> 1 channel)
        if ( SpeakerArr::getChannelCount( inputs[0])  == 1 &&
             SpeakerArr::getChannelCount( outputs[0]) == 1 )
        {
            AudioBus* bus = FCast<AudioBus>( audioInputs.at( 0 ));
            if ( bus )
            {
                // check if we are Mono => Mono, if not we need to recreate the buses
                if ( bus->getArrangement() != inputs[0])
                {
                    removeAudioBusses();
                    addAudioInput ( STR16( "Mono In" ),  inputs[0] );
                    addAudioOutput( STR16( "Mono Out" ), inputs[0] );
                }
                return kResultOk;
            }
        }
        // the host wants something else than Mono => Mono, in this case we are always Stereo => Stereo
        else
        {
            AudioBus* bus = FCast<AudioBus>( audioInputs.at(0));
            if ( bus )
            {
                tresult result = kResultFalse;

                // the host wants 2->2 (could be LsRs -> LsRs)
                if ( SpeakerArr::getChannelCount(inputs[0]) == 2 && SpeakerArr::getChannelCount( outputs[0]) == 2 )
                {
                    removeAudioBusses();
                    addAudioInput  ( STR16( "Stereo In"),  inputs[0] );
                    addAudioOutput ( STR16( "Stereo Out"), outputs[0]);
                    result = kResultTrue;
                }
                // the host want something different than 1->1 or 2->2 : in this case we want stereo
                else if ( bus->getArrangement() != SpeakerArr::kStereo )
                {
                    removeAudioBusses();
                    addAudioInput ( STR16( "Stereo In"),  SpeakerArr::kStereo );
                    addAudioOutput( STR16( "Stereo Out"), SpeakerArr::kStereo );
                    result = kResultFalse;
                }
                return result;
            }
        }
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::canProcessSampleSize( int32 symbolicSampleSize )
{
    if ( symbolicSampleSize == kSample32 )
        return kResultTrue;

    // we support double processing
    if ( symbolicSampleSize == kSample64 )
        return kResultTrue;

    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Transformant::notify( IMessage* message )
{
    if ( !message )
        return kInvalidArgument;

    if ( !strcmp( message->getMessageID(), "BinaryMessage" ))
    {
        const void* data;
        uint32 size;
        if ( message->getAttributes ()->getBinary( "MyData", data, size ) == kResultOk )
        {
            // we are in UI thread
            // size should be 100
            if ( size == 100 && ((char*)data)[1] == 1 ) // yeah...
            {
                fprintf( stderr, "[Transformant] received the binary message!\n" );
            }
            return kResultOk;
        }
    }

    return AudioEffect::notify( message );
}

void Transformant::syncModel()
{
    pluginProcess->distortionPostMix     = Calc::toBool( fDistortionChain );
    pluginProcess->distortionTypeCrusher = Calc::toBool( fDistortionType );
    pluginProcess->bitCrusher->setAmount( fDrive );
    pluginProcess->waveShaper->setAmount( fDrive );

    pluginProcess->formantFilterL->setVowel( fVowelL );

    if ( Calc::toBool( fVowelSync )) {
        pluginProcess->formantFilterR->setVowel( fVowelL );
    } else {
        pluginProcess->formantFilterR->setVowel( fVowelR );
    }
    pluginProcess->formantFilterL->setLFO( fLFOVowelL, fLFOVowelLDepth );
    pluginProcess->formantFilterR->setLFO( fLFOVowelR, fLFOVowelRDepth );
}

}
