/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "usbmgrcapi_fuzzer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "ohusb_manager.h"

namespace OHOS {
namespace USB {

/*
 * Fuzzer overview
 * ----------------
 * The first byte of the input selects one of the cases below (an operation ID),
 * and the remaining bytes are consumed as a byte stream by a small cursor reader
 * (FuzzReader) to fill in the C API parameters. Every test case drives one or
 * more of the seven interfaces exported by libohusb_manager.so, covering both the
 * normal path and the null-invalid-parameter paths, plus boundary values and
 * multi-call flows.
 *
 * Input layout
 * ------------
 *   byte 0      : case selector, mapped to an entry of kCases[] by modulo
 *   byte 1..N   : payload, interpreted on demand by FuzzReader
 *
 * Safety invariants
 * -----------------
 *  - No fuzzed pointer is ever dereferenced. Structs are always stack-built and
 *    the API receives pointers to those locals.
 *  - Character strings are always copied into fixed NUL-terminated stack buffers.
 *  - OH_UsbManager_FreeUsbDeviceList is only ever called with NULL or with a
 *    pointer previously returned by OH_UsbManager_GetUsbDeviceList.
 */

namespace {

// Global tuning knobs (kept in an anonymous namespace to avoid ODR collisions).
constexpr size_t THRESHOLD = 4;                 // minimum input length accepted
constexpr size_t MAX_DEVICE_NAME_LEN = 64;      // cap for fuzzed device names
constexpr size_t MAX_NESTED_IFACES = 4;         // cap for synthetic configs
constexpr size_t MAX_NESTED_EPS = 4;            // cap for synthetic endpoints
constexpr uint32_t PERMISSION_REQUEST_LIMIT = 64;
constexpr uint32_t BITS_PER_BYTE = 8;           // shift amount for big-endian assembly
constexpr size_t ROTATE_BY_TWO = 2;             // rotation offset for boundary tables

// Bounded byte-stream reader. Reads consume bytes sequentially; reads that fall
// past the end of the buffer return a deterministic default (zero / false).
class FuzzReader {
public:
    FuzzReader(const uint8_t *data, size_t size) : data_(data), size_(size), pos_(0) {}

    // Number of bytes left to read.
    size_t Remaining() const
    {
        return (pos_ < size_) ? (size_ - pos_) : 0;
    }

    // Reads one byte, defaulting to 0 when exhausted.
    uint8_t ReadU8()
    {
        if (pos_ >= size_) {
            return 0;
        }
        return data_[pos_++];
    }

    // Reads two bytes in big-endian order, defaulting to 0 when short.
    uint16_t ReadU16()
    {
        uint32_t value = ReadU8();
        value = (value << BITS_PER_BYTE) | ReadU8();
        return static_cast<uint16_t>(value);
    }

    // Reads one byte and folds it to a boolean.
    bool ReadBool()
    {
        return (ReadU8() & 0x1U) != 0;
    }

    // Reads one byte and folds it into [0, upper) when upper > 0.
    uint8_t ReadBoundedU8(uint8_t upper)
    {
        if (upper == 0) {
            return 0;
        }
        return static_cast<uint8_t>(ReadU8() % upper);
    }

