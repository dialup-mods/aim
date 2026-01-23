cmake_minimum_required(VERSION 4.0)

set(DIALUP_INSTALL_DIR "$ENV{LOCALAPPDATA}/DialUp")
include("${DIALUP_INSTALL_DIR}/build-tools/PreProject.cmake")

project(AllTests)

set(CMAKE_PREFIX_PATH
        "${DIALUP_PATH}"
        "${DIALUP_PATH}/sdk-plugin"
)

include("${DIALUP_INSTALL_DIR}/build-tools/GameBinaryDir.cmake")

include_directories(
    ${CMAKE_SOURCE_DIR}/mock
    ${CMAKE_SOURCE_DIR}/mock/module
    ${CMAKE_SOURCE_DIR}/mock/plugin
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/fixtures
    ${CMAKE_SOURCE_DIR}/../engine
    ${DIALUP_INSTALL_DIR}/include/v1
    ${DIALUP_INSTALL_DIR}/include/ue_model/v1
    ${DIALUP_INSTALL_DIR}/sdk-plugin/include
    ${DIALUP_INSTALL_DIR}/include/external/fmt/include
)

set(SOURCES
    main.cpp
)

#add_executable(AllTests
#    main.cpp
#    test_Runtime.cpp
#)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(DialUp-SDK REQUIRED PATHS ${DIALUP_INSTALL_DIR}/sdk-plugin)
target_link_libraries(${PROJECT_NAME} PRIVATE DialUp-SDK::DialUp-SDK)

find_package(DialUp REQUIRED PATHS ${DIALUP_INSTALL_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE DialUp::DialUp)

file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/DialUp/sdk-plugin/bin/DialUp-SDK.dll" SDK_DLL)
target_compile_definitions(${PROJECT_NAME} PRIVATE TEST_BUILD)
target_compile_definitions(${PROJECT_NAME} PRIVATE SDK_DLL="${SDK_DLL}")
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

include("${DIALUP_INSTALL_DIR}/build-tools/PostProject.cmake")