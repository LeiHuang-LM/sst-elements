// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization.h"

#include "funcSM/gatherv.h"

using namespace SST::Firefly;

GathervFuncSM::GathervFuncSM( 
                    int verboseLevel, Output::output_location_t loc,
                    Info* info, SST::Link*& progressLink, 
                    ProtocolAPI* ctrlMsg, IO::Interface*  io ) :
    FunctionSMInterface(verboseLevel,loc,info),
    m_dataReadyFunctor( IO_Functor(this,&GathervFuncSM::dataReady) ),
    m_toProgressLink( progressLink ),
    m_ctrlMsg( static_cast<CtrlMsg*>(ctrlMsg) ),
    m_io( io )
{
    m_dbg.setPrefix("@t:GathervFuncSM::@p():@l ");
}

void GathervFuncSM::handleEnterEvent( SST::Event *e ) 
{
    if ( m_setPrefix ) {
        char buffer[100];
        snprintf(buffer,100,"@t:%d:%d:GathervFuncSM::@p():@l ",
                    m_info->nodeId(), m_info->worldRank());
        m_dbg.setPrefix(buffer);

        m_setPrefix = false;
    }

    assert( NULL == m_event );
    m_event = static_cast< GathervEnterEvent* >(e);

    m_qqq = new QQQ( 2, m_info->getGroup(m_event->group)->getMyRank(),
                m_info->getGroup(m_event->group)->size(), m_event->root );

    m_dbg.verbose(CALL_INFO,1,0,"group %d, root %d, size %d, rank %d\n",
                    m_event->group, m_event->root, m_qqq->size(),
                    m_qqq->myRank());

    m_dbg.verbose(CALL_INFO,1,0,"parent %d \n",m_qqq->parent());
    for ( unsigned int i = 0; i < m_qqq->numChildren(); i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"child[%d]=%d\n",i,m_qqq->calcChild(i));
    }

    m_sendUpPending = false;
    m_waitUpPending = false;
    m_waitUpState = WaitUpRecv;
    m_sendUpState = SendUpSend;
    m_state = WaitUp;
    m_pending = true;

    m_recvReqV.resize(m_qqq->numChildren());
    m_waitUpSize.resize(m_qqq->numChildren());

    m_toProgressLink->send(0, NULL );
}

void GathervFuncSM::handleProgressEvent( SST::Event *e )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    switch( m_state ) {
    case WaitUp:
        if ( m_qqq->numChildren() ) {
            if ( waitUp() ) {
                break;
            };
        }
        m_pending = false;
        m_state = SendUp;
    case SendUp:
        if ( -1 != m_qqq->parent() ) {
            if ( sendUp() ) {
                break;
            }
        }
        m_dbg.verbose(CALL_INFO,1,0,"leave\n");
        exit( static_cast<SMEnterEvent*>(m_event), 0 );
        delete m_qqq;
        delete m_event;
        m_event = NULL;
    }
}

