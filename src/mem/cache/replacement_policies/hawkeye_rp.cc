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

#include "mem/cache/replacement_policies/hawkeye_rp.hh"

#include <cassert>
#include <memory>
#include <iostream>

#include "params/HawkEyeRP.hh"
#include "sim/core.hh"
//#include "base/trace.hh"
#include "debug/HWPrefetch.hh"

HawkEyeRP::OPTgen::OPTgen(){
    this -> reset();
}
void
HawkEyeRP::OPTgen::reset(){
    for(int i = 0; i < OPTGEN_CAP; i++){
        counters[i]=0;
    }
    currentLocation=0;
    lastAccessed.clear();
}

uint8_t HawkEyeRP::OPTgen::predict(uint32_t memAddr){
    if (lastAccessed.find(memAddr) == lastAccessed.end()){
        return 0;
    }
    uint64_t i = lastAccessed[memAddr];
    while(i != currentLocation){
        if(counters[i] >= SET_CAP){
            return 0;
        }
        i++;
        i %= OPTGEN_CAP;
    }
    return 1;
}

void HawkEyeRP::OPTgen::insert(uint32_t memAddr, uint8_t hasCapacity){
    if (lastAccessed.find(memAddr) != lastAccessed.end() && hasCapacity){
        uint64_t i = lastAccessed[memAddr];
        while(i != currentLocation){
            counters[i]++;
            if(counters[i]>=MAX_COUNTER_VAL){
                counters[i]=MAX_COUNTER_VAL;
            }
            i++;
            i %= OPTGEN_CAP;
        }
    }
    counters[currentLocation]++;
    currentLocation++;
    currentLocation %= OPTGEN_CAP;
    counters[currentLocation]=0;
    lastAccessed[memAddr] = currentLocation;
}

void HawkEyeRP::OPTgen::remove(uint32_t way){
    if (lastAccessed.find(way) == lastAccessed.end()){
        return;
    }
    lastAccessed.erase(way);
}

HawkEyeRP::HawkEyeRP(const Params *p)
    : BaseReplacementPolicy(p), numRRPVBits(p->num_bits)
{
    fatal_if(numRRPVBits <= 0, "There should be at least one bit per RRPV.\n");
}

void
HawkEyeRP::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
const
{
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint32_t set = casted_replacement_data -> set;
    uint32_t way = casted_replacement_data -> way;
    OPTgen* currentOPTgen = (OPTgen*)(&optgens[set]);
    // Clear this block from OPTgen to prevent data mishandling
    currentOPTgen -> remove(way);
    // Invalidate entry
    casted_replacement_data->valid = false;
    std::cout << "Invalidating set: " << set << " way: " << way << std::endl;
}

void
HawkEyeRP::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint32_t set = casted_replacement_data -> set;
    uint32_t way = casted_replacement_data -> way;
    OPTgen* currentOPTgen = (OPTgen*)(&optgens[set]);
    uint8_t predict = currentOPTgen -> predict(way);
    if (predict){
        casted_replacement_data -> rrpv -= 7;
    }
    else{
        casted_replacement_data -> rrpv += 7;
    }
    currentOPTgen -> insert(way, predict);

    std::cout << "Touching set: " << set <<
        " way: " << way << " with predicted result "
        << (int)(predict) << std::endl;
}

void
HawkEyeRP::reset(
    const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint32_t set = casted_replacement_data -> set;
    uint32_t way = casted_replacement_data -> way;
    OPTgen* currentOPTgen = (OPTgen*)(&optgens[set]);
    currentOPTgen -> remove(way);
    // At the beginning, all blocks are cache adverse
    casted_replacement_data -> rrpv -= 7;
    casted_replacement_data -> valid = true;

    std::cout << "Inserting at set: " << set << " way: " << way << std::endl;
}

ReplaceableEntry*
HawkEyeRP::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);
    // Update all the replacement data's set and way value
    // per Prof's suggestion
    for (const auto& candidate : candidates) {
        std::static_pointer_cast<HawkEyeReplData>(
                        candidate->replacementData) -> set =
                        candidate -> getSet();
        std::static_pointer_cast<HawkEyeReplData>(
                        candidate->replacementData) -> way =
                        candidate -> getWay();
    }
    // Visit all candidates to find victim
    ReplaceableEntry* victim = candidates[0];

    // Store victim->rrpv in a variable to improve code readability
    int victim_RRPV = std::static_pointer_cast<HawkEyeReplData>(
                        victim->replacementData)->rrpv;
    for (const auto& candidate : candidates) {
        bool candidate_valid = std::static_pointer_cast<HawkEyeReplData>(
                        candidate->replacementData) ->valid;
        int candidate_RRPV = std::static_pointer_cast<HawkEyeReplData>(
                        candidate->replacementData)->rrpv;
        // Stop searching for victims if an invalid entry is found.
        // As no cache miss happens, no need to increase other rrpvs
        if (!candidate_valid) {
            return candidate;
        }

        // Update victim entry if necessary (highest RRPV or RRPV = 7)
        if (victim_RRPV < candidate_RRPV || candidate_RRPV==7) {
            victim = candidate;
            victim_RRPV = candidate_RRPV;
        }
    }

    for (const auto& candidate : candidates) {
        std::shared_ptr<HawkEyeReplData> casted_replacement_data =
            std::static_pointer_cast<HawkEyeReplData>(candidate->replacementData);
        if(casted_replacement_data->rrpv <7){
            casted_replacement_data->rrpv+=1;
        }
    }
    std::cout << "replacing set: " <<
        std::static_pointer_cast<HawkEyeReplData>(
        victim->replacementData) -> set <<
        " way: " << std::static_pointer_cast<HawkEyeReplData>(
            victim->replacementData) -> way << std::endl;
    return victim;
}

std::shared_ptr<ReplacementData>
HawkEyeRP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new HawkEyeReplData(numRRPVBits));
}

HawkEyeRP::~HawkEyeRP(){
}

HawkEyeRP*
HawkEyeRPParams::create()
{
    return new HawkEyeRP(this);
}
