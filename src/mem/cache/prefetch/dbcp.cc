#include "mem/cache/prefetch/dbcp.hh"

#include <cassert>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/random.hh"
#include "base/trace.hh"
#include "debug/HWPrefetch.hh"
#include "mem/cache/base.hh"
#include "mem/cache/prefetch/associative_set_impl.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "params/DBCPPrefetcher.hh"

Addr replace_adress;

namespace Prefetcher {

DBCP::HistoryEntry::HistoryEntry()
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
    signature = encode(signature, pc);
    if (insert_result.second) {
        signature = encode(signature, addr);
    }
    return signature;
}

DBCP::DeadBlockEntry::DeadBlockEntry(const SatCounter& init_confidence)
  : TaggedEntry(), confidence(init_confidence)
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
    deadBlockTable(p->deadblock_table_assoc, p->deadblock_table_entries,
                  p->deadblock_table_indexing_policy,
                  p->deadblock_table_replacement_policy,
                  DeadBlockEntry(initConfidence))
{
    historyTable.resize(historyTableSize);
}

unsigned int DBCP::hash(Addr addr, unsigned int size)
{
    addr = (addr ^ (addr >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    addr = (addr ^ (addr >> 27)) * UINT64_C(0x94d049bb133111eb);
    addr = addr ^ (addr >> 31);
    return addr % size;
}

Addr DBCP::encode(Addr a, Addr b) {
    return ((a+b) & ((1 << 12) -1));
}

void
DBCP::calculatePrefetch(const PrefetchInfo &pfi,
                                    std::vector<AddrPriority> &addresses)
{
    if (!pfi.hasPC()) {
        DPRINTF(HWPrefetch, "Ignoring request with no PC.\n");
        return;
    }

    std::cout << "calcprefetch" << std::endl;

    // Get required packet info
    Addr pf_addr = pfi.getAddr();
    Addr pc = pfi.getPC();
    bool is_secure = pfi.isSecure();
    Addr block_addr = blockAddress(pf_addr);

    // Search for entry in history pc table
    HistoryEntry history_entry = historyTable[hash(block_addr,
        historyTableSize)];
    Addr deadblock_indexing = block_addr ^ encode(history_entry.signature, pc);
    int new_signature = history_entry.updateSignature(block_addr, pc);

    DeadBlockEntry *dead_block_entry =
        deadBlockTable.findEntry(deadblock_indexing, is_secure);

    if (dead_block_entry != nullptr) {
        deadBlockTable.accessEntry(dead_block_entry);

        bool signature_match = (dead_block_entry->signature == new_signature);

        if (signature_match && new_signature != 0) {
            dead_block_entry->confidence++;
            std::cout << "match" << std::endl;
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
            std::cout << "confidence is not enough" <<
                dead_block_entry->confidence.calcSaturation() << std::endl;
            return;
        }

        if (signature_match && new_signature != 0) {
            Addr new_addr = pf_addr + blkSize;
            addresses.push_back(AddrPriority(new_addr, 0));
            // replace_adress = blockAddress(pf_addr);
            cache->invalidateBlock(
                cache->tags->findBlock(block_addr, is_secure));
            std::cout << "calcprefetch hit address" << new_addr << std::endl;
        }
    }
    else {
        // Miss in table
        DPRINTF(HWPrefetch, "Miss: DBCP %x pkt_addr %x (%s)\n", pc, pf_addr,
                is_secure ? "s" : "ns");

        DeadBlockEntry* entry = deadBlockTable.findVictim(deadblock_indexing);

        // Insert new entry's data
        deadBlockTable.insertEntry(deadblock_indexing, is_secure, entry);
        std::cout << "calcprefetch miss" << std::endl;
    }
}

inline uint32_t
DBCPPrefetcherHashedSetAssociative::extractSet(const Addr addr) const
{
    const Addr hash1 = addr >> 1;
    const Addr hash2 = hash1 >> tagShift;
    return (hash1 ^ hash2) & setMask;
}

Addr
DBCPPrefetcherHashedSetAssociative::extractTag(const Addr addr) const
{
    return addr;
}

} // namespace Prefetcher

Prefetcher::DBCPPrefetcherHashedSetAssociative*
DBCPPrefetcherHashedSetAssociativeParams::create()
{
    return new Prefetcher::DBCPPrefetcherHashedSetAssociative(this);
}


Prefetcher::DBCP*
DBCPPrefetcherParams::create()
{
    return new Prefetcher::DBCP(this);
}
