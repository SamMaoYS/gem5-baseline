#include "mem/cache/prefetch/dbcp.hh"

#include <cassert>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/random.hh"
#include "base/trace.hh"
#include "debug/HWPrefetch.hh"
#include "mem/cache/prefetch/associative_set_impl.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "params/DBCPPrefetcher.hh"

namespace Prefetcher {

DBCP::HistoryEntry::HistoryEntry(const SatCounter& init_confidence)
  : confidence(init_confidence)
{
    invalidate();
}

void
DBCP::HistoryEntry::invalidate()
{
    signature = 0;
    prior_addresses.clear();
}

Addr
DBCP::HistoryEntry::updateSignature(const Addr addr, const Addr pc)
{
    auto insert_result = prior_addresses.insert(addr);
    // address not int the set
    if (insert_result.second) {
        signature = signature ^ addr ^ pc;
    }
    else {
        signature = signature ^ pc;
    }
}

DBCP::DeadBlockEntry::DeadBlockEntry(const SatCounter& init_confidence)
  : confidence(init_confidence)
{
    invalidate();
}

void
DBCP::DeadBlockEntry::invalidate()
{
    prediction = 0;
    signature = 0;
    confidence.reset();
}

Addr
DBCP::DeadBlockEntry::updateSignature(const Addr new_signature)
{
    signature = new_signature;
    return signature;
}

DBCP::DBCP(const DBCPPrefetcherParams *p)
  : Queued(p),
    initConfidence(p->confidence_counter_bits, p->initial_confidence),
    threshConf(p->confidence_threshold/100.0),
    historyTableSize(p->history_table_size),
    deadBlockTableSize(p->deadBlock_table_size)
{
}

unsigned int DBCP::hash(Addr addr, unsigned int size) const
{
    return addr % size;
}

void
DBCP::calculatePrefetch(const PrefetchInfo &pfi,
                                    std::vector<AddrPriority> &addresses)
{
    if (!pfi.hasPC()) {
        DPRINTF(HWPrefetch, "Ignoring request with no PC.\n");
        return;
    }

    // Get required packet info
    Addr pf_addr = pfi.getAddr();
    Addr pc = pfi.getPC();
    bool is_secure = pfi.isSecure();

    hash(pf_addr, historyTableSize)

    // Search for entry in history pc table
    HistoryEntry *history_entry = historyTable[hash(pf_addr, historyTableSize)];
    int new_signature = history_entry->updateSignature(pf_addr, pc);

    DeadBlockEntry *dead_block_entry = deadBlockTable[hash(pf_addr, historyTableSize)];
    bool signature_match = (dead_block_entry->signature == new_signature);

    if (signature_match && new_signature != 0) {
        dead_block_entry->confidence++;
    } else {
        dead_block_entry->confidence--;
        if (dead_block_entry->confidence.calcSaturation() < threshConf) {
            dead_block_entry->signature = new_signature;
        }
    }

    DPRINTF(HWPrefetch, "Hit: DBCP %x pkt_addr %x (%s) signature %d (%s), "
            "conf %d\n", pc, pf_addr, is_secure ? "s" : "ns",
            new_signature, signature_match ? "match" : "change",
            (int)dead_block_entry->confidence);

    if (dead_block_entry->confidence.calcSaturation() < threshConf) {
        return;
    }

    if (signature_match && new_signature != 0) {
        Addr new_addr = pf_addr + blkSize;
        addresses.push_back(AddrPriority(new_addr, 0));

        // TODO how to replace A2
    }
}

} // namespace Prefetcher

Prefetcher::DBCP*
DBCPPrefetcherParams::create()
{
    return new Prefetcher::DBCP(this);
}
