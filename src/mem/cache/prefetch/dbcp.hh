/**
 * @file
 * Describes a dead-block correlating prefetcher.
 */

#ifndef __MEM_CACHE_PREFETCH_DBCP_HH__
#define __MEM_CACHE_PREFETCH_DBCP_HH__

#include <string>
#include <unordered_map>
#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "mem/cache/prefetch/associative_set.hh"
#include "mem/cache/prefetch/queued.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/indexing_policies/set_associative.hh"
#include "mem/packet.hh"

class BaseIndexingPolicy;
class BaseReplacementPolicy;
struct DBCPPrefetcherParams;

extern Addr replace_adress;

namespace Prefetcher {

class DBCP : public Queued
{
  protected:
    const unsigned int historyTableSize;

    /** Initial confidence counter value for the pc tables. */
    const SatCounter initConfidence;

    /** Confidence threshold for prefetch generation. */
    const double threshConf;

    /** Hashed history table */
    struct HistoryEntry
    {
        HistoryEntry();

        void invalidate();

        Addr updateSignature(const Addr addr, const Addr pc);

        std::set<Addr> prior_addresses;
        Addr signature;
    };
    std::vector<HistoryEntry> historyTable;

    /** Hashed DeadBlocks. */
    struct DeadBlockEntry : public TaggedEntry
    {
        DeadBlockEntry(const SatCounter& init_confidence);

        void invalidate() override;

        Addr updateSignature(const Addr new_signature);

        Addr prediction;
        Addr signature;
        SatCounter confidence;
    };
    AssociativeSet<DeadBlockEntry> deadBlockTable;

    /** Generate a hash for the specified address to index the table
     *  @param addr: address to hash
     *  @param size: table size
     */
    unsigned int hash(Addr addr, unsigned int size);

  public:
    DBCP(const DBCPPrefetcherParams *p);

    static Addr encode(Addr a, Addr b);

    void calculatePrefetch(const PrefetchInfo &pfi,
                           std::vector<AddrPriority> &addresses) override;
};

} // namespace Prefetcher

#endif // __MEM_CACHE_PREFETCH_DBCP_HH__
