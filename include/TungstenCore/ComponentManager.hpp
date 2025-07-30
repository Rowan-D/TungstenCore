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
        template<typename T>
        class ComponentID
        {
        public:
            static inline uint32_t Get() { return s_id; }
            static inline void Set(uint32_t id) { s_id = id; }

        private:
            static uint32_t s_id;
        };
    };

}

#endif