bool GathervFuncSM::waitUp()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    switch( m_waitUpState ) { 
    case WaitUpRecv:
        if ( ! m_waitUpPending ) {
            for ( unsigned int i = 0; i < m_qqq->numChildren(); i++ ) { 
                m_dbg.verbose(CALL_INFO,1,0,"post recv for child %d[%d]\n", 
                                 i, m_qqq->calcChild(i) );
                m_ctrlMsg->recv( &m_waitUpSize[i], sizeof(m_waitUpSize[i]),
                        m_qqq->calcChild(i),
                        GathervTag, m_event->group, &m_recvReqV[i] );
            }    
            m_waitUpPending = true;
            m_toProgressLink->send(0, NULL );
            m_count = 0;
            return true;
        } else {
            bool ret = m_ctrlMsg->test( &m_recvReqV[m_count] );
            if ( ret ) {
                m_dbg.verbose(CALL_INFO,1,0,"got message from child %d[%d]\n", 
                                 m_count, m_qqq->calcChild(m_count) );
                ++m_count;
            } else {
                m_dbg.verbose(CALL_INFO,1,0,"call setDataReadyFunc\n");
                m_io->setDataReadyFunc( &m_dataReadyFunctor );
                return true;
            }
        }
        if ( m_count < m_qqq->numChildren() ) {
            m_toProgressLink->send(0, NULL );
            return true;
        } else {
            m_dbg.verbose(CALL_INFO,1,0,"all children have checked in\n");
            int len = m_info->sizeofDataType( m_event->sendtype ) * 
                                                        m_event->sendcnt;
            for ( unsigned int i = 0; i < m_qqq->numChildren(); i++ ) {
                
                m_dbg.verbose(CALL_INFO,1,0,"child %d[%d] has %d bytes\n",
                               i, m_qqq->calcChild(i), m_waitUpSize[i] );
                len += m_waitUpSize[i];
            }
            m_recvBuf.resize( len );
            len = m_info->sizeofDataType( m_event->sendtype ) *
                                                        m_event->sendcnt;
            for ( unsigned int i = 0; i < m_qqq->numChildren(); i++ ) {
                m_dbg.verbose(CALL_INFO,1,0,"post recv for child %d[%d]\n",
                               i, m_qqq->calcChild(i) );
                m_ctrlMsg->recv( &m_recvBuf[len], m_waitUpSize[i],
                        m_qqq->calcChild(i),
                        GathervTag, m_event->group, &m_recvReqV[i] );
                len += m_waitUpSize[i];
            }
        }

        m_waitUpState = WaitUpSend;
        m_waitUpPending = false;
        m_count = 0;
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitUpSend\n");
            
    case WaitUpSend:
        if ( ! m_waitUpPending ) {
            m_dbg.verbose(CALL_INFO,1,0,"send message to child %d[%d]\n",
                                    m_count, m_qqq->calcChild( m_count ) );
            m_ctrlMsg->send( &m_intBuf, sizeof(m_intBuf), 
                            m_qqq->calcChild( m_count ),
                            GathervTag, m_event->group, &m_sendReq );
            m_toProgressLink->send(0, NULL );
            m_waitUpPending = true;
            return true;
        } else {
            bool ret = m_ctrlMsg->test( &m_sendReq );
            if ( ret ) {
                m_dbg.verbose(CALL_INFO,1,0,"send completed %d[%d]\n",
                                    m_count, m_qqq->calcChild( m_count ) );
                ++m_count;
            } else {
                m_io->setDataReadyFunc( &m_dataReadyFunctor );
                return true;
            }
            if ( m_count < m_qqq->numChildren() ) {
                m_toProgressLink->send(0, NULL );
                m_waitUpPending = false;
                return true;
            }
        } 
        m_waitUpState = WaitUpRecvBody;
        m_waitUpPending = false;
        m_count = 0;
        m_dbg.verbose(CALL_INFO,1,0,"switch to WaitUpRecvBody\n");

    case WaitUpRecvBody:
        if ( m_count < m_qqq->numChildren() ) {
            bool ret = m_ctrlMsg->test( &m_recvReqV[m_count] );
            if ( ret ) {
                m_dbg.verbose(CALL_INFO,1,0,"got message from %d\n", m_count);
                ++m_count;
            } else {
                m_io->setDataReadyFunc( &m_dataReadyFunctor );
                return true;
            }
            if ( m_count < m_qqq->numChildren() ) {
                m_toProgressLink->send(0, NULL );
                return true;
            } else {
                m_dbg.verbose(CALL_INFO,1,0,"got all messages\n");
                if ( -1 == m_qqq->parent() ) {

                    doRoot( );
                }
            }
        }
        m_waitUpState = WaitUpRecv;
        m_waitUpPending = false;
    }
    return false;
}

void GathervFuncSM::doRoot()
{
    std::vector<int> map = m_qqq->getMap();

    int len = m_info->sizeofDataType( m_event->sendtype ) * m_event->sendcnt;
    m_dbg.verbose(CALL_INFO,1,0,"I'm root %lu\n", m_recvBuf.size());

    memcpy( &m_recvBuf[0], m_event->sendbuf, len ); 

#if 0
    for ( unsigned int i = 0; i < m_recvBuf.size()/4; i++ ) {
        printf("%#03x\n", ((unsigned int*) &m_recvBuf[0])[i]);
    }
#endif
    
    int offset = 0;
    for ( unsigned int  i = 0; i < map.size(); i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"%d -> %d\n", map[i], i);
        
        int rank = map[i];

        if ( m_event->recvcntPtr ) {
            int recvcnt = ((int*)m_event->recvcntPtr)[rank];
            int displs = ((int*)m_event->displsPtr)[rank];
        
            m_dbg.verbose(CALL_INFO,1,0,"rank %d, recvcnt %d, displs %d\n",
                                    rank, recvcnt, displs);

            int len = m_info->sizeofDataType( m_event->recvtype ) * recvcnt;
            memcpy( (unsigned char*) m_event->recvbuf + displs, 
                            &m_recvBuf[offset], len);
        } else {
            int len = m_info->sizeofDataType( m_event->recvtype ) * 
                                    m_event->recvcnt;
            memcpy( (unsigned char*) m_event->recvbuf + len * rank, 
                            &m_recvBuf[offset], len);
        }

        offset += len;
    }

