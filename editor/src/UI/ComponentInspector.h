#pragma once

#include <Ignis/Scene/Entity.h>

namespace Ignis
{

struct ComponentDescriptor
{
    const char* name;
    const char* category;
    bool (*has)(Entity&);
    void (*add)(Entity&);
    void (*remove)(Entity&);
    void (*draw)(Entity&);
};

class ComponentInspector
{
public:
    static void                               register_component(ComponentDescriptor desc);
    static void                               register_defaults();
    static const Vector<ComponentDescriptor>& all();
};

} // namespace Ignis
