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

/**
 * @addtogroup OH_UsbManager
 * @{
 *
 * @brief Provides the C interface for the USB Manager module, enabling USB device
 * enumeration, connection, permission management, and pipe operations.
 *
 * @since 26.1.0
 * @version 1.0
 */

/**
 * @file ohusb_manager.h
 *
 * @brief Declares the C APIs for USB device management.
 *
 * @library libohusb_manager.so
 * @kit BasicServicesKit
 * @syscap SystemCapability.USB.USBManager
 * @since 26.1.0
 * @version 1.0
 */

#ifndef OHUSB_MANAGER_H
#define OHUSB_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enumerates the USB Manager error codes.
 *
 * @since 26.1.0
 * @version 1.0
 */
typedef enum {
    /** Operation successful. */
    USB_MANAGER_SUCCESS = 0,
    /** Permission denied. Mapped from BusinessError 14400001. */
    USB_MANAGER_ERROR_PERMISSION_DENIED = 14400001,
    /** Service exception. Mapped from BusinessError 14400004. */
    USB_MANAGER_ERROR_SERVICE_EXCEPTION = 14400004,
    /** No such device (it may have been disconnected). Mapped from BusinessError 14400008. */
    USB_MANAGER_ERROR_NO_DEVICE = 14400008,
    /** Insufficient memory. Mapped from BusinessError 14400009. */
    USB_MANAGER_ERROR_NO_MEMORY = 14400009,
    /** Transmission I/O error. Mapped from BusinessError 14400012. */
    USB_MANAGER_ERROR_IO_ERROR = 14400012,
    /** Invalid parameter. Mapped from BusinessError 14400014. */
    USB_MANAGER_ERROR_INVALID_PARAMETER = 14400014,
} UsbManager_ErrorCode;

/**
 * @brief Defines a USB endpoint descriptor.
 *
 * @since 26.1.0
 * @version 1.0
 */
typedef struct {
    /** Raw endpoint address byte. */
    uint32_t address;
    /** Endpoint attributes (transfer type, synchronization type, usage type). */
    uint32_t attributes;
    /** Polling interval in frames (for interrupt/isochronous) or microframes (for HS/SS). */
    int32_t interval;
    /** Maximum packet size in bytes that this endpoint can send or receive. */
    int32_t maxPacketSize;
} UsbManager_Endpoint;

/**
 * @brief Defines a USB interface descriptor and its associated endpoints.
 *
 * @since 26.1.0
 * @version 1.0
 */
typedef struct {
    /** Interface name. */
    const char *name;
    /** Interface ID. */
    int32_t id;
    /** Interface class code. */
    int32_t clazz;
    /** Interface sub-class code. */
    int32_t subClass;
    /** Alternate setting number. */
    int32_t alternateSetting;
    /** Interface protocol code. */
    int32_t protocol;
    /** Array of endpoints belonging to this interface. May be null if endpointCount is 0. */
    UsbManager_Endpoint *endpoints;
    /** Number of endpoints in the endpoints array. */
    uint32_t endpointCount;
} UsbManager_Interface;

/**
 * @brief Defines a USB configuration descriptor and its associated interfaces.
 *
 * @since 26.1.0
 * @version 1.0
 */
typedef struct {
    /** Configuration name. */
    const char *name;
    /** Configuration ID (bConfigurationValue). */
    int32_t id;
    /** Configuration attributes bitmask (self-powered, remote-wakeup, etc.). */
    uint32_t attributes;
    /** Maximum power consumption in milliamps (raw value * 2). */
    int32_t maxPower;
    /** Array of interfaces belonging to this configuration. May be null if interfaceCount is 0. */
    UsbManager_Interface *interfaces;
    /** Number of interfaces in the interfaces array. */
    uint32_t interfaceCount;
} UsbManager_Config;

/**
 * @brief Defines a flat representation of a USB device.
 *
 * @since 26.1.0
 * @version 1.0
 */
