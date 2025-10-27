#pragma once

#include "Event.h"

namespace Ignis
{
	class GRIViewport;

	class WindowResizeEvent : public Event
	{
	private:
		GRIViewport* m_viewport;
		uint32_t m_width, m_height;
	public:
		WindowResizeEvent(GRIViewport* viewport, uint32_t width, uint32_t height)
			: m_viewport(viewport), m_width(width), m_height(height) {}

		GRIViewport* get_viewport() const { return m_viewport; }
		uint32_t get_width() const { return m_width; }
		uint32_t get_height() const { return m_height; }


		String to_string() const override
		{
			Stringstream ss;
			ss << "WindowResizeEvent: " << m_width << ", " << m_height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowCloseEvent : public Event
	{
	private:
		GRIViewport* m_viewport;
	public:
		WindowCloseEvent(GRIViewport* viewport) : m_viewport(viewport) {}

		GRIViewport* get_viewport() const { return m_viewport; }

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppTickEvent : public Event
	{
	public:
		AppTickEvent() = default;

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() = default;

		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() = default;

		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};
}