    // Reads up to (maxLen - 1) bytes into out and NUL-terminates it. Returns the
    // number of bytes copied (excluding the terminator).
    size_t ReadString(char *out, size_t maxLen)
    {
        if (out == nullptr || maxLen == 0) {
            return 0;
        }
        size_t cap = maxLen - 1;
        size_t copied = 0;
        while (copied < cap && pos_ < size_) {
            out[copied++] = static_cast<char>(data_[pos_++]);
        }
        out[copied] = '\0';
        return copied;
    }

private:
    const uint8_t *data_;
    size_t size_;
    size_t pos_;
};

} // namespace

/*
 * Struct builders
 * ---------------
 * Each builder turns the current position of a FuzzReader into a fully valid
 * (zero-initialized) struct. Values not consumed from the reader default to zero.
 * String fields are backed by process-owned stack buffers so the API never reads
 * from a fuzzed pointer.
 */

// Builds a flat OH_UsbManager_UsbPipe from two reader bytes.
OH_UsbManager_UsbPipe BuildPipe(FuzzReader &reader)
{
    OH_UsbManager_UsbPipe pipe = {};
    pipe.busNum = reader.ReadU8();
    pipe.devAddress = reader.ReadU8();
    return pipe;
}

// Builds an OH_UsbManager_UsbEndpoint and fills every scalar field.
OH_UsbManager_UsbEndpoint BuildEndpoint(FuzzReader &reader)
{
    OH_UsbManager_UsbEndpoint ep = {};
    ep.address = reader.ReadU8();
    ep.attributes = reader.ReadU8();
    ep.interval = reader.ReadU8();
    ep.maxPacketSize = reader.ReadU16();
    ep.direction = static_cast<OH_UsbManager_RequestDirection>(reader.ReadU8());
    ep.number = reader.ReadU8();
    ep.type = reader.ReadU8();
    ep.interfaceId = reader.ReadU8();
    return ep;
}

// Builds an OH_UsbManager_UsbInterface with a synthetic endpoint array. The name
// field is a NUL-terminated stack buffer owned by the caller's frame.
OH_UsbManager_UsbInterface BuildInterface(FuzzReader &reader, char *nameBuf, size_t nameBufLen)
{
    OH_UsbManager_UsbInterface iface = {};
    iface.id = reader.ReadU8();
    iface.protocol = reader.ReadU8();
    iface.clazz = reader.ReadU8();
    iface.subClass = reader.ReadU8();
    iface.alternateSetting = reader.ReadU8();
    if (nameBuf != nullptr && nameBufLen > 0) {
        reader.ReadString(nameBuf, nameBufLen);
        iface.name = nameBuf;
    }
    return iface;
}

// Builds an OH_UsbManager_UsbConfig with a synthetic interface array.
OH_UsbManager_UsbConfig BuildConfig(FuzzReader &reader, char *nameBuf, size_t nameBufLen)
{
    OH_UsbManager_UsbConfig cfg = {};
    cfg.id = reader.ReadU8();
    cfg.attributes = reader.ReadU8();
    cfg.maxPower = reader.ReadU8();
    if (nameBuf != nullptr && nameBufLen > 0) {
        reader.ReadString(nameBuf, nameBufLen);
        cfg.name = nameBuf;
    }
    cfg.isRemoteWakeup = reader.ReadBool();
    cfg.isSelfPowered = reader.ReadBool();
    return cfg;
}

// Builds an OH_UsbManager_UsbDevice and fills every scalar field, leaving the
// string/descriptor pointers null (they are documented as ignored by the
// connection path). A caller may attach a synthetic config array afterwards.
OH_UsbManager_UsbDevice BuildDevice(FuzzReader &reader)
{
    OH_UsbManager_UsbDevice device = {};
    device.busNum = reader.ReadU8();
    device.devAddress = reader.ReadU8();
    device.vendorId = reader.ReadU16();
    device.productId = reader.ReadU16();
    device.clazz = reader.ReadU8();
    device.subClass = reader.ReadU8();
    device.protocol = reader.ReadU8();
    return device;
}

/*
 * Shared helpers
 * --------------
 */

// No-op asynchronous permission callback. Unnamed parameters avoid
// unused-parameter warnings while keeping the signature intact.
void PermissionCallback(OH_UsbManager_ErrorCode /* errorCode */, bool /* result */, void * /* userContext */)
{
}

// Asynchronous marker callback used by the user-context test case. It forwards the
// received context so callers can observe pass-through behaviour without touching
// the fuzzed buffer.
void ContextCommonCallback(OH_UsbManager_ErrorCode /* errorCode */, bool /* result */, void * /* userContext */)
{
}

// Caps the total number of detached worker threads spawned by
// OH_UsbManager_RequestPermission across the whole fuzz run.
static uint32_t g_permRequestCount = 0;

/*
 * OH_UsbManager_GetUsbDeviceList / OH_UsbManager_FreeUsbDeviceList
 * -----------------------------------------------------------------
 * These two interfaces form a producer/consumer pair. The cases below cover the
 * normal enumerate-and-free flow, repeated enumeration, the null parameter
 * matrix, and the no-op free paths.
 */

// Enumerates and releases the returned list, exercising the deep copy and free
// logic inside libohusb_manager.
bool TestGetDeviceList()
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t deviceCount = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &deviceCount);
    if (ret == OH_USBMANAGER_SUCCESS && devices != nullptr && deviceCount > 0) {
        OH_UsbManager_FreeUsbDeviceList(devices, deviceCount);
    }
    return true;
}

// Enumerates twice in a row and frees both results, checking that the API does
// not rely on a single global backing buffer for the returned array.
bool TestGetDeviceListRepeated()
{
    OH_UsbManager_UsbDevice *first = nullptr;
    uint32_t firstCount = 0;
    OH_UsbManager_ErrorCode firstRet = OH_UsbManager_GetUsbDeviceList(&first, &firstCount);

    OH_UsbManager_UsbDevice *second = nullptr;
    uint32_t secondCount = 0;
    OH_UsbManager_ErrorCode secondRet = OH_UsbManager_GetUsbDeviceList(&second, &secondCount);

    if (firstRet == OH_USBMANAGER_SUCCESS && first != nullptr && firstCount > 0) {
        OH_UsbManager_FreeUsbDeviceList(first, firstCount);
    }
    if (secondRet == OH_USBMANAGER_SUCCESS && second != nullptr && secondCount > 0) {
        OH_UsbManager_FreeUsbDeviceList(second, secondCount);
    }
    return true;
}

// Invalid parameter matrix: every null combination must return
// OH_USBMANAGER_ERROR_INVALID_PARAMETER without crashing.
bool TestGetDeviceListNull()
{
    uint32_t deviceCount = 0;
    OH_UsbManager_UsbDevice *devices = nullptr;

    (void)OH_UsbManager_GetUsbDeviceList(nullptr, nullptr);
    (void)OH_UsbManager_GetUsbDeviceList(nullptr, &deviceCount);
    (void)OH_UsbManager_GetUsbDeviceList(&devices, nullptr);
    (void)OH_UsbManager_GetUsbDeviceList(nullptr, nullptr);
    return true;
}

// Frees a list pointer with a NULL base and zero/non-zero counts. Per the API
// contract, a NULL device pointer is always a safe no-op.
bool TestFreeDeviceListNoop()
{
    OH_UsbManager_FreeUsbDeviceList(nullptr, 0);
    OH_UsbManager_FreeUsbDeviceList(nullptr, 1);
    OH_UsbManager_FreeUsbDeviceList(nullptr, UINT32_MAX);
    return true;
}

