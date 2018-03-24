#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* data used by A */
int WindowSize;
int send_base;

/* calculate the checksum of the package  */
int CalculateCheckSum(pkt packet){
  /* calculate the sum of payload's ascii number */
  int pl = 0;
  int index = 0;
  while(index < sizeof(packet.payload)){
    pl += packet.payload[index];
    index++;
  }

  int checksum = 0;
  checksum = packet.seqnum + packet.acknum + pl;
  return checksum;
}


/* make packet */
pkt make_pkt(msg message,int sequence){
  struct pkt packet;
  packet.seqnum = sequence;
  packet.acknum = 0;
  bzero(&packet.payload,sizeof(packet.payload));
  strncpy(packet.payload,message.data,sizeof(packet.payload));
  packet.checksum = CalculateCheckSum(packet);
  return packet;
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

}

/* called when A's timer goes off */
void A_timerinterrupt()
{

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  WindowSize = getwinsize();
  send_base = 1;
}



/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* data used by B */

/* make ack packet */
pkt make_pkt_ack(pkt packet){
  struct pkt ack;
  ack.seqnum = packet.seqnum;
  ack.acknum = 0;
  bzero(&ack.payload,sizeof(ack.payload));
  strcpy(ack.payload,"ack");
  ack.checksum = CalculateCheckSum(ack);
  return ack;
}


/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}
