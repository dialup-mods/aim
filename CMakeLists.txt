cmake_minimum_required(VERSION 4.0)

set(DIALUP_INSTALL_DIR "$ENV{LOCALAPPDATA}/DialUp")
include("${DIALUP_INSTALL_DIR}/build-tools/PreProject.cmake")

project(AIM VERSION 1.0 LANGUAGES CXX)

#include("${DIALUP_DIR}/build-tools/GameBinaryDir.cmake")

set(SOURCES
    ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.cpp
    ${CMAKE_SOURCE_DIR}/dispatch/Dispatch.cpp
    ${CMAKE_SOURCE_DIR}/engine/EngineLocator.cpp
    ${CMAKE_SOURCE_DIR}/engine/ObjectProvider.cpp
    ${CMAKE_SOURCE_DIR}/memory/PatchBuilder.cpp
    ${CMAKE_SOURCE_DIR}/memory/PatchManager.cpp
    ${CMAKE_SOURCE_DIR}/model/UEModel.cpp
    ${CMAKE_SOURCE_DIR}/process/CallFunction.cpp
    ${CMAKE_SOURCE_DIR}/process/ProcessEvent.cpp
    ${CMAKE_SOURCE_DIR}/process/ProcessInternal.cpp
    ${CMAKE_SOURCE_DIR}/task/TaskQueue.cpp

    ${CMAKE_SOURCE_DIR}/abstract/IProcessEvent.cpp
)

set(INCLUDES
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/context
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/dispatch
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/lifecycle
    ${CMAKE_SOURCE_DIR}/memory
    ${CMAKE_SOURCE_DIR}/process
    ${CMAKE_SOURCE_DIR}/task
    ${CMAKE_SOURCE_DIR}/util

    ${DIALUP_INSTALL_DIR}/include
    ${DIALUP_INSTALL_DIR}/include/v1
    ${DIALUP_INSTALL_DIR}/include/external
    ${CMAKE_SOURCE_DIR}/model
    ${CMAKE_SOURCE_DIR}/model/abstract
    ${CMAKE_SOURCE_DIR}/model/base
    ${CMAKE_SOURCE_DIR}/model/helpers
    ${CMAKE_SOURCE_DIR}/model/properties
    ${CMAKE_SOURCE_DIR}/model/resolve
    ${DIALUP_INSTALL_DIR}/sdk-plugin/include
)

include("${DIALUP_INSTALL_DIR}/build-tools/Plugin.cmake")

dialup_setup_module(${PROJECT_NAME}
    SOURCES ${SOURCES}
    INCLUDES ${INCLUDES}
    DESTINATION "plugin/${PROJECT_NAME}"
)

find_package(DialUp-SDK REQUIRED PATHS ${DIALUP_INSTALL_DIR}/sdk-plugin)
target_link_libraries(${PROJECT_NAME} PRIVATE DialUp-SDK::DialUp-SDK)

#add_link_options(/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)

#target_link_libraries(${PROJECT_NAME} PRIVATE psapi)

#configure_plugin(${PROJECT_NAME})
#set_target_properties(${PROJECT_NAME} PROPERTIES
#    WINDOWS_EXPORT_ALL_SYMBOLS OFF
#)

#add_custom_command(
#        TARGET ${PROJECT_NAME}
#        POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${GAME_BINARY_PATH}/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.dll
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:${PROJECT_NAME}> ${GAME_BINARY_PATH}/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.pdb
#        COMMAND ${CMAKE_COMMAND} -E echo "Post-build copy fired (dll and config)"
#        COMMAND ${CMAKE_COMMAND} -E echo ${GAME_BINARY_PATH}
#)

target_compile_definitions(${PROJECT_NAME} PRIVATE AIM_BUILD)
