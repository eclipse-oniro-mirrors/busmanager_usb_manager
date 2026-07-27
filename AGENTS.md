# USB Manager - Agentic Coding Guide

High-signal context for agentic coding in this OpenHarmony USB Manager repository.

## Scope

Root-level guidance for the OpenHarmony USB Manager repository (`base/usb/usb_manager`).
Target: generic coding agents (OpenCode, Claude Code, Copilot, Codex, etc.).

**High-risk paths (require extra caution when modifying):**
- `services/native/src/usb_right_manager.cpp` — Permission logic (security-critical)
- `interfaces/innerkits/native/include/` — Public API boundary definitions
- `sa_profile/4201.json` — System Ability configuration
- `services/native/src/usb_service.cpp` — Service lifecycle entry point
- `usbmgr.gni` — Feature flags (affects global conditional compilation)

**Nested guidance:** No nested `AGENTS.md` or `.cursor/rules/` exist in this repo. All agent guidance is in this file.

## Build System

**GN (Generate Ninja) build system for OpenHarmony.**

```bash
# Build entire usb_manager component
./build.sh --product-name <product> --build-target usb_manager

# Build specific service targets
./build.sh --product-name <product> --build-target usbservice
./build.sh --product-name <product> --build-target //base/usb/usb_manager/interfaces/kits/js/napi:usbmanager
./build.sh --product-name <product> --build-target //base/usb/usb_manager/interfaces/kits/js/napi:serial

# Build tests
./build.sh --product-name <product> --build-target usb_unittest_test
./build.sh --product-name <product> --build-target usb_device_test
```

**Feature Flags** (`usbmgr.gni`) - Critical for conditional compilation:
- `usb_manager_feature_host` - USB host mode (device access, bulk/control transfer)
- `usb_manager_feature_device` - USB device mode (function switching)
- `usb_manager_feature_port` - USB port management (role switching)
- `usb_manager_pass_through` - Pass-through mode vs HAL mode
- `usb_manager_feature_support_cli` - Command-line tools

## Project Structure

```
usb_manager/
├── interfaces/
│   ├── innerkits/       # C++ APIs (UsbSrvClient, data models) + IPC stubs/proxies
│   └── kits/js/napi/    # NAPI bindings: usb, usbmanager, serial modules
├── services/
│   ├── native/          # C++ service impl (usb_service.cpp + host/device/port managers)
│   └── zidl/            # IPC interface definitions (.idl files)
├── frameworks/
│   ├── dialog/          # UI dialog for device permission requests
│   └── ets/taihe/       # ArkTS UI components and settings
├── test/
│   ├── native/          # C++ gtest (service_unittest, benchmarktest, fuzztest)
│   └── js_unittest/     # JS/TS test suites
├── utils/               # Common utilities (logger, errors, parcel helpers)
└── sa_profile/          # System Ability 4201 config
```

**Key Build Targets:**
- `usbservice` - Main system ability (libusbservice.z.so)
- `usbsrv_client` - Internal client library (libusbsrv_client.z.so)
- `usb`, `usbmanager`, `serial` - NAPI modules for JS/TS

**Frequently modified paths (by commit history):**
- `services/native/src/usb_right_manager.cpp`, `usb_device_manager.cpp`, `usb_host_manager.cpp` — Active service logic
- `frameworks/dialog/dialog_ui/usb_right_dialog/src/main/ets/pages/` — Dialog UI iteration
- `services/native/src/usb_service.cpp`, `serial_manager.cpp` — Service entry and serial handling

## Where to look for common tasks

| Task | Primary paths | Secondary paths |
| --- | --- | --- |
| Add new USB host API | `services/native/src/usb_host_manager.cpp`, `interfaces/kits/js/napi/src/` | `interfaces/innerkits/native/include/` |
| Fix permission flow | `services/native/src/usb_right_manager.cpp`, `services/native/src/usb_right_db_helper.cpp` | `frameworks/dialog/dialog_ui/` |
| Update NAPI bindings | `interfaces/kits/js/napi/src/`, `interfaces/kits/js/napi/BUILD.gn` | `interfaces/innerkits/native/include/` |
| Modify System Ability | `services/native/src/usb_service.cpp`, `sa_profile/4201.json` | `services/usb_service.cfg` |
| Change dialog UI | `frameworks/dialog/dialog_ui/usb_right_dialog/src/main/ets/` | `interfaces/kits/js/napi/src/` |
| Add feature flag | `usbmgr.gni`, `services/BUILD.gn`, `interfaces/innerkits/BUILD.gn` | `interfaces/kits/js/napi/BUILD.gn` |

