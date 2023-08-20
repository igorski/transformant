
cmake_minimum_required(VERSION 3.15.0)

project(smtg-vst3-au
    VERSION 1.0.3.0
    DESCRIPTION "igorski.nl transformant"
)

if(NOT SMTG_ADD_VSTGUI)
    return()
endif()

set(plugin_sources
    src/global.h
    src/audiobuffer.h
    src/audiobuffer.cpp
    src/bitcrusher.h
    src/bitcrusher.cpp
    src/formantfilter.h
    src/formantfilter.cpp
    src/lfo.h
    src/lfo.cpp
    src/limiter.h
    src/limiter.cpp
    src/paramids.h
    src/pluginprocess.h
    src/pluginprocess.cpp
    src/vst.h
    src/vst.cpp
    src/vstentry.cpp
    src/version.h
    src/waveshaper.h
    src/waveshaper.cpp
    src/ui/controller.h
    src/ui/controller.cpp
    src/ui/uimessagecontroller.h
    resource/plugin.uidesc
)
set(target transformant)

smtg_add_vst3plugin(${target} ${plugin_sources})
smtg_target_configure_version_file(${target})

#smtg_add_vst3plugin(${target} PACKAGE_NAME "transformant" SOURCES_LIST ${plugin_sources})
set_target_properties(${target}
    PROPERTIES
        ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER}
)

target_compile_features(${target}
    PUBLIC
        cxx_std_17
)

target_link_libraries(${target}
    PRIVATE
        sdk
        vstgui_support
)
smtg_target_add_plugin_resources(${target}
    RESOURCES
        resource/plugin.uidesc
        resource/background.png
        resource/version.png
)

#smtg_target_add_plugin_snapshots (${target}
#    RESOURCES
#        resource/84E8DE5F92554F5396FAE4133C935A18_snapshot.png
#        resource/84E8DE5F92554F5396FAE4133C935A18_snapshot_2.0x.png
#        resource/41347FD6FED64094AFBB12B7DBA1D441_snapshot.png
#        resource/41347FD6FED64094AFBB12B7DBA1D441_snapshot_2.0x.png
#)

if(SMTG_MAC)
    smtg_target_set_bundle(${target}
        BUNDLE_IDENTIFIER "nl.igorski.vst.${target}"
        COMPANY_NAME "igorski.nl"
    )
elseif(SMTG_WIN)
    target_sources(${target}
        PRIVATE
            resource/plugin.rc
    )
endif(SMTG_MAC)

# Add an AUv2 target
if (SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)
	include(SMTG_AddVST3AuV2)
	smtg_target_add_auv2(${target}-au
		BUNDLE_NAME transformant
		BUNDLE_IDENTIFIER nl.igorski.vst.${target}.audiounit
		INFO_PLIST_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/audio-unit/Info.plist
		VST3_PLUGIN_TARGET ${target}
    )
endif(SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)
