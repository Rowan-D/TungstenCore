#ifndef TUNGSTEN_CORE_APPLICATION_HPP
#define TUNGSTEN_CORE_APPLICATION_HPP

#include "TungstenCore/ComponentSystem.hpp"

namespace wCore {
    class Application
    {
    public:
        struct RunOutput
        {
            RunOutput(int a_exitCode)
                : exitCode(a_exitCode)
            {
            }

            int exitCode;
        };

        Application();
        RunOutput Run();

        inline ComponentSystem& GetComponentSystem() { return m_componentSystem; }

    private:
        ComponentSystem m_componentSystem;
    };
}

#endif