## When to read deeper documentation

**Task-based routing:**
- Adding/modifying public APIs → Read `README.md` "Available APIs" section
- Changing permission flow → Read `services/native/src/usb_right_manager.cpp` and `services/native/src/usb_right_db_helper.cpp`
- Modifying dialog UI → Read `frameworks/dialog/dialog_ui/usb_right_dialog/` source files
- Working with HDI → Check `usbmgr.gni` for `USB_MANAGER_PASS_THROUGH` flag, then read relevant HDI interface docs
- Serial port changes → Read `interfaces/innerkits/native/include/usb_serial_type.h`
- System Ability changes → Read `sa_profile/4201.json` and `services/native/src/usb_service.cpp`

**Path-based routing:**
- Touching `interfaces/kits/js/napi/` → Read NAPI `BUILD.gn` and corresponding C++ API headers in `interfaces/innerkits/native/include/`
- Touching `services/zidl/` → Read IDL documentation and generated-code boundaries section below
- Touching `frameworks/dialog/` → Read UI architecture and verify NAPI API compatibility
- Touching `sa_profile/` → Read System Ability lifecycle docs and `services/usb_service.cfg`
- Touching `services/native/src/usb_right_manager.cpp` → Read permission flow section and `usb_right_db_helper.cpp`
- Touching `test/` → Mirror source structure under corresponding test directory

**Vocabulary-based routing:**
- "HDI", "pass-through", "HAL" → Check `usbmgr.gni` and `services/BUILD.gn` conditional compilation
- "SA 4201", "System Ability" → Read `sa_profile/4201.json` and `services/usb_service.cfg`
- "NAPI", "JS binding" → Read `interfaces/kits/js/napi/BUILD.gn` and source files
- "Permission", "right", "UsbRightManager" → Read permission flow section and `usb_right_manager.cpp`
- "Bulk transfer", "Control transfer" → Read `services/native/src/usb_host_manager.cpp`
- "Port role", "data role" → Read `services/native/src/usb_port_manager.cpp`

**Before editing:**
1. Identify task category (API change, permission flow, UI, HDI, NAPI, etc.)
2. Read relevant domain docs and key files
3. State constraints found in those docs

## Code Style

**Naming:**
- C++ Classes: PascalCase (`UsbHostManager`)
- C++ Methods: PascalCase (`GetDevices`, `OpenDevice`)  
- C++ Members: snake_case with trailing underscore (`usbRightManager_`)
- JS Functions: camelCase (`usb.connectDevice`)
- Constants: UPPER_SNAKE_CASE (`USB_ENDPOINT_XFER_CONTROL`)

**Conventions:**
- Apache 2.0 license header in all new files
- Imports: Own headers → System headers → Third-party
- Error codes: Use `UsbErrCode` enum, return `int32_t` (0 = success, negative = error)
- Logging: `USB_HILOGI/E/D(MODULE_USB_SERVICE, "message")`
- Namespace: `OHOS::USB` for all USB manager code
- Smart pointers: `std::shared_ptr` and `sptr<T>` (OHOS)
- Thread safety: `std::mutex` or `std::shared_mutex` for shared data
- Formatting: Webkit-based, 120 char limit (see .clang-format)

**Conditional Compilation Pattern:**
```cpp
#ifdef USB_MANAGER_FEATURE_HOST
    // Host mode code
#endif
#ifdef USB_MANAGER_FEATURE_DEVICE  
    // Device mode code
#endif
#ifdef USB_MANAGER_PASS_THROUGH
    // Pass-through mode code (v2.0 HDI)
#else
    // HAL mode code (v1.1/v1.2 HDI)
#endif
```

## Testing

**C++ Tests:** gtest framework, `<Module>Test` classes, `HWTEST_F` macros
**ArkTS Tests:** Hypium framework
**Locations:** `test/native/` mirrors source structure

