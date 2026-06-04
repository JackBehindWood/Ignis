#pragma once
#include "Ignis/Foundation/Foundation.h"

namespace Ignis
{

class Shell
{
public:
    static void reveal_in_file_manager(const Path& path);
    static int  exec(const String& cmd, String* output = nullptr);
};

} // namespace Ignis
