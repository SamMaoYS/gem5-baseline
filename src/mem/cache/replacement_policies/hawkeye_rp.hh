/**
 * Copyright (c) 2018 Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Declaration of a Least Recently Used replacement policy.
 * The victim is chosen using the last touch timestamp.
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_HAWKEYE_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "base/sat_counter.hh"
#include <unordered_map>
#define TOTAL_SET 256 // Equal to size of cache in KB
#define HISTORY_SIZE TOTAL_SET*8
#define TOTAL_WAY 1024 // assoc

struct HawkEyeRPParams;

class HawkEyeRP : public BaseReplacementPolicy
{
  protected:
    /** LRU-specific implementation of replacement data. */
    struct HawkEyeReplData : ReplacementData
    {
        /** Whether the entry is valid. */
        bool valid;
        SatCounter rrpv;
        uint32_t set;
        uint32_t way;
        Addr tag; // Reserved. Not used
        uint8_t data;
        /**
         * Default constructor. Invalidate data.
         */
        HawkEyeReplData(const int num_bits) :
          rrpv(num_bits), valid(false), set(0), way(0), tag(0) {}
    };
    class OPTgen
    {
        protected:
            uint64_t counters[HISTORY_SIZE];
            std::unordered_map<uint32_t, uint64_t> lastAccessed;
            uint64_t currentLocation;
            uint64_t lastFullLocation;
        public:
            OPTgen();
            ~OPTgen(){}
            uint8_t predict(uint32_t);
            void insert(uint32_t, uint8_t hasCapacity);
            void reset();
            void remove(uint32_t);
    };
    OPTgen optgens[TOTAL_SET];
    const unsigned numRRPVBits;
  public:
    /** Convenience typedef. */
    typedef HawkEyeRPParams Params;

    /**
     * Construct and initiliaze this replacement policy.
     */
    HawkEyeRP(const Params *p);

    /**
     * Destructor.
     */
    ~HawkEyeRP();

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Sets its last touch tick as the starting tick.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                              const override;

    /**
     * Touch an entry to update its replacement data.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be touched.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be reset.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find replacement victim using LRU timestamps.
     *
     * @param candidates Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_LRU_RP_HH__
