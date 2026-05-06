cmake_minimum_required(VERSION 3.10)
project(RTOS_Driver_Framework C)

set(CMAKE_C_STANDARD 11)

# Target selection: windows (default) or qemu
set(RTOS_TARGET "windows" CACHE STRING "Target platform: windows or qemu")
message(STATUS "Building for target: ${RTOS_TARGET}")

# ==================== WINDOWS TARGET ====================
if(RTOS_TARGET STREQUAL "windows")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
    endif()
    add_definitions(-DTARGET_WINDOWS)
# ==================== QEMU/ARM TARGET ====================
elseif(RTOS_TARGET STREQUAL "qemu")
    # Use arm-none-eabi toolchain
    set(CMAKE_C_COMPILER arm-none-eabi-gcc)
    set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wall -Wextra -O2 -fno-common -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -T${PROJECT_SOURCE_DIR}/src/hal/stm32f429_linker.ld -Wl,--gc-sections -Wl,--print-memory-usage")
    add_definitions(-DTARGET_QEMU -DSTM32F429xx)
else()
    message(FATAL_ERROR "Invalid RTOS_TARGET: ${RTOS_TARGET}. Use 'windows' or 'qemu'")
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/kernel
    ${PROJECT_SOURCE_DIR}/include/drivers
    ${PROJECT_SOURCE_DIR}/include/hal
    ${PROJECT_SOURCE_DIR}/include/ipc
    ${PROJECT_SOURCE_DIR}/include/bootloader
)

set(HAL_SOURCES
    src/hal/arm_cortex_m4.c
    src/hal/interrupt_dispatcher.c
)

# Source files - HAL startup (conditional based on target)
if(RTOS_TARGET STREQUAL "qemu")
    list(APPEND HAL_SOURCES src/hal/stm32f429_startup.c)
endif()

set(KERNEL_SOURCES
    src/kernel/scheduler.c
    src/kernel/task.c
    src/kernel/memory.c
)

set(DRIVER_SOURCES
    src/drivers/uart_driver.c
    src/drivers/i2c_driver.c
    src/drivers/spi_driver.c
)

set(IPC_SOURCES
    src/ipc/message_queue.c
    src/ipc/semaphore.c
    src/ipc/mutex.c
)

set(BOOTLOADER_SOURCES
    src/bootloader/bootloader.c
)

set(DRIVER_FRAMEWORK_SOURCES
    src/driver_framework.c
)

# Create static libraries
add_library(rtos_kernel STATIC ${KERNEL_SOURCES})
add_library(rtos_drivers STATIC ${DRIVER_SOURCES})
add_library(rtos_hal STATIC ${HAL_SOURCES})
add_library(rtos_ipc STATIC ${IPC_SOURCES})
add_library(rtos_bootloader STATIC ${BOOTLOADER_SOURCES})
add_library(rtos_driver_framework STATIC ${DRIVER_FRAMEWORK_SOURCES})

# Example application
if(RTOS_TARGET STREQUAL "windows")
    add_executable(rtos_demo examples/demo_app.c)
elseif(RTOS_TARGET STREQUAL "qemu")
    add_executable(rtos_demo.elf examples/demo_app.c)
endif()

target_link_libraries(rtos_demo 
    rtos_kernel 
    rtos_drivers 
    rtos_hal 
    rtos_ipc
    rtos_bootloader
    rtos_driver_framework
)

# Enable testing (only for Windows target)
if(RTOS_TARGET STREQUAL "windows")
    enable_testing()
    add_subdirectory(tests)
endif()