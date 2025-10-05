#ifndef TUNGSTEN_CORE_COMPONENT_SETUP_HPP
#define TUNGSTEN_CORE_COMPONENT_SETUP_HPP

#include <string>
#include <string_view>
#include <vector>
#include "TungstenUtils/TungstenUtils.hpp"

namespace wCore
{
    class Application;

    using ComponentTypeIndex = wIndex;
    inline constexpr ComponentTypeIndex InvalidComponentType = 0;
    inline constexpr ComponentTypeIndex ComponentTypeIndexStart = 1;

    class ComponentSetup
    {
    public:
        struct DefaultGrowthPolicy
        {
            static constexpr wIndex InitialCapacity = 8;

            template<typename T, wIndex PageSize>
            [[nodiscard]] static inline constexpr wIndex Next(wIndex requested, wIndex current) noexcept
            {
                if constexpr (PageSize)
                {
                    return NextCapacity(requested, current);
                }
                else
                {
                    return NextPageCount(requested, current);
                }
            }

            [[nodiscard]] static inline constexpr wIndex NextCapacity(wIndex requested, wIndex current) noexcept
            {
                if (current)
                {
                    return std::max(current * 2, requested);
                }
                return std::max(InitialCapacity, requested);
            }

            [[nodiscard]] static inline constexpr wIndex NextPageCount(wIndex requestedPageCount, wIndex currentPageCount) noexcept { return requestedPageCount; }
        };

        ComponentSetup() noexcept = default;
        static_assert(std::is_nothrow_default_constructible_v<std::string>);

        ComponentSetup(const ComponentSetup&) = delete;
        ComponentSetup& operator=(const ComponentSetup&) = delete;

        template<typename T,
                 wIndex PageSize = 0,
                 typename GrowthPolicy = DefaultGrowthPolicy>
        void Add(std::string_view typeName)
        {
            static_assert(std::is_nothrow_destructible_v<T>, "Components must be nothrow-destructible");
            W_ASSERT(!StaticComponentID<T>::Get(), "Component: {} Already added to ComponentSetup!", typeName);
            m_names.emplace_back(typeName);
            if constexpr (PageSize)
            {
                m_types.emplace_back(sizeof(T), alignof(T), &ReallocatePages<T, PageSize>, /*&CreateComponent<T, PageSize, GrowthPolicy>*/ nullptr, /*&DestroyKnownListUnchecked<T>*/ nullptr, PageSize);
            }
            else
            {
                m_types.emplace_back(sizeof(T), alignof(T), &ReallocateComponents<T>, /*&CreateComponent<T, PageSize, GrowthPolicy>*/ nullptr, /*&DestroyKnownListUnchecked<T>*/ nullptr);
            }
            StaticComponentID<T>::Set(m_names.size());
        }

        // internal
        template<typename T>
        inline ComponentTypeIndex GetComponentTypeIndex() const noexcept { W_ASSERT(StaticComponentID<T>::Get(), "Type: {} not added to ComponentSetup", wUtils::DebugGetTypeName<T>()); return StaticComponentID<T>::Get(); }

        template<typename T>
        inline std::string_view GetComponentTypeName() const noexcept { W_ASSERT(GetComponentTypeIndex<T>(), "Type: {} not added to ComponentSetup", wUtils::DebugGetTypeName<T>()); return GetComponentTypeNameFromTypeIndex(StaticComponentID<T>::Get()); }

        inline std::string_view GetComponentTypeNameFromTypeIndex(ComponentTypeIndex componentTypeIndex) const noexcept { W_ASSERT(componentTypeIndex != InvalidComponentType, "ComponentTypeIndex {} is Invalid", InvalidComponentType); W_ASSERT(componentTypeIndex <= m_names.size(), "ComponentTypeIndex: {} out of Range! Component Type Count: {}", componentTypeIndex, m_names.size()); return m_names[componentTypeIndex - 1]; }
        inline wIndex GetComponentTypeCount() const noexcept { return m_names.size(); }

    private:
        struct ComponentListHeader
        {
            void* data{};
            wIndex count{};
            wIndex pageCount{};

            template<typename T>
            [[nodiscard]] inline T** Data() noexcept { return static_cast<T*>(data); }

            template<typename T>
            [[nodiscard]] inline const T** Data() const noexcept { return static_cast<const T*>(data); }
        };

        using ReallocateComponentsFn = void(*)(void*& data, wIndex slotCount, wIndex& capacity, wIndex newSlotCapacity);
        using ReallocatePagesFn = void(*)(void*& data, wIndex& pageCount, wIndex newPageCount);
        using ComponentCreateFn = wIndex(*)(ComponentListHeader& header, Application& app);
        using ComponentDestroyFn = void(*)(ComponentListHeader& header);

