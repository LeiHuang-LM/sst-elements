/* 
 * File:   coherenceControllers.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#ifndef COHERENCECONTROLLERS_H
#define COHERENCECONTROLLERS_H

#include "assert.h"
#include "sst/core/link.h"
#include <bitset>
#include <cstring>
#include "cacheArray.h"
#include "sst/core/output.h"
#include "cacheListener.h"
#include <boost/assert.hpp>
#include <queue>
#include "memNIC.h"

using namespace std;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

namespace SST { namespace MemHierarchy {

class Cache;

class CoherencyController{
public:
    typedef CacheArray::CacheLine CacheLine;
    typedef unsigned int uint;

    struct response {
        Link* deliveryLink;
        MemEvent* event;
        uint64_t deliveryTime;
        bool cpuResponse;
    };
    
    MemNIC*    directoryLink_;
    uint64_t   timestamp_;
    uint64_t   accessLatency_;
    virtual bool sendResponse(MemEvent* _event, BCC_MESIState _newState, vector<uint8_t>* _data, int _childId);
    virtual bool sendAckResponse(MemEvent *event);
    virtual void sendCommand(Command cmd, CacheLine* cacheLine, Link* deliveryLink);
    virtual void sendOutgoingCommands() = 0;
    
protected:
    CoherencyController(const Cache* _cache, Output* _dbg, uint _lineSize): owner_(_cache), d_(_dbg),
                        lineSize_(_lineSize), sentEvents_(0), timestamp_(0), accessLatency_(1){}
    ~CoherencyController(){}
    const Cache* owner_;
    Output*    d_;
    uint       lineSize_;
    bool       L1_;
    int        sentEvents_;
    queue<response> outgoingEventQueue_;
};




}}

#endif	/* COHERENCECONTROLLERS_H */