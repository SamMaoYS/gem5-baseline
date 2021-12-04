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
#include "mem/cache/cache_blk.hh"

HawkEyeRP::OPTgen::OPTgen()
{
    this->reset();
}
void HawkEyeRP::OPTgen::reset()
{
    for (int i = 0; i < HISTORY_SIZE; i++)
    {
        counters[i] = 0;
    }
    currentLocation = 0;
    lastAccessed.clear();
}

uint8_t HawkEyeRP::OPTgen::predict(uint32_t memAddr)
{
    if (lastAccessed.find(memAddr) == lastAccessed.end())
    {
        return 0;
    }
    uint64_t i = lastAccessed[memAddr];
    // std::cout << "We're looking at potision between "
    // << (currentLocation+1)%(HISTORY_SIZE) << " and " << i << "[";
    // uint64_t j = currentLocation;
    // for (int i = 0; i < 10; i++)
    // {
    //     std::cout << counters[j] << " ";
    //     if (j > 0)
    //     {
    //         j--;
    //     }
    //     else
    //     {
    //         j = HISTORY_SIZE - 1;
    //     }
    // }
    // std::cout << "]" << std::endl;
    while (i != (currentLocation+1)%(HISTORY_SIZE))
    {
        //std::cout << i << ' ';
        if (counters[i] >= TOTAL_WAY)
        {
            //std::cout << "counter["<< i << "] is at capacity! "
            // <<"with value " << counters[i] << std::endl;
            return 0;
        }
        i++;
        i %= HISTORY_SIZE;
    }
    return 1;
}

void HawkEyeRP::OPTgen::insert(uint32_t memAddr, uint8_t hasCapacity)
{
    currentLocation++;
    currentLocation %= HISTORY_SIZE;
    counters[currentLocation] = 0;
    // std::cout << "The history before update is [";
    // uint64_t j = currentLocation;
    // for (int i = 0; i < 10; i++){
    //     std:: cout << counters[j] << " ";
    //     if (j>0){
    //         j--;
    //     }
    //     else{
    //         j = HISTORY_SIZE-1;
    //     }
    // }
    // std::cout << "]" << std::endl;
    // std::cout << "current location is :" << currentLocation << std::endl;
    if (lastAccessed.find(memAddr) != lastAccessed.end() && hasCapacity)
    {
        uint64_t i = lastAccessed[memAddr];
        //std::cout << "update from " << i << std::endl;
        while (i != currentLocation)
        {
            counters[i]++;
            if (counters[i] >= TOTAL_WAY)
            {
                counters[i] = TOTAL_WAY;
                //lastFullLocation = i;
            }
            i++;
            i %= HISTORY_SIZE;
        }
    }
    lastAccessed[memAddr] = currentLocation;
    // std::cout << "The history after update is [";
    // j = currentLocation;
    // for (int i = 0; i < 10; i++){
    //     std:: cout << counters[j] << " ";
    //     if (j>0){
    //         j--;
    //     }
    //     else{
    //         j = HISTORY_SIZE-1;
    //     }
    // }
    // std::cout << "]" << std::endl;
}

void HawkEyeRP::OPTgen::remove(uint32_t way)
{
    if (lastAccessed.find(way) == lastAccessed.end())
    {
        return;
    }
    lastAccessed.erase(way);
}

HawkEyeRP::HawkEyeRP(const Params *p)
    : BaseReplacementPolicy(p), numRRPVBits(p->num_bits)
{
    fatal_if(numRRPVBits <= 0, "There should be at least one bit per RRPV.\n");
    std::cout << "Initializing HawkEye Replacement Policy..." << std::endl;
}

void
HawkEyeRP::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
    const
{
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint32_t set = casted_replacement_data->set;
    uint32_t way = casted_replacement_data->way;
    OPTgen *currentOPTgen = (OPTgen *)(&optgens[set]);
    // Clear this block from OPTgen to prevent data mishandling
    currentOPTgen->remove(way);
    // Invalidate entry
    casted_replacement_data->valid = false;
    // std::cout << "Invalidating set: "
    // << set << " way: " << way << std::endl;
}

