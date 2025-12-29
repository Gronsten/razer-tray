#pragma once

#include <windows.h>
#include <setupapi.h>
#include <memory>

// RAII wrapper for HDEVINFO handles from SetupAPI
class DeviceInfoHandle {
private:
    HDEVINFO handle;

public:
    DeviceInfoHandle() : handle(INVALID_HANDLE_VALUE) {}

    explicit DeviceInfoHandle(HDEVINFO h) : handle(h) {}

    // Prevent copying (handles should not be copied)
    DeviceInfoHandle(const DeviceInfoHandle&) = delete;
    DeviceInfoHandle& operator=(const DeviceInfoHandle&) = delete;

    // Allow moving
    DeviceInfoHandle(DeviceInfoHandle&& other) noexcept : handle(other.handle) {
        other.handle = INVALID_HANDLE_VALUE;
    }

    DeviceInfoHandle& operator=(DeviceInfoHandle&& other) noexcept {
        if (this != &other) {
            cleanup();
            handle = other.handle;
            other.handle = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    ~DeviceInfoHandle() {
        cleanup();
    }

    HDEVINFO get() const { return handle; }

    bool isValid() const {
        return handle != INVALID_HANDLE_VALUE && handle != nullptr;
    }

    // Release ownership without cleanup
    HDEVINFO release() {
        HDEVINFO temp = handle;
        handle = INVALID_HANDLE_VALUE;
        return temp;
    }

private:
    void cleanup() {
        if (isValid()) {
            SetupDiDestroyDeviceInfoList(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }
};

// RAII wrapper for GDI objects (HICON, HBITMAP, etc.)
template<typename T>
class GdiHandle {
private:
    T handle;

public:
    GdiHandle() : handle(nullptr) {}

    explicit GdiHandle(T h) : handle(h) {}

    // Prevent copying
    GdiHandle(const GdiHandle&) = delete;
    GdiHandle& operator=(const GdiHandle&) = delete;

    // Allow moving
    GdiHandle(GdiHandle&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    GdiHandle& operator=(GdiHandle&& other) noexcept {
        if (this != &other) {
            cleanup();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    ~GdiHandle() {
        cleanup();
    }

    T get() const { return handle; }

    bool isValid() const {
        return handle != nullptr;
    }

    T release() {
        T temp = handle;
        handle = nullptr;
        return temp;
    }

private:
    void cleanup() {
        if (isValid()) {
            DeleteObject(handle);
            handle = nullptr;
        }
    }
};

// Specialization for HICON (uses DestroyIcon instead of DeleteObject)
template<>
class GdiHandle<HICON> {
private:
    HICON handle;

public:
    GdiHandle() : handle(nullptr) {}

    explicit GdiHandle(HICON h) : handle(h) {}

    GdiHandle(const GdiHandle&) = delete;
    GdiHandle& operator=(const GdiHandle&) = delete;

    GdiHandle(GdiHandle&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    GdiHandle& operator=(GdiHandle&& other) noexcept {
        if (this != &other) {
            cleanup();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    ~GdiHandle() {
        cleanup();
    }

    HICON get() const { return handle; }

    bool isValid() const {
        return handle != nullptr;
    }

    HICON release() {
        HICON temp = handle;
        handle = nullptr;
        return temp;
    }

private:
    void cleanup() {
        if (isValid()) {
            DestroyIcon(handle);
            handle = nullptr;
        }
    }
};

using SafeIcon = GdiHandle<HICON>;
using SafeBitmap = GdiHandle<HBITMAP>;
