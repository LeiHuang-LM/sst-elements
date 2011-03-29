// Copyright 2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2011, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SSTDISKSIM_TRACE_ENTRIES_H
#define _SSTDISKSIM_TRACE_ENTRIES_H

#include <stdlib.h>

#define __ARG_MAX 8

union __argument {
  char* s;
  long l;
  int i;
  unsigned int u;
  size_t t;
  void* p;
};

enum __call
{
  READ = 0,
  WRITE,
  FSYNC,
  CLOSE,
  OPEN,
  END
};

struct sstdisksim_trace_call
{
  char name[16];
  char fmt[16];
};

struct sstdisksim_trace_type
{
  sstdisksim_trace_call* call;
  __argument args[__ARG_MAX];
  sstdisksim_trace_type* next;
};

class sstdisksim_trace_entries
{
public:
  sstdisksim_trace_entries();
  void print_entries();
  void add_entry(__call call, __argument args[__ARG_MAX]);
  sstdisksim_trace_type* pop_entry();
  
private:
  sstdisksim_trace_call __calls[END];
  sstdisksim_trace_type* head;
  sstdisksim_trace_type* tail;  
};

#endif // _SSTDISKSIM_TRACE_ENTRIES_H
