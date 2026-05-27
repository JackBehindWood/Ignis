#include "UUID.h"
#include <random>

namespace Ignis
{

uint64_t UUID::generate()
{
    static std::random_device                      rd;
    static std::mt19937_64                         engine(rd());
    static std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);
    return dist(engine);
}

} // namespace Ignis