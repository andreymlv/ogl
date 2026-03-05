#include "layer.h"

namespace engine
{

Layer::Layer(QString name)
    : m_name(std::move(name))
{
}

Layer::~Layer() = default;

QString Layer::name() const
{
    return m_name;
}

} // namespace engine
