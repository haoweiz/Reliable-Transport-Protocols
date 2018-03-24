#include "../include/simulator.h"
#include <string.h>
#include <stdio.h>
#include <queue>
using namespace std;

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
float increment;
int sequenceA;
bool CanSend;        // whether ready to send new data
struct msg buffer;
queue<pkt> wait_queue;

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
  if(CanSend){
    if(wait_queue.empty()){
      CanSend = false;
      struct pkt packet = make_pkt(message,sequenceA);
      tolayer3(0, packet); 
      starttimer(0, increment);

      /* buffer packet in order to retransmit */
      bzero(&buffer,sizeof(buffer));
      strncpy(buffer.data,message.data,sizeof(message.data));

      //printf("%d:A send:%s\n",packet.seqnum,packet.payload);
    }
    else{
      struct pkt packet = wait_queue.front();
      wait_queue.pop();
      tolayer3(0,packet);
      CanSend = false;
      starttimer(0, increment);

      bzero(&buffer,sizeof(buffer));
      strncpy(buffer.data,packet.payload,sizeof(packet.payload));
    }
  }
  else{
    //printf("Buffer message:%s\n",message.data);
    struct pkt buffer_pkt = make_pkt(message,sequenceA + wait_queue.size() + 1);
    wait_queue.push(buffer_pkt);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  //printf("Get ack input:sequenceA:%d,packet.seqnum:%d,packet.payload:%s\n",sequenceA,packet.seqnum,packet.payload);
  int ReceiveCheckSum = CalculateCheckSum(packet);
  if(ReceiveCheckSum == packet.checksum){
    if(sequenceA == packet.seqnum){
      //printf("%d:ack\n",packet.seqnum);
      sequenceA++;
      stoptimer(0);
      CanSend = true;
    }
  }
  if(CanSend && !wait_queue.empty()){
    //printf("Send buffer message:%s\n",wait_queue.front().payload);
    struct pkt packet = wait_queue.front();
    wait_queue.pop();
    tolayer3(0,packet);
    CanSend = false;
    starttimer(0, increment);

    bzero(&buffer,sizeof(buffer));
    strncpy(buffer.data,packet.payload,sizeof(packet.payload));
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  //printf("resend:%s\n",buffer.data);
  struct pkt packet = make_pkt(buffer,sequenceA);
  tolayer3(0, packet); 
  starttimer(0, increment);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  increment = 5.0;
  sequenceA = 1;
  CanSend = true;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */


/*data used by B */
int sequenceB;

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
  int ReceiveCheckSum = CalculateCheckSum(packet);
  if(ReceiveCheckSum == packet.checksum){
    if(packet.seqnum == sequenceB){
      //packet.payload[20] = '\0';
      //printf("%d:B receive:%s\n",packet.seqnum,packet.payload);
      tolayer5(1, packet.payload);
      struct pkt ack = make_pkt_ack(packet);
      tolayer3(1, ack);

      sequenceB++;
    }
    else{
      /* Resend ack packet */
      struct pkt ack = make_pkt_ack(packet);
      tolayer3(1, ack);
    }
  }
  else{
    /* No NACK! */
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  sequenceB = 1;
}
