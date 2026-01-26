# RTOS Architecture

## System Layers
1. **Bootloader** - Firmware verification and startup
2. **HAL** - Hardware abstraction
3. **Kernel** - Scheduler, tasks, memory
4. **Drivers** - UART, I2C, SPI
5. **IPC** - Semaphores, mutexes, queues
6. **Application** - User tasks

## Task Scheduling
- Preemptive priority-based
- 32 priority levels
- Round-robin for equal priorities
- Context switch: ~40 CPU cycles on ARM

## Memory Management
- Custom heap allocator
- Memory pools for fixed-size blocks
- Stack overflow detection
- Fragmentation tracking