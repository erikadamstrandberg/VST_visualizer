juce_add_plugin(erik_sat
    COMPANY_NAME "Erik"
    PLUGIN_MANUFACTURER_CODE Erik
    PLUGIN_CODE Esat
    FORMATS VST3 Standalone
    PRODUCT_NAME "erik_sat"
    DESCRIPTION "Erik Saturation Module with Realtime Visualizer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

target_sources(erik_sat PRIVATE
    Source/SaturationDSP.h
    Source/TransferCurveVisualizer.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

target_compile_definitions(erik_sat PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(erik_sat PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Automated post-build copy of the .vst3 bundle to the root VST3/ folder
add_custom_command(TARGET erik_sat_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/VST3"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/erik_sat_artefacts/$<CONFIG>/VST3/erik_sat.vst3"
        "${CMAKE_SOURCE_DIR}/VST3/erik_sat.vst3"
    COMMENT "Copying erik_sat.vst3 to root VST3 folder..."
)

# Automated DSP Unit Test Executable
add_executable(erik_sat_dsp_test Source/DSPTestRunner.cpp)
target_link_libraries(erik_sat_dsp_test PRIVATE
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_core
)

