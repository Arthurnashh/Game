/**
    @file   directchannel.cpp
    @brief  NetChannel for direct (local) communication.

    Copyright &copy; 2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  21.4.2003
    @date   24.4.2003
*/

#include "El/Network/netpch.hpp"
#include "El/Network/directchannel.hpp"

//------------------------------------------------------------
//  DirectChannel: basic implementation of local communication channel

DirectChannel::DirectChannel ()
{
    LockRegister(lock,"DirectChannel");
    opened = false;
}

#include <Es/Memory/normalNew.hpp>

void* DirectChannel::operator new ( size_t size )
{
    return safeNew(size);
}

void* DirectChannel::operator new ( size_t size, const char *file, int line )
{
    return safeNew(size);
}

void DirectChannel::operator delete ( void *mem )
{
    safeDelete(mem);
}

#include <Es/Memory/debugNew.hpp>

NetStatus DirectChannel::open ( NetPeer *_peer, struct sockaddr_in &distant )
{
    return nsError;
}

NetStatus DirectChannel::connect ( RefD<NetChannel> dist )
{
    if ( dist ) {
        enter();
        opened = true;
        distant = dist;
        leave();
        serial = MSG_SERIAL_NULL + 1;
        return nsOK;
        }
    return nsError;
}

NetStatus DirectChannel::reconnect ( struct sockaddr_in &distant )
{
    return nsOK;
}

unsigned DirectChannel::getLatency ( unsigned *actLat, unsigned *minLat )
{
    if ( actLat ) *actLat = 1;
    if ( minLat ) *minLat = 1;
    return 1;
}

unsigned DirectChannel::getOutputBandWidth ( EnhancedBWInfo *data )
{
    if ( data ) memset(data,0,sizeof(EnhancedBWInfo));
    return 10000000;
}

unsigned64 DirectChannel::getLastMessageArrival () const
{
    return getSystemTime();
}

void DirectChannel::getOutputQueueStatistics ( int &msgs, int &bytes, int &vimMsgs, int &vimBytes )
{
    msgs = bytes = vimMsgs = vimBytes = 0;
}

void DirectChannel::getDistantAddress ( struct sockaddr_in &distant ) const
{
    distant.sin_addr.s_addr = INADDR_LOOPBACK;
}

void DirectChannel::getLocalAddress ( struct sockaddr_in &local ) const
{
    local.sin_addr.s_addr = INADDR_LOOPBACK;
}


void DirectChannel::processData ( MsgHeader *hdr, const struct sockaddr_in &distant )
{
    Assert( hdr );
    Assert( hdr->length );
    enter();
    RefD<NetMessage> msg = NetMessagePool::pool()->newMessage(hdr->length-MSG_HEADER_LEN,this);
    if ( !msg ) {
        leave();
        return;
        }
    msg->setMessage(hdr);
#ifdef MSG_ID
    msg->id = msg->header->id;
#endif
    const unsigned16 flags = hdr->flags;
        // determine message's processing routine:
    msg->msgProcessRoutine = processRoutine;   // default processing
    msg->dta = data;
    int i;
    for ( i = 0; i < subsets.Size(); i++ )
        if ( (flags & subsets[i].andFlag) == subsets[i].eqFlag ) {
            msg->msgProcessRoutine = subsets[i].processRoutine;
            msg->dta = subsets[i].data;
            break;
            }
    msg->distant = distant;
    msg->nextEvent = msg->status = nsInputReceived;
    leave();

        // process message attributes: drop DUMMY message
    if ( flags & MSG_DUMMY_FLAG ) return;

        // ... and finally call user call-back routine:
    if ( msg->msgProcessRoutine ) msg->nextEvent = (*msg->msgProcessRoutine)(msg,nsInputReceived,msg->dta);
}

void DirectChannel::sendRaw( const sockaddr_in &ia, const void *data, int size, int sizeEncrypted )
{
  Fail("sendRaw needs UDP, not supported on DirectChannel");
}

void DirectChannel::dispatchMessage ( NetMessage *msg, bool urgent )
{
    if ( !opened ) return;
    Assert( msg );
    Assert( msg->channel == this );
    if ( msg->getLength() > maxMessageData() ) 
    {
      if ( msg->getFlags() & MSG_VOICE_FLAG ) 
        GDebugVoNBank.SetFeature(DebugVoNBank::VoNFeature_VoNPacketSizeOverflow);
      else
        GDebugVoNBank.SetFeature(DebugVoNBank::VoNFeature_PacketSizeOverflow);
      return; // but the message is not sent as packet, there is no need to cancel it!?
    }

        // send the data:
    RefD<NetChannel> _distant = distant;
    if ( _distant ) {
            // prepare some message header data:
        msg->header->serial = serial++;
        msg->sendTimeout = 0;
        msg->firstTime = msg->refTime = getSystemTime();
        msg->next = NULL;
        msg->status = nsOutputPending;
        msg->header->ackOrigin = MSG_SERIAL_NULL;
        msg->header->ackBitmask = 0;
        
		  // crc is calculated from CRC field as well - we need to initialize it
          msg->header->crc = 0;
          msg->header->crc = crc32(0,(const unsigned8*)msg->header,msg->header->length);
        
		// virtual network address:
        struct sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_port = 0;
        local.sin_addr.s_addr = INADDR_LOOPBACK;
            // sent the data using 'processData' routine:
        _distant->processData(msg->header,local);
        _distant = NULL;
        }

        // post-processing:
    if ( msg->msgProcessRoutine &&
         msg->nextEvent == nsOutputSent )
        msg->nextEvent = (*msg->msgProcessRoutine)(msg,nsOutputSent,msg->dta);
    if ( msg->msgProcessRoutine &&
         msg->nextEvent == nsOutputAck )
        msg->nextEvent = (*msg->msgProcessRoutine)(msg,nsOutputAck,msg->dta);
}

NetMessage *DirectChannel::getLastVIM ( bool urgent )
{
    return NULL;
}

void DirectChannel::nextDispatcherStatus ( DispatcherStatus *data )
{
}

bool DirectChannel::getPreparedMessage ( void *data )
{
    prepared = NULL;
    return false;
}

unsigned64 DirectChannel::preSend ( unsigned64 bunchStart )
{
    return getSystemTime();
}

void DirectChannel::postSend ()
{
}

unsigned64 DirectChannel::getMessageTime ( MsgSerial ser )
{
    return 0;
}

void DirectChannel::cancelAllMessages ()
{
}

void DirectChannel::checkConnectivity ( unsigned64 now )
{
}

bool DirectChannel::dropped ()
{
    return false;
}

void DirectChannel::tick ()
{
}

void DirectChannel::close ()
{
    if ( !opened ) {
        distant = NULL;
        return;
        }
    enter();
    opened = false;
    if ( distant ) {
        distant->close();
        distant = NULL;
        }
    leave();
}

DirectChannel::~DirectChannel ()
{
    close();
}
