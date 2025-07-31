#ifndef TUNGSTEN_CORE_COMPONENT_SETUP_HPP
#define TUNGSTEN_CORE_COMPONENT_SETUP_HPP

#include <string>
#include <string_view>
#include <vector>
#include "TungstenUtils/TungstenUtils.hpp"

namespace wCore
{
    class ComponentSetup
    {
    public:
        static constexpr wIndex IDStart = 1;

        ComponentSetup()
            : m_typeNames()
        {
        }

        template<typename T>
        void Add(std::string_view typeName)
        {
            W_ASSERT(!ComponentID<T>::Get(), "Component: {} Allready added to ComponentSetup!", typeName);
            m_typeNames.emplace_back(typeName);
            ComponentID<T>::Set(m_typeNames.size());
        }

        template<typename T>
        const std::string& GetComponentTypeName() const
        {
            W_ASSERT(ComponentID<T>::Get(), "Type: {} not added to ComponentSetup", wUtils::DebugGetTypeName<T>());
            return GetComponentTypeNameFromID(ComponentID<T>::Get());
        }

        inline const std::string& GetComponentTypeNameFromID(wIndex id) const { W_ASSERT(id != 0, "Component Type Index 0 is null"); W_ASSERT(id <= m_typeNames.size(), "Component Type Index out of Range! id: {} Component Type Count: {}", id, m_typeNames.size()); return m_typeNames[id - 1]; }

        inline wIndex GetComponentTypeCount() { return m_typeNames.size(); }

    private:
        template<typename T>
        class ComponentID
        {
        public:
            static inline wIndex Get() { return s_id; }
            static inline void Set(wIndex id) { s_id = id; }

        private:
            static wIndex s_id;
        };

        std::vector<std::string> m_typeNames;

        friend class ComponentSystem;
    };

    template<typename T>
    wIndex ComponentSetup::ComponentID<T>::s_id = 0;
}

#endif