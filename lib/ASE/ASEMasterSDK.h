int ASEMaster_init(int createsocket);

/* Initializes the master reporting functions. Does a DNS lookup, */
/* call only when net already up and running, preferably from an  */
/* asynchronous thread to avoid stalls.                           */
/*                                                                */
/* createsocket = flag indicating whether we should create a new  */
/*                socket for master packets (!=0) or if we can    */
/*                use one provided by the game (==0)              */
/*                                                                */
/* returns non-zero on success                                    */

int ASEMaster_heartbeat(SOCKET s, int port, char *gamename);

/* Checks if it's time to send another heartbeat to the ASE       */
/* master server. Should be called at least once every few        */
/* minutes, can be called more often, for example every frame.    */
/*                                                                */
/* s = UDP socket to use when sending packets (if createsocket    */
/*     was 0 when calling init, otherwise ignored)                */
/*                                                                */
/* port = query port for the server                               */
/*                                                                */
/* gamename = unique identifier for the game                      */
/*                                                                */
/* return value:  >0 == heartbeat was sent                        */
/*               ==0 == too soon to send another heartbeat        */
/*                <0 == sendto error code                         */
/*              -999 == not initialized                           */

