#pragma once

#include "engine_export.h"

#include <QString>

namespace engine
{

class ENGINE_EXPORT Layer
{
public:
    explicit Layer(QString name);
    virtual ~Layer();

    Layer(const Layer &) = delete;
    Layer &operator=(const Layer &) = delete;

    QString name() const;

    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(float dt) { Q_UNUSED(dt) }
    virtual void onRender() {}

private:
    QString m_name;
};

} // namespace engine
