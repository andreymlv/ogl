#pragma once

#include "engine_export.h"
#include "layer.h"

#include <memory>
#include <vector>

namespace engine
{

class ENGINE_EXPORT LayerStack
{
public:
    LayerStack();
    ~LayerStack();

    void pushLayer(std::unique_ptr<Layer> layer);
    std::unique_ptr<Layer> popLayer(Layer *layer);
    void clear();

    void update(float dt);
    void render();

private:
    std::vector<std::unique_ptr<Layer>> m_layers;
};

} // namespace engine
