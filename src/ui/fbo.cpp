#include <mutils/ui/fbo.h>
#include <mutils/logger/logger.h>

#include <GL/gl3w.h>

#undef ERROR
namespace MUtils
{

void fbo_bind(const Fbo& fbo)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    glViewport(0, 0, (int)fbo.size.x, (int)fbo.size.y);
}

void fbo_unbind(const Fbo& fbo, const NVec2i& displaySize)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, (int)displaySize.x, (int)displaySize.y);
}

Fbo fbo_create()
{
    Fbo fbo;
    glGenFramebuffers(1, &fbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

    // The texture we're going to render to
    glGenTextures(1, &fbo.texture);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, fbo.texture);

    glGenRenderbuffers(1, &fbo.depth);
    return fbo;
}

void fbo_clear(const NVec4f& color)
{
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void fbo_resize(Fbo& fbo, const NVec2i& newFboSize)
{
    if (fbo.size == newFboSize)
    {
        return;
    }

    fbo.size = newFboSize;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    glBindTexture(GL_TEXTURE_2D, fbo.texture);

    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbo.size.x, fbo.size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, fbo.depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, fbo.size.x, fbo.size.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo.texture, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo.depth);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG(ERROR, "FBO Error: " << (uint32_t)status);
    }
}

void fbo_destroy(const Fbo& fbo)
{
    glDeleteFramebuffers(1, &fbo.fbo);
    glDeleteRenderbuffers(1, &fbo.depth);
    glDeleteTextures(1, &fbo.texture);
}

} // namespace MUtils
