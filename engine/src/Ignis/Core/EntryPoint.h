#pragma once

#include "Application.h"

extern Ignis::Application* Ignis::create_application(const Ignis::ApplicationCommandLineArgs& spec);


int main(int argc, char** argv)
{
	Ignis::Log::init();

	auto app = Ignis::create_application(Ignis::ApplicationCommandLineArgs{ argc, argv });

	app->run();

	delete app;
}