/*
 * OH_UsbManager_ConnectDevice
 * ---------------------------
 * Only busNum and devAddress are interpreted; the remaining fields are ignored.
 * The cases exercise the minimal path, a fully populated (but synthetically
 * nested) struct, per-field fuzzing, boundary bus/address values, the null
 * parameter matrix, and a complete connect -> fd -> close flow.
 */

// Minimal connect: busNum and devAddress come from the first two payload bytes.
bool TestConnectDevice(const uint8_t *data, size_t size)
{
    FuzzReader reader(data, size);
    OH_UsbManager_UsbDevice device = {};
    device.busNum = reader.ReadU8();
    device.devAddress = reader.ReadU8();

    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    if (ret == OH_USBMANAGER_SUCCESS) {
        int32_t fd = -1;
        (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
        (void)OH_UsbManager_ClosePipe(&pipe);
    }
    return true;
}

// Connect with a fully populated device structure. All scalar fields are filled
// from the reader and a synthetic nested config/interface/endpoint tree is
// attached. The implementation must ignore everything but busNum/devAddress.
bool TestConnectDeviceFullStruct(const uint8_t *data, size_t size)
{
    FuzzReader reader(data, size);
    OH_UsbManager_UsbDevice device = BuildDevice(reader);

    // Attach a synthetic descriptor tree owned by this stack frame.
    OH_UsbManager_UsbConfig cfgs[MAX_NESTED_IFACES] = {};
    OH_UsbManager_UsbInterface ifaces[MAX_NESTED_IFACES] = {};
    OH_UsbManager_UsbEndpoint eps[MAX_NESTED_EPS] = {};
    char cfgNames[MAX_NESTED_IFACES][MAX_DEVICE_NAME_LEN] = {};
    char ifaceNames[MAX_NESTED_IFACES][MAX_DEVICE_NAME_LEN] = {};

    uint8_t cfgCount = reader.ReadBoundedU8(MAX_NESTED_IFACES);
    for (uint8_t c = 0; c < cfgCount; ++c) {
        cfgs[c] = BuildConfig(reader, cfgNames[c], MAX_DEVICE_NAME_LEN);
        uint8_t ifaceCount = reader.ReadBoundedU8(MAX_NESTED_IFACES);
        for (uint8_t i = 0; i < ifaceCount; ++i) {
            ifaces[i] = BuildInterface(reader, ifaceNames[i], MAX_DEVICE_NAME_LEN);
            uint8_t epCount = reader.ReadBoundedU8(MAX_NESTED_EPS);
            for (uint8_t e = 0; e < epCount; ++e) {
                eps[e] = BuildEndpoint(reader);
            }
            ifaces[i].endpoints = eps;
            ifaces[i].endpointCount = epCount;
        }
        cfgs[c].interfaces = ifaces;
        cfgs[c].interfaceCount = ifaceCount;
    }
    device.configs = cfgs;
    device.configCount = cfgCount;

    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);
    if (ret == OH_USBMANAGER_SUCCESS) {
        (void)OH_UsbManager_ClosePipe(&pipe);
    }
    return true;
}

// Connect with extreme boundary bus/address values selected by a low byte.
bool TestConnectDeviceBoundary(const uint8_t *data, size_t size)
{
    (void)data;
    (void)size;
    static constexpr uint8_t kBoundaries[] = { 0, 1, 127, 128, 254, 255 };
    constexpr size_t kCount = sizeof(kBoundaries) / sizeof(kBoundaries[0]);
    OH_UsbManager_UsbPipe pipe;

    for (size_t i = 0; i < kCount; ++i) {
        OH_UsbManager_UsbDevice device = {};
        device.busNum = kBoundaries[i];
        device.devAddress = kBoundaries[(i + 1) % kCount];
        pipe = {};
        (void)OH_UsbManager_ConnectDevice(&device, &pipe);
    }
    return true;
}

// Null parameter matrix for ConnectDevice.
bool TestConnectDeviceNull()
{
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_UsbDevice device = {};
    (void)OH_UsbManager_ConnectDevice(nullptr, &pipe);
    (void)OH_UsbManager_ConnectDevice(&device, nullptr);
    (void)OH_UsbManager_ConnectDevice(nullptr, nullptr);
    return true;
}

/*
 * OH_UsbManager_HasPermission
 * ---------------------------
 * The device name is the only input. Cases cover a fuzzed name, empty string,
 * single-character names, maximum-length names, separator/punctuation heavy
 * names, and the null parameter matrix.
 */

// Normal path with a fuzzed device name.
bool TestHasPermission(const uint8_t *data, size_t size)
{
    FuzzReader reader(data, size);
    char deviceName[MAX_DEVICE_NAME_LEN] = { 0 };
    reader.ReadString(deviceName, MAX_DEVICE_NAME_LEN);

    bool result = false;
    (void)OH_UsbManager_HasPermission(deviceName, &result);
    return true;
}

// Empty and single-character device names.
bool TestHasPermissionShortNames()
{
    char empty[MAX_DEVICE_NAME_LEN] = { 0 };
    empty[0] = '\0';
    bool result = false;
    (void)OH_UsbManager_HasPermission(empty, &result);

    char single[MAX_DEVICE_NAME_LEN] = { '1', '\0' };
    (void)OH_UsbManager_HasPermission(single, &result);
    return true;
}

