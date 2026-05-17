#pragma once

// Foundation
#include "Ignis/Foundation/Filesystem.h"
#include "Ignis/Foundation/Memory.h"
#include "Ignis/Foundation/RefCounted.h"
#include "Ignis/Foundation/SharedPtr.h"
#include "Ignis/Foundation/UniquePtr.h"
#include "Ignis/Foundation/String.h"
#include "Ignis/Foundation/Vector.h"
#include "Ignis/Foundation/UnordererMap.h"

// Core
#include "Ignis/Core/Base.h"
#include "Ignis/Core/UUID.h"
#include "Ignis/Core/Application.h"
#include "Ignis/Core/Layer.h"
#include "Ignis/Core/Log.h"
#include "Ignis/Core/Assert.h"
#include "Ignis/Core/Timestep.h"
#include "Ignis/Core/Platform.h"

// Asset
#include "Ignis/Asset/Asset.h"
#include "Ignis/Asset/AssetHandle.h"
#include "Ignis/Asset/AssetLoader.h"
#include "Ignis/Asset/AssetCompiler.h"
#include "Ignis/Asset/AssetRegistry.h"
#include "Ignis/Asset/AssetManager.h"

// Rendering
#include "Ignis/Rendering/RenderSystem.h"
#include "Ignis/Rendering/GRI/GRI.h"
#include "Ignis/Rendering/GRI/GRIDefinitions.h"
#include "Ignis/Rendering/GRI/GRIResource.h"

// Events
#include "Ignis/Events/Event.h"
#include "Ignis/Events/KeyEvent.h"
#include "Ignis/Events/MouseEvent.h"
#include "Ignis/Events/ApplicationEvent.h"
