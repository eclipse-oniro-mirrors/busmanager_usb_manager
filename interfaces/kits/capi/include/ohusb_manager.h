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
 * @addtogroup UsbManager
 * @{
 *
 * @brief Provides the C interface for the USB Manager module, enabling USB device
 * enumeration, connection, permission management, and pipe operations.
 *
 * @since 26.1.0
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
 */
typedef enum OH_UsbManager_ErrorCode {
    /**
     * Operation successful.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_SUCCESS = 0,

    /**
     * Permission denied.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_ERROR_PERMISSION_DENIED = 14400001,

    /**
     * Service exception.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_ERROR_SERVICE_EXCEPTION = 14400004,

    /**
     * No such device (it may have been disconnected).
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_ERROR_NO_DEVICE = 14400008,

    /**
     * Insufficient memory.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_ERROR_NO_MEMORY = 14400009,

    /**
     * Transmission I/O error.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_ERROR_IO_ERROR = 14400012,

    /**
     * Invalid parameter. A null pointer is passed for a parameter that must not be null.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_ERROR_INVALID_PARAMETER = 14400014
} OH_UsbManager_ErrorCode;

/**
 * @brief Enumerates USB request directions.
 *
 * @since 26.1.0
 */
typedef enum OH_UsbManager_RequestDirection {
    /**
     * Request for writing data from the host to the device.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_REQUEST_DIR_TO_DEVICE = 0,

    /**
     * Request for reading data from the device to the host.
     *
     * @since 26.1.0
     */
    OH_USBMANAGER_REQUEST_DIR_FROM_DEVICE = 0x80
} OH_UsbManager_RequestDirection;

/**
 * @brief Defines the USB endpoint from which data is sent or received. An endpoint
 * is obtained from {@link OH_UsbManager_UsbInterface}.
 *
 * @since 26.1.0
 */
typedef struct OH_UsbManager_UsbEndpoint {
    /**
     * Endpoint address.
     *
     * @since 26.1.0
     */
    uint8_t address;

    /**
     * Endpoint attributes.
     *
     * @since 26.1.0
     */
    uint8_t attributes;

    /**
     * Endpoint interval for data transfers. For interrupt endpoints, the value
     * is in milliseconds. For isochronous endpoints, the unit depends on the
     * device speed.
     *
     * @since 26.1.0
     */
    uint8_t interval;

    /**
     * Maximum size of data packets on the endpoint. Unit: bytes.
     *
     * @since 26.1.0
     */
    uint16_t maxPacketSize;

    /**
     * Endpoint direction.
     *
     * @since 26.1.0
     */
    OH_UsbManager_RequestDirection direction;

    /**
     * Endpoint number.
     *
     * @since 26.1.0
     */
    uint8_t number;

    /**
     * Endpoint type.
     *
     * @since 26.1.0
     */
    uint8_t type;

    /**
     * Unique ID of the interface to which the endpoint belongs.
     *
     * @since 26.1.0
     */
    uint8_t interfaceId;
} OH_UsbManager_UsbEndpoint;

/**
 * @brief Defines a USB interface. One {@link OH_UsbManager_UsbConfig} can contain
 * multiple OH_UsbManager_UsbInterface instances, each providing a specific function.
 *
 * @since 26.1.0
 */
typedef struct OH_UsbManager_UsbInterface {
    /**
     * Unique ID of the USB interface.
     *
     * @since 26.1.0
     */
    uint8_t id;

    /**
     * Interface protocol.
     *
     * @since 26.1.0
     */
    uint8_t protocol;

    /**
     * Interface class.
     *
     * @since 26.1.0
     */
    uint8_t clazz;

    /**
     * Interface subclass.
     *
     * @since 26.1.0
     */
    uint8_t subClass;

    /**
     * Alternate setting number of this USB interface, as defined in the USB
     * interface descriptor. Value 0 indicates the default alternate setting.
     *
     * @since 26.1.0
     */
    uint8_t alternateSetting;

    /**
     * Interface name.
     *
     * @since 26.1.0
     */
    const char *name;

    /**
     * Endpoints that belong to the USB interface.
     *
     * @since 26.1.0
     */
    OH_UsbManager_UsbEndpoint *endpoints;

    /**
     * Number of endpoints in the interface.
     *
     * @since 26.1.0
     */
    uint32_t endpointCount;
} OH_UsbManager_UsbInterface;

/**
 * @brief Defines a USB configuration. One {@link OH_UsbManager_UsbDevice} can contain multiple
 * **OH_UsbManager_UsbConfig** instances.
 *
 * @since 26.1.0
 */
typedef struct OH_UsbManager_UsbConfig {
    /**
     * Unique ID of the USB configuration.
     *
     * @since 26.1.0
     */
    uint8_t id;

    /**
     * Configuration attributes.
     *
     * @since 26.1.0
     */
    uint8_t attributes;

    /**
     * Maximum power consumption. Unit: mA.
     *
     * @since 26.1.0
     */
    uint8_t maxPower;

    /**
     * Configuration name, which can be left empty.
     *
     * @since 26.1.0
     */
    const char *name;

    /**
     * Whether remote wakeup is supported. true indicates that remote wakeup is supported;
     * false indicates the opposite.
     *
     * @since 26.1.0
     */
    bool isRemoteWakeup;

    /**
     * Whether an independent power supply is supported. true indicates that an independent
     * power supply is supported; false indicates the opposite.
     *
     * @since 26.1.0
     */
    bool isSelfPowered;

    /**
     * Supported interface attributes.
     *
     * @since 26.1.0
     */
    OH_UsbManager_UsbInterface *interfaces;

    /**
     * Number of interfaces in the configuration.
     *
     * @since 26.1.0
     */
    uint32_t interfaceCount;
} OH_UsbManager_UsbConfig;

/**
 * @brief Defines a flat representation of a USB device.
 *
 * @since 26.1.0
 */
typedef struct OH_UsbManager_UsbDevice {
    /**
     * Bus number of the USB device.
     *
     * @since 26.1.0
     */
    uint8_t busNum;

    /**
     * Device address on the bus.
     *
     * @since 26.1.0
     */
    uint8_t devAddress;

    /**
     * Device name, in the format of <bus number>-<device address>.
     *
     * @since 26.1.0
     */
    const char *name;

    /**
     * Manufacturer name.
     *
     * @since 26.1.0
     */
    const char *manufacturerName;

    /**
     * Product name.
     *
     * @since 26.1.0
     */
    const char *productName;

    /**
     * Device version.
     *
     * @since 26.1.0
     */
    const char *version;

    /**
     * Vendor ID.
     *
     * @since 26.1.0
     */
    uint16_t vendorId;

    /**
     * Product ID.
     *
     * @since 26.1.0
     */
    uint16_t productId;

    /**
     * Device class.
     *
     * @since 26.1.0
     */
    uint8_t clazz;

    /**
     * Device subclass.
     *
     * @since 26.1.0
     */
    uint8_t subClass;

    /**
     * Device protocol.
     *
     * @since 26.1.0
     */
    uint8_t protocol;

    /**
     * Device configuration descriptor information.
     *
     * @since 26.1.0
     */
    OH_UsbManager_UsbConfig *configs;

    /**
     * Number of configurations in the device.
     *
     * @since 26.1.0
     */
    uint32_t configCount;
} OH_UsbManager_UsbDevice;

/**
 * @brief Defines the USB device pipe used to communicate with an opened device.
 *
 * @since 26.1.0
 */
typedef struct OH_UsbManager_UsbPipe {
    /**
     * Bus number of the connected device.
     *
     * @since 26.1.0
     */
    uint8_t busNum;

    /**
     * Device address of the connected device.
     *
     * @since 26.1.0
     */
    uint8_t devAddress;
} OH_UsbManager_UsbPipe;

/**
 * @brief Obtains the list of all connected USB devices. The caller must release the
 * returned array by calling {@link OH_UsbManager_FreeUsbDeviceList}.
 *
 * @param devices [out] Double pointer to the array of {@link OH_UsbManager_UsbDevice}. On success,
 *     the function allocates the array and all internal string buffers. The caller
 *     must NOT free individual fields; use {@link OH_UsbManager_FreeUsbDeviceList} instead.
 *     Must not be null.
 * @param deviceCount [out] Pointer to the number of devices returned. On success, this is
 *     set to the number of elements in the array. Zero indicates no devices present.
 *     Must not be null.
 * @return {@link OH_USBMANAGER_SUCCESS} if the operation is successful.
 *     <br>{@link OH_USBMANAGER_ERROR_SERVICE_EXCEPTION} if the USB service is unavailable. Possible cause:
 *     a USB service fault, for example the service is not running or has stopped unexpectedly.
 *     <br>{@link OH_USBMANAGER_ERROR_NO_MEMORY} if memory allocation for the device array or strings fails.
 *     Possible causes: insufficient system memory or too many connected devices. Suggested action: release
 *     unused memory and retry.
 *     <br>{@link OH_USBMANAGER_ERROR_INVALID_PARAMETER} if devices or deviceCount is NULL. Possible cause:
 *     a required parameter is not provided. Suggested action: pass valid non-null pointers.
 * @release OH_UsbManager_FreeUsbDeviceList {devices}
 * @since 26.1.0
 */
OH_UsbManager_ErrorCode OH_UsbManager_GetUsbDeviceList(OH_UsbManager_UsbDevice **devices,
    uint32_t *deviceCount);

/**
 * @brief Frees a device array previously returned by {@link OH_UsbManager_GetUsbDeviceList}.
 *
 * After this call, the pointer is invalid and must not be used. Passing null or a
 * count of 0 is a safe no-op.
 *
 * @param devices [in] Pointer to the array returned by {@link OH_UsbManager_GetUsbDeviceList}.
 * @param deviceCount [in] Number of elements in the array, as returned by
 *     {@link OH_UsbManager_GetUsbDeviceList}.
 * @since 26.1.0
 */
void OH_UsbManager_FreeUsbDeviceList(OH_UsbManager_UsbDevice *devices, uint32_t deviceCount);

/**
 * @brief Connects to a USB device and opens a pipe for communication. The returned pipe must be closed by calling
 * {@link OH_UsbManager_ClosePipe} to avoid resource leaks.
 *
 * Only the **busNum** and **devAddress** fields in the device structure are required. Other fields are ignored.
 *
 * @param device Pointer to the {@link OH_UsbManager_UsbDevice} to connect. This is an input parameter. This parameter cannot be left empty.
 * @param pipe Pointer to the {@link OH_UsbManager_UsbPipe}, which is used to receive the handle upon successful operation.
 *     This is an output parameter. This parameter cannot be left empty.
 * @return {@link OH_USBMANAGER_SUCCESS}: The connection is successful.
 *     <br>{@link OH_USBMANAGER_ERROR_PERMISSION_DENIED}: The app does not have the permission to access the device. Possible causes: The access permission has not been requested, the permission has been revoked, or the user has rejected the request. Suggestion: Call {@link OH_UsbManager_RequestPermission} to request the access permission.
 * <br>{@link OH_USBMANAGER_ERROR_SERVICE_EXCEPTION}: The USB service fails to open the device. Possible causes: The USB service is abnormal (for example, the service is not running or has stopped unexpectedly), or the input **device** is invalid. Suggestion: If **device** is invalid, call {@link OH_UsbManager_GetUsbDeviceList} to obtain valid device data and try again.
 *     <br>{@link OH_USBMANAGER_ERROR_IO_ERROR}: The device cannot be opened. For example, the device is disconnected or an I/O error occurs. Possible causes: The device is disconnected or an I/O error occurs on the USB bus. Suggestion: Check the physical connection and device status, and try again.
 *     <br>{@link OH_USBMANAGER_ERROR_INVALID_PARAMETER}: **device** or **pipe** is null. Possible cause: Mandatory parameters are not provided. Suggestion: Pass a valid non-null pointer.
 * @since 26.1.0
 */
OH_UsbManager_ErrorCode OH_UsbManager_ConnectDevice(const OH_UsbManager_UsbDevice *device,
    OH_UsbManager_UsbPipe *pipe);


/**
 * @brief Checks whether the application has permission to access the specified device.
 *
 * @param deviceName [in] Device name, in the format of <bus number>-<device address>. Must not be null.
 * @param result [out] Pointer to receive the result. true if the application has been
 *     granted permission to access the device; false if permission has not been
 *     granted or has not been requested. Must not be null.
 * @return {@link OH_USBMANAGER_SUCCESS} if the operation is successful.
 *     <br>{@link OH_USBMANAGER_ERROR_SERVICE_EXCEPTION} if the USB service is unavailable. Possible causes:
 *     a USB service fault (for example, the service is not running or has stopped unexpectedly), or the
 *     passed deviceName is invalid. Suggested action: if deviceName is invalid, call
 *     {@link OH_UsbManager_GetUsbDeviceList} to obtain a valid device name and retry.
 *     <br>{@link OH_USBMANAGER_ERROR_INVALID_PARAMETER} if deviceName or result is NULL. Possible cause:
 *     a required parameter is not provided. Suggested action: pass valid non-null pointers.
 * @since 26.1.0
 */
OH_UsbManager_ErrorCode OH_UsbManager_HasPermission(const char *deviceName, bool *result);

/**
 * @brief Defines the callback type used to return the result of
 * {@link OH_UsbManager_RequestPermission}.
 *
 * @param errorCode [out] Error code of the request. {@link OH_USBMANAGER_SUCCESS} means the
 *     request completed normally; other values indicate a service exception.
 * @param result [out] true if the permission is granted; false if the user denied the request.
 *     This parameter is meaningful only when errorCode is {@link OH_USBMANAGER_SUCCESS}.
 * @param userContext [out] User context passed through from {@link OH_UsbManager_RequestPermission}.
 * @since 26.1.0
 */
typedef void (*OH_UsbManager_PermissionCallback)(OH_UsbManager_ErrorCode errorCode, bool result,
    void *userContext);

/**
 * @brief Requests permission to access the specified USB device asynchronously.
 * This may trigger a system dialog asking the user for permission. The function
 * returns immediately and the result is delivered via the callback.
 *
 * @param deviceName [in] Device name, in the format of <bus number>-<device address>. Must not be null.
 * @param callback [in] {@link OH_UsbManager_PermissionCallback} invoked when the request completes.
 *     Must not be null.
 * @param userContext [in] User context pointer passed to the callback. May be null.
 * @return {@link OH_USBMANAGER_SUCCESS} if the request is successfully initiated.
 *     <br>{@link OH_USBMANAGER_ERROR_SERVICE_EXCEPTION} if the service fails to start the request. Possible
 *     causes: a USB service fault (for example, the service is not running or has stopped unexpectedly), or
 *     the passed deviceName is invalid. Suggested action: if deviceName is invalid, call
 *     {@link OH_UsbManager_GetUsbDeviceList} to obtain a valid device name and retry.
 *     <br>{@link OH_USBMANAGER_ERROR_INVALID_PARAMETER} if deviceName or callback is NULL. Possible cause:
 *     a required parameter is not provided. Suggested action: pass valid non-null pointers.
 * @since 26.1.0
 */
OH_UsbManager_ErrorCode OH_UsbManager_RequestPermission(const char *deviceName,
    OH_UsbManager_PermissionCallback callback, void *userContext);

/**
 * @brief Obtains the file descriptor for the opened USB device pipe. The fd can be
 * used for low-level ioctl-based USB transfers.
 *
 * @param pipe [in] Pointer to the {@link OH_UsbManager_UsbPipe} obtained from
 *     {@link OH_UsbManager_ConnectDevice}. Must not be null.
 * @param fd [out] Pointer to receive the file descriptor on success. Must not be null.
 * @return {@link OH_USBMANAGER_SUCCESS} if the operation is successful.
 *     <br>{@link OH_USBMANAGER_ERROR_PERMISSION_DENIED} if the app lacks device access permission. Possible causes:
 *     the access permission has not been requested, has been revoked, or the user denied the request. Suggested
 *     action: call {@link OH_UsbManager_RequestPermission} to request the access permission.
 *     <br>{@link OH_USBMANAGER_ERROR_SERVICE_EXCEPTION} if the pipe is invalid or the service fails. Possible
 *     causes: a USB service fault, or the pipe was not obtained from {@link OH_UsbManager_ConnectDevice} or
 *     has been closed. Suggested action: if the pipe is invalid or closed, obtain a valid open pipe from
 *     {@link OH_UsbManager_ConnectDevice} and retry.
 *     <br>{@link OH_USBMANAGER_ERROR_NO_DEVICE} if the device is not present or has been disconnected. Possible
 *     cause: the device has been unplugged. Suggested action: enumerate devices again with
 *     {@link OH_UsbManager_GetUsbDeviceList} and reconnect.
 *     <br>{@link OH_USBMANAGER_ERROR_INVALID_PARAMETER} if pipe or fd is NULL. Possible cause: a required
 *     parameter is not provided. Suggested action: pass valid non-null pointers.
 * @since 26.1.0
 */
OH_UsbManager_ErrorCode OH_UsbManager_GetFileDescriptor(const OH_UsbManager_UsbPipe *pipe,
    int32_t *fd);

/**
 * @brief Closes the USB device pipe and releases the underlying resources.
 * The pipe must be obtained from {@link OH_UsbManager_ConnectDevice}.
 *
 * @param pipe [in] Pointer to the {@link OH_UsbManager_UsbPipe} obtained from
 *     {@link OH_UsbManager_ConnectDevice} to close. Must not be null.
 * @return {@link OH_USBMANAGER_SUCCESS} if the pipe is closed successfully.
 *     <br>{@link OH_USBMANAGER_ERROR_PERMISSION_DENIED} if the app lacks device access permission. Possible causes:
 *     the access permission has not been requested, has been revoked, or the user denied the request. Suggested
 *     action: call {@link OH_UsbManager_RequestPermission} to request the access permission.
 *     <br>{@link OH_USBMANAGER_ERROR_SERVICE_EXCEPTION} if the close operation fails. Possible causes: a USB
 *     service fault, or the pipe is invalid or has already been closed. Suggested action: if the pipe is
 *     invalid or closed, obtain a valid open pipe from {@link OH_UsbManager_ConnectDevice} and retry.
 *     <br>{@link OH_USBMANAGER_ERROR_INVALID_PARAMETER} if pipe is NULL. Possible cause: a required parameter
 *     is not provided. Suggested action: pass valid non-null pointers.
 * @since 26.1.0
 */
OH_UsbManager_ErrorCode OH_UsbManager_ClosePipe(const OH_UsbManager_UsbPipe *pipe);

#ifdef __cplusplus
}
#endif

#endif  // OHUSB_MANAGER_H

/** @} */