// Maximum-length device name built from repeated fuzzed bytes.
bool TestHasPermissionLongName(const uint8_t *data, size_t size)
{
    FuzzReader reader(data, size);
    char deviceName[MAX_DEVICE_NAME_LEN] = {};
    for (size_t i = 0; i < MAX_DEVICE_NAME_LEN - 1; ++i) {
        deviceName[i] = static_cast<char>(reader.ReadU8());
    }
    deviceName[MAX_DEVICE_NAME_LEN - 1] = '\0';

    bool result = false;
    (void)OH_UsbManager_HasPermission(deviceName, &result);
    return true;
}

// Device names heavy on separators and punctuation, which stress the underlying
// bus/address parsing in HasRightEx.
bool TestHasPermissionMalformedNames()
{
    static constexpr const char *kNames[] = { "1-1", "1-", "-1", "1-1-1", "0-0", "255-255", "a-b",
        "1:1", "1.1", "1-1%", "%d-%d", "1 - 1", "1\\-1" };
    constexpr size_t kCount = sizeof(kNames) / sizeof(kNames[0]);

    for (size_t i = 0; i < kCount; ++i) {
        bool result = false;
        (void)OH_UsbManager_HasPermission(kNames[i], &result);
    }
    return true;
}

// Null parameter matrix for HasPermission.
bool TestHasPermissionNull()
{
    bool result = false;
    (void)OH_UsbManager_HasPermission(nullptr, &result);
    (void)OH_UsbManager_HasPermission("1-1", nullptr);
    (void)OH_UsbManager_HasPermission(nullptr, nullptr);
    return true;
}

/*
 * OH_UsbManager_RequestPermission
 * -------------------------------
 * RequestPermission spawns a detached worker thread and returns immediately, so
 * the total number of invocations is rate-limited. Cases cover the normal path,
 * a non-null user context, and the null parameter matrix.
 */

// Normal path, rate-limited to bound detached thread creation.
bool TestRequestPermission(const uint8_t *data, size_t size)
{
    if (g_permRequestCount >= PERMISSION_REQUEST_LIMIT) {
        return true;
    }
    ++g_permRequestCount;

    FuzzReader reader(data, size);
    char deviceName[MAX_DEVICE_NAME_LEN] = { 0 };
    reader.ReadString(deviceName, MAX_DEVICE_NAME_LEN);

    (void)OH_UsbManager_RequestPermission(deviceName, PermissionCallback, nullptr);
    return true;
}

// Request permission with a non-null user context pointer.
bool TestRequestPermissionWithContext(const uint8_t *data, size_t size)
{
    if (g_permRequestCount >= PERMISSION_REQUEST_LIMIT) {
        return true;
    }
    ++g_permRequestCount;

    FuzzReader reader(data, size);
    char deviceName[MAX_DEVICE_NAME_LEN] = { 0 };
    reader.ReadString(deviceName, MAX_DEVICE_NAME_LEN);

    int contextMarker = 0x5A5A;
    (void)OH_UsbManager_RequestPermission(deviceName, ContextCommonCallback, &contextMarker);
    return true;
}

// Null parameter matrix for RequestPermission.
bool TestRequestPermissionNull()
{
    (void)OH_UsbManager_RequestPermission(nullptr, PermissionCallback, nullptr);
    (void)OH_UsbManager_RequestPermission("1-1", nullptr, nullptr);
    (void)OH_UsbManager_RequestPermission(nullptr, nullptr, nullptr);
    return true;
}

/*
 * OH_UsbManager_GetFileDescriptor
 * --------------------------------
 * Only busNum and devAddress of the pipe are meaningful. Cases cover fuzzed
 * pipes, extreme boundary values, and the null parameter matrix.
 */

