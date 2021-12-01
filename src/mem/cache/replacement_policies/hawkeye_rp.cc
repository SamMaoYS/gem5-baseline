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
    //DPRINTF(HWPrefetch, "Constructor of HawkEye.\n");
    
}
void
HawkEyeRP::OPTgen::reset(){
    for(int i = 0; i < OPTGEN_CAP; i++){
        counters[i]=0;
    }
    currentLocation=0;
    lastAccessed.clear();
}

uint8_t HawkEyeRP::OPTgen::predict(Addr memAddr){
    if(lastAccessed.find(memAddr) == lastAccessed.end()){
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

void HawkEyeRP::OPTgen::insert(Addr memAddr, uint8_t hasCapacity){
    if(lastAccessed.find(memAddr) != lastAccessed.end() && hasCapacity){
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

HawkEyeRP::HawkEyeRP(const Params *p)
    : BaseReplacementPolicy(p), numRRPVBits(p->num_bits)
{
    std::cout<<"Entering Hawkeye Replacement\nAbdelrahman\n";
    fatal_if(numRRPVBits <= 0, "There should be at least one bit per RRPV.\n");
}

void
HawkEyeRP::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
const
{
    std::cout << "HawkEye invalidated" << std::endl;
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);

    // Invalidate entry
    casted_replacement_data->valid = false;
}

void
HawkEyeRP::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::cout << "HawkEye touched" << std::endl;
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint8_t predict = 0; //dummy here. not sure how to invoke OPTgen
    if (predict){
        casted_replacement_data -> rrpv -= 7;
    }
    else{
        casted_replacement_data -> rrpv += 7;
    }
}

void
HawkEyeRP::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    std::cout << "HawkEye resetted" << std::endl;
    std::shared_ptr<HawkEyeReplData> casted_replacement_data =
        std::static_pointer_cast<HawkEyeReplData>(replacement_data);
    uint8_t predict = 0; //dummy here. not sure how to invoke OPTgen
    if (predict){
        casted_replacement_data -> rrpv -= 7;
    }
    else{
        casted_replacement_data -> rrpv += 7;
    }
    casted_replacement_data->valid = true;
}

ReplaceableEntry*
HawkEyeRP::getVictim(const ReplacementCandidates& candidates) const
{
    std::cout << "HawkEye getting victim" << std::endl;
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

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
        // Stop searching for victims if an invalid entry is found
        if (!candidate_valid) {
            return candidate;
        }
        if (candidate_RRPV==7) {
            victim = candidate;
            break;
        }
        // Update victim entry if necessary
        if (victim_RRPV < candidate_RRPV) {
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
    return victim;
}

std::shared_ptr<ReplacementData>
HawkEyeRP::instantiateEntry()
{
    std::cout << "HawkEye IE" << std::endl;
    return std::shared_ptr<ReplacementData>(new HawkEyeReplData(numRRPVBits));
}

HawkEyeRP::~HawkEyeRP(){
    std::cout << "HawkEye exiting" << std::endl;
}

HawkEyeRP*
HawkEyeRPParams::create()
{
    return new HawkEyeRP(this);
}
