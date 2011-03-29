// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SSTDISKSIM_EVENT_H
#define _SSTDISKSIM_EVENT_H

#include "sst/core/serialization/element.h"
#include <sst/core/event.h>

enum eventtype {DISKSIMREAD, DISKSIMWRITE, DISKSIMEND};

class sstdisksim_event : public SST::Event {
public:
  eventtype etype;
  unsigned long pos;
  unsigned long count;
  unsigned long devno;
  unsigned long time;

  sstdisksim_event* next_event;

  bool completed;
  void* bossClass;

  void (*finishedCall)(void);

  sstdisksim_event();

private:
  friend class boost::serialization::access;
  template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
      ar & BOOST_SERIALIZATION_NVP(etype);
      ar & BOOST_SERIALIZATION_NVP(pos);
      ar & BOOST_SERIALIZATION_NVP(count);
      ar & BOOST_SERIALIZATION_NVP(devno);
      ar & BOOST_SERIALIZATION_NVP(time);
      ar & BOOST_SERIALIZATION_NVP(completed);      
    }
}; 
    
#endif /* _SSTDISKSIM_EVENT_H */
