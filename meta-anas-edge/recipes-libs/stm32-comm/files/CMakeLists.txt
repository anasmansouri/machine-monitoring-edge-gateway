cmake_minimum_required(VERSION 3.10)

project(stm32_comm VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(common_utils REQUIRED)

add_library(stm32_comm SHARED
    manager/uartManager.cpp
    manager/protocolParser.cpp
)


target_link_libraries(stm32_comm
    PUBLIC common_utils::common_utils
)


target_include_directories(stm32_comm
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/stm32-comm>
)

set_target_properties(stm32_comm PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME stm32_comm
)

install(TARGETS stm32_comm
    EXPORT stm32_commTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY manager 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/stm32-comm
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT stm32_commTargets
    FILE stm32_commTargets.cmake
    NAMESPACE stm32_comm::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stm32_comm
)

configure_package_config_file(
    cmake/stm32_commConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/stm32_commConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stm32_comm
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/stm32_commConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/stm32_commConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/stm32_commConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stm32_comm
)
