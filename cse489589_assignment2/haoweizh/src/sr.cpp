#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <queue>
#include <vector>
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

/* called from layer 5, passed the data to be sent to other side */

/* data used by A */
int A_WindowSize;
int A_send_base;
int sequenceA;
float increment;
queue<pkt> wait_queue;
vector<pkt> A_windows;

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


void A_output(struct msg message)
{
  struct pkt packet = make_pkt(message,sequenceA);
  sequenceA++;
  if(A_windows.size() == 0) {
    ////printf("starttimer in A_output.\n");
    starttimer(0,increment);
    //printf("in this place.\n");
  }
  if(A_windows.size() < A_WindowSize){
    if(wait_queue.empty()){
      //printf("%d:send %s\n",packet.seqnum,packet.payload);
      tolayer3(0,packet);
      A_windows.push_back(packet);
    }
    else{
      while(A_windows.size() < A_WindowSize && !wait_queue.empty()){
        struct pkt p = wait_queue.front();
        wait_queue.pop();
        A_windows.push_back(p);
        //printf("%d:send %s\n",p.seqnum,p.payload);
        tolayer3(0,p);
      }
      if(A_windows.size() < A_WindowSize){
        A_windows.push_back(packet);
        //printf("%d:send %s\n",packet.seqnum,packet.payload);
        tolayer3(0,packet);
      }
      else{
        wait_queue.push(packet);
      }
    }
  }
  else{
    //printf("buffer\n");
    wait_queue.push(packet);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  //printf("Windows.size() == %d,Receive ACK:%d but could be false.\n",A_windows.size(),packet.seqnum);
  int ReceiveCheckSum = CalculateCheckSum(packet);
  if(ReceiveCheckSum == packet.checksum){
    //printf("%d:ack\n",packet.seqnum);
    if(packet.seqnum == A_send_base){
      A_windows[0].acknum = 1;
      while(!A_windows.empty() && A_windows[0].acknum == 1){
        A_windows.erase(A_windows.begin());
        A_send_base++;
      }

      while(!wait_queue.empty() && A_windows.size() <= A_WindowSize){
        struct pkt p = wait_queue.front();
        wait_queue.pop();
        A_windows.push_back(p);
        //printf("%d:send %s\n",p.seqnum,p.payload);
        tolayer3(0,p);
      }
    }
    else if(packet.seqnum > A_send_base){
      A_windows[packet.seqnum - A_send_base].acknum = 1;
    }

    stoptimer(0);
    if(A_windows.size() != 0)
      starttimer(0,increment);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  //printf("resend,A_windows[0]:%d\n",A_windows[0].seqnum);
  starttimer(0,increment);
  for(int i = 0;i != A_windows.size();++i){
    if(A_windows[i].acknum == 0)
      tolayer3(0,A_windows[i]);
  }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_WindowSize = getwinsize();
  A_send_base = 1;
  sequenceA = 1;
  increment = 100.0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* data used by B */
int B_WindowSize;
int B_receive_base;
vector<pkt> B_windows;


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
    //printf("%d:receive %s\n",packet.seqnum,packet.payload);
    struct pkt ack = make_pkt_ack(packet);
    tolayer3(1,ack);
    if(packet.seqnum == B_receive_base){
      packet.acknum = 1;
      B_windows[0] = packet;
      while(!B_windows.empty() && B_windows[0].acknum == 1){
        tolayer5(1,B_windows[0].payload);
        B_windows.erase(B_windows.begin());
        struct pkt p;
        B_windows.push_back(p);
        B_receive_base++;
      }
    }
    else if(packet.seqnum > B_receive_base && packet.seqnum < B_receive_base + B_WindowSize){
      packet.acknum = 1;
      B_windows[packet.seqnum - B_receive_base] = packet;
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
  B_WindowSize = getwinsize();
  B_receive_base = 1;
  for(int i = 0;i != B_WindowSize;++i){
    struct pkt p;
    B_windows.push_back(p);
  }
}
