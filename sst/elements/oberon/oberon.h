// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SST_OberonComponent_H
#define _SST_OberonComponent_H

#include <sst/core/sst_types.h>

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "oberonmodel.h"
#include "oberonengine.h"

namespace SST {
class Params;
namespace Oberon {

class OberonComponent : public SST::Component {
public:

  OberonComponent(SST::ComponentId_t id, SST::Params& params);
  void setup() { }
  void finish() {
	if(dumpAtEnd)
		model->dumpMemory();

	delete model;
	delete engine;
  }

private:
  OberonComponent();  // for serialization only
  OberonComponent(const OberonComponent&); // do not implement
  void operator=(const OberonComponent&); // do not implement

  OberonEngine* engine;
  OberonModel* model;
  Output* output;
  bool dumpAtEnd;

  void handleEvent( SST::Event *ev );
  virtual bool clockTic( SST::Cycle_t );

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()

};

}
}

#endif /* _OberonComponent_H */
