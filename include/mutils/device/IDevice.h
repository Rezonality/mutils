#pragma once

#include <cstdint>
#include <memory>

#include <mutils/device/IDeviceBuffer.h>

namespace MUtils
{

struct IDevice
{
    virtual void Init() = 0;
    virtual void Destroy() = 0;
    virtual std::shared_ptr<IDeviceBuffer> CreateBuffer(uint32_t size, uint32_t flags) = 0;
    virtual void Draw(IDeviceBuffer* pVertices, IDeviceBuffer* pIndices, uint32_t indexCount) = 0;
};

} // MUtils