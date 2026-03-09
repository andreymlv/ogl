#include "layer.h"

#include <QLoggingCategory>

#include "engine_logging.h"

namespace engine
{

Layer::Layer(QString name)
    : m_name(std::move(name))
{
    qCDebug(Engine) << "Layer created:" << m_name;
}

Layer::~Layer()
{
    qCDebug(Engine) << "Layer destroyed:" << m_name;
}

QString Layer::name() const
{
    return m_name;
}

} // namespace engine
