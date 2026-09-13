cmake_minimum_required(VERSION 3.24)
project(ZZZTouchUI LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Windows x64 is required")
endif()
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
add_compile_options(/W4 /EHsc /utf-8)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(Hde64 STATIC third_party/hde64/hde64.c)
target_compile_options(Hde64 PRIVATE /W0)
add_library(ZZZTouchUI SHARED payload.cpp)
target_link_libraries(ZZZTouchUI PRIVATE user32 Hde64)
add_executable(ZZZTouchLauncher launcher.cpp)
target_link_libraries(ZZZTouchLauncher PRIVATE user32 bcrypt shell32 advapi32 ole32 Hde64)
add_custom_command(TARGET ZZZTouchLauncher POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "$<TARGET_FILE_DIR:ZZZTouchLauncher>/LICENSE"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/hde64/LICENSE.txt"
    "$<TARGET_FILE_DIR:ZZZTouchLauncher>/THIRD-PARTY-NOTICES.txt"
  VERBATIM)
