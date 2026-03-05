#include "layerstack.h"

#include <algorithm>

namespace engine
{

LayerStack::LayerStack() = default;

LayerStack::~LayerStack()
{
    clear();
}

void LayerStack::pushLayer(std::unique_ptr<Layer> layer)
{
    layer->onAttach();
    m_layers.push_back(std::move(layer));
}

std::unique_ptr<Layer> LayerStack::popLayer(Layer *layer)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(), [layer](const std::unique_ptr<Layer> &l) {
        return l.get() == layer;
    });
    if (it == m_layers.end()) {
        return nullptr;
    }
    (*it)->onDetach();
    auto owned = std::move(*it);
    m_layers.erase(it);
    return owned;
}

void LayerStack::clear()
{
    for (auto &layer : m_layers) {
        layer->onDetach();
    }
    m_layers.clear();
}

void LayerStack::update(float dt)
{
    for (auto &layer : m_layers) {
        layer->onUpdate(dt);
    }
}

void LayerStack::render()
{
    for (auto &layer : m_layers) {
        layer->onRender();
    }
}

} // namespace engine
