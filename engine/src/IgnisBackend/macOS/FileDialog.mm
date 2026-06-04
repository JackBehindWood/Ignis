#include "Ignis/Core/FileDialog.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#import <AppKit/AppKit.h>
#pragma clang diagnostic pop

namespace Ignis
{

Optional<Path> FileDialog::open(FileDialogMode mode, const FileDialogOptions& opts)
{
    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = NO;
    panel.canChooseDirectories    = NO;
    panel.canChooseFiles          = YES;

    NSArray<NSString*>* types = nil;
    switch (mode)
    {
        case FileDialogMode::OpenProject:
            panel.title   = @"Open Project";
            panel.message = @"Select an Ignis project file";
            types         = @[ @"igproject" ];
            break;
        case FileDialogMode::OpenScene:
            panel.title   = @"Open Scene";
            panel.message = @"Select an Ignis scene file";
            types         = @[ @"igscene" ];
            break;
        case FileDialogMode::ImportAsset:
            panel.title   = @"Import Asset";
            panel.message = @"Select an asset file to import";
            types         = @[ @"png", @"jpg", @"jpeg", @"obj", @"fbx", @"mtl", @"igmat" ];
            break;
    }

    if (opts.filters)
    {
        NSMutableArray<NSString*>* custom = [NSMutableArray arrayWithCapacity:opts.filters->size()];
        for (const String& ext : *opts.filters)
        {
            [custom addObject:[NSString stringWithUTF8String:ext.c_str()]];
        }
        types = custom;
    }

    if (opts.initial_dir)
    {
        NSString* dir      = [NSString stringWithUTF8String:opts.initial_dir->string().c_str()];
        panel.directoryURL = [NSURL fileURLWithPath:dir isDirectory:YES];
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    panel.allowedFileTypes = types;
#pragma clang diagnostic pop

    if ([panel runModal] == NSModalResponseOK)
    {
        return Path{[panel.URL.path UTF8String]};
    }
    return NullOpt;
}

Optional<Path> FileDialog::save_scene(const FileDialogOptions& opts)
{
    NSSavePanel* panel         = [NSSavePanel savePanel];
    panel.title                = @"Save Scene";
    panel.message              = @"Choose a location to save the scene";
    panel.nameFieldStringValue = @"untitled";

    if (opts.initial_dir)
    {
        NSString* dir      = [NSString stringWithUTF8String:opts.initial_dir->string().c_str()];
        panel.directoryURL = [NSURL fileURLWithPath:dir isDirectory:YES];
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    panel.allowedFileTypes = @[ @"igscene" ];
#pragma clang diagnostic pop

    if ([panel runModal] == NSModalResponseOK)
    {
        return Path{[panel.URL.path UTF8String]};
    }
    return NullOpt;
}

} // namespace Ignis
