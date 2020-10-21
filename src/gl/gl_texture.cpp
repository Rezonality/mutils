#include <mutils/gl/gl_texture.h>
#include <stb_image.h>

#include <cassert>

using namespace std;

namespace MUtils
{

GLuint gl_load_texture(const fs::path& path)
{
    GLuint textureName = 0;
    int w;
    int h;
    int comp;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(path.string().c_str(), &w, &h, &comp, STBI_default);

    assert(image != nullptr);
    if (image != nullptr)
    {
        glGenTextures(1, &textureName);
        glBindTexture(GL_TEXTURE_2D, textureName);

        if (comp == 3)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        }
        else if (comp == 4)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(image);
    }
    return textureName;
}

} // namespace MUtils
