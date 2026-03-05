#include "texture.h"

#include "globject.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <QLoggingCategory>

#include "engine_logging.h"

namespace engine
{

// ── Deleter ───────────────────────────────────────────────────────────────────

inline void deleteTexture(GLuint id)
{
    glDeleteTextures(1, &id);
}

using GlTexture = GlObject<deleteTexture>;

// ── Private ───────────────────────────────────────────────────────────────────

class TexturePrivate
{
public:
    GlTexture m_texture{0};
    int m_width = 0;
    int m_height = 0;
    bool m_valid = false;
};

// ── Texture ───────────────────────────────────────────────────────────────────

Texture::Texture()
    : d(std::make_unique<TexturePrivate>())
{
}

Texture::~Texture() = default;

// Shared upload logic — takes decoded pixel data + dimensions + channel count.
static bool uploadPixels(TexturePrivate &priv, unsigned char *pixels, int width, int height, int channels)
{
    const GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    GLuint id = 0;
    glGenTextures(1, &id);
    priv.m_texture = GlTexture{id};

    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format),
                 width, height, 0,
                 format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    priv.m_width = width;
    priv.m_height = height;
    priv.m_valid = true;
    return true;
}

bool Texture::init(std::string_view path)
{
    d->m_valid = false;
    d->m_texture = GlTexture{0};
    d->m_width = 0;
    d->m_height = 0;

    stbi_set_flip_vertically_on_load(1);

    int w = 0;
    int h = 0;
    int channels = 0;
    unsigned char *data = stbi_load(path.data(), &w, &h, &channels, 0);
    if (!data) {
        qCCritical(Engine) << "stb_image failed to load:" << path.data() << stbi_failure_reason();
        return false;
    }

    const bool ok = uploadPixels(*d, data, w, h, channels);
    stbi_image_free(data);
    return ok;
}

bool Texture::initFromData(const unsigned char *data, int size)
{
    d->m_valid = false;
    d->m_texture = GlTexture{0};
    d->m_width = 0;
    d->m_height = 0;

    stbi_set_flip_vertically_on_load(1);

    int w = 0;
    int h = 0;
    int channels = 0;
    unsigned char *pixels = stbi_load_from_memory(data, size, &w, &h, &channels, 0);
    if (!pixels) {
        qCCritical(Engine) << "stb_image failed to decode from memory:" << stbi_failure_reason();
        return false;
    }

    const bool ok = uploadPixels(*d, pixels, w, h, channels);
    stbi_image_free(pixels);
    return ok;
}

bool Texture::isValid() const
{
    return d->m_valid;
}

int Texture::width() const
{
    return d->m_width;
}

int Texture::height() const
{
    return d->m_height;
}

void Texture::bind(int unit) const
{
    glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, d->m_texture.id());
}

unsigned int Texture::id() const
{
    return d->m_texture.id();
}

} // namespace engine
