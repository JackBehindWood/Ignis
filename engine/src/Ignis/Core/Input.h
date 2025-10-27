#pragma once

#include "KeyCodes.h"
#include "MouseCodes.h"


namespace Ignis
{
	class Input
	{
	public:
		static bool is_key_pressed(KeyCode key);

		static bool is_mouse_button_pressed(MouseCode button);
		static void get_mouse_position(float& x, float& y);
		static float get_mouse_x();
		static float get_mouse_y();
	};
}