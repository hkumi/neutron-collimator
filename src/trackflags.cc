#include "trackflags.hh"

namespace TrackFlags
{
    std::unordered_map<int, int>& Map()
    {
        static thread_local std::unordered_map<int, int> flags;
        return flags;
    }
}
