# ohos-usbManager Usage Guide

## Overview

`ohos-usbManager` is a command-line tool for querying USB device information on OpenHarmony systems. It provides read-only access to USB peripheral, accessory, and virtual serial port lists via JSON output.

## Installation Path

```
/system/bin/cli_tool/executable
```

## Command Syntax

```
ohos-usbManager [--help] [--version] <subcommand>
```

## Subcommands

### get-device-list

List all attached USB peripheral devices.

```bash
ohos-usbManager get-device-list
```

Output:
```json
{
  "number": 1,
  "list": [
    {
      "busNum": 1,
      "devAddress": 2,
      "serial": "serial_string",
      "name": "device_name",
      "manufacturerName": "manufacturer",
      "productName": "product",
      "version": "version_string",
      "vendorId": 1234,
      "productId": 5678,
      "clazz": 0,
      "subClass": 0,
      "protocol": 0,
      "configs": [...]
    }
  ]
}
```

### get-accessory-list

List all connected USB accessories.

```bash
ohos-usbManager get-accessory-list
```

Output:
```json
{
  "number": 1,
  "list": [
    {
      "manufacturer": "manufacturer_string",
      "product": "product_string",
      "description": "description_string",
      "version": "version_string",
      "serialNumber": "serial_string"
    }
  ]
}
```

### get-usbserial-list

List all available USB virtual serial ports.

```bash
ohos-usbManager get-usbserial-list
```

Output:
```json
{
  "number": 1,
  "list": [
    {
      "portId": 1,
      "deviceName": "1-2"
    }
  ]
}
```

## Flags

| Flag | Description |
|------|-------------|
| `--help` | Show usage information |
| `--version` | Show version information |

Flags can be used on the main command or any subcommand.

## Output Format

### Success Response

```json
{
  "number": <int>,
  "list": [<item>, ...]
}
```

When no devices are found:
```json
{"number": 0, "list": []}
```

### Error Response

```json
{
  "code": <int>,
  "message": "<string>"
}
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Failure (service error, permission denied, invalid arguments) |

## Permissions

The tool runs as a normal (non-system) process. It queries USB information through `UsbSrvClient` interfaces accessible at the normal permission level. If a query requires elevated permissions, a JSON error response is returned.

## Examples

```bash
# List all USB devices
ohos-usbManager get-device-list

# List all USB accessories
ohos-usbManager get-accessory-list

# List all virtual serial ports
ohos-usbManager get-usbserial-list

# Show help
ohos-usbManager --help

# Show version
ohos-usbManager --version

# Subcommand help
ohos-usbManager get-device-list --help
```
