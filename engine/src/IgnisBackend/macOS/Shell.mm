#include "Ignis/Core/Shell.h"
#import <AppKit/AppKit.h>

namespace Ignis
{

void Shell::reveal_in_file_manager(const Path& path)
{
    NSString* str = [NSString stringWithUTF8String:path.string().c_str()];
    NSURL*    url = [NSURL fileURLWithPath:str];
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[ url ]];
}

int Shell::exec(const String& cmd, String* output)
{
    const String full = output ? cmd + " 2>&1" : cmd;
    FILE*        pipe = popen(full.c_str(), "r");
    if (!pipe)
    {
        return -1;
    }
    if (output)
    {
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe))
        {
            *output += buf;
        }
    }
    return pclose(pipe);
}

} // namespace Ignis
