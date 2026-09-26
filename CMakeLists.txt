cmake_minimum_required(VERSION 3.24)

project(TouchUILaunch LANGUAGES C CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "TouchUILaunch requires Windows x64")
endif()

option(TOUCHUI_ENABLE_EXTERNAL_SAMPLES "Register tests that read supplied game samples" OFF)
set(TOUCHUI_ZZZ_SAMPLE_ROOT "" CACHE PATH "Directory containing versioned ZZZ sample folders")
set(TOUCHUI_SR_SAMPLE "" CACHE FILEPATH "Optional SR GameAssembly.dll for read-only resolution")

add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/W4;/EHsc;/utf-8>")

# Header-only modules remain in the service directory that owns them. Every
# target consumes this explicit, project-local include surface.
add_library(TouchUIHeaders INTERFACE)
target_include_directories(TouchUIHeaders INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/launcher/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/windows/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/binary/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/process/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/testing/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/zzz/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/gi/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/sr/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/ww/include")

add_library(Hde64 STATIC third_party/hde64/hde64.c)
target_compile_options(Hde64 PRIVATE /W0)

add_library(RemoteLoaderStubs STATIC common/process/src/load_library.asm)
target_link_libraries(RemoteLoaderStubs PUBLIC TouchUIHeaders)
add_library(GIMobileStubs STATIC games/gi/src/gi_stubs.asm)
target_link_libraries(GIMobileStubs PUBLIC TouchUIHeaders)
add_library(SRMobileStubs STATIC games/sr/src/sr_stubs.asm)
target_link_libraries(SRMobileStubs PUBLIC TouchUIHeaders)

add_library(TouchUIZZZDiscovery INTERFACE)
target_link_libraries(TouchUIZZZDiscovery INTERFACE TouchUIHeaders Hde64)

add_library(TouchUIProcess INTERFACE)
target_link_libraries(TouchUIProcess INTERFACE TouchUIHeaders Hde64 RemoteLoaderStubs GIMobileStubs SRMobileStubs user32 psapi)

add_library(TouchUIGI INTERFACE)
target_link_libraries(TouchUIGI INTERFACE TouchUIHeaders bcrypt)
add_library(TouchUISR INTERFACE)
target_link_libraries(TouchUISR INTERFACE TouchUIProcess)
add_library(TouchUIWW INTERFACE)
target_link_libraries(TouchUIWW INTERFACE TouchUIHeaders)

add_library(TouchUILaunchPayload SHARED games/zzz/src/payload.cpp)
set_target_properties(TouchUILaunchPayload PROPERTIES OUTPUT_NAME TouchUILaunch)
target_link_libraries(TouchUILaunchPayload PRIVATE TouchUIZZZDiscovery user32)

add_executable(TouchUILaunch launcher/src/launcher.cpp)
target_link_libraries(TouchUILaunch PRIVATE TouchUIGI TouchUISR TouchUIWW user32 bcrypt shell32 advapi32 ole32)

add_executable(TouchStateTests games/zzz/tests/touch_state_tests.cpp)
target_link_libraries(TouchStateTests PRIVATE TouchUIHeaders)

add_library(TouchUIZZZBridgeTestHarness OBJECT games/zzz/src/payload.cpp)
target_compile_definitions(TouchUIZZZBridgeTestHarness PRIVATE TOUCHUI_BRIDGE_TESTING=1)
target_link_libraries(TouchUIZZZBridgeTestHarness PRIVATE TouchUIZZZDiscovery user32)

add_executable(TouchBridgeTests games/zzz/tests/bridge_tests.cpp $<TARGET_OBJECTS:TouchUIZZZBridgeTestHarness>)
target_link_libraries(TouchBridgeTests PRIVATE TouchUIZZZDiscovery user32 Hde64)

add_executable(LauncherElevationTests common/windows/tests/elevation_tests.cpp)
target_link_libraries(LauncherElevationTests PRIVATE TouchUIHeaders shell32 advapi32 ole32)

add_executable(LauncherLogTests launcher/tests/launcher_log_tests.cpp)
target_link_libraries(LauncherLogTests PRIVATE TouchUIHeaders user32)

add_executable(MemoryReaderTests games/zzz/tests/memory_tests.cpp)
target_link_libraries(MemoryReaderTests PRIVATE TouchUIZZZDiscovery)

add_executable(MobileUITestChild common/testing/mobile_test_child.cpp)
target_link_libraries(MobileUITestChild PRIVATE TouchUIHeaders)

add_executable(MobileUITests common/process/mobile_tests.cpp)
target_link_libraries(MobileUITests PRIVATE TouchUIProcess)
add_dependencies(MobileUITests MobileUITestChild)

add_executable(GITouch71Tests games/gi/tests/gi_touch71_tests.cpp)
target_link_libraries(GITouch71Tests PRIVATE TouchUIHeaders bcrypt)

add_executable(ProfileDiscoveryTests games/zzz/tests/discovery_tests.cpp)
target_link_libraries(ProfileDiscoveryTests PRIVATE TouchUIZZZDiscovery bcrypt)

enable_testing()
add_test(NAME GITouch71 COMMAND GITouch71Tests)
set_tests_properties(GITouch71 PROPERTIES TIMEOUT 30)
add_test(NAME MobileUI COMMAND MobileUITests)
set_tests_properties(MobileUI PROPERTIES TIMEOUT 60)
add_test(NAME TouchState COMMAND TouchStateTests)
add_test(NAME TouchBridge31 COMMAND TouchBridgeTests 3.1)
add_test(NAME TouchBridge32 COMMAND TouchBridgeTests 3.2)
add_test(NAME LauncherElevation COMMAND LauncherElevationTests)
add_test(NAME LauncherLog COMMAND LauncherLogTests)
add_test(NAME MemoryReader COMMAND MemoryReaderTests)

if(TOUCHUI_ENABLE_EXTERNAL_SAMPLES)
  if(TOUCHUI_SR_SAMPLE)
    add_test(NAME MobileUIStarRailSample COMMAND MobileUITests "${TOUCHUI_SR_SAMPLE}")
  endif()
  if(NOT TOUCHUI_ZZZ_SAMPLE_ROOT)
    message(FATAL_ERROR "TOUCHUI_ENABLE_EXTERNAL_SAMPLES requires TOUCHUI_ZZZ_SAMPLE_ROOT")
  endif()
  foreach(version IN ITEMS 2.5 2.6 3.1 3.2)
    add_test(NAME "ProfileDiscovery${version}"
      COMMAND ProfileDiscoveryTests "${TOUCHUI_ZZZ_SAMPLE_ROOT}/${version}/GameAssembly.dll")
  endforeach()
endif()

install(TARGETS TouchUILaunch TouchUILaunchPayload RUNTIME DESTINATION . LIBRARY DESTINATION .)
install(FILES README.md DESTINATION .)
install(FILES LICENSE DESTINATION .)
install(FILES docs/THIRD-PARTY-NOTICES.txt DESTINATION .)
install(FILES third_party/hde64/LICENSE.txt DESTINATION licenses/hde64)
install(FILES third_party/gi_sr/LICENSE.txt DESTINATION licenses/gi_sr)
