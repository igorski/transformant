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
#include "../global.h"
#include "controller.h"
#include "uimessagecontroller.h"
#include "../paramids.h"

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "base/source/fstring.h"

#include "vstgui/uidescription/delegationcontroller.h"

#include <stdio.h>
#include <math.h>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// Controller Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API Controller::initialize( FUnknown* context )
{
    tresult result = EditControllerEx1::initialize( context );

    if ( result != kResultOk )
        return result;

    //--- Create Units-------------
    UnitInfo unitInfo;
    Unit* unit;

    // create root only if you want to use the programListId
    /*	unitInfo.id = kRootUnitId;	// always for Root Unit
    unitInfo.parentUnitId = kNoParentUnitId;	// always for Root Unit
    Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name)).assign (USTRING ("Root"));
    unitInfo.programListId = kNoProgramListId;

    unit = new Unit (unitInfo);
    addUnitInfo (unit);*/

    // create a unit1
    unitInfo.id = 1;
    unitInfo.parentUnitId = kRootUnitId;    // attached to the root unit

    Steinberg::UString( unitInfo.name, USTRINGSIZE( unitInfo.name )).assign( USTRING( "FormantPlaceholder" ));

    unitInfo.programListId = kNoProgramListId;

    unit = new Unit( unitInfo );
    addUnit( unit );
    int32 unitId = 1;

    // Formant filter controls

    parameters.addParameter( new RangeParameter(
        USTRING( "Vowel L" ), kVowelLId, USTRING( "0 - 1" ),
        0.f, 1.f, 0.f,
        0, ParameterInfo::kCanAutomate, unitId
    ));

    parameters.addParameter( new RangeParameter(
        USTRING( "Vowel R" ), kVowelRId, USTRING( "0 - 1" ),
        0.f, 1.f, 0.f,
        0, ParameterInfo::kCanAutomate, unitId
    ));

    parameters.addParameter( new RangeParameter(
        USTRING( "Vowel Sync" ), 0, 1, 0, ParameterInfo::kCanAutomate, kVowelSyncId, unitId
    ));

    // LFO controls

    parameters.addParameter( new RangeParameter(
        USTRING( "Vowel L LFO rate" ), kLFOVowelLId, USTRING( "Hz" ),
        Igorski::VST::MIN_LFO_RATE(), Igorski::VST::MAX_LFO_RATE(), Igorski::VST::MIN_LFO_RATE(),
        0, ParameterInfo::kCanAutomate, unitId
    ));

    parameters.addParameter( new RangeParameter(
        USTRING( "Vowel R LFO rate" ), kLFOVowelRId, USTRING( "Hz" ),
        Igorski::VST::MIN_LFO_RATE(), Igorski::VST::MAX_LFO_RATE(), Igorski::VST::MIN_LFO_RATE(),
        0, ParameterInfo::kCanAutomate, unitId
    ));

    parameters.addParameter( new RangeParameter(
        USTRING( "Vowel L LFO depth" ), kLFOVowelLDepthId, USTRING( "%" ),
        0.f, 1.f, 0.f,
        0, ParameterInfo::kCanAutomate, unitId
    ));

    parameters.addParameter( new RangeParameter(
        USTRING( "Vowel R LFO depth" ), kLFOVowelRDepthId, USTRING( "%" ),
        0.f, 1.f, 0.f,
        0, ParameterInfo::kCanAutomate, unitId
    ));

    // distortion controls

    parameters.addParameter( new RangeParameter(
        USTRING( "Distortion Type" ), 0, 1, 0, ParameterInfo::kCanAutomate, kDistortionType, unitId
    ));

    parameters.addParameter( new RangeParameter(
        USTRING( "Drive" ), kDriveId, USTRING( "0 - 1" ),
        0.f, 1.f, 0.f,
        0, ParameterInfo::kCanAutomate, unitId
    ));

    parameters.addParameter(
        USTRING( "Distortion pre/post" ), 0, 1, 0, ParameterInfo::kCanAutomate, kDistortionChainId, unitId
    );

    // initialization

    String str( "FORMANTPLACEHOLDER" );
    str.copyTo16( defaultMessageText, 0, 127 );

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::terminate()
{
    return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::setComponentState( IBStream* state )
{
    // we receive the current state of the component (processor part)
    if ( state )
    {
        float savedVowelL = 1.f;
        if ( state->read( &savedVowelL, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedVowelR = 1.f;
        if ( state->read( &savedVowelR, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedVowelSync = 1.f;
        if ( state->read( &savedVowelSync, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedLFOVowelL = Igorski::VST::MIN_LFO_RATE();
        if ( state->read( &savedLFOVowelL, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedLFOVowelR = Igorski::VST::MIN_LFO_RATE();
        if ( state->read( &savedLFOVowelR, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedLFOVowelLDepth = 1.f;
        if ( state->read( &savedLFOVowelLDepth, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedLFOVowelRDepth = 1.f;
        if ( state->read( &savedLFOVowelRDepth, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedDistortionType = 1.f;
        if ( state->read( &savedDistortionType, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedDrive = 1.f;
        if ( state->read( &savedDrive, sizeof( float )) != kResultOk )
            return kResultFalse;

        float savedDistortionChain = 0.f;
        if ( state->read( &savedDistortionChain, sizeof( float )) != kResultOk )
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

        setParamNormalized( kVowelLId,          savedVowelL );
        setParamNormalized( kVowelRId,          savedVowelR );
        setParamNormalized( kVowelSyncId,       savedVowelSync );
        setParamNormalized( kLFOVowelLId,       savedLFOVowelL );
        setParamNormalized( kLFOVowelRId,       savedLFOVowelR );
        setParamNormalized( kLFOVowelLDepthId,  savedLFOVowelLDepth );
        setParamNormalized( kLFOVowelRDepthId,  savedLFOVowelRDepth );
        setParamNormalized( kDistortionType,    savedDistortionType );
        setParamNormalized( kDriveId,           savedDrive );
        setParamNormalized( kDistortionChainId, savedDistortionChain );

        state->seek( sizeof ( float ), IBStream::kIBSeekCur );
    }
    return kResultOk;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API Controller::createView( const char* name )
{
    // create the visual editor
    if ( name && strcmp( name, "editor" ) == 0 )
    {
        VST3Editor* view = new VST3Editor( this, "view", "formantplaceholder.uidesc" );
        return view;
    }
    return 0;
}

//------------------------------------------------------------------------
IController* Controller::createSubController( UTF8StringPtr name,
                                                    const IUIDescription* /*description*/,
                                                    VST3Editor* /*editor*/ )
{
    if ( UTF8StringView( name ) == "MessageController" )
    {
        UIMessageController* controller = new UIMessageController( this );
        addUIMessageController( controller );
        return controller;
    }
    return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::setState( IBStream* state )
{
    tresult result = kResultFalse;

    int8 byteOrder;
    if (( result = state->read( &byteOrder, sizeof( int8 ))) != kResultTrue )
        return result;

    if (( result = state->read( defaultMessageText, 128 * sizeof( TChar ))) != kResultTrue )
        return result;

    // if the byteorder doesn't match, byte swap the text array ...
    if ( byteOrder != BYTEORDER )
    {
        for ( int32 i = 0; i < 128; i++ )
            SWAP_16( defaultMessageText[ i ])
    }

    // update our editors
    for ( UIMessageControllerList::iterator it = uiMessageControllers.begin (), end = uiMessageControllers.end (); it != end; ++it )
        ( *it )->setMessageText( defaultMessageText );

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::getState( IBStream* state )
{
    // here we can save UI settings for example

    // as we save a Unicode string, we must know the byteorder when setState is called
    int8 byteOrder = BYTEORDER;
    if ( state->write( &byteOrder, sizeof( int8 )) == kResultTrue )
    {
        return state->write( defaultMessageText, 128 * sizeof( TChar ));
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult Controller::receiveText( const char* text )
{
    // received from Component
    if ( text )
    {
        fprintf( stderr, "[Controller] received: " );
        fprintf( stderr, "%s", text );
        fprintf( stderr, "\n" );
    }
    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::setParamNormalized( ParamID tag, ParamValue value )
{
    // called from host to update our parameters state
    tresult result = EditControllerEx1::setParamNormalized( tag, value );
    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::getParamStringByValue( ParamID tag, ParamValue valueNormalized, String128 string )
{
    switch ( tag )
    {
        // these controls are floating point values in 0 - 1 range, we can
        // simply read the normalized value which is in the same range

        case kVowelLId:
        case kVowelRId:
        case kVowelSyncId:
        case kLFOVowelLDepthId:
        case kLFOVowelRDepthId:
        case kDistortionTypeId:
        case kDriveId:
        case kDistortionChainId:
        {
            char text[32];

            switch ( tag ) {
                default:
                    sprintf( text, "%.2f", ( float ) valueNormalized );
                    break;

                case kVowelSync:
                    sprintf( text, "%s", ( valueNormalized == 0 ) ? "Off": "On" );
                    break;

                case kDistortionTypeId:
                    sprintf( text, "%s", ( valueNormalized == 0 ) ? "Waveshaper": "Bitcrusher" );
                    break;

                case kDistortionChainId:
                    sprintf( text, "%s", ( valueNormalized == 0 ) ? "Pre-formant mix" : "Post-formant mix" );
                    break;
            }
            Steinberg::UString( string, 128 ).fromAscii( text );

            return kResultTrue;
        }

        // vowel LFO setting is also floating point but in a custom range
        // request the plain value from the normalized value

        case kLFOVowelLId:
        case kLFOVowelRId:
        {
            char text[32];
            if (valueNormalized == 0 )
                sprintf( text, "%s", "Off" );
            else
                sprintf( text, "%.2f", normalizedParamToPlain( tag, valueNormalized ));
            Steinberg::UString( string, 128 ).fromAscii( text );

            return kResultTrue;
        }

        // everything else
        default:
            return EditControllerEx1::getParamStringByValue( tag, valueNormalized, string );
    }
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::getParamValueByString( ParamID tag, TChar* string, ParamValue& valueNormalized )
{
    /* example, but better to use a custom Parameter as seen in RangeParameter
    switch (tag)
    {
        case kAttackId:
        {
            Steinberg::UString wrapper ((TChar*)string, -1); // don't know buffer size here!
            double tmp = 0.0;
            if (wrapper.scanFloat (tmp))
            {
                valueNormalized = expf (logf (10.f) * (float)tmp / 20.f);
                return kResultTrue;
            }
            return kResultFalse;
        }
    }*/
    return EditControllerEx1::getParamValueByString( tag, string, valueNormalized );
}

//------------------------------------------------------------------------
void Controller::addUIMessageController( UIMessageController* controller )
{
    uiMessageControllers.push_back( controller );
}

//------------------------------------------------------------------------
void Controller::removeUIMessageController( UIMessageController* controller )
{
    UIMessageControllerList::const_iterator it = std::find(
        uiMessageControllers.begin(), uiMessageControllers.end (), controller
    );
    if ( it != uiMessageControllers.end())
        uiMessageControllers.erase( it );
}

//------------------------------------------------------------------------
void Controller::setDefaultMessageText( String128 text )
{
    String tmp( text );
    tmp.copyTo16( defaultMessageText, 0, 127 );
}

//------------------------------------------------------------------------
TChar* Controller::getDefaultMessageText()
{
    return defaultMessageText;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::queryInterface( const char* iid, void** obj )
{
    QUERY_INTERFACE( iid, obj, IMidiMapping::iid, IMidiMapping );
    return EditControllerEx1::queryInterface( iid, obj );
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::getMidiControllerAssignment( int32 busIndex, int16 /*midiChannel*/,
    CtrlNumber midiControllerNumber, ParamID& tag )
{
    // we support for the Gain parameter all MIDI Channel but only first bus (there is only one!)
/*
    if ( busIndex == 0 && midiControllerNumber == kCtrlVolume )
    {
        tag = kDelayTimeId;
        return kResultTrue;
    }
*/
    return kResultFalse;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
