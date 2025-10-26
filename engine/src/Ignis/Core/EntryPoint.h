#pragma once

#include "Application.h"
#include "Platform.h"

namespace Ignis
{
	void init()
	{
		Log::init(false); // Initialize the logger without file logging

		Platform::init();
	}

	extern Application* create_application(const ApplicationCommandLineArgs& spec);
}

int main(int argc, char** argv)
{
	Ignis::init();

	Ignis::Application* app = Ignis::create_application(Ignis::ApplicationCommandLineArgs{ argc, argv });

	app->run();

	app->close();

	delete app;

	Ignis::Platform::shutdown();
}