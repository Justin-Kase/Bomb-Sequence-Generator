cmake_minimum_required(VERSION 3.21)
project(IllbombSeqGenerator VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build universal (arm64 + x86_64) on macOS for better DAW compatibility
if (APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures" FORCE)
  # Signing and notarization configuration
  set(CODE_SIGN_IDENTITY "-" CACHE STRING "Codesign identity (\"-\" for ad-hoc or \"Developer ID Application: Your Name (TEAMID)\")")
  set(NOTARY_PROFILE "" CACHE STRING "Keychain profile for xcrun notarytool (set up with notarytool store-credentials)")
  option(PLUGIN_NOTARIZE "Add a notarization target for the VST3 (requires NOTARY_PROFILE and non-ad-hoc signature)" OFF)
endif()

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG master # Use latest JUCE for macOS 15 (Sequoia) fixes
)
FetchContent_MakeAvailable(juce)

# Enable VST3 only, no AU to keep it simple
option(BUILD_AU "Build AudioUnit" OFF)

add_subdirectory(Source)

if (APPLE AND PLUGIN_NOTARIZE)
  # Create a helper target to notarize and staple the built VST3 bundle
  add_custom_target(notarize_vst3
    COMMAND ${CMAKE_COMMAND} -E echo "Signing with Developer ID: ${CODE_SIGN_IDENTITY}"
    COMMAND codesign --force --deep --options runtime -s "${CODE_SIGN_IDENTITY}" "$<TARGET_FILE_DIR:IllbombSeqGenerator_VST3>/../.."
    COMMAND ${CMAKE_COMMAND} -E echo "Preparing VST3 for notarization"
    COMMAND /usr/bin/ditto -c -k --keepParent
            "$<TARGET_FILE_DIR:IllbombSeqGenerator_VST3>/../.."
            "${CMAKE_BINARY_DIR}/IllbombSeqGenerator.vst3.zip"
    COMMAND ${CMAKE_COMMAND} -E echo "Submitting to Apple notarization service (notarytool)"
    COMMAND xcrun notarytool submit "${CMAKE_BINARY_DIR}/IllbombSeqGenerator.vst3.zip" --keychain-profile "${NOTARY_PROFILE}" --wait
    COMMAND ${CMAKE_COMMAND} -E echo "Stapling notarization ticket"
    COMMAND xcrun stapler staple "$<TARGET_FILE_DIR:IllbombSeqGenerator_VST3>/../.."
    DEPENDS IllbombSeqGenerator_VST3
    COMMENT "Notarizing VST3 bundle"
    VERBATIM
  )
endif()

if (APPLE)
  # Post-install hook: sanitize installed moduleinfo.json and ad-hoc sign the bundle
  install(CODE [[
    set(bundle "${CMAKE_INSTALL_PREFIX}/Library/Audio/Plug-Ins/VST3/Illbomb Sequence Generator.vst3")
    set(json   "${bundle}/Contents/Resources/moduleinfo.json")
    if (EXISTS "${json}")
      execute_process(COMMAND /usr/bin/python3 "${CMAKE_SOURCE_DIR}/scripts/fix_moduleinfo_json.py" "${json}" RESULT_VARIABLE rv)
      execute_process(COMMAND codesign --force --deep --options runtime -s - "${bundle}")
    endif()
  ]] COMPONENT IllbombSeqGenerator_VST3)

  # Convenience target: fix and re-sign the installed VST3 in the user's plug-ins folder
  add_custom_target(fix_installed_vst3
    COMMAND /usr/bin/python3 "${CMAKE_SOURCE_DIR}/scripts/fix_moduleinfo_json.py" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Illbomb Sequence Generator.vst3/Contents/Resources/moduleinfo.json"
    COMMAND codesign --force --deep --options runtime -s - "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Illbomb Sequence Generator.vst3"
    COMMENT "Sanitizing installed moduleinfo.json and re-signing VST3 bundle"
    VERBATIM
  )
endif()
