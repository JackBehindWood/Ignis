#pragma Once

namespace Ignis
{

    class GRIViewport;
    class GRICommandContext
    {
    public:
        virtual ~GRICommandContext() = default;

        virtual void begin_drawing_viewport(GRIViewport* viewport) = 0;
        virtual void end_drawing_viewport(GRIViewport* viewport) = 0;
    };
} // namespace Ignis
