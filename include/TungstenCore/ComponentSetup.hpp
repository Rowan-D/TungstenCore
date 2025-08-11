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

    class ComponentSetup
    {
    public:
        ComponentSetup();

        ComponentSetup(const ComponentSetup&) = delete;
        ComponentSetup& operator=(const ComponentSetup&) = delete;

        template<typename T>
        void Add(std::string_view typeName)
        {
            W_ASSERT(!StaticComponentID<T>::Get(), "Component: {} Already added to ComponentSetup!", typeName);
            m_names.emplace_back(typeName);
            m_emptySentinals.emplace_back(EmptySentinel<T>());
            m_types.emplace_back(sizeof(T), alignof(T), &ReallocateKnownList<T> &ConstructInPlace<T>, &DestroyInPlace<T>);
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

        inline std::string_view GetComponentTypeNameFromTypeIndex(ComponentTypeIndex componentTypeIndex) const { W_ASSERT(componentTypeIndex != InvalidComponentType, "ComponentTypeIndex {} is Invalid", InvalidComponentType); W_ASSERT(componentTypeIndex <= m_names.size(), "ComponentTypeIndex: {} out of Range! Component Type Count: {}", componentTypeIndex, m_names.size()); return m_names[componentTypeIndex - 1]; }
        inline wIndex GetComponentTypeCount() const noexcept { return m_names.size(); }

    private:
        struct ListHeader
        {
            void* begin{};
            void* end{};
            void* capacity{};

            template<typename T>
            inline T* Begin() noexcept { return static_cast<T*>(begin); }
            template<typename T>
            inline T* End() noexcept { return static_cast<T*>(end); }
            template<typename T>
            inline T* Capacity() noexcept { return static_cast<T*>(capacity); }

            template<typename T>
            inline const T* Begin() const noexcept { return static_cast<const T*>(begin); }
            template<typename T>
            inline const T* End() const noexcept { return static_cast<const T*>(end); }
            template<typename T>
            inline const T* Capacity() const noexcept { return static_cast<const T*>(capacity); }

            static constexpr ListHeader Filled(void* ptr) noexcept { return { ptr, ptr, ptr }; }
        };

        using ComponentReallocateFn = void(*)(ListHeader& header, wIndex newCapacity);

        // TODO: Remove if unnesesseary
        using ComponentConstructorFn = void(*)(void* destination);
        using ComponentDestructorFn = void(*)(void* destination);

        struct ComponentType
        {
            ComponentType(std::size_t a_size, std::size_t a_alignment, ComponentReallocateFn a_reallocate, ComponentConstructorFn a_constructor, ComponentDestructorFn a_destructor)
                : size(a_size), alignment(a_alignment), reallocate(a_reallocate), constructor(a_constructor), destructor(a_destructor) {}

            std::size_t size;
            std::size_t alignment;
            ComponentReallocateFn reallocate;

            // TODO: Remove if unnesesseary
            ComponentConstructorFn constructor;
            ComponentDestructorFn destructor;
        };

        template<typename T>
        static void ReallocateKnownList(ListHeader& header, wIndex newCapacity)
        {
            const T* const end = header.End<T>();
            T* const begin = header.Begin<T>();
            const std::size_t oldCount = end - begin;

            T* newMemory = static_cast<T*>(
                ::operator new(newCapacity * sizeof(T), std::align_val_t(alignof(T)))
            );

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                if (oldCount)
                {
                    std::memcpy(newMemory, header.begin, oldCount * sizeof(T));
                }
            }
            else
            {
#if TUNGSTENCORE_COMPONENT_LIST_NOEXCEPT_POLICY
                static_assert(std::is_trivially_copyable_v<T> || std::is_nothrow_move_constructible_v<T>, "TUNGSTENCORE_COMPONENT_LIST_NOEXCEPT_POLICY requires trivially copyable or nothrow-move T");
                T* dst = newMemory;
                for (T* src = begin; src != end; ++src, ++dst)
                {
                    std::construct_at(dst, std::move(*src));
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        std::destroy_at(src);
                    }
                }
#else
                T* dst = newMemory;
                T* src = begin;
                try
                {
                    for (; src != end; ++src, ++dst)
                    {
                        std::construct_at(dst, std::move_if_noexcept(*src));
                    }
                }
                catch (...)
                {
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (T* element = newMemory; element != dst; ++element)
                        {
                            std::destroy_at(element);
                        }
                    }
                    ::operator delete(newMemory, std::align_val_t(alignof(T)));
                    throw;
                }
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (T* element = begin; element != end; ++element)
                    {
                        std::destroy_at(element);
                    }
                }
#endif
            }

            if (!ComponentSetup::IsSentinel(begin))
            {
                ::operator delete(header.begin, std::align_val_t(alignof(T)));
            }

            header.begin = newMemory;
            header.end = newMemory + oldCount;
            header.capacity = newMemory + newCapacity;
        }

        // TODO: Remove if unnesessary
        template<typename T>
        static void ConstructInPlace(void* dest) {
            std::construct_at(static_cast<T*>(dest));  // default-construct T at dest
        }

        template<typename T>
        static void DestroyInPlace(void* ptr) {
            std::destroy_at(static_cast<T*>(ptr));  // calls ~T() if non-trivial
        }

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
        struct SentinelStorage
        {
            alignas(T) static inline std::byte raw[sizeof(T)];
        };

        template<typename T0>
        static auto EmptySentinel() noexcept
        {
            using T    = std::remove_reference_t<T0>;
            using Base = std::remove_cv_t<T>;
            using P    = std::conditional_t<std::is_const_v<T>, const Base*, Base*>;
            return reinterpret_cast<P>(SentinelStorage<Base>::raw);
        }

        template<typename T>
        static inline bool IsSentinel(const T* ptr) noexcept { return ptr == EmptySentinel<T>(); }

        template<typename T>
        static inline bool IsSentinel(const void* ptr) noexcept { return static_cast<const T*>(ptr) == EmptySentinel<T>(); }

        std::vector<std::string> m_names;
        std::vector<void*> m_emptySentinals;
        std::vector<ComponentType> m_types;

        friend class ComponentSystem;
    };
}

#endif