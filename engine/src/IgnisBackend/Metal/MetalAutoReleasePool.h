
namespace NS
{
    class AutoreleasePool;
} // namespace NS


namespace Ignis
{
    class MetalScopedAutoreleasePool
    {
    private:
        NS::AutoreleasePool* m_autorelease_pool;
    public:
        MetalScopedAutoreleasePool();
        ~MetalScopedAutoreleasePool();
    };

    #define MTL_AUTORELEASE_POOL MetalScopedAutoreleasePool m_autorelease_pool_instance;
} // namespace Ignis

