# ohos-usbManager

OpenHarmony USB CLI query tool for listing USB devices, accessories, and virtual serial ports.

## Directory Structure

```
ohos-usbManager/
├── src/                        # Source code
│   ├── usb_cli.cpp             # CLI entry point
│   ├── usb_cli_serialization.cpp  # JSON serialization
│   └── usb_cli_serialization.h    # Serialization header
├── tests/                      # Test code
├── docs/                       # Documentation
│   └── README.md               # Usage documentation
├── BUILD.gn                    # GN build configuration
├── config.json                 # Tool description file
└── README.md                   # This file
```

## Build

Add to build arguments:
```
usb_manager_feature_support_cli = true
```

Build target:
```
./build.sh --product-name <product> --build-target ohos-usbManager
```

## Installation

The binary is installed to `/system/bin/cli_tool/executable`.
