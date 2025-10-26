#pragma once

#include "Event.h"
#include <Ignis/Core/KeyCodes.h>

namespace Ignis
{
	class KeyEvent : public Event
	{
	public:
		KeyCode get_key_code() const { return m_key_code; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(const KeyCode keycode)
			: m_key_code(keycode) {
		}

		KeyCode m_key_code;
	};

	class KeyPressedEvent : public KeyEvent
	{
	private:
		uint16_t m_repeat_count;
	public:
		KeyPressedEvent(const KeyCode keycode, const uint16_t repeat_count)
			: KeyEvent(keycode), m_repeat_count(repeat_count) {
		}

		uint16_t get_repeat_count() const { return m_repeat_count; }

		String to_string() const override
		{
			Stringstream ss;
			ss << "KeyPressedEvent: " << m_repeat_count << " (" << m_repeat_count << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(const KeyCode keycode)
			: KeyEvent(keycode) {
		}

		String to_string() const override
		{
			Stringstream ss;
			ss << "KeyReleasedEvent: " << m_key_code;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(const KeyCode keycode)
			: KeyEvent(keycode) {
		}

		String to_string() const override
		{
			Stringstream ss;
			ss << "KeyTypedEvent: " << m_key_code;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};

}