cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)

project(pico_examples C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# If you want debug output from USB (pass -DPICO_STDIO_USB=1) this ensures you don't lose any debug output while USB is set up
if (NOT DEFINED PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
    set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 3000)
endif()

set(PICO_BOARD pico_w)

# Initialize the SDK
pico_sdk_init()

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Og")
add_executable(pico-hf-oscillator-test)

pico_generate_pio_header(pico-hf-oscillator-test ${CMAKE_CURRENT_LIST_DIR}/piodco/dco2.pio)

target_sources(pico-hf-oscillator-test PUBLIC
	      ${CMAKE_CURRENT_LIST_DIR}/lib/assert.c
        ${CMAKE_CURRENT_LIST_DIR}/piodco/piodco.c
        ${CMAKE_CURRENT_LIST_DIR}/gpstime/GPStime.c
        ${CMAKE_CURRENT_LIST_DIR}/debug/logutils.c
        ${CMAKE_CURRENT_LIST_DIR}/test.c
        ${CMAKE_CURRENT_LIST_DIR}/conswrapper.c
        ${CMAKE_CURRENT_LIST_DIR}/hfconsole/hfconsole.c
        )

pico_set_program_name(pico-hf-oscillator-test "pico-hf-oscillator-test")
pico_set_program_version(pico-hf-oscillator-test "0.9")

pico_enable_stdio_uart(pico-hf-oscillator-test 0)
pico_enable_stdio_usb(pico-hf-oscillator-test 1)

# Add the standard include files to the build
target_include_directories(pico-hf-oscillator-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/gpstime
  ${CMAKE_CURRENT_LIST_DIR}/hfconsole
  ${CMAKE_CURRENT_LIST_DIR}/..
)

add_compile_options(-Wall)

# Add any user requested libraries
target_link_libraries(
        pico-hf-oscillator-test
        pico_stdlib
        pico_sync
        pico_multicore
        hardware_timer
        hardware_clocks
        hardware_pio
        hardware_uart
	pico_cyw43_arch_lwip_threadsafe_background
        )

pico_add_extra_outputs(pico-hf-oscillator-test)
