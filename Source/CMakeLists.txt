# Binary resources (logo)
juce_add_binary_data(MidiSeqBinaryData
    SOURCES
        ../Resources/logo.png
)

juce_add_plugin(MidiSequenceGenerator
    COMPANY_NAME "OpenSource"
    BUNDLE_ID com.opensource.midisequencegenerator
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE OSrc
    PLUGIN_CODE MSG1
    FORMATS VST3
    PRODUCT_NAME "MIDI Sequence Generator"
)

# Add the plugin sources
set(PLUGIN_SOURCES
    PluginEditor.h
    PluginEditor.cpp
    PluginProcessor.h
    PluginProcessor.cpp
    SequenceGenerator.h
    SequenceGenerator.cpp
)

target_sources(MidiSequenceGenerator PRIVATE ${PLUGIN_SOURCES})

target_compile_definitions(MidiSequenceGenerator
    PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=1
)

target_link_libraries(MidiSequenceGenerator
    PRIVATE
        MidiSeqBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
)

# CLI tool for exporting MIDI
add_executable(midigen_cli
    main_cli.cpp
    SequenceGenerator.h
    SequenceGenerator.cpp
)

target_link_libraries(midigen_cli PRIVATE juce::juce_core juce::juce_audio_basics)

set_target_properties(midigen_cli PROPERTIES OUTPUT_NAME midigen)

# Post-build: fix moduleinfo.json (remove trailing commas) and re-sign
if (APPLE)
    add_custom_command(TARGET MidiSequenceGenerator_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env PYTHONUNBUFFERED=1
                        /usr/bin/python3 ${CMAKE_SOURCE_DIR}/scripts/fix_moduleinfo_json.py
                        "$<TARGET_FILE_DIR:MidiSequenceGenerator_VST3>/../Resources/moduleinfo.json"
        COMMAND codesign --force --deep --options runtime -s - "$<TARGET_FILE_DIR:MidiSequenceGenerator_VST3>/../.."
        VERBATIM
        COMMENT "Fixing moduleinfo.json and ad-hoc signing VST3 bundle"
    )
endif()
