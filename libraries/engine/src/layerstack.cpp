#include "layerstack.h"

#include <QLoggingCategory>

#include "engine_logging.h"

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
    qCDebug(Engine) << "Pushing layer:" << layer->name();
    layer->onAttach();
    m_layers.push_back(std::move(layer));
    qCDebug(Engine) << "Layer stack size:" << m_layers.size();
}

std::unique_ptr<Layer> LayerStack::popLayer(Layer *layer)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(), [layer](const std::unique_ptr<Layer> &l) {
        return l.get() == layer;
    });
    if (it == m_layers.end()) {
        qCWarning(Engine) << "Attempted to pop layer not in stack";
        return nullptr;
    }
    qCDebug(Engine) << "Popping layer:" << (*it)->name();
    (*it)->onDetach();
    auto owned = std::move(*it);
    m_layers.erase(it);
    qCDebug(Engine) << "Layer stack size:" << m_layers.size();
    return owned;
}

void LayerStack::clear()
{
    qCDebug(Engine) << "Clearing layer stack," << m_layers.size() << "layers to detach";
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