**Test Workflows:**
```bash
# Run all C++ unit tests
./build.sh --product-name <product> --build-target usb_unittest_test --test

# Run specific test suites
./build.sh --product-name <product> --build-target service_unittest --test
./build.sh --product-name <product> --build-target usb_device_test --test
```

## Verification

**Minimum checks before reporting done:**
```bash
# Build the component (must pass with zero errors)
./build.sh --product-name <product> --build-target usb_manager

# Run C++ unit tests (must all pass)
./build.sh --product-name <product> --build-target usb_unittest_test --test

# Format check against .clang-format (Webkit-based, 120 char limit)
find services interfaces utils -name "*.cpp" -o -name "*.h" | \
  xargs clang-format --dry-run --Werror
```

**Task-specific validation:**
- API changes → Build `usb_device_test` + relevant JS unit tests in `test/native/js_unittest/`
- Permission flow changes → Build `service_unittest` + verify dialog UI loads correctly
- NAPI binding changes → Build all NAPI modules: `usb`, `usbmanager`, `serial`
- HDI changes → Verify against `min_hdi_proxy_version` in `sa_profile/4201.json`
- Feature flag changes → Build with flag both enabled and disabled to verify conditional compilation

**Done definition:**
Task is done when ALL of the following are true:
1. Code compiles without errors or new warnings
2. Relevant unit tests pass
3. No new lint/format violations (run against `.clang-format`)
4. Changes match task requirements exactly
5. No regressions in related functionality
6. New code follows namespace (`OHOS::USB`) and error code (`UsbErrCode`) conventions

**Final response should include:**
- What was changed and why (with `file:line` references)
- Commands run and their results
- Tests executed and outcomes (pass/fail counts)
- Any known limitations, edge cases, or follow-up work

**If validation cannot be run:**
- State which checks were skipped and the reason
- Provide manual verification steps for a reviewer to follow
- Explicitly flag risks of unverified changes

## Critical Business Logic

### USB Device Permission Request Flow

**Host Mode Access Flow:**
1. App calls `UsbSrvClient` API → `UsbService::HasRight()` check
2. If no right, `UsbService::RequestRight()` → `UsbRightManager::GetUserAgreementByDiag()`
3. `ShowUsbDialog()` sets dialog params, connects to UI Extension Ability
4. `UsbDialogAbility` loads `UsbDialog.ets`, shows permission dialog
5. User clicks "Allow"/"Deny" → Dialog calls NAPI API (`usbMgr.addDeviceAccessRight()`)
6. `OnAbilityDisconnectDone()` releases semaphore, returns to caller
7. Permission record stored in `UsbRightDatabase` via `UsbRightDbHelper`

**Key Files:**
- `services/native/src/usb_right_manager.cpp` - Permission management
- `services/native/src/usb_right_db_helper.cpp` - Database operations
- `frameworks/dialog/dialog_ui/usb_right_dialog/src/main/ets/ServiceExtAbility/UsbDialogAbility.ts` - Dialog lifecycle
- `frameworks/dialog/dialog_ui/usb_right_dialog/src/main/ets/pages/UsbDialog.ets` - Permission UI

**Dialog Components:**
- **UsbDialogAbility**: Extends `UIExtensionAbility`, `onSessionCreate()` stores want params in `globalThis.want`
- **UsbDialog.ets**: Shows dialog with app/device names, supports USB Device/Accessory/Serial Port types
- **DialogUtil**: `terminateDialog()` calls `session.terminateSelf()`
- **UserAuth**: `getUserAuth()` provides PIN/FACE/FINGERPRINT auth via `cryptoFramework`

### Permission Database Management

**Database** (`services/native/src/usb_right_db_helper.cpp`):
- Stores device access rights per app and device (temporary + permanent)
- Tracks expiration timestamps
- Auto-cleanup: uninstalled apps, deleted users, expired permissions

**Event Listeners** (`services/native/src/usb_right_manager.cpp:77`):
- `COMMON_EVENT_PACKAGE_REMOVED/BUNDLE_REMOVED` - App uninstall cleanup
- `COMMON_EVENT_UID_REMOVED/USER_REMOVED` - User deletion cleanup
- `COMMON_EVENT_PACKAGE_ADDED` - App reinstallation detection

