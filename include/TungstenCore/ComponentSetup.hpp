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

    using SceneIndex = wIndex;
    inline constexpr SceneIndex InvalidScene = 0;
    inline constexpr SceneIndex SceneIndexStart = 1;

    using ComponentIndex = wIndex;
    inline constexpr ComponentIndex InvalidComponent = 0;
    inline constexpr ComponentIndex ComponentIndexStart = 1;

    class ComponentGeneration
    {
        uint32_t generation;
        friend class ComponentSystem;
    };

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

        ComponentSetup() noexcept;

        static_assert(std::is_nothrow_default_constructible_v<std::string>);

        ComponentSetup(const ComponentSetup&) = delete;
        ComponentSetup& operator=(const ComponentSetup&) = delete;

        template<typename T,
                 wIndex PageSize = 0,
                 typename GrowthPolicy = DefaultGrowthPolicy>
        void Add(std::string_view typeName)
        {
            static_assert(std::is_nothrow_destructible_v<T>, "Components must be nothrow-destructible");
            W_ASSERT(!StaticComponentID<T>::GetID(), "Component: {} Already added to ComponentSetup!", typeName);
            m_names.emplace_back(typeName);
            if constexpr (PageSize)
            {
                const wIndex listIndex = m_types.size() - m_componentListCount;
                m_types.emplace_back(sizeof(T), alignof(T), &ReallocatePages<T, PageSize>, &CreateComponent<T, PageSize, GrowthPolicy>, /*&DestroyKnownListUnchecked<T>*/ nullptr, PageSize, listIndex);
                StaticComponentID<T>::Set(m_types.size(), listIndex);
            }
            else
            {
                const wIndex listIndex = m_componentListCount++;
                m_types.emplace_back(sizeof(T), alignof(T), &ReallocateComponents<T>, &CreateComponent<T, PageSize, GrowthPolicy>, /*&DestroyKnownListUnchecked<T>*/ nullptr, listIndex);
                StaticComponentID<T>::Set(m_types.size(), listIndex);
            }
        }

        // internal
        template<typename T>
        inline ComponentTypeIndex GetComponentTypeIndex() const noexcept { W_ASSERT(StaticComponentID<T>::Get(), "Type: {} not added to ComponentSetup", wUtils::DebugGetTypeName<T>()); return StaticComponentID<T>::Get(); }

        template<typename T>
        inline std::string_view GetComponentTypeName() const noexcept { W_ASSERT(GetComponentTypeIndex<T>(), "Type: {} not added to ComponentSetup", wUtils::DebugGetTypeName<T>()); return GetComponentTypeNameFromTypeIndex(StaticComponentID<T>::Get()); }

        inline std::string_view GetComponentTypeNameFromTypeIndex(ComponentTypeIndex componentTypeIndex) const noexcept { W_ASSERT(componentTypeIndex != InvalidComponentType, "ComponentTypeIndex {} is Invalid", InvalidComponentType); W_ASSERT(componentTypeIndex <= m_names.size(), "ComponentTypeIndex: {} out of Range! Component Type Count: {}", componentTypeIndex, m_names.size()); return m_names[componentTypeIndex - 1]; }
        inline wIndex GetComponentTypeCount() const noexcept { return m_types.size(); }

    private:
        struct ComponentListHeaderHot
        {
            void* dense;
            ComponentIndex* slotToDense; // scene 0
            ComponentGeneration* generations;
        };

        struct ComponentListHeaderCold
        {
            wIndex slotCount;
            wIndex denseCount;
            wIndex capacity;
            wUtils::RelocatableFreeListHeader<ComponentIndex> freeList;
        };

        struct PageListHeaderHot
        {
            void* data;
            ComponentGeneration* generations;
        };

        struct PageListHeaderCold
        {
            wIndex slotCount;
            wIndex pageCount;
            wUtils::RelocatableFreeListHeader<ComponentIndex> freeList;
        };

        struct CreateCtx
        {
            [[nodiscard]] inline wIndex GetCurrentComponentTypeCount() noexcept { return currentComponentTypeCount; }
            [[nodiscard]] inline wIndex GetCurrentComponentListCount() noexcept { return currentComponentListCount; }
            [[nodiscard]] inline wIndex GetCurrentPageListCount() noexcept { return currentComponentTypeCount - currentComponentListCount; }

            void UpdateCurrentComponentListCount(wIndex componentTypeCount, wIndex componentListCount) noexcept;

            ComponentListHeaderHot* componentListsHot;
            PageListHeaderHot* pageListsHot;
            ComponentListHeaderCold* componentListsCold;
            PageListHeaderCold* pageListsCold;
            wIndex currentComponentTypeCount;
            wIndex currentComponentListCount;
        };

        using ReallocateComponentsFn = void(*)(ComponentListHeaderHot& headerHot, ComponentListHeaderCold& headerCold, wIndex newSlotCapacity);
        using ReallocatePagesFn = void(*)(PageListHeaderHot& headerHot, PageListHeaderCold& headerCold, wIndex newPageCount);
        using ComponentCreateFn = std::pair<ComponentIndex, ComponentGeneration>(*)(SceneIndex sceneIndex, CreateCtx& createCtx, Application& app);
        using ComponentDestroyFn = void(*)(ComponentListHeaderHot& headerHot, ComponentListHeaderCold& headerCold);
        using PageDestroyFn = void(*)(PageListHeaderHot& headerHot, PageListHeaderCold& headerCold);

        struct ComponentType
        {
            ComponentType(std::size_t a_size, std::size_t a_alignment, ReallocateComponentsFn a_reallocateComponents, ComponentCreateFn a_create, ComponentDestroyFn a_destroy, wIndex a_listIndex)
                : size(a_size), alignment(a_alignment), reallocateComponents(a_reallocateComponents), create(a_create), componentDestroy(a_destroy), pageSize(0), listIndex(a_listIndex) {}

            ComponentType(std::size_t a_size, std::size_t a_alignment, ReallocatePagesFn a_reallocatePages, ComponentCreateFn a_create, PageDestroyFn a_destroy, wIndex a_listIndex, wIndex a_pageSize)
                : size(a_size), alignment(a_alignment), reallocatePages(a_reallocatePages), create(a_create), pageDestroy(a_destroy), pageSize(a_pageSize), listIndex(a_listIndex) {}

            std::size_t size;
            std::size_t alignment;
            union
            {
                ReallocateComponentsFn reallocateComponents;
                ReallocatePagesFn reallocatePages;
            };
            ComponentCreateFn create;
            union
            {
                ComponentDestroyFn componentDestroy;
                PageDestroyFn pageDestroy;
            };
            wIndex pageSize;
            wIndex listIndex;
        };

        template<typename T, wIndex PageSize, typename GrowthPolicy>
        static std::pair<ComponentIndex, ComponentGeneration> CreateComponent(SceneIndex sceneIndex, CreateCtx& createCtx, Application& app)
        {
            if (PageSize)
            {
                const std::size_t sceneStartPageIndex = (sceneIndex - 1) * createCtx.GetCurrentPageListCount();
                const std::size_t pageListHeaderIndex = sceneStartPageIndex + StaticComponentID<T>::GetListIndex();
                PageListHeaderHot& heagerHot = createCtx.pageListsHot[pageListHeaderIndex];
                PageListHeaderCold& heagerCold = createCtx.pageListsCold[pageListHeaderIndex];
            }
            else
            {
                const std::size_t sceneStartComponentIndex = (sceneIndex - 1) * createCtx.GetCurrentComponentListCount();
                const std::size_t componentListHeaderIndex = sceneStartComponentIndex + StaticComponentID<T>::GetListIndex();
                ComponentListHeaderHot& heagerHot = createCtx.componentListsHot[componentListHeaderIndex];
                ComponentListHeaderCold& heagerCold = createCtx.componentListsCold[componentListHeaderIndex];
            }
        }

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

        /*template<typename T, wIndex PageSize, typename GrowthPolicy, typename... Args>
        static wIndex EmplacePages(wUtils::RelocatableFreeListHeader<ComponentIndex>& freeList, )
        {
            if (freeList.Empty())
            {
                if (slotCount == pageCount * PageSize)
                {

                }
            }
            const ComponentIndex componentIndex = freeList.Remove();
        }*/

        template<typename T, wIndex PageSize>
        static void ReallocatePages(PageListHeaderHot& headerHot, PageListHeaderCold& headerCold, wIndex newPageCount)
        {
            T** newMemory = static_cast<T**>(
                ::operator new(newPageCount * sizeof(T*), std::align_val_t(alignof(T*)))
            );

            if (headerHot.data)
            {
                std::memcpy(newMemory, headerHot.data, headerCold.pageCount * sizeof(T*));
                ::operator delete(headerHot.data, std::align_val_t(alignof(T*)));
                for (wIndex pageIndex = headerCold.pageCount; pageIndex < newPageCount; ++pageIndex)
                {
                    newMemory[pageIndex] = static_cast<T*>(
                        ::operator new(PageSize * sizeof(T), std::align_val_t(alignof(T)))
                    );
                }
            }

            headerHot.data = newMemory;
            headerCold.pageCount = newPageCount;
        }

        template<typename T>
        static void ReallocateComponents(ComponentListHeaderHot& headerHot, ComponentListHeaderCold& headerCold, wIndex newCapacity)
        {
            std::size_t offset = newCapacity * sizeof(T);

            offset = wUtils::AlignUp(offset, alignof(ComponentIndex));
            const std::size_t slotToDenceOffset = offset;
            offset += newCapacity * sizeof(ComponentIndex);

            constexpr std::size_t alignment = wUtils::Max(alignof(T), alignof(ComponentIndex));

            std::byte* newMemory = static_cast<std::byte*>(
                ::operator new(offset, std::align_val_t(alignment))
            );

            if (headerHot.dense)
            {
                if constexpr (std::is_trivially_copyable_v<T>)
                {
                    if (headerCold.denseCount)
                    {
                        std::memcpy(newMemory, headerHot.dense, headerCold.denseCount * sizeof(T));
                    }
                }
                else
                {
                    T* const begin = static_cast<T*>(headerHot.dense);
                    T* end = begin + headerCold.denseCount;
                    T* dst = newMemory;
                    for (T* src = begin; src != end; ++src, ++dst)
                    {
                        std::construct_at(dst, std::move(*src));
                        if constexpr (!std::is_trivially_destructible_v<T>)
                        {
                            std::destroy_at(src);
                        }
                    }
                }
                if (headerCold.slotCount)
                {
                    std::memcpy(newMemory, headerHot.dense, headerCold.slotCount * sizeof(ComponentIndex));
                }
                ::operator delete(headerHot.dense, std::align_val_t(alignment));
            }

            headerHot.dense = newMemory;
            headerHot.slotToDense = reinterpret_cast<ComponentIndex*>(newMemory + slotToDenceOffset);

            headerCold.capacity = newCapacity;
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
            static inline ComponentTypeIndex GetID() { return s_id; }
            static inline wIndex GetListIndex() { return s_listIndex; }
            static inline void Set(ComponentTypeIndex id, wIndex listIndex, wIndex pageSize) { s_id = id; s_listIndex = listIndex; s_pageSize = pageSize; }

        private:
            static inline ComponentTypeIndex s_id;
            static inline wIndex s_listIndex;
            static inline wIndex s_pageSize;
        };



        std::vector<std::string> m_names;
        std::vector<ComponentType> m_types;
        wIndex m_componentListCount;

        friend class ComponentSystem;
    };
}

#endif