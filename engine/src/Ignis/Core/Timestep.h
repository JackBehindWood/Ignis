#pragma once

namespace Ignis 
{

	class Timestep
	{
	public:
		Timestep(float time = 0.0f)
			: m_time(time)
		{
		}

		operator float() const { return m_time; }

		inline float get_seconds() const { return m_time; }
		inline float get_milliseconds() const { return m_time * 1000.0f; }
	private:
		float m_time;
	};

}