### Device Access Flow

**OpenDevice Process:**
1. `UsbHostManager::OpenDevice()` checks if device has pipe
2. If no pipe, requests permission via `UsbRightManager::RequestRight()`
3. If permission granted, creates device pipe via `UsbDevicePipe`
4. Returns device handle to caller

**Serial Port Access:**
- Similar flow using `SerialDeviceIdentity`
- Permissions via `serialManager.addSerialRight()`
- Uses `portId` instead of device VID-PID

## Do not change without escalation

- **Public API signatures** in `interfaces/innerkits/native/include/` and `interfaces/kits/js/napi/` without compatibility review
- **Permission logic** in `services/native/src/usb_right_manager.cpp` without security review  
- **Database schema** in `services/native/src/usb_right_db_helper.cpp` without data migration plan
- **HDI version requirements** in `sa_profile/4201.json` without verifying system compatibility
- **System Ability lifecycle** in `services/native/src/usb_service.cpp` without understanding SA manager constraints

## Ask before

- Changing `USB_MANAGER_PASS_THROUGH` flag behavior
- Modifying event listener registrations for package/user removal
- Adding new feature flags that affect conditional compilation
- Changing memory limits (`MEMSIZE_MAX`) or transfer size constants
- Modifying authentication integration (PIN/FACE/FINGERPRINT)

## Generated-code boundaries

- IDL files in `services/zidl/` generate IPC stubs/proxies—do not edit generated code
- NAPI bindings in `interfaces/kits/js/napi/` must match C++ APIs—update both together
- `.clang-format` is the formatting source of truth—do not bypass

## Compatibility risks

- HDI v1.1/v1.2 vs v2.0: `USB_MANAGER_PASS_THROUGH` flag determines entire interaction layer
- Permission records: Database schema changes require migration strategy
- Dialog UI: Changes must maintain compatibility with existing NAPI APIs

## Hardware/device operation warnings

- Never execute USB device reset or port role switch on connected hardware without confirmation
- Transfer sizes must respect `MEMSIZE_MAX` (512MB) and endpoint-specific size constraints
- Port role changes may affect connected devices; verify with hardware team before testing
- Bulk transfer timeout values affect real device behavior; do not set to 0 (infinite) in production code

## DFX and observability constraints

- Do not remove or downgrade `USB_HILOGI/E/D` log points (fault attribution depends on them)
- `hisysevent.yaml` event definition changes require alignment with DFX team
- `USB_HILOGE` calls must not log sensitive data (device serial numbers, user tokens, auth challenges)

## Third-party dependency constraints

- New `external_deps` additions in `services/BUILD.gn` require license review (Apache 2.0 compatible only)
- 30+ existing components listed in `services/BUILD.gn:101-132` — do not change version pins without verifying all dependents
- Conditional dependencies (e.g., `libusb_proxy_2.0` vs `libusb_proxy_1.1`) are controlled by feature flags; never hardcode a version

## Important Gotchas

- **Always check feature flags** - Code is heavily conditional; never assume features are enabled
- **NAPI binding updates** - Adding new C++ APIs requires updating both C++ and NAPI bindings
- **System Ability lifecycle** - Service loads on-demand (`run-on-create: false`), auto-restarts
- **HDI version dependencies** - Service requires `min_hdi_proxy_version` compatibility
- **Thread safety critical** - Permission DB and shared state require mutex protection
- **Memory limits** - Transfers bounded by `MEMSIZE_MAX` (512MB) and size constants
- **Authentication integration** - Permission dialogs integrate with PIN/FACE/FINGERPRINT auth
- **Database cleanup** - Event listeners handle permission record lifecycle
- **Pass-through vs HAL** - `USB_MANAGER_PASS_THROUGH` flag changes entire HDI interaction layer

## System Ability Details

**SA 4201** - USB System Ability:
- Process: `usb_service`
- Library: `libusbservice.z.so`  
- On-demand loading with auto-restart
- Requires HDI proxies: `libusb_proxy_2.0.z.so`, `libserial_proxy_1.0.z.so`
