#include "wCorePCH.hpp"
#include "TungstenCore/ComponentSystem.hpp"

// invalid:
// start random
// end true
// capacity false

namespace wCore
{
    static constexpr std::uintptr_t InvalidComponentListTrue = static_cast<std::uintptr_t>(1);

    ComponentSystem::ComponentSystem()
        : m_componentSetup(),
        m_componentLists(nullptr), m_componentListsCapacity(0),
        m_sceneData(), m_sceneFreeList()
    {
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
        const std::size_t sceneSizePointers = GetSceneSizePointers();
        const std::size_t sceneStartPtrIndex = sceneIndex * sceneSizePointers;
        void*& seccondElement = m_componentLists[sceneStartPtrIndex + 1];
        void*& thirdElement = m_componentLists[sceneStartPtrIndex + 2];
        if (!seccondElement && thirdElement)
        {
            // TODO: Destroy Components

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

    void ComponentSystem::ReserveComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex, wIndex componentCapacity)
    {
        const std::size_t componentListStartPtrIndex = GetComponentStartPtrIndex(sceneIndex, componentTypeIndex);
        const ComponentLayout layout = m_componentSetup.GetComponentLayoutFromTypeIndex(componentTypeIndex);

        const std::byte* componentListCapacity = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + CapacityPtrOffset]);
        const std::byte* componentListBegin = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + BeginPtrOffset]);

        if (componentListCapacity - componentListBegin > componentCapacity * layout.size)
        {
            ReallocateComponentList(sceneIndex, componentTypeIndex, componentCapacity);
        }
    }

    ComponentIndex ComponentSystem::CreateComponent(ComponentTypeIndex componentTypeIndex, SceneIndex sceneIndex)
    {
        const std::size_t componentListStartPtrIndex = GetComponentStartPtrIndex(sceneIndex, componentTypeIndex);
        const ComponentLayout layout = m_componentSetup.GetComponentLayoutFromTypeIndex(componentTypeIndex);

        void*& componentListEndVoid = m_componentLists[componentListStartPtrIndex + EndPtrOffset];
        const std::byte* const componentListEnd = static_cast<std::byte*>(componentListEndVoid);
        const std::byte* const componentListBegin = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + BeginPtrOffset]);
        const std::byte* const componentListCapacity = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + CapacityPtrOffset]);
        const wIndex oldCount = (componentListEnd - componentListBegin) / layout.size;

        if (componentListEnd == componentListBegin)
        {
            ReallocateComponentList(sceneIndex, componentTypeIndex, CalculateNextCapacity(oldCount + 1, (componentListCapacity - componentListBegin) / layout.size));
        }

        m_componentSetup.ConstructComponentFromTypeIndex(componentTypeIndex, componentListEndVoid);

        componentListEndVoid = const_cast<void*>(static_cast<const void*>(componentListEnd + layout.size));
        return oldCount;
    }

    void ComponentSystem::ReallocateScenes(wIndex newCapacity)
    {
        const std::size_t sceneSizeBytes = GetSceneSizeBytes();
        void** newComponentLists = static_cast<void**>(
            ::operator new(newCapacity * sceneSizeBytes, std::align_val_t(alignof(void*)))
        );

        if (m_sceneData.empty())
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

    void ComponentSystem::ReallocateComponentList(SceneIndex sceneIndex, ComponentTypeIndex componentTypeIndex, wIndex newCapacity)
    {
        const std::size_t componentListStartPtrIndex = GetComponentStartPtrIndex(sceneIndex, componentTypeIndex);
        const ComponentLayout layout = m_componentSetup.GetComponentLayoutFromTypeIndex(componentTypeIndex);

        const std::byte* componentListEnd = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + EndPtrOffset]);
        const std::byte* componentListBegin = static_cast<std::byte*>(m_componentLists[componentListStartPtrIndex + BeginPtrOffset]);
        const std::size_t oldSizeBytes = componentListEnd - componentListBegin;

        std::byte* newComponentList = static_cast<std::byte*>(
            ::operator new(newCapacity * layout.size, std::align_val_t(layout.alignment))
        );

        if (oldSizeBytes > 0)
        {
            std::memcpy(newComponentList, m_componentLists, m_sceneData.size() * layout.size);
        }

        if (m_componentLists[componentListStartPtrIndex + BeginPtrOffset])
        {
            ::operator delete(m_componentLists, std::align_val_t(layout.alignment));
        }

        m_componentLists[componentListStartPtrIndex + BeginPtrOffset] = newComponentList;
        m_componentLists[componentListStartPtrIndex + EndPtrOffset] = newComponentList + oldSizeBytes;
        m_componentLists[componentListStartPtrIndex + CapacityPtrOffset] = newComponentList + newCapacity * layout.size;
    }
}