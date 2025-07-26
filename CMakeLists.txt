cmake_minimum_required(VERSION 3.14)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" TUNGSTENCORE_VERSION_RAW)
string(STRIP "${TUNGSTENCORE_VERSION_RAW}" TUNGSTENCORE_VERSION)
string(REPLACE "." ";" TUNGSTENCORE_VERSION_LIST ${TUNGSTENCORE_VERSION})

list(LENGTH TUNGSTENCORE_VERSION_LIST _ver_len)
if(NOT _ver_len EQUAL 3)
    message(FATAL_ERROR "VERSION file contains unknown format expected: MAJOR.MINOR.PATCH")
endif()

list(GET TUNGSTENCORE_VERSION_LIST 0 TUNGSTENCORE_VERSION_MAJOR)
list(GET TUNGSTENCORE_VERSION_LIST 1 TUNGSTENCORE_VERSION_MINOR)
list(GET TUNGSTENCORE_VERSION_LIST 2 TUNGSTENCORE_VERSION_PATCH)

project(TungstenCore VERSION ${TUNGSTENCORE_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/TungstenCore/Version.hpp
  @ONLY
)

# Add TungstenUtils
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/TungstenUtils)

add_library(TungstenCore
    include/TungstenCore/TungstenCore.hpp
    src/wCorePCH.cpp
    src/TungstenCore.cpp
)

target_include_directories(TungstenCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(TungstenCore PUBLIC cxx_std_20)
target_precompile_headers(TungstenCore PRIVATE src/wCorePCH.hpp)

target_link_libraries(TungstenCore PUBLIC TungstenUtils)

# Installation logic
include(GNUInstallDirs)

install(TARGETS TungstenCore
        EXPORT TungstenCoreTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/include/TungstenCore/Version.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/TungstenCore
)

install(EXPORT TungstenCoreTargets
        FILE TungstenCoreTargets.cmake
        NAMESPACE Tungsten::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TungstenCore)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TungstenCoreConfigVersion.cmake"
    VERSION ${TUNGSTENCORE_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/TungstenCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TungstenCoreConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TungstenCore
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/TungstenCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/TungstenCoreConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TungstenCore
)

# Uninstall logic
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)