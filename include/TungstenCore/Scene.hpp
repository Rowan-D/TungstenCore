#ifndef TUNGSTEN_CORE_SCENE_HPP
#define TUNGSTEN_CORE_SCENE_HPP

#include <cstdint>
#include <vector>

namespace wCore
{
    class Scene
    {
    public:
        Scene() : exists(true), m_components() {}

        bool exists;
    private:
        struct Component
        {
            Component(uint32_t a_type, uint32_t a_index) : type(type), index(index) {}

            uint32_t type;
            uint32_t index;
        };

        std::vector<Component> m_components;
    };
}

#endif