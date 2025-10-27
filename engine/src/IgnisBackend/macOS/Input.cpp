#include "igpch.h"

#ifdef IG_PLATFORM_MACOS
#include <Ignis/Core/Input.h>

#include <Ignis/Core/Application.h>

#include <GLFW/glfw3.h>

namespace Ignis
{
	bool Input::is_key_pressed(const KeyCode key)
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::get().get_window().get_viewport()->get_native_handle());
		int32_t state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS;
	}

	bool Input::is_mouse_button_pressed(const MouseCode button)
	{
		GLFWwindow* window = static_cast<GLFWwindow*>(Application::get().get_window().get_viewport()->get_native_handle());	
		int32_t state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	void Input::get_mouse_position(float& x, float& y)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::get().get_window().get_viewport()->get_native_handle());
        double dx, dy;
        glfwGetCursorPos(window, &dx, &dy);
        x = static_cast<float>(dx);
        y = static_cast<float>(dy);
    }

	float Input::get_mouse_x()
	{
        float x, y;
        get_mouse_position(x, y);
        return x;
	}

	float Input::get_mouse_y()
	{
		float x, y;
        get_mouse_position(x, y);
        return y;
	}

}

#endif