        struct ComponentType
        {
            ComponentType(std::size_t a_size, std::size_t a_alignment, ReallocateComponentsFn reallocateComponents, ComponentCreateFn a_create, ComponentDestroyFn a_destroy, wIndex a_pageSize)
                : size(a_size), alignment(a_alignment), reallocateComponents(reallocateComponents), create(a_create), destroy(a_destroy), pageSize(a_pageSize) {}

            ComponentType(std::size_t a_size, std::size_t a_alignment, ReallocatePagesFn reallocatePages, ComponentCreateFn a_create, ComponentDestroyFn a_destroy)
                : size(a_size), alignment(a_alignment), reallocatePages(reallocatePages), create(a_create), destroy(a_destroy), pageSize(0) {}

            std::size_t size;
            std::size_t alignment;
            union
            {
                ReallocateComponentsFn reallocateComponents;
                ReallocatePagesFn reallocatePages;
            };
            ComponentCreateFn create;
            ComponentDestroyFn destroy;
            wIndex pageSize;
        };

        /*template<typename T, wIndex PageSize, typename GrowthPolicy>
        static wIndex CreateComponent(ComponentListHeader& header, Application& app)
        {
            if constexpr (std::is_constructible_v<T, Application&>)
            {
                return EmplaceComponentList<T, PageSize, GrowthPolicy>(header, app);
            }
            else
            {
                return EmplaceComponentList<T, PageSize, GrowthPolicy>(header);
            }
        }

        template<typename T, wIndex PageSize, typename GrowthPolicy, typename... Args>
        static wIndex EmplaceComponentList(ComponentListHeader& header, Args&&... args)
        {
            if constexpr (PageSize)
            {
                if (header.count == header.pageCount * PageSize)
                {
                    ReallocatePages<T>(header, GrowthPolicy::Next(header.pageCount + 1, header.pageCount));
                }
            }
            else
            {
                if (header.count == header.pageCount * PageSize)
                {
                    ReallocateComponents<T>(header, GrowthPolicy::Next(header.pageCount + 1, header.pageCount));
                }
            }

            std::construct_at(header.Data<T>() + header.count, std::forward<Args>(args)...);

            return ++header.count;
        }*/

        template<typename T, wIndex PageSize, typename GrowthPolicy, typename... Args>
        static wIndex EmplacePages(wUtils::RelocatableFreeListHeader<ComponentIndex>& freeList)
        {
            if (freeList.Empty())
            {
            }
            const ComponentIndex componentIndex = freeList.Remove();
        }

        template<typename T, wIndex PageSize>
        static void ReallocatePages(void*& data, wIndex& pageCount, wIndex newPageCount)
        {
            T** newMemory = static_cast<T**>(
                ::operator new(newPageCount * sizeof(T*), std::align_val_t(alignof(T*)))
            );

            if (data)
            {
                std::memcpy(newMemory, data, pageCount * sizeof(T*));
                ::operator delete(data, std::align_val_t(alignof(T*)));
                for (wIndex pageIndex = pageCount; pageIndex < newPageCount; ++pageIndex)
                {
                    newMemory[pageIndex] = ::operator new(PageSize * sizeof(T), std::align_val_t(alignof(T)));
                }
            }

            data = newMemory;
            pageCount = newPageCount;
        }

        template<typename T>
        static void ReallocateComponents(void*& data, wIndex count, wIndex& capacity, wIndex newCapacity)
        {
            T* newMemory = static_cast<T*>(
                ::operator new(newCapacity * sizeof(T), std::align_val_t(alignof(T)))
            );

            if (data)
            {
                if constexpr (std::is_trivially_copyable_v<T>)
                {
                    if (count)
                    {
                        std::memcpy(newMemory, data, count * sizeof(T));
                    }
                }
                else
                {
                    T* end = data + count;
                    T* dst = newMemory;
                    for (T* src = data; src != end; ++src, ++dst)
                    {
                        std::construct_at(dst, std::move(*src));
                        if constexpr (!std::is_trivially_destructible_v<T>)
                        {
                            std::destroy_at(src);
                        }
                    }
                }
                ::operator delete(data, std::align_val_t(alignof(T)));
            }

            data = newMemory;
            capacity = newCapacity;
        }

        /*template<typename T>
        static void DestroyKnownListUnchecked(ComponentListHeader& header) noexcept
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                T* const data = header.Data<T>();
                std::destroy_n(data, header.count);
            }
            ::operator delete(header.data, std::align_val_t(alignof(T)));
        }*/

        template<typename T>
        class StaticComponentID
        {
        public:
            static inline ComponentTypeIndex Get() { return s_id; }
            static inline void Set(ComponentTypeIndex id) { s_id = id; }

        private:
            static inline ComponentTypeIndex s_id;
        };

        std::vector<std::string> m_names;
        std::vector<ComponentType> m_types;

        friend class ComponentSystem;
    };
}

#endif