// Normal path with a fuzzed pipe.
bool TestGetFileDescriptor(const uint8_t *data, size_t size)
{
    FuzzReader reader(data, size);
    OH_UsbManager_UsbPipe pipe = BuildPipe(reader);
    int32_t fd = -1;
    (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
    return true;
}

// Boundary bus/address values for GetFileDescriptor.
bool TestGetFileDescriptorBoundary()
{
    static constexpr uint8_t kBoundaries[] = { 0, 1, 127, 128, 254, 255 };
    constexpr size_t kCount = sizeof(kBoundaries) / sizeof(kBoundaries[0]);

    for (size_t i = 0; i < kCount; ++i) {
        for (size_t j = 0; j < kCount; ++j) {
            OH_UsbManager_UsbPipe pipe = {};
            pipe.busNum = kBoundaries[i];
            pipe.devAddress = kBoundaries[j];
            int32_t fd = -1;
            (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
        }
    }
    return true;
}

// Null parameter matrix for GetFileDescriptor.
bool TestGetFileDescriptorNull()
{
    int32_t fd = -1;
    OH_UsbManager_UsbPipe pipe = {};
    (void)OH_UsbManager_GetFileDescriptor(nullptr, &fd);
    (void)OH_UsbManager_GetFileDescriptor(&pipe, nullptr);
    (void)OH_UsbManager_GetFileDescriptor(nullptr, nullptr);
    return true;
}

/*
 * OH_UsbManager_ClosePipe
 * ------------------------
 * Only busNum and devAddress are meaningful. Cases cover fuzzed pipes, boundary
 * values, and the null parameter path.
 */

// Normal path with a fuzzed pipe.
bool TestClosePipe(const uint8_t *data, size_t size)
{
    FuzzReader reader(data, size);
    OH_UsbManager_UsbPipe pipe = BuildPipe(reader);
    (void)OH_UsbManager_ClosePipe(&pipe);
    return true;
}

// Boundary bus/address values for ClosePipe.
bool TestClosePipeBoundary()
{
    static constexpr uint8_t kBoundaries[] = { 0, 1, 127, 128, 254, 255 };
    constexpr size_t kCount = sizeof(kBoundaries) / sizeof(kBoundaries[0]);

    for (size_t i = 0; i < kCount; ++i) {
        OH_UsbManager_UsbPipe pipe = {};
        pipe.busNum = kBoundaries[i];
        pipe.devAddress = kBoundaries[(i + 1) % kCount];
        (void)OH_UsbManager_ClosePipe(&pipe);
    }
    return true;
}

// Null parameter path for ClosePipe.
bool TestClosePipeNull()
{
    (void)OH_UsbManager_ClosePipe(nullptr);
    return true;
}

/*
 * Multi-call flows
 * ----------------
 * These composite cases chain several C API calls within a single fuzz iteration
 * to cover interactions between interfaces that would otherwise only run in
 * isolation.
 */

// Full lifecycle flow: enumerate, then connect with the first device's
// bus/address (from real API output, not fuzzed data), fetch the fd and close.
bool TestEnumerateThenConnect()
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t deviceCount = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &deviceCount);
    if (ret != OH_USBMANAGER_SUCCESS || devices == nullptr || deviceCount == 0) {
        return true;
    }

    OH_UsbManager_UsbDevice target = {};
    target.busNum = devices[0].busNum;
    target.devAddress = devices[0].devAddress;

    OH_UsbManager_UsbPipe pipe = {};
    ret = OH_UsbManager_ConnectDevice(&target, &pipe);
    if (ret == OH_USBMANAGER_SUCCESS) {
        int32_t fd = -1;
        (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
        (void)OH_UsbManager_ClosePipe(&pipe);
    }
    OH_UsbManager_FreeUsbDeviceList(devices, deviceCount);
    return true;
}

// Enumerate, query permission on the first device's name, then free the list.
// The device name is taken from the API output rather than the fuzzed buffer.
bool TestEnumerateThenHasPermission()
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t deviceCount = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &deviceCount);
    if (ret != OH_USBMANAGER_SUCCESS || devices == nullptr || deviceCount == 0) {
        return true;
    }

    bool result = false;
    if (devices[0].name != nullptr) {
        (void)OH_UsbManager_HasPermission(devices[0].name, &result);
    }
    OH_UsbManager_FreeUsbDeviceList(devices, deviceCount);
    return true;
}

// Connect with a fuzzed device, then redundantly close an unrelated fuzzed pipe
// and get the fd of a third pipe, stressing independent pipe descriptors.
bool TestMixedPipeFlow(const uint8_t *data, size_t size)
{
    FuzzReader reader(data, size);

    OH_UsbManager_UsbDevice device = {};
    device.busNum = reader.ReadU8();
    device.devAddress = reader.ReadU8();
    OH_UsbManager_UsbPipe pipe = {};
    OH_UsbManager_ErrorCode ret = OH_UsbManager_ConnectDevice(&device, &pipe);

    OH_UsbManager_UsbPipe unrelated = BuildPipe(reader);
    int32_t fd = -1;
    (void)OH_UsbManager_GetFileDescriptor(&unrelated, &fd);
    (void)OH_UsbManager_ClosePipe(&unrelated);

    if (ret == OH_USBMANAGER_SUCCESS) {
        (void)OH_UsbManager_ClosePipe(&pipe);
    }
    return true;
}

/*
 * Descriptor-tree walking
 * -----------------------
 * The enumerated device list returned by OH_UsbManager_GetUsbDeviceList contains
 * deeply nested config/interface/endpoint descriptors that were copied out of the
 * C++ data model by libohusb_manager. Read-only walking of that tree exercises
 * every accessor path of the deep-copy output. All reads are folded into a
 * volatile checksum so the compiler cannot eliminate them.
 */

// Volatile sink that keeps the compiler from optimizing away descriptor reads.
static volatile uint32_t g_checksum = 0;

// Maximum number of nested descriptors walked per level, bounding the read-only
// traversal of API-returned data.
constexpr uint32_t MAX_DESCRIPTOR_WALK = 64;

// Reads a single character from a possibly-null device string without ever
// dereferencing a fuzzed pointer (the string is API-owned).
uint32_t SummarizeString(const char *str)
{
    if (str == nullptr) {
        return 0;
    }
    return static_cast<uint32_t>(static_cast<uint8_t>(str[0]));
}

// Accumulates every scalar field of a single endpoint into the checksum sink.
void WalkEndpoint(const OH_UsbManager_UsbEndpoint &ep)
{
    g_checksum += ep.address;
    g_checksum += ep.attributes;
    g_checksum += ep.interval;
    g_checksum += ep.maxPacketSize;
    g_checksum += static_cast<uint32_t>(ep.direction);
    g_checksum += ep.number;
    g_checksum += ep.type;
    g_checksum += ep.interfaceId;
}

