cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME BatTester)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    App/Src/synth.cpp
    App/Src/scenario.cpp
    App/Src/cli.cpp
    Core/Src/wave_table.c
    Core/Src/clock.cpp
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    App/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# ---------------------------------------------------------------------------
# LoopSource: the second image from this tree (digitalSpine
# todo/fluxStationLoopSource.md). It shares the CubeMX drivers, the startup
# file, the MSP and IRQ files and Core/Src/clock.cpp with BatTester, and has
# its own main in Loop/. Every build compiles both images, so a change to the
# shared code that breaks one of them fails the build.
# ---------------------------------------------------------------------------
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE LOOP_GIT_REV
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT LOOP_GIT_REV)
    set(LOOP_GIT_REV "unknown")
endif()

add_executable(LoopSource)
target_sources(LoopSource PRIVATE
    Loop/Src/main.cpp
    Loop/Src/sine_out.cpp
    Loop/Src/adc_in.cpp
    Loop/Src/analysis.cpp
    Loop/Src/cli.cpp
    Loop/Src/pga.cpp
    Core/Src/clock.cpp
    Core/Src/stm32l4xx_it.c
    Core/Src/stm32l4xx_hal_msp.c
    Core/Src/sysmem.c
    Core/Src/syscalls.c
    startup_stm32l476xx.s
)
target_include_directories(LoopSource PRIVATE Loop/Inc)
target_compile_definitions(LoopSource PRIVATE LOOP_GIT_REV="${LOOP_GIT_REV}")
# The toolchain file names the map after the project; the last -Map wins.
# printf needs %f for the measurement lines.
target_link_options(LoopSource PRIVATE -Wl,-Map=LoopSource.map -u _printf_float)
target_link_libraries(LoopSource stm32cubemx STM32_Drivers ${TOOLCHAIN_LINK_LIBRARIES})
set_target_properties(LoopSource PROPERTIES ADDITIONAL_CLEAN_FILES LoopSource.map)
