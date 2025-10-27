#pragma once

#include "Event.h"
#include <Ignis/Core/MouseCodes.h>

namespace Ignis
{
	class MouseMovedEvent : public Event
	{
	private:
		float m_mouse_x, m_mouse_y;
	public:
		MouseMovedEvent(const float x, const float y)
			: m_mouse_x(x), m_mouse_y(y) {
		}

		float get_x() const { return m_mouse_x; }
		float get_y() const { return m_mouse_y; }

		String to_string() const override
		{
			Stringstream ss;
			ss << "MouseMovedEvent: " << m_mouse_x << ", " << m_mouse_y;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	};

	class MouseScrolledEvent : public Event
	{
	private:
		float m_x_offset, m_y_offset;
	public:
		MouseScrolledEvent(const float x_offset, const float y_offset)
			: m_x_offset(x_offset), m_y_offset(y_offset) {
		}

		float get_x_offset() const { return m_x_offset; }
		float get_y_offset() const { return m_y_offset; }

		String to_string() const override
		{
			Stringstream ss;
			ss << "MouseScrolledEvent: " << m_x_offset << ", " << m_y_offset;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	};

	class MouseButtonEvent : public Event
	{
	public:
		MouseCode get_mouse_button() const { return m_button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
	protected:
		MouseButtonEvent(const MouseCode button)
			: m_button(button) {
		}

		MouseCode m_button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(const MouseCode button)
			: MouseButtonEvent(button) {
		}

		String to_string() const override
		{
			Stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(const MouseCode button)
			: MouseButtonEvent(button) {
		}

		String to_string() const override
		{
			Stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}