// Walks one interface, folding its scalar fields and each of its endpoints.
void WalkInterface(const OH_UsbManager_UsbInterface &iface)
{
    g_checksum += iface.id;
    g_checksum += iface.protocol;
    g_checksum += iface.clazz;
    g_checksum += iface.subClass;
    g_checksum += iface.alternateSetting;
    g_checksum += SummarizeString(iface.name);

    uint32_t epCount = iface.endpointCount;
    if (epCount > MAX_DESCRIPTOR_WALK) {
        epCount = MAX_DESCRIPTOR_WALK;
    }
    for (uint32_t e = 0; iface.endpoints != nullptr && e < epCount; ++e) {
        WalkEndpoint(iface.endpoints[e]);
    }
}

// Walks one config, folding its scalar fields and each of its interfaces.
void WalkConfig(const OH_UsbManager_UsbConfig &cfg)
{
    g_checksum += cfg.id;
    g_checksum += cfg.attributes;
    g_checksum += cfg.maxPower;
    g_checksum += cfg.isRemoteWakeup ? 1U : 0U;
    g_checksum += cfg.isSelfPowered ? 1U : 0U;
    g_checksum += SummarizeString(cfg.name);

    uint32_t ifaceCount = cfg.interfaceCount;
    if (ifaceCount > MAX_DESCRIPTOR_WALK) {
        ifaceCount = MAX_DESCRIPTOR_WALK;
    }
    for (uint32_t i = 0; cfg.interfaces != nullptr && i < ifaceCount; ++i) {
        WalkInterface(cfg.interfaces[i]);
    }
}

// Walks one device, folding its scalar fields and each of its configs.
void WalkDevice(const OH_UsbManager_UsbDevice &dev)
{
    g_checksum += dev.busNum;
    g_checksum += dev.devAddress;
    g_checksum += dev.vendorId;
    g_checksum += dev.productId;
    g_checksum += dev.clazz;
    g_checksum += dev.subClass;
    g_checksum += dev.protocol;
    g_checksum += SummarizeString(dev.name);
    g_checksum += SummarizeString(dev.manufacturerName);
    g_checksum += SummarizeString(dev.productName);
    g_checksum += SummarizeString(dev.version);

    uint32_t cfgCount = dev.configCount;
    if (cfgCount > MAX_DESCRIPTOR_WALK) {
        cfgCount = MAX_DESCRIPTOR_WALK;
    }
    for (uint32_t c = 0; dev.configs != nullptr && c < cfgCount; ++c) {
        WalkConfig(dev.configs[c]);
    }
}

// Walks the returned device array and every nested descriptor, folding all
// scalar field values and string first characters into the checksum sink.
bool TestEnumerateAndWalkDeviceTree()
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t deviceCount = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &deviceCount);
    if (ret != OH_USBMANAGER_SUCCESS || devices == nullptr || deviceCount == 0) {
        return true;
    }

    uint32_t devicesToWalk = (deviceCount < MAX_DESCRIPTOR_WALK) ? deviceCount : MAX_DESCRIPTOR_WALK;
    for (uint32_t d = 0; d < devicesToWalk; ++d) {
        WalkDevice(devices[d]);
    }

    OH_UsbManager_FreeUsbDeviceList(devices, deviceCount);
    return true;
}

/*
 * Field-level boundary coverage
 * -----------------------------
 * These cases drive the remaining scalar fields of OH_UsbManager_UsbDevice beyond
 * busNum/devAddress so that future implementations which begin interpreting
 * vendorId/productId/class/protocol still receive boundary inputs.
 */

// Connect with vendor/product/class/protocol at the extremes while keeping
// busNum/devAddress at a fixed valid-looking value.
bool TestConnectDeviceFieldBoundaries()
{
    static constexpr uint16_t kVidPid[] = { 0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF };
    static constexpr uint8_t kByte[] = { 0, 1, 127, 128, 254, 255 };
    constexpr size_t kCount = sizeof(kVidPid) / sizeof(kVidPid[0]);

    OH_UsbManager_UsbPipe pipe;
    for (size_t i = 0; i < kCount; ++i) {
        OH_UsbManager_UsbDevice device = {};
        device.busNum = 1;
        device.devAddress = 1;
        device.vendorId = kVidPid[i];
        device.productId = kVidPid[(i + 1) % kCount];
        device.clazz = kByte[i];
        device.subClass = kByte[(i + 1) % kCount];
        device.protocol = kByte[(i + ROTATE_BY_TWO) % kCount];
        pipe = {};
        (void)OH_UsbManager_ConnectDevice(&device, &pipe);
    }
    return true;
}

// A static corpus of device-name shapes covering common and adversarial formats.
static constexpr const char *kDeviceNameCorpus[] = {
    "1-1", "01-001", "1-1.1", "2-1:1.0", "0-0", "255-127", "128-254", "-1", "1-",
    "1-1-", "--", "1-1-1-1", "1.1", "1:1", "1%01", "1-1%20", "bus-dev", "a-b",
    "1 - 1", " 1-1", "1-1 ", "1\t-\t1", "999999999-999999999", "0x1-0x2",
};

