#ifndef TUNGSTEN_CORE_COMPONENT_MANAGER_HPP
#define TUNGSTEN_CORE_COMPONENT_MANAGER_HPP

#include "TungstenCore.hpp"
#include "TungstenUtils/macros/nameof.hpp"

namespace wCore
{
    class ComponentManager
    {
    public:
        struct Component
        {
            uint32_t type;
            uint32_t index;
        };

        template<typename T>
        void Add()
        {
            const std::string_view name = W_NAME_OF(T);
        }

    private:
    };
}

#endif