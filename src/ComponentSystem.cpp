#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSystem.hpp"

// invalid:
// start random
// end true
// capacity internal

namespace wCore
{
    static constexpr std::uintptr_t InvalidComponentListTrue = static_cast<std::uintptr_t>(1);

    ComponentSystem::ComponentSystem()
        : m_componentSetup(),
        m_componentLists(nullptr), m_componentListsCapacity(0),
        m_sceneData(), m_sceneFreeList()
    {
    }

    ComponentSystem::~ComponentSystem()
    {
        if (m_componentLists)
        {
            for (SceneIndex sceneIndex = SceneIndexStart; sceneIndex <= m_sceneData.size(); ++sceneIndex)
            {
                const std::size_t sceneStartPtrIndex = GetSceneStartPtrIndex(sceneIndex);
                void*& seccondElement = m_componentLists[sceneStartPtrIndex + 1];
                void*& thirdElement = m_componentLists[sceneStartPtrIndex + 2];
                if (!seccondElement || thirdElement)
                {
                    DeleteSceneContent(sceneStartPtrIndex);
                }
            }
            ::operator delete(m_componentLists, std::align_val_t(alignof(void*)));
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
        const std::size_t sceneSizePointers = GetSceneSizePointers();
        if (m_sceneFreeList.empty())
        {
            if (m_sceneData.size() == m_componentListsCapacity)
            {
                ReallocateScenes(CalculateNextCapacity(m_sceneData.size() + 1, m_componentListsCapacity));
            }

            // Zero the memory
            const std::size_t sceneStartPtrIndex = m_sceneData.size() * sceneSizePointers;
            std::memset(m_componentLists + sceneStartPtrIndex, 0, sceneSizePointers * sizeof(void*));

            m_sceneData.emplace_back();

            return m_sceneData.size();
        }
        const wIndex sceneIndex = m_sceneFreeList.back();
        m_sceneFreeList.pop_back();

        // Zero the memory
        const std::size_t sceneStartPtrIndex = sceneIndex * sceneSizePointers;
        const std::size_t sceneEndPtrIndex = sceneStartPtrIndex + sceneSizePointers;
        m_componentLists[sceneStartPtrIndex] = nullptr;
        W_ASSERT(m_componentLists[sceneStartPtrIndex + 1], "An Free/Invalid ComponentList Should Have the seccond pointer be true (Set to something other than nullptr).");
        m_componentLists[sceneStartPtrIndex + 1] = nullptr;
        W_ASSERT(!m_componentLists[sceneStartPtrIndex + 2], "An Free/Invalid ComponentList Should Have the third pointer be false (Set to nullptr)");
        std::memset(m_componentLists + sceneStartPtrIndex + 3, 0, (sceneSizePointers - 3) * sizeof(void*));

        m_sceneData[sceneIndex] = SceneData{};

        return sceneIndex;
    }

    void ComponentSystem::DestroyScene(wIndex sceneIndex)
    {
        const std::size_t sceneStartPtrIndex = GetSceneStartPtrIndex(sceneIndex);
        void*& seccondElement = m_componentLists[sceneStartPtrIndex + 1];
        void*& thirdElement = m_componentLists[sceneStartPtrIndex + 2];
        if (!seccondElement || thirdElement)
        {
            DeleteSceneContent(sceneStartPtrIndex);

            // Set invalid
            if (seccondElement) [[unlikely]]
            {
                seccondElement = reinterpret_cast<void*>(InvalidComponentListTrue); // Set true
            }
            thirdElement = nullptr; // Set false

            m_sceneFreeList.push_back(sceneIndex);
        }
    }

    bool ComponentSystem::SceneExists(wIndex sceneIndex)
    {
        const std::size_t sceneSizePointers = GetSceneSizePointers();
        const std::size_t sceneStartPtrIndex = sceneIndex * sceneSizePointers;
        return !m_componentLists[sceneStartPtrIndex + 1] && m_componentLists[sceneStartPtrIndex + 2];
    }

    wIndex ComponentSystem::GetComponentCount(uint32_t sceneIndex) const
    {
        const std::size_t sceneStartPtrIndex = GetSceneStartPtrIndex(sceneIndex);
        return KnownListCount<Component>(m_componentLists + sceneStartPtrIndex + ComponentListPtrOffset);
    }

    void ComponentSystem::ReserveComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex componentCapacity)
    {
        const std::size_t componentListStartPtrIndex = GetComponentStartPtrIndex(sceneIndex, componentTypeIndex);
        const ComponentLayout layout = m_componentSetup.GetComponentLayoutFromTypeIndex(componentTypeIndex);

        const std::byte* componentListCapacity = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + CapacityPtrOffset]);
        const std::byte* componentListBegin = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + BeginPtrOffset]);

        if (componentCapacity * layout.size > componentListCapacity - componentListBegin)
        {
            ReallocateComponentList(componentListStartPtrIndex, componentTypeIndex, componentCapacity);
        }
    }

    ComponentIndex ComponentSystem::CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex)
    {
        const std::size_t sceneStartPtrIndex = GetSceneStartPtrIndex(sceneIndex);
        const std::size_t componentListStartPtrIndex = sceneStartPtrIndex + GetComponentPointerOffset(componentTypeIndex);
        const ComponentLayout layout = m_componentSetup.GetComponentLayoutFromTypeIndex(componentTypeIndex);

        void*& componentListEndVoid = m_componentLists[componentListStartPtrIndex + EndPtrOffset];
        const std::byte* componentListEnd = static_cast<std::byte*>(componentListEndVoid);
        const std::byte* const componentListBegin = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + BeginPtrOffset]);
        const std::byte* const componentListCapacity = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + CapacityPtrOffset]);
        const wIndex listIndex = (componentListEnd - componentListBegin) / layout.size;

        if (componentListEnd == componentListCapacity)
        {
            ReallocateComponentList(componentListStartPtrIndex, componentTypeIndex, CalculateNextCapacity(listIndex + 1, (componentListCapacity - componentListBegin) / layout.size));
            componentListEnd = static_cast<std::byte*>(componentListEndVoid);
        }

        m_componentSetup.ConstructComponentInPlaceFromTypeIndex(componentTypeIndex, componentListEndVoid);

        componentListEndVoid = const_cast<void*>(static_cast<const void*>(componentListEnd + layout.size));

        return KnownListEmplace<Component>(m_componentLists + sceneStartPtrIndex + ComponentListPtrOffset, 0, listIndex);
    }

    void ComponentSystem::ReallocateScenes(wIndex newCapacity)
    {
        const std::size_t sceneSizeBytes = GetSceneSizeBytes();
        void** newComponentLists = static_cast<void**>(
            ::operator new(newCapacity * sceneSizeBytes, std::align_val_t(alignof(void*)))
        );

        if (!m_sceneData.empty())
        {
            std::memcpy(newComponentLists, m_componentLists, m_sceneData.size() * sceneSizeBytes);
        }

        if (m_componentLists)
        {
            ::operator delete(m_componentLists, std::align_val_t(alignof(void*)));
        }

        m_componentLists = newComponentLists;
        m_componentListsCapacity = newCapacity;
    }

    void ComponentSystem::DeleteSceneContent(std::size_t sceneStartPtrIndex)
    {
        DestroyKnownList<Component>(m_componentLists + sceneStartPtrIndex + ComponentListPtrOffset);
        DestroyKnownList<std::string>(m_componentLists + sceneStartPtrIndex + ComponentNameListPtrOffset);
        for (wIndex componentTypeIndex = wCore::ComponentTypeIndexStart; componentTypeIndex <= m_componentSetup.GetComponentTypeCount(); ++componentTypeIndex)
        {
            DestroyComponentList(sceneStartPtrIndex, componentTypeIndex);
        }
    }

    void ComponentSystem::ReallocateComponentList(std::size_t componentListStartPtrIndex, ComponentTypeIndex componentTypeIndex, wIndex newCapacity)
    {
        const ComponentLayout layout = m_componentSetup.GetComponentLayoutFromTypeIndex(componentTypeIndex);

        void*& componentListEndVoid = m_componentLists[componentListStartPtrIndex + EndPtrOffset];
        void*& componentListBeginVoid = m_componentLists[componentListStartPtrIndex + BeginPtrOffset];
        const std::byte* componentListEnd = static_cast<std::byte*>(componentListEndVoid);
        const std::byte* componentListBegin = static_cast<std::byte*>(componentListBeginVoid);
        const std::size_t oldSizeBytes = componentListEnd - componentListBegin;

        std::byte* newComponentList = static_cast<std::byte*>(
            ::operator new(newCapacity * layout.size, std::align_val_t(layout.alignment))
        );

        if (oldSizeBytes)
        {
            std::memcpy(newComponentList, componentListBeginVoid, oldSizeBytes);
        }

        if (componentListBeginVoid)
        {
            ::operator delete(componentListBeginVoid, std::align_val_t(layout.alignment));
        }

        componentListBeginVoid = newComponentList;
        componentListEndVoid = newComponentList + oldSizeBytes;
        m_componentLists[componentListStartPtrIndex + CapacityPtrOffset] = newComponentList + newCapacity * layout.size;
    }

    void ComponentSystem::DestroyComponentList(std::size_t sceneStartPtrIndex, ComponentTypeIndex componentTypeIndex)
    {
        const std::size_t componentListStartPtrIndex = sceneStartPtrIndex + GetComponentPointerOffset(componentTypeIndex);
        void*& componentListBeginVoid = m_componentLists[componentListStartPtrIndex + BeginPtrOffset];
        if (componentListBeginVoid)
        {
            std::byte* const componentListBegin = static_cast<std::byte*>(componentListBeginVoid);
            std::byte* const componentListEnd = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + EndPtrOffset]);
            const ComponentLayout layout = m_componentSetup.GetComponentLayoutFromTypeIndex(componentTypeIndex);
            for (std::byte* element = componentListBegin; element != componentListEnd; element += layout.size)
            {
                m_componentSetup.DestroyComponentInPlaceFromTypeIndex(componentTypeIndex, element);
            }
            ::operator delete(componentListBeginVoid, std::align_val_t(layout.alignment));
        }
    }
}