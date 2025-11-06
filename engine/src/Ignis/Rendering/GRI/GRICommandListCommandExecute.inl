#pragma once

#if !defined(INTERNAL_DECORATOR)
	#define INTERNAL_DECORATOR(method) cmd_list.get_context().method
#endif

namespace Ignis
{
	void GRICommandBeginDrawingViewport::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(begin_drawing_viewport)(viewport, render_target);
	}

	void GRICommandEndDrawingViewport::execute(GRICommandListBase& cmd_list)
	{
		INTERNAL_DECORATOR(end_drawing_viewport)(viewport);
	}
}

#undef INTERNAL_DECORATOR