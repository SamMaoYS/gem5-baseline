#ifndef __MEM_CACHE_PREFETCH_DBCP_HH__
#define __MEM_CACHE_PREFETCH_DBCP_HH__

#include "base/sat_counter.hh"
#include "mem/cache/prefetch/associative_set.hh"
#include "mem/cache/prefetch/queued.hh"
#include "mem/packet.hh"

struct DBCPPrefetcherParams;

namespace Prefetcher {

class DBCP : public Queued
{
  protected:
    /** Number of entries for history table */
    const unsigned int history_table_size;
    /** Number of entries for correlation table */
    const unsigned int correlation_table_size;

    struct HistoryTableEntry {
        Addr prior_mem_address;
        std::vector<Addr> trace;

        HistoryTableEntry() {}
        UpdateEntry(Addr mem_addr, Addr pc);
    };

    using HistoryTable = std::vector<HistoryTableEntry>;
    HistoryTable history_table;

    size_t ComputeSignature(const Addr addr, const HistoryTableEntry &history_table_entry);

    struct CorrelationTableEntry {
        size_t signature;
        Addr prediction;

        CorrelationTableEntry() {}
        UpdateEntry(size_t s, Addr correlation);
    };

    bool AddrIsDead(Addr addr);

    Addr GetPredictedAddr(const Addr pc);

    /**
     * Updates the prefetcher structures upon an instruction retired
     * @param pc PC of the instruction being retired
     */
    void notifyRetiredInst(const Addr pc);

    /**
     * Probe Listener to handle probe events from the CPU
     */
    class PrefetchListenerPC : public ProbeListenerArgBase<Addr>
    {
        public:
        PrefetchListenerPC(DBCP &_parent, ProbeManager *pm,
                            const std::string &name)
            : ProbeListenerArgBase(pm, name),
                parent(_parent) {}
        void notify(const Addr& pc) override;
        protected:
        DBCP &parent;
    };

    /** Array of probe listeners */
    std::vector<PrefetchListenerPC *> listenersPC;
    
  public:
    DBCP(const DBCPPrefetcherParams* p);
    ~DBCP() = default;

    void calculatePrefetch(const PrefetchInfo &pfi,
                           std::vector<AddrPriority> &addresses) override;
};

} // namespace Prefetcher

#endif//__MEM_CACHE_PREFETCH_DBCP_HH__
