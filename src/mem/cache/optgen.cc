#include "mem/cache/optgen.hh"
#include <iostream>

OPTgen::OPTgen(){
    this -> reset();    
}
void
OPTgen::reset(){
    for(int i = 0; i < OPTGEN_CAP; i++){
        counters[i]=0;
    }
    currentLocation=0;
    lastAccessed.clear();
}

uint8_t OPTgen::predict(Addr tag){
    if(lastAccessed.find(tag) == lastAccessed.end()){
        return 0;
    }
    uint64_t i = lastAccessed[tag];
    while(i != currentLocation){
        if(counters[i] >= SET_CAP){
            return 0;
        }
        i++;
        i %= OPTGEN_CAP;
    }
    return 1;
}

void OPTgen::insert(Addr tag, uint8_t hasCapacity){
    if(lastAccessed.find(tag) != lastAccessed.end() && hasCapacity){
        uint64_t i = lastAccessed[tag];
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
    lastAccessed[tag] = currentLocation;
}

void OPTgenList::insert(uint32_t set, Addr tag, uint8_t hasCapacity){
    std::cout << "added set: "<<set << " tag: " << tag << " into history"<< std::endl;
    if(list.find(set)==list.end()){
        list.insert(std::make_pair(set, OPTgen()));
    }
    list[set].insert(tag, hasCapacity);
}

uint8_t OPTgenList::predict(uint32_t set, Addr tag){
    if(list.find(set)==list.end()){
        return 0;
    }
    return list[set].predict(tag);
}