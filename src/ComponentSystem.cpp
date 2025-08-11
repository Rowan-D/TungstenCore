#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSystem.hpp"

// A scene is invalid if the first void* is nullptr, empty if sentinel, contains something if neather.

namespace wCore
{
    static constexpr std::uintptr_t InvalidComponentListTrue = static_cast<std::uintptr_t>(1);

    ComponentSystem::ComponentSystem()
        : m_componentSetup(), m_currentComponentTypeCount(0),
        m_emptyComponentLists(nullptr), m_componentLists(nullptr), m_componentListsCapacity(0),
        m_sceneData(), m_sceneFreeList()
    {

    }

    ComponentSystem::~ComponentSystem()
    {
        if (m_componentLists)
        {
            for (SceneIndex sceneIndex = SceneIndexStart; sceneIndex <= m_sceneData.size(); ++sceneIndex)
            {
                const std::size_t sceneStartPtrIndex = GetSceneStartListIndex(sceneIndex);
                if (m_componentLists[sceneStartPtrIndex].begin)
                {
                    DeleteSceneContent(sceneStartPtrIndex);
                }
            }
            W_ASSERT(m_emptyComponentLists, "m_emptyComponentList should not be nullptr if m_componentLists has been created!");
            ::operator delete(m_emptyComponentLists, std::align_val_t(alignof(ListHeader)));
            ::operator delete(m_componentLists, std::align_val_t(alignof(ListHeader)));
        }
    }

    void ComponentSystem::ReserveScenes(wIndex sceneCapacity)
    {
        m_sceneData.reserve(sceneCapacity);
        if (sceneCapacity > m_componentListsCapacity)
        {
            ReallocateScenes(sceneCapacity);
        }
    }

    wIndex ComponentSystem::CreateScene()
    {
        const std::size_t sceneSizeLists = GetSceneSizeLists();
        if (m_sceneFreeList.empty())
        {
            if (m_sceneData.size() == m_componentListsCapacity)
            {
                ReallocateScenes(CalculateNextCapacity(m_sceneData.size() + 1, m_componentListsCapacity));
            }

            const std::size_t sceneStartListIndex = m_sceneData.size() * sceneSizeLists;
            std::memcpy(m_componentLists + sceneStartListIndex, m_emptyComponentLists, sceneSizeLists * sizeof(void*));

            m_sceneData.emplace_back();

            return m_sceneData.size();
        }
        const wIndex sceneIndex = m_sceneFreeList.back();
        m_sceneFreeList.pop_back();

        // Reset the memory to empty sentinels
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        W_ASSERT(m_componentLists[sceneStartListIndex].begin, "A Free/Invalid ComponentList Should Have the first pointer be false (Set to nullptr).");
        std::memcpy(m_componentLists + sceneStartListIndex, m_emptyComponentLists, sceneSizeLists * sizeof(ListHeader));

        m_sceneData[sceneIndex - 1] = SceneData{};

        return sceneIndex;
    }

    void ComponentSystem::DestroyScene(wIndex sceneIndex)
    {
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        ComponentSetup::ListHeader& firstElement = m_componentLists[sceneStartListIndex];
        if (firstElement.begin)
        {
            DeleteSceneContent(sceneStartListIndex);
            firstElement.begin = nullptr;
            m_sceneFreeList.push_back(sceneIndex);
        }
    }

    wIndex ComponentSystem::GetComponentCount(uint32_t sceneIndex) const
    {
        const std::size_t sceneStartListIndex = GetSceneStartListIndex(sceneIndex);
        return GetKnownListCount<Component>(m_componentLists[sceneStartListIndex + ComponentListOffset]);
    }