typedef struct {
    /** Bus number of the USB device. */
    uint8_t busNum;
    /** Device address on the bus. */
    uint8_t devAddress;
    /** Serial number of the device. */
    const char *serial;
    /** Device name, for example, /dev/bus/usb/001/002. */
    const char *name;
    /** Manufacturer name. */
    const char *manufacturerName;
    /** Product name. */
    const char *productName;
    /** Device version. */
    const char *version;
    /** Vendor ID. */
    int32_t vendorId;
    /** Product ID. */
    int32_t productId;
    /** Device class. */
    int32_t clazz;
    /** Device subclass. */
    int32_t subClass;
    /** Device protocol. */
    int32_t protocol;
    /** Array of USB configurations. May be null if configCount is 0. */
    UsbManager_Config *configs;
    /** Number of configurations in the configs array. */
    uint32_t configCount;
} UsbManager_Device;

/**
 * @brief Defines the USB device pipe used to communicate with an opened device.
 *
 * @since 26.1.0
 * @version 1.0
 */
typedef struct {
    /** Bus number of the connected device. */
    uint8_t busNum;
    /** Device address of the connected device. */
    uint8_t devAddress;
} UsbManager_DevicePipe;

/**
 * @brief Obtains the list of all connected USB devices. The caller must release the
 * returned array by calling {@link OH_UsbManager_FreeDeviceList}.
 *
 * @param devices Double pointer to the array of {@link UsbManager_Device}. On success,
 *     the function allocates the array and all internal string buffers. The caller
 *     must NOT free individual fields; use {@link OH_UsbManager_FreeDeviceList} instead.
 * @param deviceCount Pointer to the number of devices returned. On success, this is
 *     set to the number of elements in the array. Zero indicates no devices present.
 * @return {@link USB_MANAGER_SUCCESS} if the operation is successful.
 *     <br>{@link USB_MANAGER_ERROR_INVALID_PARAMETER} if devices or deviceCount is null.
 *     <br>{@link USB_MANAGER_ERROR_SERVICE_EXCEPTION} if the USB service is unavailable.
 *     <br>{@link USB_MANAGER_ERROR_NO_MEMORY} if memory allocation for the device array or strings fails.
 * @since 26.1.0
 */
UsbManager_ErrorCode OH_UsbManager_GetUsbDeviceList(UsbManager_Device **devices,
    uint32_t *deviceCount);

/**
 * @brief Frees a device array previously returned by {@link OH_UsbManager_GetUsbDeviceList}.
 *
 * After this call, the pointer is invalid and must not be used. Passing null or a
 * count of 0 is a safe no-op.
 *
 * @param devices Pointer to the array returned by {@link OH_UsbManager_GetUsbDeviceList}.
 * @param deviceCount Number of elements in the array, as returned by
 *     {@link OH_UsbManager_GetUsbDeviceList}.
 * @since 26.1.0
 */
void OH_UsbManager_FreeDeviceList(UsbManager_Device *devices, uint32_t deviceCount);

/**
 * @brief Connects to a USB device and opens a pipe for communication.
 *
 * Only the busNum and devAddress fields of the device struct are required; other
 * fields are ignored.
 *
 * @param device Pointer to the {@link UsbManager_Device} to connect. Must not be null.
 * @param pipe Pointer to a {@link UsbManager_DevicePipe} that receives the handle on
 *     success. Must not be null.
 * @return {@link USB_MANAGER_SUCCESS} if the connection is successful.
 *     <br>{@link USB_MANAGER_ERROR_INVALID_PARAMETER} if device or pipe is null.
 *     <br>{@link USB_MANAGER_ERROR_PERMISSION_DENIED} if the app lacks device access rights.
 *     <br>{@link USB_MANAGER_ERROR_SERVICE_EXCEPTION} if the USB service fails to open the device.
 *     <br>{@link USB_MANAGER_ERROR_IO_ERROR} if the device cannot be opened (e.g. disconnected or I/O failure).
 * @since 26.1.0
 */
UsbManager_ErrorCode OH_UsbManager_ConnectDevice(const UsbManager_Device *device,
    UsbManager_DevicePipe *pipe);

