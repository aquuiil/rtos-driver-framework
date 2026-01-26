# Test executable
add_executable(rtos_tests
    test_main.c
)

# Link with all RTOS libraries
target_link_libraries(rtos_tests
    rtos_kernel
    rtos_drivers
    rtos_hal
    rtos_ipc
    rtos_bootloader
)

# Add tests
add_test(NAME rtos_all_tests COMMAND rtos_tests)