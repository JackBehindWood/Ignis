#pragma once

namespace Ignis
{
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = 1 << 0,
		EventCategoryInput = 1 << 1,
		EventCategoryKeyboard = 1 << 2,
		EventCategoryMouse = 1 << 3,
		EventCategoryMouseButton = 1 << 4
	};

	#define EVENT_CLASS_TYPE(type) static EventType get_static_type() { return EventType::type; }\
								virtual EventType get_event_type() const override { return get_static_type(); }\
								virtual const char* get_name() const override { return #type; }

	#define EVENT_CLASS_CATEGORY(category) virtual int get_category_flags() const override { return category; }

	class Event
	{
	public:
		bool handled = false;

		virtual ~Event() = default;

		virtual EventType get_event_type() const = 0;
		virtual const char* get_name() const = 0;
		virtual int get_category_flags() const = 0;
		virtual String to_string() const { return get_name(); };

		bool is_in_category(EventCategory category)
		{
			return get_category_flags() & category;
		}
	};

	class EventDispatcher
	{
	private:
		Event& m_event;
	public:
		EventDispatcher(Event& event) : m_event(event) {};

		template<typename T, typename F>
		bool dispatch(const F& func)
		{
			if (m_event.get_event_type() == T::get_static_type())
			{
				m_event.handled |= func(static_cast<T&>(m_event));
				return true;
			}
			return false;
		}
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.to_string();
	}
}