/**
 * @brief Checks whether the application has the right to access the specified device.
 *
 * @param deviceName Device name, for example, /dev/bus/usb/001/002. Must not be null.
 * @param result Pointer to receive the result. true if the application has the right;
 *     false otherwise. Must not be null.
 * @return {@link USB_MANAGER_SUCCESS} if the operation is successful.
 *     <br>{@link USB_MANAGER_ERROR_INVALID_PARAMETER} if deviceName or result is null.
 *     <br>{@link USB_MANAGER_ERROR_SERVICE_EXCEPTION} if the USB service is unavailable.
 * @since 26.1.0
 */
UsbManager_ErrorCode OH_UsbManager_HasPermission(const char *deviceName, bool *result);

/**
 * @brief Defines the callback type used to return the result of
 * {@link OH_UsbManager_RequestPermission}.
 *
 * @param errorCode Error code of the request. {@link USB_MANAGER_SUCCESS} means the
 *     request completed normally; other values indicate a service exception.
 * @param result true if the right is granted; false if the user denied the request.
 *     This parameter is meaningful only when errorCode is {@link USB_MANAGER_SUCCESS}.
 * @param userData User data passed through from {@link OH_UsbManager_RequestPermission}.
 * @since 26.1.0
 */
typedef void (*UsbManager_PermissionCallback)(UsbManager_ErrorCode errorCode, bool result,
    void *userData);

/**
 * @brief Requests the right to access the specified USB device asynchronously.
 * This may trigger a system dialog asking the user for permission. The function
 * returns immediately and the result is delivered via the callback.
 *
 * @param deviceName Device name, for example, /dev/bus/usb/001/002. Must not be null.
 * @param callback {@link UsbManager_PermissionCallback} invoked when the request completes.
 *     Must not be null.
 * @param userData User data pointer passed to the callback. May be null.
 * @return {@link USB_MANAGER_SUCCESS} if the request is successfully initiated.
 *     <br>{@link USB_MANAGER_ERROR_INVALID_PARAMETER} if deviceName or callback is null.
 *     <br>{@link USB_MANAGER_ERROR_SERVICE_EXCEPTION} if the service fails to start the request.
 * @since 26.1.0
 */
UsbManager_ErrorCode OH_UsbManager_RequestPermission(const char *deviceName,
    UsbManager_PermissionCallback callback, void *userData);

/**
 * @brief Obtains the file descriptor for the opened USB device pipe. The fd can be
 * used for low-level ioctl-based USB transfers.
 *
 * @param pipe Pointer to the {@link UsbManager_DevicePipe} obtained from
 *     {@link OH_UsbManager_ConnectDevice}. Must not be null.
 * @param fd Pointer to receive the file descriptor on success. Must not be null.
 * @return {@link USB_MANAGER_SUCCESS} if the operation is successful.
 *     <br>{@link USB_MANAGER_ERROR_INVALID_PARAMETER} if pipe or fd is null.
 *     <br>{@link USB_MANAGER_ERROR_PERMISSION_DENIED} if the app lacks device access rights.
 *     <br>{@link USB_MANAGER_ERROR_SERVICE_EXCEPTION} if the pipe is invalid or the service fails.
 *     <br>{@link USB_MANAGER_ERROR_NO_DEVICE} if the device is not present or has been disconnected.
 * @since 26.1.0
 */
UsbManager_ErrorCode OH_UsbManager_GetFileDescriptor(const UsbManager_DevicePipe *pipe,
    int32_t *fd);

/**
 * @brief Closes the USB device pipe and releases the underlying resources.
 *
 * @param pipe Pointer to the {@link UsbManager_DevicePipe} to close. Must not be null.
 * @return {@link USB_MANAGER_SUCCESS} if the pipe is closed successfully.
 *     <br>{@link USB_MANAGER_ERROR_INVALID_PARAMETER} if pipe is null.
 *     <br>{@link USB_MANAGER_ERROR_PERMISSION_DENIED} if the app lacks device access rights.
 *     <br>{@link USB_MANAGER_ERROR_SERVICE_EXCEPTION} if the close operation fails.
 * @since 26.1.0
 */
UsbManager_ErrorCode OH_UsbManager_ClosePipe(const UsbManager_DevicePipe *pipe);

#ifdef __cplusplus
}
#endif

#endif  // OHUSB_MANAGER_H

/** @} */