void
HawkEyeRP::touch(const std::shared_ptr<ReplacementData> &replacement_data)
    const
{
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint32_t set = casted_replacement_data->set;
    uint32_t way = casted_replacement_data->way;
    OPTgen *currentOPTgen = (OPTgen *)(&optgens[set]);
    uint8_t predict = currentOPTgen->predict(way);
    if (predict)
    {
        casted_replacement_data->rrpv -= 7;
    }
    else
    {
        casted_replacement_data->rrpv += 7;
    }
    currentOPTgen->insert(way, predict);

    // std::cout << "Touching set: " << set <<
    //     " way: " << way << " with predicted result "
    //     << (int)(predict) << std::endl;
}

void HawkEyeRP::reset(
    const std::shared_ptr<ReplacementData> &replacement_data) const
{
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint32_t set = casted_replacement_data->set;
    uint32_t way = casted_replacement_data->way;
    OPTgen *currentOPTgen = (OPTgen *)(&optgens[set]);
    currentOPTgen->remove(way);
    currentOPTgen->insert(way, 0);
    // At the beginning, all blocks are cache adverse
    casted_replacement_data->rrpv += 7;
    casted_replacement_data->valid = true;

    //std::cout << "Inserting at set: " << set << " way: " << way << std::endl;
}

ReplaceableEntry *
HawkEyeRP::getVictim(const ReplacementCandidates &candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);
    // Update all the replacement data's set and way value
    // per Prof's suggestion
    for (const auto &candidate : candidates)
    {
        std::static_pointer_cast<HawkEyeReplData>(
            candidate->replacementData)
            ->set =
            candidate->getSet();
        std::static_pointer_cast<HawkEyeReplData>(
            candidate->replacementData)
            ->way =
            candidate->getWay();
        std::static_pointer_cast<HawkEyeReplData>(
            candidate->replacementData)
            ->tag = ((CacheBlk *)(candidate))->tag;
    }
    // Visit all candidates to find victim
    ReplaceableEntry *victim = candidates[0];

    // Store victim->rrpv in a variable to improve code readability
    int victim_RRPV = std::static_pointer_cast<HawkEyeReplData>(
                          victim->replacementData)
                          ->rrpv;
    for (const auto &candidate : candidates)
    {
        bool candidate_valid = std::static_pointer_cast<HawkEyeReplData>(
                                   candidate->replacementData)
                                   ->valid;
        int candidate_RRPV = std::static_pointer_cast<HawkEyeReplData>(
                                 candidate->replacementData)
                                 ->rrpv;
        // Stop searching for victims if an invalid entry is found.
        // As no cache miss happens, no need to increase other rrpvs
        if (!candidate_valid)
        {
            // std::cout << "inserting at an empty location" << std::endl;
            victim = candidate;
            break;
        }
        // Update victim entry if necessary (highest RRPV or RRPV = 7)
        if (victim_RRPV < candidate_RRPV || candidate_RRPV == 7)
        {
            victim = candidate;
            victim_RRPV = candidate_RRPV;
        }
        // if (candidate_RRPV==7){
        //     std::cout << "find a cache adverse line! ";
        // }
        // if (candidate_RRPV > 7){
        //     std::cout << "RRPV is NOT valid!" << std::endl;
        // }
    }

    for (const auto &candidate : candidates)
    {
        int candidate_RRPV = std::static_pointer_cast<HawkEyeReplData>(
                                 candidate->replacementData)
                                 ->rrpv;

        if (candidate_RRPV < 6)
        {
            std::static_pointer_cast<HawkEyeReplData>(
                candidate->replacementData)
                ->rrpv++;
        }
    }
    // std::cout << "replacing set: " <<
    //     std::static_pointer_cast<HawkEyeReplData>(
    //     victim->replacementData) -> set <<
    //     " way: " << std::static_pointer_cast<HawkEyeReplData>(
    //         victim->replacementData) -> way << std::endl;
    return victim;
}

std::shared_ptr<ReplacementData>
HawkEyeRP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new HawkEyeReplData(numRRPVBits));
}

HawkEyeRP::~HawkEyeRP()
{
}

HawkEyeRP *
HawkEyeRPParams::create()
{
    return new HawkEyeRP(this);
}
