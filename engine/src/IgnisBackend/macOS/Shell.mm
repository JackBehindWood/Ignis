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

} // namespace Ignis
