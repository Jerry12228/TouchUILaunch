cmake_minimum_required(VERSION 3.24)
project(TouchUILaunch LANGUAGES C CXX ASM_MASM)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Windows x64 is required")
endif()
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/W4;/EHsc;/utf-8>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(Hde64 STATIC third_party/hde64/hde64.c)
target_compile_options(Hde64 PRIVATE /W0)
add_library(TouchUILaunchPayload SHARED payload.cpp)
set_target_properties(TouchUILaunchPayload PROPERTIES OUTPUT_NAME TouchUILaunch)
target_link_libraries(TouchUILaunchPayload PRIVATE user32 Hde64)
add_executable(TouchUILaunch launcher.cpp)
add_library(MobileUIStubs STATIC mobile_stubs.asm)
target_link_libraries(TouchUILaunch PRIVATE user32 bcrypt shell32 advapi32 ole32 Hde64 MobileUIStubs)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/third_party/hde64/LICENSE.txt" HDE64_NOTICE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gi_sr/LICENSE.txt" GI_SR_NOTICE)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/THIRD-PARTY-NOTICES.txt"
  "HDE64\n\n${HDE64_NOTICE}\n\nGI/SR mobile UI (Genshin_StarRail_fps_unlocker)\n\n${GI_SR_NOTICE}\n")
add_custom_command(TARGET TouchUILaunch POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "$<TARGET_FILE_DIR:TouchUILaunch>/LICENSE"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/THIRD-PARTY-NOTICES.txt"
    "$<TARGET_FILE_DIR:TouchUILaunch>/THIRD-PARTY-NOTICES.txt"
  VERBATIM)