    void ComponentSystem::ReserveComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex componentCapacity)
    {
        const std::size_t componentListStartListIndex = GetComponentStartListIndex(sceneIndex, componentTypeIndex);
        ComponentSetup::ListHeader& header = m_componentLists[componentListStartListIndex];

        const std::byte* componentListCapacity = header.Capacity<std::byte>();
        const std::byte* componentListBegin = header.Begin<std::byte>();

        const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];
        if (componentCapacity * type.size > componentListCapacity - componentListBegin)
        {
            type.reallocate(header, componentCapacity);
        }
    }

    ComponentIndex ComponentSystem::CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex)
    {
        const std::size_t sceneStartPtrIndex = GetSceneStartPtrIndex(sceneIndex);
        const std::size_t componentListStartPtrIndex = sceneStartPtrIndex + GetComponentPointerOffset(componentTypeIndex);
        const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];

        void*& componentListEndVoid = m_componentLists[componentListStartPtrIndex + EndPtrOffset];
        const std::byte* componentListEnd = static_cast<std::byte*>(componentListEndVoid);
        const std::byte* const componentListBegin = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + BeginPtrOffset]);
        const std::byte* const componentListCapacity = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + CapacityPtrOffset]);
        const wIndex listIndex = (componentListEnd - componentListBegin) / type.size;

        if (componentListEnd == componentListCapacity)
        {
            ReallocateComponentList(componentListStartPtrIndex, componentTypeIndex, CalculateNextCapacity(listIndex + 1, (componentListCapacity - componentListBegin) / type.size));
            componentListEnd = static_cast<std::byte*>(componentListEndVoid);
        }

        type.constructor(componentListEndVoid);

        componentListEndVoid = const_cast<void*>(static_cast<const void*>(componentListEnd + type.size));

        return EmplaceKnownList<Component>(m_componentLists + sceneStartPtrIndex + ComponentListPtrOffset, 0, listIndex);
    }

    void ComponentSystem::ReallocateScenes(wIndex newCapacity)
    {
        m_componentListsCapacity = newCapacity;

        if (m_componentLists)
        {
            const std::size_t sceneSizeBytes = GetSceneSizeBytes();
            ComponentSetup::ListHeader* newComponentLists = static_cast<ComponentSetup::ListHeader*>(
                ::operator new(newCapacity * sceneSizeBytes, std::align_val_t(alignof(ComponentSetup::ListHeader)))
            );

            if (!m_sceneData.empty())
            {
                std::memcpy(newComponentLists, m_componentLists, m_sceneData.size() * sceneSizeBytes);
            }

            ::operator delete(m_componentLists, std::align_val_t(alignof(ListHeader*)));

            m_componentLists = newComponentLists;
        }
        else
        {
            m_currentComponentTypeCount = m_componentSetup.GetComponentTypeCount();
            const std::size_t sceneSizeBytes = GetSceneSizeBytes();

            m_emptyComponentLists = static_cast<ComponentSetup::ListHeader*>(
                ::operator new(sceneSizeBytes, std::align_val_t(alignof(ComponentSetup::ListHeader)))
            );

            void* const componentSentinel = ComponentSetup::EmptySentinel<Component>();
            void* const standardStringSentinel = ComponentSetup::EmptySentinel<std::string>();

            m_emptyComponentLists[0] = { componentSentinel, componentSentinel, componentSentinel };
            m_emptyComponentLists[1] = { standardStringSentinel, standardStringSentinel, standardStringSentinel };

            ComponentSetup::ListHeader* dst = m_emptyComponentLists + KnownListCount;
            void* const* src = m_componentSetup.m_emptySentinals.data();

            for (wIndex i = 0; i < m_currentComponentTypeCount; ++i)
            {
                *dst++ = ComponentSetup::ListHeader::Filled(src[i]);
            }

            m_componentLists = static_cast<ComponentSetup::ListHeader*>(
                ::operator new(newCapacity * sceneSizeBytes, std::align_val_t(alignof(ComponentSetup::ListHeader)))
            );
        }
    }

    void ComponentSystem::DeleteSceneContent(std::size_t sceneStartPtrIndex)
    {
        DestroyKnownList<Component>(m_componentLists[sceneStartPtrIndex + ComponentListOffset]);
        DestroyKnownList<std::string>(m_componentLists[sceneStartPtrIndex + ComponentNameListOffset]);
        for (wIndex componentTypeIndex = wCore::ComponentTypeIndexStart; componentTypeIndex <= m_currentComponentTypeCount; ++componentTypeIndex)
        {
            DestroyComponentList(sceneStartPtrIndex, componentTypeIndex);
        }
    }

    void ComponentSystem::ReallocateComponentList(std::size_t componentListStartPtrIndex, ComponentTypeIndex componentTypeIndex, wIndex newCapacity)
    {
        const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];

        ComponentSetup::ListHeader& header = m_componentLists[componentListStartPtrIndex];
        const std::byte* componentListEnd = header.End<std::byte>();
        const std::byte* componentListBegin = header.End<std::byte>();
        const std::size_t oldSizeBytes = componentListEnd - componentListBegin;

        std::byte* newComponentList = static_cast<std::byte*>(
            ::operator new(newCapacity * type.size, std::align_val_t(type.alignment))
        );

        if (oldSizeBytes)
        {
            std::memcpy(newComponentList, header.begin, oldSizeBytes);
        }

        if (header.begin != m_componentSetup.m_emptySentinals[componentTypeIndex - 1])
        {
            ::operator delete(header.begin, std::align_val_t(type.alignment));
        }

        header.begin = newComponentList;
        header.end = newComponentList + oldSizeBytes;
        header.capacity = newComponentList + newCapacity * type.size;
    }

    void ComponentSystem::DestroyComponentList(std::size_t sceneStartPtrIndex, ComponentTypeIndex componentTypeIndex)
    {
        const std::size_t componentListStartPtrIndex = sceneStartPtrIndex + GetComponentPointerOffset(componentTypeIndex);
        void*& componentListBeginVoid = m_componentLists[componentListStartPtrIndex + BeginPtrOffset];
        if (componentListBeginVoid != m_componentSetup.m_emptySentinals[componentTypeIndex - 1])
        {
            std::byte* const componentListBegin = static_cast<std::byte*>(componentListBeginVoid);
            std::byte* const componentListEnd = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + EndPtrOffset]);
            const ComponentSetup::ComponentType type = m_componentSetup.m_types[componentTypeIndex - 1];
            for (std::byte* element = componentListBegin; element != componentListEnd; element += type.size)
            {
                type.destructor(element);
            }
            ::operator delete(componentListBeginVoid, std::align_val_t(type.alignment));
        }
    }
}