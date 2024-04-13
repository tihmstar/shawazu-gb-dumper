# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include(pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(shawazu-gb-dumper C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb -DCFG_TUSB_DEBUG=3")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

set(PICO_TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)

# Tell CMake where to find the executable source file
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} ${PICO_TINYUSB_PATH}/src)

add_executable(${PROJECT_NAME} 
    src/Cartridge.cpp
    src/CartridgeGB.cpp
    src/CartridgeGBA.cpp
    src/CartridgeGBMBC1.cpp
    src/CartridgeGBMBC3.cpp
    src/EmuFATFS.cpp
    src/fakefiles.cpp
    src/gbrw.c
    src/gbarw.c
    src/msc_disk.cpp
    src/main.cpp
    src/usb_cdc.cpp
    src/usb_descriptors.c
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PICO_TINYUSB_PATH}/src
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    hardware_pio
    hardware_dma
    tinyusb_device
    tinyusb_board
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/gb_read.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/gb_write.pio)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/gba_read.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/gba_sram_read.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/gba_sram_write.pio)


# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
