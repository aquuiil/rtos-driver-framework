# Clean Build (Start Fresh)
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

# Rebuild Everything
cd build
del /Q *                  # Delete all build files (Windows)
cmake -G "MinGW Makefiles" ..
mingw32-make

# Build Only Specific Target
mingw32-make rtos_kernel     # Build only kernel library
mingw32-make rtos_demo       # Build only demo executable
mingw32-make rtos_tests      # Build only tests

# Run demo
.\rtos_demo.exe

# Run tests
.\tests\rtos_tests.exe