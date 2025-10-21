#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSetup.hpp"

namespace wCore
{
    ComponentSetup::ComponentSetup() noexcept
        : m_names(), m_types(), m_componentListCount(0), m_currentComponentTypeCount(0), m_currentComponentListCount(0)
    {
    }

    void ComponentSetup::CreateCtx::UpdateCurrentComponentListCount(wIndex componentTypeCount, wIndex componentListCount) noexcept
    {
        currentComponentTypeCount = componentTypeCount;
        currentComponentListCount = componentListCount;
    }
}