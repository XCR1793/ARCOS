# ARCOS Library - PlatformIO Integration

Since the actual library is located in the `ARCOS_library` subdirectory, you have several options for including this library in PlatformIO:

## Option 1: Direct Subdirectory Reference (Recommended)

```ini
[env:your_board]
platform = espressif32
board = esp32s3usbotg
framework = espidf

lib_deps = 
    https://github.com/XCR1793/ARCOS.git#main

# Tell PlatformIO where to find the library within the repository
lib_extra_dirs = ARCOS_library

build_flags = 
    -std=c++17
    -DTARGET_ESP32_Wroom32S3_Module
```

## Option 2: Manual Clone Method

```bash
# In your project directory
cd lib
git clone https://github.com/XCR1793/ARCOS.git
cd ARCOS
# Use only the library folder
cp -r ARCOS_library ../ARCOS_lib
cd ..
rm -rf ARCOS
mv ARCOS_lib ARCOS
```

## Option 3: Git Subtree (Advanced)

If you want to move the library to the repository root:

```bash
# In the ARCOS repository
git subtree push --prefix=ARCOS_library origin library-root
# Then users can reference the library-root branch
```

## Recommended Repository Restructuring

For the best PlatformIO experience, consider this structure:

```
ARCOS/
├── library.json          # Move from ARCOS_library/
├── library.properties    # Move from ARCOS_library/  
├── include/              # Move from ARCOS_library/include/
├── examples/             # Move from ARCOS_library/examples/
├── README.md             # Library README
├── LICENSE               # Move from ARCOS_library/
└── ARCOS_library/        # Keep as legacy/development folder
```

This would allow simple inclusion:
```ini
lib_deps = https://github.com/XCR1793/ARCOS.git#main
```