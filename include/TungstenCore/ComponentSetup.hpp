#ifndef TUNGSTEN_CORE_COMPONENT_SETUP_HPP
#define TUNGSTEN_CORE_COMPONENT_SETUP_HPP

#include <string>
#include <string_view>
#include <vector>
#include "TungstenUtils/TungstenUtils.hpp"

namespace wCore
{
    using ComponentTypeIndex = wIndex;
    inline constexpr ComponentTypeIndex InvalidComponentType = 0;
    inline constexpr ComponentTypeIndex ComponentTypeIndexStart = 1;

    struct ComponentLayout
    {
        std::size_t size{};
        std::size_t alignment{};
    };

    class ComponentSetup
    {
    public:
        ComponentSetup();

        ComponentSetup(const ComponentSetup&) = delete;
        ComponentSetup& operator=(const ComponentSetup&) = delete;

        template<typename T>
        void Add(std::string_view typeName)
        {
            W_ASSERT(!StaticComponentID<T>::Get(), "Component: {} Allready added to ComponentSetup!", typeName);
            m_names.emplace_back(typeName);
            m_types.emplace_back(ComponentLayout{sizeof(T), alignof(T)}, &ConstructInPlace<T>, &DestroyInPlace<T>);
            StaticComponentID<T>::Set(m_names.size());
        }

        template<typename T>
        ComponentTypeIndex GetComponentTypeIndex() const
        {
            W_ASSERT(StaticComponentID<T>::Get(), "Type: {} not added to ComponentSetup", wUtils::DebugGetTypeName<T>());
            return StaticComponentID<T>::Get();
        }

        template<typename T>
        std::string_view GetComponentTypeName() const
        {
            W_ASSERT(GetComponentTypeIndex<T>(), "Type: {} not added to ComponentSetup", wUtils::DebugGetTypeName<T>());
            return GetComponentTypeNameFromTypeIndex(StaticComponentID<T>::Get());
        }

        inline std::string_view GetComponentTypeNameFromTypeIndex(ComponentTypeIndex componentTypeIndex) const { W_ASSERT(componentTypeIndex != 0, "ComponentTypeIndex 0 is Invalid"); W_ASSERT(componentTypeIndex <= m_names.size(), "ComponentTypeIndex: {} out of Range! Component Type Count: {}", componentTypeIndex, m_names.size()); return m_names[componentTypeIndex - 1]; }
        inline ComponentLayout GetComponentLayoutFromTypeIndex(ComponentTypeIndex componentTypeIndex) const { W_ASSERT(componentTypeIndex != 0, "ComponentTypeIndex 0 is Invalid"); W_ASSERT(componentTypeIndex <= m_layouts.size(), "ComponentTypeIndex: {} out of Range! Component Type Count: {}", componentTypeIndex, m_layouts.size()); return m_types[componentTypeIndex - 1].layout; }
        inline void ConstructComponentInPlaceFromTypeIndex(ComponentTypeIndex componentTypeIndex, void* dest) const { W_ASSERT(componentTypeIndex != 0, "ComponentTypeIndex 0 is Invalid"); W_ASSERT(componentTypeIndex <= m_layouts.size(), "ComponentTypeIndex: {} out of Range! Component Type Count: {}", componentTypeIndex, m_layouts.size()); m_types[componentTypeIndex - 1].constructor(dest); }
        inline void DestroyComponentInPlaceFromTypeIndex(ComponentTypeIndex componentTypeIndex, void* dest) const { W_ASSERT(componentTypeIndex != 0, "ComponentTypeIndex 0 is Invalid"); W_ASSERT(componentTypeIndex <= m_layouts.size(), "ComponentTypeIndex: {} out of Range! Component Type Count: {}", componentTypeIndex, m_layouts.size()); m_types[componentTypeIndex - 1].destructor(dest); }

        inline wIndex GetComponentTypeCount() const noexcept { return m_names.size(); }

    private:
        using ComponentConstructorFn = void(*)(void* destination);
        using ComponentDestructorFn = void(*)(void* destination);

        struct ComponentType
        {
            ComponentType(ComponentLayout a_componentLayout, ComponentConstructorFn a_constructor, ComponentDestructorFn a_destructor)
                : layout(a_componentLayout), constructor(a_constructor), destructor(a_destructor) {}

            ComponentLayout layout;
            ComponentConstructorFn constructor;
            ComponentDestructorFn destructor;
        };

        template<typename T>
        class StaticComponentID
        {
        public:
            static inline ComponentTypeIndex Get() { return s_id; }
            static inline void Set(ComponentTypeIndex id) { s_id = id; }

        private:
            static inline ComponentTypeIndex s_id;
        };

        template<typename T>
        static void ConstructInPlace(void* dest) {
            new (dest) T();  // calls T's default constructor
        }

        template<typename T>
        static void DestroyInPlace(void* ptr) {
            static_cast<T*>(ptr)->~T();
        }

        std::vector<std::string> m_names;
        std::vector<ComponentType> m_types;
    };
}

#endif