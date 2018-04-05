#include "../include/simulator.h"
#include <stdio.h>
#include <limits.h>
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
struct pkt_with_begin_time{
  pkt_with_begin_time(struct pkt pack,float start):packet(pack),start_time(start){}
  struct pkt packet;
  float start_time;
};


int A_WindowSize;
int A_send_base;
int sequenceA;
float increment;
float EstimateRTT;
float DevRTT;
int largest_confirm_num;
queue<pkt> wait_queue;
vector<pkt_with_begin_time> A_windows;


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
  float start_time = get_sim_time();
  if(A_windows.size() == 0) {
    //printf("starttimer in A_output.\n");
    starttimer(0,increment);
  }
  if(sequenceA - A_send_base < A_WindowSize){
    if(wait_queue.empty()){
      //printf("%d:send %s\n",packet.seqnum,packet.payload);
      tolayer3(0,packet);
      pkt_with_begin_time pwbt(packet,start_time);
      A_windows.push_back(pwbt);
    }
    else{
      while(!wait_queue.empty() && wait_queue.front().seqnum - A_send_base < A_WindowSize){
        struct pkt p = wait_queue.front();
        wait_queue.pop();
        pkt_with_begin_time pwbt(p,start_time);
        A_windows.push_back(pwbt);
        //printf("%d:send %s\n",p.seqnum,p.payload);
        tolayer3(0,p);
      }
      if(sequenceA - A_send_base < A_WindowSize){
        pkt_with_begin_time pwbt(packet,start_time);
        A_windows.push_back(pwbt);
        //printf("%d:send %s\n",packet.seqnum,packet.payload);
        tolayer3(0,packet);
      }
      else{
        wait_queue.push(packet);
      }
    }
  }
  else{
    //printf("buffer: %d\n",packet.seqnum);
    wait_queue.push(packet);
  }
  sequenceA++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  //printf("Windows.size() == %d,Receive ACK:%d but could be false.\n",A_windows.size(),packet.seqnum);
  int ReceiveCheckSum = CalculateCheckSum(packet);
  if(ReceiveCheckSum == packet.checksum){
    /* Remove correspond packet from A_Windows */
    for(vector<pkt_with_begin_time>::iterator iter = A_windows.begin();iter != A_windows.end();++iter){
      if(iter->packet.seqnum == packet.seqnum){
        float SampleRTT = get_sim_time() - iter->start_time;
        EstimateRTT = 0.875 * EstimateRTT + 0.125 * SampleRTT;
        float dev = (EstimateRTT - SampleRTT > 0) ? (EstimateRTT - SampleRTT) : (SampleRTT - EstimateRTT);
        DevRTT = 0.75 * DevRTT + 0.25 * dev;
        increment = EstimateRTT + 4 * DevRTT;
        //printf("increment = %f\n",increment);

        A_windows.erase(iter);
        break;
      }
    }
    largest_confirm_num = max(largest_confirm_num,packet.seqnum);
    //printf("%d:ack,A_send_base:%d\n",packet.seqnum,A_send_base);
    /* If packet.seqnum equals to A_send_base, update A_send_base and send buffer packet */
    if(packet.seqnum == A_send_base){
      stoptimer(0);

      float now = get_sim_time();
      int smallest = INT_MAX;
      int greatest = 0;
      if(A_windows.size() == 0){
        A_send_base = largest_confirm_num + 1;
        greatest = largest_confirm_num;
      }
      else{
        for(vector<pkt_with_begin_time>::iterator iter = A_windows.begin();iter != A_windows.end();++iter){
          smallest = min(smallest,iter->packet.seqnum);
          greatest = max(greatest,iter->packet.seqnum);
        }
        A_send_base = smallest;
        starttimer(0,increment - (now - A_windows[0].start_time)); 
      }

      while(!wait_queue.empty() && greatest - A_send_base + 1 < A_WindowSize){
        //printf("greatest:%d,A_send_base:%d,A_WindowSize:%d\n",greatest,A_send_base,A_windows.size());
        if(A_windows.size() == 0){
          starttimer(0,increment);
        }
        struct pkt p = wait_queue.front();
        wait_queue.pop();
        float start_time = get_sim_time();
        pkt_with_begin_time pwbt(p,start_time);
        A_windows.push_back(pwbt);
        //printf("%d:send %s\n",p.seqnum,p.payload);
        tolayer3(0,p);
        greatest = p.seqnum;
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  //printf("resend,A_windows[0]:%d\n",A_windows[0].packet.seqnum);

  pkt_with_begin_time head = A_windows[0];
  head.start_time = get_sim_time();
  A_windows.push_back(head);
  A_windows.erase(A_windows.begin());
  tolayer3(0,head.packet);

  float now = get_sim_time();
  starttimer(0,increment - (now - A_windows[0].start_time));
  //printf("new start_time:%f\n",increment - (now - A_windows[0].start_time));
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  A_WindowSize = getwinsize();
  largest_confirm_num = 0;
  A_send_base = 1;
  sequenceA = 1;
  increment = 20.0;
  DevRTT = 0.0;
  EstimateRTT = 20.0;
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
    //printf("%d:receive %s,B_receive_base:%d\n",packet.seqnum,packet.payload,B_receive_base);
    struct pkt ack = make_pkt_ack(packet);
    tolayer3(1,ack);
    if(packet.seqnum == B_receive_base){
      packet.acknum = 1;
      B_windows[0] = packet;
      while(!B_windows.empty() && B_windows[0].acknum == 1){
        //printf("*****************%d:tolayer5.**************************************\n",B_windows[0].seqnum);
        tolayer5(1,B_windows[0].payload);
        B_windows.erase(B_windows.begin());
        struct pkt p;
        B_windows.push_back(p);
        B_receive_base++;
      }
      //printf("B_receive_base:%d\n",B_receive_base);
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