// HasPermission over the full device-name corpus, one result per entry.
bool TestHasPermissionNameCorpus()
{
    constexpr size_t kCount = sizeof(kDeviceNameCorpus) / sizeof(kDeviceNameCorpus[0]);
    for (size_t i = 0; i < kCount; ++i) {
        bool result = false;
        (void)OH_UsbManager_HasPermission(kDeviceNameCorpus[i], &result);
    }
    return true;
}

// RequestPermission over the device-name corpus, strictly rate-limited so at most
// a bounded number of detached worker threads are created per run.
bool TestRequestPermissionNameCorpus()
{
    constexpr size_t kCount = sizeof(kDeviceNameCorpus) / sizeof(kDeviceNameCorpus[0]);
    for (size_t i = 0; i < kCount; ++i) {
        if (g_permRequestCount >= PERMISSION_REQUEST_LIMIT) {
            break;
        }
        ++g_permRequestCount;
        (void)OH_UsbManager_RequestPermission(kDeviceNameCorpus[i], PermissionCallback, nullptr);
    }
    return true;
}

// Exhaustive bus/address matrix for GetFileDescriptor.
bool TestGetFileDescriptorExhaustive()
{
    static constexpr uint8_t kValues[] = { 0, 1, 2, 3, 64, 127, 128, 254, 255 };
    constexpr size_t kCount = sizeof(kValues) / sizeof(kValues[0]);

    for (size_t i = 0; i < kCount; ++i) {
        for (size_t j = 0; j < kCount; ++j) {
            OH_UsbManager_UsbPipe pipe = {};
            pipe.busNum = kValues[i];
            pipe.devAddress = kValues[j];
            int32_t fd = -1;
            (void)OH_UsbManager_GetFileDescriptor(&pipe, &fd);
        }
    }
    return true;
}

// Exhaustive bus/address matrix for ClosePipe.
bool TestClosePipeExhaustive()
{
    static constexpr uint8_t kValues[] = { 0, 1, 2, 3, 64, 127, 128, 254, 255 };
    constexpr size_t kCount = sizeof(kValues) / sizeof(kValues[0]);

    for (size_t i = 0; i < kCount; ++i) {
        for (size_t j = 0; j < kCount; ++j) {
            OH_UsbManager_UsbPipe pipe = {};
            pipe.busNum = kValues[i];
            pipe.devAddress = kValues[j];
            (void)OH_UsbManager_ClosePipe(&pipe);
        }
    }
    return true;
}

// Three-stage flow: enumerate, query permission on the first device's name, then
// attempt a connect with its bus/address and clean up.
bool TestEnumerateHasPermissionThenConnect()
{
    OH_UsbManager_UsbDevice *devices = nullptr;
    uint32_t deviceCount = 0;
    OH_UsbManager_ErrorCode ret = OH_UsbManager_GetUsbDeviceList(&devices, &deviceCount);
    if (ret != OH_USBMANAGER_SUCCESS || devices == nullptr || deviceCount == 0) {
        return true;
    }

    if (devices[0].name != nullptr) {
        bool result = false;
        (void)OH_UsbManager_HasPermission(devices[0].name, &result);
    }

    OH_UsbManager_UsbDevice target = {};
    target.busNum = devices[0].busNum;
    target.devAddress = devices[0].devAddress;
    OH_UsbManager_UsbPipe pipe = {};
    ret = OH_UsbManager_ConnectDevice(&target, &pipe);
    if (ret == OH_USBMANAGER_SUCCESS) {
        (void)OH_UsbManager_ClosePipe(&pipe);
    }

    OH_UsbManager_FreeUsbDeviceList(devices, deviceCount);
    return true;
}

/*
 * Dispatch table
 * --------------
 * Each entry pairs a human-readable description with a case function. The first
 * input byte selects the entry (modulo the table size).
 */

using FuzzCaseFn = bool (*)(const uint8_t *data, size_t size);

struct CaseEntry {
    const char *name;    // human-readable case description
    FuzzCaseFn fn;       // case implementation
};