#if 0
    for ( unsigned int i = 0; i < m_recvBuf.size()/4; i++ ) {
        printf("%#03x\n", ((unsigned int*) m_event->recvbuf)[i]);
    }
#endif
}

bool GathervFuncSM::sendUp()
{ 
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    switch ( m_sendUpState ) {
    case SendUpSend:
        if ( ! m_sendUpPending ) {
            size_t len = m_info->sizeofDataType( m_event->sendtype ) *
                    m_event->sendcnt;
            if ( 0 == m_qqq->numChildren() ) {
                m_recvBuf.resize( len );
            }
            memcpy( &m_recvBuf[0], m_event->sendbuf, len ); 
            m_intBuf = m_recvBuf.size();
            m_dbg.verbose(CALL_INFO,1,0,"send size %d message to %d\n", 
                                m_intBuf, m_qqq->parent());
            m_ctrlMsg->send( &m_intBuf, sizeof(m_intBuf), 
                            m_qqq->parent(),
                            GathervTag, m_event->group, &m_sendReq );
            
            m_sendUpPending = true;
            m_toProgressLink->send(0, NULL );
            return true;
        } else {
            bool ret = m_ctrlMsg->test( &m_sendReq );
            if ( ret ) {
                m_dbg.verbose(CALL_INFO,1,0,"send completed\n");
            } else {
                m_io->setDataReadyFunc( &m_dataReadyFunctor );
                return true;
            }
        }

        m_sendUpPending = false;
        m_sendUpState = SendUpWait;

        m_dbg.verbose(CALL_INFO,1,0,"move to SendUpWait\n");

    case SendUpWait:
        if ( ! m_sendUpPending ) {
            m_dbg.verbose(CALL_INFO,1,0,"post receive for parent %d\n", 
                                        m_qqq->parent());
            m_ctrlMsg->recv( &m_intBuf, sizeof(m_intBuf),
                        m_qqq->parent(),
                        GathervTag, m_event->group, &m_recvReq );
            m_sendUpPending = true;
            m_toProgressLink->send(0, NULL );
            return true;
        } else {
            bool ret = m_ctrlMsg->test( &m_recvReq );
            if ( ret ) {
                m_dbg.verbose(CALL_INFO,1,0,"got message from parent %d\n", 
                                        m_qqq->parent());
            } else {
                m_dbg.verbose(CALL_INFO,1,0,"setDataReadyFunc\n");
                m_io->setDataReadyFunc( &m_dataReadyFunctor );
                return true;
            }
        }
        m_sendUpPending = false;
        m_sendUpState = SendUpSendBody;
        m_dbg.verbose(CALL_INFO,1,0,"move to SendUpSendBody\n");

    case SendUpSendBody:

        if ( ! m_sendUpPending ) {
            m_dbg.verbose(CALL_INFO,1,0,"sending body to parent %d\n", 
                                            m_qqq->parent());
            m_ctrlMsg->send( &m_recvBuf[0], m_recvBuf.size(), m_qqq->parent(),
                            GathervTag, m_event->group, &m_sendReq );
#if 0
            for ( unsigned int i = 0; i < m_recvBuf.size(); i++ ) {
                printf("%#03x\n", m_recvBuf[i]);
            }
#endif
            m_sendUpPending = true;
            m_toProgressLink->send(0, NULL );
            return true;
        }  else {
            bool ret = m_ctrlMsg->test( &m_sendReq );
            if ( ret ) {
                m_dbg.verbose(CALL_INFO,1,0,"sent body to parent %d\n", 
                                            m_qqq->parent());
            } else {
                m_io->setDataReadyFunc( &m_dataReadyFunctor );
                return true;
            }
        }
        m_sendUpPending = false;
        m_sendUpState = SendUpSend;
    }
    return false;
}

void GathervFuncSM::dataReady( IO::NodeId src )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    assert( m_event );
    m_io->setDataReadyFunc( NULL );
    m_toProgressLink->send(0, NULL );
}

