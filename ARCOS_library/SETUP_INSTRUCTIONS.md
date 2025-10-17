# ARCOS PlatformIO Setup Guide

## Problem
Your library is located in `ARCOS_library/` subdirectory, but PlatformIO expects library files at the repository root when using Git dependencies.

## Solution Options

### Option 1: Repository Restructure (Recommended)

Move these files to the repository root:
```
From ARCOS_library/ → To repository root:
├── library.json
├── library.properties  
├── include/
├── examples/
├── README.md
└── LICENSE
```

After restructuring, users can simply use:
```ini
lib_deps = https://github.com/XCR1793/ARCOS.git#main
```

### Option 2: Use Specific Git Path (Current Workaround)

For now, users need to specify the subdirectory path:

```ini
[env:your_board]
platform = espressif32
board = esp32s3usbotg
framework = espidf

lib_deps = 
    https://github.com/XCR1793/ARCOS.git#main

; Add the library subdirectory to the include path
build_flags = 
    -std=c++17
    -DTARGET_ESP32_Wroom32S3_Module
    -I.pio/libdeps/$PIOENV/ARCOS/ARCOS_library/include
```

### Option 3: Manual Clone Method

```bash
# In your project's lib/ folder:
git clone https://github.com/XCR1793/ARCOS.git
cd ARCOS/ARCOS_library
# Copy library files to parent or use symlink
```

## Recommended Actions

1. **Copy core library files to repository root:**
   ```bash
   cp ARCOS_library/library.json ./
   cp ARCOS_library/library.properties ./
   cp -r ARCOS_library/include ./
   cp -r ARCOS_library/examples ./
   cp ARCOS_library/README.md ./library_README.md
   cp ARCOS_library/LICENSE ./
   ```

2. **Update repository README.md to include library usage instructions**

3. **Keep ARCOS_library/ as development folder**

4. **Test with simple PlatformIO inclusion:**
   ```ini
   lib_deps = https://github.com/XCR1793/ARCOS.git#main
   ```

## Current Workaround Example

Until you restructure, here's how users can include your library:

```ini
[env:esp32s3]
platform = espressif32
board = esp32s3usbotg
framework = espidf

lib_deps = 
    https://github.com/XCR1793/ARCOS.git#main

build_flags = 
    -std=c++17
    -DTARGET_ESP32_Wroom32S3_Module
    -I.pio/libdeps/$PIOENV/ARCOS/ARCOS_library/include

# Then in code:
# #include <arcos.hpp>  # This will work with the include path
```