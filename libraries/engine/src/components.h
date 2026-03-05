#pragma once

#include "texture.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>

namespace engine
{

// ── Transform ─────────────────────────────────────────────────────────────────

struct Transform {
    glm::vec2 position{0.F, 0.F};
    glm::vec2 scale{1.F, 1.F};
    float rotation = 0.F; // degrees
};

// ── RigidBody2D ───────────────────────────────────────────────────────────────

struct RigidBody2D {
    glm::vec2 velocity{0.F, 0.F};
    float gravityScale = 1.F;
};

// ── BoxCollider2D ─────────────────────────────────────────────────────────────

struct BoxCollider2D {
    glm::vec2 size{1.F, 1.F};
    bool isTrigger = false;
};

// ── Sprite ────────────────────────────────────────────────────────────────────

struct Sprite {
    std::shared_ptr<Texture> texture; // null → draw solid color quad
    glm::vec4 color{1.F, 1.F, 1.F, 1.F};
    int zOrder = 0;
};

// ── Tag ───────────────────────────────────────────────────────────────────────

struct Tag {
    std::string name;
};

} // namespace engine
