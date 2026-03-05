#pragma once

#include "engine_export.h"

#include <memory>
#include <string_view>

namespace engine
{

class TexturePrivate;

// RAII wrapper around an OpenGL 2D texture.
// Loads image data from disk via stb_image on init().
//
// Usage:
//   Texture tex;
//   if (!tex.init("assets/sprite.png")) { /* handle error */ }
//   tex.bind(0); // bind to texture unit 0 before draw
class ENGINE_EXPORT Texture
{
public:
    Texture();
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // Loads the image at path, uploads to GPU, generates mipmaps.
    // Returns false if the file cannot be read or GL upload fails.
    bool init(std::string_view path);

    // Loads from an in-memory image buffer (PNG, JPG, etc.).
    bool initFromData(const unsigned char *data, int size);

    bool isValid() const;

    int width() const;
    int height() const;

    // Binds the texture to the given texture unit (0-based).
    void bind(int unit) const;

    // Raw GL handle — used by Renderer2D to fill texture slots.
    unsigned int id() const;

private:
    std::unique_ptr<TexturePrivate> d;
};

} // namespace engine
