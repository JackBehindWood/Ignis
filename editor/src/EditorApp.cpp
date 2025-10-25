#include "igpch.h"
#include <Ignis.h>
#include <Ignis/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Ignis 
{

	class Editor : public Application
	{
	public:
		Editor(const ApplicationSpecification& spec)
			: Application(spec)
		{
			push_layer(new EditorLayer());
		}
	};

	Application* create_application(const ApplicationCommandLineArgs& args)
	{
		ApplicationSpecification spec;
		spec.name = "Editor";
		spec.command_line_args = args;

		return new Editor(spec);
	}

}