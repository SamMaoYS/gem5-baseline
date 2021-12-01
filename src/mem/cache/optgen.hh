#ifndef __MEM_CACHE_OPTGEN_HH__
#define __MEM_CACHE_OPTGEN_HH__

#include <cassert>
#include <cstdint>
#include <string>

#include "base/addr_range.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "debug/Cache.hh"
#include "debug/CachePort.hh"
#include "enums/Clusivity.hh"
#include "mem/cache/cache_blk.hh"
#include "mem/cache/compressors/base.hh"
#include "mem/cache/mshr_queue.hh"
#include "mem/cache/tags/base.hh"
#include "mem/cache/write_queue.hh"
#include "mem/cache/write_queue_entry.hh"
#include "mem/packet.hh"
#include "mem/packet_queue.hh"
#include "mem/qport.hh"
#include "mem/request.hh"
#include "params/WriteAllocator.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"
#include "sim/probe/probe.hh"
#include "sim/serialize.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

#include <unordered_map>

#define SET_CAP 10
#define OPTGEN_CAP SET_CAP*8
#define NUM_SET 10
#define MAX_COUNTER_VAL 16
class OPTgen
{
    protected:
        uint64_t counters[OPTGEN_CAP];
        std::unordered_map<uint32_t, uint64_t> lastAccessed;
        uint64_t currentLocation;
    public:
        OPTgen();
        ~OPTgen(){}
        uint8_t predict(Addr tag);
        void insert(Addr tag, uint8_t hasCapacity);
        void reset();
};

class OPTgenList
{
    protected:
        std::unordered_map<uint32_t, OPTgen> list;
    public:
        OPTgenList(){}
        ~OPTgenList(){}
        uint8_t predict(uint32_t set, Addr tag);
        void insert(uint32_t set, Addr tag, uint8_t hasCapacity);
};

#endif