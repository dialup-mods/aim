cmake_minimum_required(VERSION 4.0)

set(DIALUP_INSTALL_DIR "$ENV{LOCALAPPDATA}/DialUp")
include("${DIALUP_INSTALL_DIR}/build-tools/PreProject.cmake")

project(PatchUtilsTests)

add_executable(${PROJECT_NAME}
        tests/test_PatchUtils.cpp
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ../../memory
        ${DIALUP_INSTALL_DIR}/include/v1
        ${DIALUP_INSTALL_DIR}/include/external
)

include("${DIALUP_INSTALL_DIR}/build-tools/PostProject.cmake")