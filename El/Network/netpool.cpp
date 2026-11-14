/*
    @file   netpool.cpp
    @brief  Global network pool routines.
    
    Default implementation is for client computers.
    Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  27.11.2001
    @date   2.4.2003
*/

#include "El/Network/netpch.hpp"
#include "El/Network/netpeer.hpp"
#include "El/Network/netchannel.hpp"

//------------------------------------------------------------
//  NetPool class used on client computers:

NetPool::NetPool ( PeerChannelFactory *fact, NetNegotiator *neg, BitMask *ports )
{
    LockRegister(lock,"NetPool");
#ifdef NET_LOG_DESTRUCTOR
    NetLog("NetPool[%08x]: neg=%x, ports=%d",
           (unsigned)this,(unsigned)neg,ports?ports->card():0);
#endif
    factory = fact;
    if ( ports )
        localPorts = new BitMask(*ports);
    if ( neg ) {                            // initial network negotiation
        neg->negotiate(this);               // creates the first NetChannel (including its control-channel),
                                            // establishes connection to neighbour computers (server[s] [and clients])
        }
}

BitMask *NetPool::getLocalPorts () const
{
    return localPorts;
}

PeerChannelFactory *NetPool::getFactory () const
{
    return factory;
}

NetPeer *NetPool::createPeer ( BitMask *tryPorts, bool useVDP, RawMessageCallback callback )
    // creates a new net-peer
{
    if ( factory.IsNull() ) return NULL;
        // tryPorts has higher priority than localPorts
    NetPeer *newPeer = factory->createPeer(this,tryPorts,useVDP,callback);
    if ( !newPeer ) return NULL;
    peers.put(newPeer);
    return newPeer;
}

NetPeer *NetPool::findPeer ( unsigned16 localPort )
    // finds an existing net-peer
{
    RefD<NetPeer> result;
    peers.get(localPort,result);
    return result;
}

void NetPool::deletePeer ( NetPeer *peer )
    // deletes the given net-peer
{
    if ( !peer ) return;
    peer->close();
    peers.removeValue(peer);
}

NetChannel *NetPool::createChannel ( struct sockaddr_in &distant, NetPeer *peer )
{
    IteratorState it;
    RefD<NetPeer> pee = peer;
    if ( !pee )
        peers.getFirst(it,pee);             // !!! TODO: load-balancing !!!
    if ( !pee || factory.IsNull() )
        return NULL;                        // cannot create a channel..
    NetChannel *ch = factory->createChannel(this,false);
    if ( !ch ) return NULL;
    if ( ch->open(pee,distant) != nsOK ) {
        delete ch;
        return NULL;
        }
    channels.put(ch);
    return ch;
}

NetChannel *NetPool::findChannel ( struct sockaddr_in &distant )
    // finds an existing net-channel
{
    RefD<NetChannel> result;
    channels.get(sockaddrKey(distant),result);
    return result;
}

NetChannel *NetPool::getBroadcastChannel ()
{
    IteratorState it;
    RefD<NetPeer> pe;
    if ( !peers.getFirst(it,pe) ) return NULL;
    return pe->getBroadcastChannel();
}

void NetPool::deleteChannel ( NetChannel *channel )
    // deletes the given net-peer
{
    if ( !channel ) return;
    channel->close();
    channels.removeValue(channel);
}

NetPool::~NetPool ()
{
#ifdef NET_LOG_DESTRUCTOR
    NetLog("~NetPool[%08x]: peers=%u, channels=%u",
           (unsigned)this,peers.card(),channels.card());
#endif
}
