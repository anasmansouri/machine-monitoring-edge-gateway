cmake_minimum_required(VERSION 3.10)

project(common_utils VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(common_utils SHARED
    utils/Result.cpp
)

target_include_directories(common_utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/common_utils>
)

set_target_properties(common_utils PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME common_utils
)

install(TARGETS common_utils
    EXPORT common_utilsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY utils
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/common_utils
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT common_utilsTargets
    FILE common_utilsTargets.cmake
    NAMESPACE common_utils::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/common_utils
)

configure_package_config_file(
    cmake/common_utilsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/common_utilsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/common_utils
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/common_utilsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/common_utilsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/common_utilsConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/common_utils
)
