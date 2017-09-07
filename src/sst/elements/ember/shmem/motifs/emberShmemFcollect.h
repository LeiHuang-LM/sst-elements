// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SHMEM_FCOLLECT
#define _H_EMBER_SHMEM_FCOLLECT

#include <strings.h>
#include "shmem/emberShmemGen.h"
#include <cxxabi.h>

namespace SST {
namespace Ember {

template <class TYPE>
class EmberShmemFcollectGenerator : public EmberShmemGenerator {

public:
	EmberShmemFcollectGenerator(SST::Component* owner, Params& params) :
		EmberShmemGenerator(owner, params, "ShmemFcollect" ), m_phase(0) 
	{ 
        m_nelems = params.find<int>("arg.nelems", 1 );
        m_printResults = params.find<bool>("arg.printResults", false );
        int status;
        std::string tname = typeid(TYPE).name();
		char* tmp = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
		m_type_name = tmp; 
		free(tmp);

        assert( 4 == sizeof(TYPE) || 8 == sizeof(TYPE) );
    }

    bool generate( std::queue<EmberEvent*>& evQ) 
	{
        bool ret = false;
        switch ( m_phase ) {
          case 0: 
            enQ_init( evQ );
            enQ_my_pe( evQ, &m_my_pe );
            enQ_n_pes( evQ, &m_num_pes );
            break;

          case 1:
            if ( 0 == m_my_pe ) {
                printf("%d:%s: num_pes=%d nelems=%d type=\"%s\"\n",m_my_pe,
                        getMotifName().c_str(), m_num_pes, m_nelems, m_type_name.c_str());
            }
            { 
                size_t buffer_size = 3 * sizeof(long);                   // for pSync
                buffer_size += m_nelems * sizeof(TYPE);              // for source  
                buffer_size += m_nelems * sizeof(TYPE) * m_num_pes;  // for dest
                enQ_malloc( evQ, &m_pSync, buffer_size );
            }
            break;

          case 2:
            m_pSync.at<long>(0) = 0;
            m_pSync.at<long>(1) = 0;
            m_pSync.at<long>(2) = 0;

            m_src = m_pSync.offset<long>(3);

            for ( int i = 0; i < m_nelems; i++ ) { 
				int shift = (sizeof(TYPE) * 8 )/ 2;	
                m_src.at<TYPE>( i ) = ((TYPE) (m_my_pe + 1) << shift) | i + 1;
            }

            m_dest = m_src.offset<TYPE>( m_nelems );
			if ( m_printResults && 0 == m_my_pe ) {
            	printf("%d:%s: pSync=%#" PRIx64 " src=%#" PRIx64 " dest=%#" PRIx64 "\n",m_my_pe, 
                        getMotifName().c_str(), m_pSync.getSimVAddr(), 
                        m_src.getSimVAddr(), m_dest.getSimVAddr());
			}
            bzero( &m_dest.at<TYPE>(0), sizeof(TYPE) * m_num_pes * m_nelems);

            enQ_barrier_all( evQ );
            break;

          case 3:
			if ( 4 == sizeof(TYPE) ) {
            	enQ_fcollect32( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
			} else {
            	enQ_fcollect64( evQ, m_dest, m_src, m_nelems, 0, 0, m_num_pes, m_pSync );
			}
            break;

          case 4:
            for ( int pe = 0; pe < m_num_pes; pe++ ) {
				int shift = (sizeof(TYPE) * 8 )/ 2;
                for ( int i = 0; i < m_nelems; i++ ) {
					if ( m_printResults ) {
                    	printf("%d:%s: pe=%d i=%d %#" PRIx64 "\n",m_my_pe, getMotifName().c_str(), 
                            pe, i, (uint64_t) m_dest.at<TYPE>( pe * m_nelems + i));
					}
                    assert( m_dest.at<TYPE>( pe * m_nelems + i) == ( ((TYPE) (pe + 1) << shift) | i + 1  )  );
                }
            }
			if ( 0 == m_my_pe ) {
            	printf("%d:%s: exit\n",m_my_pe, getMotifName().c_str());
			}
            ret = true;

        }
        ++m_phase;

        return ret;
	}

  private:
	bool m_printResults;
	std::string m_type_name;
    Hermes::MemAddr m_pSync;
    Hermes::MemAddr m_src;
    Hermes::MemAddr m_dest;
    int m_nelems;
    int m_my_pe;
    int m_num_pes;
    int m_phase;
};

}
}

#endif
