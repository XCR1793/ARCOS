# OWO Configuration Parser

A header-only C++ parser for `.owo` configuration files, designed for the ARCOS hardware abstraction framework.

## Features

- **Hierarchical Sections**: Support for nested sections using dot notation (e.g., `[item.subitem]`)
- **Variable Scoping**: Three levels of variable scope:
  - `local` (default): Available only in current section
  - `scoped`: Available to current section and all sub-sections  
  - `global`: Available globally across all sections
- **Multiple Data Types**: 
  - Numbers (floating-point)
  - Strings (quoted)
  - Booleans (`true`/`false`)
  - Arrays (mixed types supported)
  - Variable references (`{variableName}`)
- **Firmware Variables**: Runtime substitution of `{variable}` placeholders
- **Expression Support**: Basic arithmetic expressions
- **Comments**: Line comments using `;`
- **Version Headers**: Support for `# version x.x` headers

## File Format Specification

```owo
# version 0.0

[section]
variable = value
scoped_variable(scoped) = value
global_variable(global) = value

[section.subsection]
string_value = "Hello World"
number_value = 42.5
boolean_value = true
array_value = [1, 2, 3, "mixed", true]
firmware_ref = {firmwareVariable}
expression = (1 + 2) * 3
```

## Usage

### Basic Parsing

```cpp
#include "processing/parser/config_owo/config_owo.hpp"

using namespace arcos::processing::parser::config_owo;

// Create parser
OwoParser parser;

// Set firmware variables
parser.setFirmwareVariable("deviceId", "ESP32S3");

// Parse file
if(parser.parseFile("config.owo")){
  // Access values
  auto section = parser.getSection("device.network");
  auto ip = section->getVariable("ip_address");
  
  if(ip && ip->isString()){
    std::string ip_addr = ip->asString();
  }
}
```

### Type-Safe Value Extraction

```cpp
// Extract with automatic type conversion
std::string device_name;
int port_number;
std::vector<std::string> server_list;

OwoUtils::getValueByPath(parser, "device.name", device_name);
OwoUtils::getValueByPath(parser, "network.port", port_number);  
OwoUtils::getValueByPath(parser, "servers.list", server_list);
```

### Configuration Management

```cpp
// Use config manager for caching and global variables
OwoConfigManager manager;
manager.setGlobalFirmwareVariable("build_type", "DEBUG");

auto config = manager.loadConfig("app.owo");
```

## File Structure

```
include/processing/parser/config_owo/
├── config_owo.hpp          # Main include file
├── owo_parser.hpp          # Core parser declarations
├── owo_parser_impl.hpp     # Parser implementation
├── owo_utils.hpp           # Utility functions
├── owo_utils_impl.hpp      # Utilities implementation
├── owo_examples.hpp        # Usage examples
└── README.md               # This file
```

## Integration with ARCOS

This parser follows ARCOS coding standards:

- **Header-only design**: All implementation in `_impl.hpp` files
- **Namespace structure**: `arcos::processing::parser::config_owo`
- **Coding style**: ARCOS style guide compliance
- **No external dependencies**: Standard library only

## Example Configuration

```owo
# version 0.0

[device]
name = "ARCOS Device"
type = {DEVICE_TYPE}
firmware_version = {FIRMWARE_VERSION}

[network]
ip = "192.168.1.100"
port = 8080
enabled = true

[network.wifi]
ssid = "MyNetwork"
password = {WIFI_PASSWORD}
channel = 6

[sensors]
enabled_sensors = ["temperature", "humidity", "pressure"]
sample_rates = [1000, 500, 100]

[sensors.calibration]
temperature_offset = -2.5
humidity_gain = 1.05
pressure_coefficients = [
  [1.0, 0.0],
  [0.0, 1.0]
]
```

## Error Handling

The parser provides comprehensive error reporting:

```cpp
if(!parser.parseFile("config.owo")){
  std::cout << "Parse error: " << parser.getLastError() << std::endl;
}
```

## Performance Notes

- **Lazy evaluation**: Values parsed on-demand
- **Memory efficient**: Shared pointers for value storage
- **Caching**: Configuration manager provides file-level caching
- **No exceptions**: Error handling via return values and status methods

## Thread Safety

- Parser instances are **not thread-safe**
- Use separate parser instances per thread
- Configuration manager handles thread-safety internally