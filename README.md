\# Custom RTOS with Device Driver Framework



A lightweight Real-Time Operating System (RTOS) built from scratch with a comprehensive device driver framework, demonstrating embedded systems expertise.



\## 🎯 Project Overview



This project implements a preemptive multitasking RTOS with:

\- Priority-based task scheduler with round-robin for equal priorities

\- Complete device driver framework (UART, I2C, SPI)

\- DMA support and interrupt handling

\- Memory management with pool allocation

\- Inter-Process Communication (IPC) mechanisms

\- Simple bootloader with image verification



\## 🏗️ Architecture

```

┌─────────────────────────────────────┐

│        Application Layer            │

├─────────────────────────────────────┤

│     IPC Layer (Queues, Semaphores)  │

├─────────────────────────────────────┤

│    Device Drivers (UART,I2C,SPI)    │

├─────────────────────────────────────┤

│   Hardware Abstraction Layer (HAL)  │

├─────────────────────────────────────┤

│   RTOS Kernel (Scheduler, Memory)   │

├─────────────────────────────────────┤

│          Bootloader                 │

└─────────────────────────────────────┘

```



\## 🚀 Features



\### RTOS Kernel

\- Preemptive priority-based scheduling

\- Context switching

\- Task management (create, delete, suspend, resume)

\- Configurable number of priority levels



\### Device Drivers

\- \*\*UART\*\*: Interrupt-driven with circular buffers, DMA support

\- \*\*I2C\*\*: Master/Slave modes, 7/10-bit addressing

\- \*\*SPI\*\*: Full-duplex, configurable CPOL/CPHA



\### Memory Management

\- Custom heap allocator

\- Memory pool allocation

\- Stack overflow detection

\- Memory usage tracking



\### IPC Mechanisms

\- Message queues

\- Binary and counting semaphores

\- Mutexes with priority inheritance

\- Event flags



\## 📋 Prerequisites



\- MinGW-w64 (GCC for Windows) or MSYS2

\- CMake 3.10+

\- Git

\- Windows 10/11



\## 🔧 Building the Project (Windows)



\### Using CMake and MinGW:

```cmd

mkdir build

cd build

cmake -G "MinGW Makefiles" ..

mingw32-make

```



\### Using Visual Studio Code:

1\. Open folder in VS Code

2\. Install CMake Tools extension

3\. Press Ctrl+Shift+P → "CMake: Configure"

4\. Press Ctrl+Shift+P → "CMake: Build"



\## 🧪 Running Tests

```cmd

cd build

tests\\rtos\_tests.exe

```



\## 📚 Documentation



See \[docs/](docs/) folder for detailed documentation:

\- \[Architecture Design](docs/ARCHITECTURE.md)

\- \[API Reference](docs/API.md)

\- \[Memory Map](docs/MEMORY\_MAP.md)



\## 🎓 Skills Demonstrated



\- ✅ RTOS concepts and implementation

\- ✅ Device driver development

\- ✅ Low-level system programming in C

\- ✅ Interrupt handling and DMA

\- ✅ Memory management

\- ✅ Bootloader development

\- ✅ Git version control



\## 📄 License



MIT License



\## 👤 Author



\[Aquil]

\- GitHub: \[@aquuiil](https://github.com/aquuiil)

\- LinkedIn: \(https://www.linkedin.com/in/muhammed-aquil-k/)

