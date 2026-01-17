cmake_minimum_required(VERSION 4.0)

set(DIALUP_PATH "$ENV{LOCALAPPDATA}/DialUpFramework")
include("${DIALUP_PATH}/SupportedTerminal.cmake")
include("${DIALUP_PATH}/Toolchain.cmake")

project(AllTests)

set(CMAKE_PREFIX_PATH
        "${DIALUP_PATH}"
        "${DIALUP_PATH}/sdk-plugin"
)

set(GAME_BINARY_PATH CACHE STRING "path of the game's executable")
if(NOT GAME_BINARY_PATH)
    if(DEFINED ENV{platform} AND "$ENV{platform}" STREQUAL "steam")
        set(GAME_BINARY_PATH "C:/Program Files (x86)/Steam/steamapps/common/rocketleague/Binaries/Win64")
    else()
        set(GAME_BINARY_PATH "C:/Program Files/Epic Games/rocketleague/Binaries/Win64")
    endif()
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/mock
    ${CMAKE_SOURCE_DIR}/mock/module
    ${CMAKE_SOURCE_DIR}/mock/plugin
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/fixtures
    ${CMAKE_SOURCE_DIR}/../engine
    ${DIALUP_PATH}/include/v1
    ${DIALUP_PATH}/include/external/fmt/include
)

set(SOURCES
    main.cpp
)

#add_executable(AllTests
#    main.cpp
#    test_Runtime.cpp
#)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(DialUpFramework REQUIRED)
find_package(DialUp-SDK REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE DialUpFramework::DialUpFramework)
target_link_libraries(${PROJECT_NAME} PRIVATE DialUp-SDK::DialUp-SDK)
target_compile_definitions(AllTests PRIVATE TEST_BUILD)

include("${DIALUP_PATH}/EnsureABI.cmake")
file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/DialUpFramework/SDK-Plugin/bin/DialUp-SDK.dll" SDK_DLL)
target_compile_definitions(${PROJECT_NAME} PRIVATE SDK_DLL="${SDK_DLL}")
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
target_compile_definitions(${PROJECT_NAME} PRIVATE FMT_HEADER_ONLY)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

target_compile_options(AllTests PRIVATE /utf-8 /std:c++20)