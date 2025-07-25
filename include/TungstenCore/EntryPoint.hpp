#ifdef TUNGSTEN_CORE_ENTRY_POINT_ALREADY_INCLUDED
    #error "EntryPoint.hpp included more than once! Only include in one translation unit."
#else
    #define TUNGSTEN_CORE_ENTRY_POINT_ALREADY_INCLUDED

    #include "TungstenCore.hpp"

    extern void Awake(wCore::ComponentManager& componentManager);

    int main()
    {
        wCore::Application app;
        Awake(app.GetComponentManager());
        return app.Run().exitCode;
    }

#endif