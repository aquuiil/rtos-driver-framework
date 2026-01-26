cmake_minimum_required(VERSION 3.10)
project(RTOS_Driver_Framework C)

set(CMAKE_C_STANDARD 11)

# Windows-specific settings
if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
    # For MinGW
    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
    endif()
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

# Source files
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

set(HAL_SOURCES
    src/hal/arm_cortex_m4.c
)

set(IPC_SOURCES
    src/ipc/message_queue.c
    src/ipc/semaphore.c
    src/ipc/mutex.c
)

set(BOOTLOADER_SOURCES
    src/bootloader/bootloader.c
)

# Create static libraries
add_library(rtos_kernel STATIC ${KERNEL_SOURCES})
add_library(rtos_drivers STATIC ${DRIVER_SOURCES})
add_library(rtos_hal STATIC ${HAL_SOURCES})
add_library(rtos_ipc STATIC ${IPC_SOURCES})
add_library(rtos_bootloader STATIC ${BOOTLOADER_SOURCES})

# Example application
add_executable(rtos_demo examples/demo_app.c)
target_link_libraries(rtos_demo 
    rtos_kernel 
    rtos_drivers 
    rtos_hal 
    rtos_ipc
    rtos_bootloader
)

# Enable testing
enable_testing()
add_subdirectory(tests)