#pragma once

#include "mutils/logger/logger.h"

namespace MUtils
{
inline void CheckGL(const char* call, const char* file, int line)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        LOG(ERROR, std::hex << err << ", " << file << "(" << line << "): " << call);
    }
}

#ifdef _DEBUG
#ifdef _FULLGLCHECK
#define CHECK_GL(stmt) do { stmt; CheckGL(#stmt, __FILE__, __LINE__);  } while (0)
#else
// Mostly, the callback from GL will inform us of errors
#define CHECK_GL(stmt) do { stmt; } while(0)
#endif
#else
#define CHECK_GL(stmt) do { stmt; } while (0)
#endif

} // MUtils

