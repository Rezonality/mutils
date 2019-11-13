#include <cassert>
#include <cstdint>

#include <GL/gl3w.h>

#include <mutils/device/IDevice.h>
#include <mutils/gl/gl_buffer.h>
#include <mutils/gl/gl_helpers.h>

namespace MUtils
{

GLBuffer::GLBuffer(uint32_t size, uint32_t flags)
    : m_size(size)
    , m_flags(flags)
{
    CHECK_GL(glGenBuffers(1, &m_bufferID));
    Bind();
    CHECK_GL(glBufferData(GetBufferType(), size, NULL, GL_DYNAMIC_DRAW));
}

GLBuffer::~GLBuffer()
{
    CHECK_GL(glDeleteBuffers(1, &m_bufferID));
}

void GLBuffer::Bind() const
{
    CHECK_GL(glBindBuffer(GetBufferType(), m_bufferID));
}

void GLBuffer::UnBind() const
{
    CHECK_GL(glBindBuffer(GetBufferType(), 0));
}

void GLBuffer::EnsureSize(uint32_t size)
{
    // Force it to grow
    if (size > m_size)
    {
        Bind();
        CHECK_GL(glBufferData(GetBufferType(), size, NULL, GL_DYNAMIC_DRAW));
        m_size = size;
        m_offset = 0;
    }
}

// In this implementation, GL does not make a backing buffer, you are effectively writing to geometry
// you will directly draw from.  The device may decide to make a backing buffer behind your back,
// but we don't try to do it manually.  The DX12 implementation has a flag to pick...
void GLBuffer::Upload()
{
    return;
}

void* GLBuffer::Map(unsigned int num, unsigned int typeSize, unsigned int& offset)
{
    assert(!m_mapped);
    if (m_mapped)
    {
        return nullptr;
    }

    if (m_bufferID == 0)
    {
        return nullptr;
    }

    m_mapped = true;

    Bind();

    uint32_t byteSize = num * typeSize;

    EnsureSize(byteSize);

    if (m_reset || ((m_offset + byteSize) > m_size))
    {
        offset = 0;
        m_offset = byteSize;

        m_reset = false;
        return glMapBufferRange(GetBufferType(), 0, byteSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    }
    else
    {
        // Return the last offset we were up to
        offset = m_offset / typeSize;
        m_offset = m_offset + byteSize;

        return glMapBufferRange(GetBufferType(), offset * typeSize, byteSize, GL_MAP_WRITE_BIT /*| GL_MAP_INVALIDATE_RANGE_BIT*/ | GL_MAP_UNSYNCHRONIZED_BIT);
    }
}

void GLBuffer::UnMap()
{
    assert(m_mapped);
    if (m_mapped)
    {
        Bind();
        CHECK_GL(glUnmapBuffer(GetBufferType()));
        m_mapped = false;
    }
}

uint32_t GLBuffer::GetBufferType() const
{
    return (m_flags & DeviceBufferFlags::IndexBuffer) ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
}

} // namespace MUtils
