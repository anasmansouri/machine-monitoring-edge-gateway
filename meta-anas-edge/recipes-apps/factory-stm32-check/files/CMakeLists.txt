cmake_minimum_required(VERSION 3.10)

project(factory-stm32-check LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(stm32_comm REQUIRED)

add_executable(factory-stm32-check main.cpp)

target_link_libraries(factory-stm32-check
    PRIVATE stm32_comm::stm32_comm
)

install(TARGETS factory-stm32-check
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