// Adapters that ignore the payload and match the no-argument cases to the FuzzCaseFn type.
bool GetDeviceListNoArg(const uint8_t *, size_t)
{
    return TestGetDeviceList();
}
bool GetDeviceListRepeatedNoArg(const uint8_t *, size_t)
{
    return TestGetDeviceListRepeated();
}
bool GetDeviceListNullNoArg(const uint8_t *, size_t)
{
    return TestGetDeviceListNull();
}
bool FreeDeviceListNoopNoArg(const uint8_t *, size_t)
{
    return TestFreeDeviceListNoop();
}
bool ConnectDeviceBoundaryNoArg(const uint8_t *, size_t)
{
    return TestConnectDeviceBoundary(nullptr, 0);
}
bool ConnectDeviceNullNoArg(const uint8_t *, size_t)
{
    return TestConnectDeviceNull();
}
bool HasPermissionShortNoArg(const uint8_t *, size_t)
{
    return TestHasPermissionShortNames();
}
bool HasPermissionMalformedNoArg(const uint8_t *, size_t)
{
    return TestHasPermissionMalformedNames();
}
bool HasPermissionNullNoArg(const uint8_t *, size_t)
{
    return TestHasPermissionNull();
}
bool RequestPermissionNullNoArg(const uint8_t *, size_t)
{
    return TestRequestPermissionNull();
}
bool GetFileDescriptorBoundaryNoArg(const uint8_t *, size_t)
{
    return TestGetFileDescriptorBoundary();
}
bool GetFileDescriptorNullNoArg(const uint8_t *, size_t)
{
    return TestGetFileDescriptorNull();
}
bool ClosePipeBoundaryNoArg(const uint8_t *, size_t)
{
    return TestClosePipeBoundary();
}
bool ClosePipeNullNoArg(const uint8_t *, size_t)
{
    return TestClosePipeNull();
}
bool EnumerateThenConnectNoArg(const uint8_t *, size_t)
{
    return TestEnumerateThenConnect();
}
bool EnumerateThenHasPermissionNoArg(const uint8_t *, size_t)
{
    return TestEnumerateThenHasPermission();
}
bool EnumerateAndWalkDeviceTreeNoArg(const uint8_t *, size_t)
{
    return TestEnumerateAndWalkDeviceTree();
}
bool ConnectDeviceFieldBoundariesNoArg(const uint8_t *, size_t)
{
    return TestConnectDeviceFieldBoundaries();
}
bool HasPermissionNameCorpusNoArg(const uint8_t *, size_t)
{
    return TestHasPermissionNameCorpus();
}
bool RequestPermissionNameCorpusNoArg(const uint8_t *, size_t)
{
    return TestRequestPermissionNameCorpus();
}
bool GetFileDescriptorExhaustiveNoArg(const uint8_t *, size_t)
{
    return TestGetFileDescriptorExhaustive();
}
bool ClosePipeExhaustiveNoArg(const uint8_t *, size_t)
{
    return TestClosePipeExhaustive();
}
bool EnumerateHasPermissionThenConnectNoArg(const uint8_t *, size_t)
{
    return TestEnumerateHasPermissionThenConnect();
}

// The ordered table of cases, each selected by the modulo of the first byte.
static const CaseEntry kCases[] = {
    { "GetUsbDeviceList + FreeUsbDeviceList normal path", GetDeviceListNoArg },
    { "GetUsbDeviceList repeated enumeration", GetDeviceListRepeatedNoArg },
    { "GetUsbDeviceList null parameter matrix", GetDeviceListNullNoArg },
    { "FreeUsbDeviceList no-op with NULL base", FreeDeviceListNoopNoArg },
    { "ConnectDevice normal path", TestConnectDevice },
    { "ConnectDevice full nested struct", TestConnectDeviceFullStruct },
    { "ConnectDevice boundary bus/address", ConnectDeviceBoundaryNoArg },
    { "ConnectDevice null parameter matrix", ConnectDeviceNullNoArg },
    { "HasPermission fuzzed name", TestHasPermission },
    { "HasPermission empty/single-char name", HasPermissionShortNoArg },
    { "HasPermission max-length name", TestHasPermissionLongName },
    { "HasPermission malformed names", HasPermissionMalformedNoArg },
    { "HasPermission null parameter matrix", HasPermissionNullNoArg },
    { "RequestPermission normal path (rate-limited)", TestRequestPermission },
    { "RequestPermission with user context", TestRequestPermissionWithContext },
    { "RequestPermission null parameter matrix", RequestPermissionNullNoArg },
    { "GetFileDescriptor fuzzed pipe", TestGetFileDescriptor },
    { "GetFileDescriptor boundary pipe", GetFileDescriptorBoundaryNoArg },
    { "GetFileDescriptor null parameter matrix", GetFileDescriptorNullNoArg },
    { "ClosePipe fuzzed pipe", TestClosePipe },
    { "ClosePipe boundary pipe", ClosePipeBoundaryNoArg },
    { "ClosePipe null parameter path", ClosePipeNullNoArg },
    { "Enumerate then Connect flow", EnumerateThenConnectNoArg },
    { "Enumerate then HasPermission flow", EnumerateThenHasPermissionNoArg },
    { "Mixed pipe flow", TestMixedPipeFlow },
    { "Enumerate and walk descriptor tree", EnumerateAndWalkDeviceTreeNoArg },
    { "ConnectDevice field-level boundaries", ConnectDeviceFieldBoundariesNoArg },
    { "HasPermission device-name corpus", HasPermissionNameCorpusNoArg },
    { "RequestPermission device-name corpus", RequestPermissionNameCorpusNoArg },
    { "GetFileDescriptor exhaustive bus/address", GetFileDescriptorExhaustiveNoArg },
    { "ClosePipe exhaustive bus/address", ClosePipeExhaustiveNoArg },
    { "Enumerate HasPermission then Connect flow", EnumerateHasPermissionThenConnectNoArg },
};

/*
 * Public fuzz entry
 * -----------------
 * Dispatches to one of kCases based on the first byte of the input.
 */
bool UsbMgrCapiFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < 1) {
        return false;
    }

    const uint8_t *payload = data + 1;
    size_t payloadSize = size - 1;

    constexpr size_t kCaseCount = sizeof(kCases) / sizeof(kCases[0]);
    size_t caseIndex = size_t{ data[0] } % kCaseCount;

    return kCases[caseIndex].fn(payload, payloadSize);
}

} // namespace USB
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < OHOS::USB::THRESHOLD) {
        return 0;
    }
    (void)OHOS::USB::UsbMgrCapiFuzzTest(data, size);
    return 0;
}