#ifndef TRACKFLAGS_HH
#define TRACKFLAGS_HH

#include <unordered_map>

// Remembers which parts of the geometry each track has passed through (one map per thread,
// cleared at the start of every event). Bits:
//   1 = central moderator ("HDPE")        2 = lead brick ("Lead")
//   4 = side HDPE blocks                  8 = borated sheet ("BoratedHDPE")
//  16 = rear lead plate ("RearLead")    32 = graphite channel block ("ChannelGraphite")
namespace TrackFlags
{
    std::unordered_map<int, int>& Map();   // key = track ID, value = bits
}

#endif
