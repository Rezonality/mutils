#pragma once

#include <cstdint>

namespace MUtils
{

struct DeviceBufferFlags
{
    enum
    {
        IndexBuffer = (1 << 0),
        VertexBuffer = (1 << 1),
        UseUploadBuffer = (1 << 2)
    };
};

struct IDeviceBuffer
{
    // Ensure big enough
    virtual void EnsureSize(uint32_t size) = 0;

    // Map the buffer and get a pointer to the data - may be permanently mapped in some cases
    virtual void* Map(uint32_t num, uint32_t typeSize, uint32_t& offset) = 0;
    virtual void UnMap() = 0;

    // Upload before render
    virtual void Upload() = 0;

    virtual uint32_t GetByteSize() const = 0;

    // Bind to the device for geometry drawing
    virtual void Bind() const = 0;
    virtual void UnBind() const = 0;
};

} // MUtils