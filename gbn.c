#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BIDIRECTIONAL 0

/********* SECTION 0: GLOBAL DATA STRUCTURES*********/
/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */

struct msg
{
   char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
};

int starttimer(int AorB, float);
int stoptimer(int AorB);
void tolayer3(int, struct pkt);
void tolayer5(int, char *);

void ComputeChecksum(struct pkt *);
int CheckCorrupted(struct pkt);
void A_output(struct msg);
void A_input(struct pkt);
void B_input(struct pkt);
void B_init(void);
void A_init(void);

float currenttime(void);
void printevlist(void);
void generate_next_arrival(void);
void init(void);
void A_timerinterrupt(void);

/********* SECTION I: GLOBAL VARIABLES*********/
/* the following global variables will be used by the routines to be implemented.
   you may define new global variables if necessary. However, you should reduce
   the number of new global variables to the minimum. Excessive new global variables
   will result in point deduction.
*/

#define MAXBUFSIZE 5000

#define RTT 15.0

#define NOTUSED 0

#define TRUE 1
#define FALSE 0

#define A 0
#define B 1

int expectedseqnum;  /* expected sequence number at receiver side */
float currenttime(); /* get the current time */

int base;       /* the head of sender window */
int nextseqnum; /* next sequence number to use in sender side */
int WINDOWSIZE = 8;

struct msg buffer[MAXBUFSIZE]; /* sender message buffer */

struct pkt *winbuf;    /* window packets buffer */
int winfront, winrear; /* front and rear points of window buffer */
int pktnum;            /* the # of packets in window buffer, i.e., the # of packets to be resent when timeout */

int buffront, bufrear; /* front and rear pointers of buffer */
int msgnum;            /* # of messages in buffer */
int totalmsg = -1;

int packet_lost = 0;
int packet_corrupt = 0;
int packet_sent = 0;
int packet_correct = 0;
int packet_resent = 0;
int packet_timeout = 0;

/********* SECTION II: FUNCTIONS TO BE COMPLETED BY STUDENTS*********/
/* layer 5: application layer which calls functions of layer 4 to send messages; */
/* layer 4: transport layer (where your program is implemented); */
/* layer 3: networking layer which calls functions of layer 4 to deliver the messages that have arrived. */

/* compute the checksum of the packet to be sent from the sender */
void ComputeChecksum(packet) struct pkt *packet;
{
   int cs = 0;
   cs = packet->acknum + packet->seqnum; // include payload, sequence number, and ACK number
   for (int i = 0; i < 20; i++)
   {
      cs += packet->payload[i];
   }
   cs = 0 - cs;
   packet->checksum = cs;
}

/* check the checksum of the packet received, return TRUE if packet is corrupted, FALSE otherwise */
int CheckCorrupted(packet)
struct pkt packet;
{
   int cs = packet.seqnum + packet.acknum;
   for (int i = 0; i < 20; i++)
   {
      cs = cs + packet.payload[i];
   }
   if ((packet.checksum + cs) == 0)
   {

      return FALSE;
   }
   else
   {
      return TRUE;
   }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message) struct msg message;
{
   struct pkt tempPacket; // make a temp pkt to store the folloing data

   if (nextseqnum < (base + WINDOWSIZE)) // as the pdf said that check if ‘nextseqnum’ falls within the sender window
   {
      tempPacket.acknum = NOTUSED; // As this is a data packet, set ACK number to ‘NOTUSED’
      tempPacket.seqnum = nextseqnum;
      strncpy(tempPacket.payload, message.data, 20); // copy the data to the temp pkt
      ComputeChecksum(&tempPacket);
      tolayer3(A, tempPacket); // send data
      printf("[%.1f] A: send packet [%d] base [%d]\n", currenttime(), nextseqnum, base);
      packet_sent++;
      winbuf[winrear] = tempPacket; // Copy the packet to the buffer defined by winbuf
      pktnum++;
      winrear = (winrear + 1) % WINDOWSIZE;
      if (pktnum == 1) // If it is the first packet in window, start the timer.
      {
         starttimer(A, RTT); // The timeout of the timer should be set to RTT
      }
      nextseqnum++;
   }
   else
   {
      if (msgnum == MAXBUFSIZE) // ‘nextseqnum’ does not fall within the sender window
      {
         printf("buffer fulled\n");
         exit(1);
      }
      else
      { // buffer the message if the sender message buffer is not full, exit otherwise
         printf("[%.1f] A: buffer packet [%d] base [%d]\n", currenttime(), nextseqnum + msgnum, base);
         buffer[bufrear] = message;
         bufrear = (bufrear + 1) % MAXBUFSIZE;
         msgnum++;
      }
   }
}

void A_input(packet) struct pkt packet;
{
   struct pkt tempPacket;
   if (CheckCorrupted(packet) == FALSE && packet.acknum != -1) // check if the packet is corrupted
   {

      printf("[%.1f] A: ACK [%d] received\n", currenttime(), packet.acknum);
      packet_correct++;
      winfront = (winfront + 1) % WINDOWSIZE;
      pktnum--;
      if (packet.acknum >= base)
      {
         stoptimer(A);
      }

      if (packet.acknum >= base || packet.acknum == base + 2)
      {
         base = packet.acknum + 1;
      }

      while ((msgnum != 0) && (nextseqnum < (base + WINDOWSIZE)))
      {
         tempPacket.seqnum = nextseqnum;
         tempPacket.acknum = NOTUSED;
         strncpy(tempPacket.payload, buffer[buffront].data, 20);
         ComputeChecksum(&tempPacket);
         buffront = (buffront + 1) % MAXBUFSIZE;
         tolayer3(A, tempPacket);
         printf("[%.1f] A: send packet [%d] base [%d]\n", currenttime(), tempPacket.seqnum, base);
         packet_sent++;
         winbuf[winrear] = tempPacket;
         winrear = (winrear + 1) % WINDOWSIZE;
         pktnum++;
         starttimer(A, RTT);
         nextseqnum++;
         msgnum--;
      }
   }
   else
   {

      if (CheckCorrupted(packet) == TRUE)
      {
         printf("[%.1f] A: ACK corrupted\n", currenttime());
         packet_corrupt++;
      }
      else
      {
         printf("[%.1f] A: ACK [-1] received\n", currenttime());
      }
   }
}

void A_timerinterrupt() // when A's timer goes off. Restart the timer and resend all packets not acked.
{
   printf("[%.1f] time out, resend packets [", currenttime());
   for (int i = 0; i < pktnum; i++)
   {
      printf("%d ", (winfront + i) % WINDOWSIZE);
      tolayer3(A, winbuf[(winfront + i) % WINDOWSIZE]);
      packet_resent++;
      packet_sent++;
   }
   starttimer(A, RTT);
   printf("]\n");
}

void B_input(packet) struct pkt packet;
{
   struct pkt ackpacket;

   if ((CheckCorrupted(packet) == FALSE) && (packet.seqnum == expectedseqnum)) // If the packet is not corrupted and is in order, create an ACK packet (setting seqnum to NOTUSED)
   {
      printf("[%.1f] B: packet [%d] received, send ACK [%d]\n", currenttime(), packet.seqnum, expectedseqnum);
      ackpacket.seqnum = NOTUSED;
      ackpacket.acknum = expectedseqnum;
      strncpy(ackpacket.payload, packet.payload, 20);
      ComputeChecksum(&ackpacket);
      expectedseqnum++;
      tolayer3(B, ackpacket);
      tolayer5(B, packet.payload);
   }
   else
   {
      if (CheckCorrupted(packet) == TRUE)
      {
         printf("[%.1f] B: packet corrupted, send ACK [-1]\n", currenttime());
         packet_lost++;
      }
      else
      {
         printf("[%.1f] B: packet [%d] unexpected, send ACK [-1]\n", currenttime(), packet.seqnum);
      }
      ackpacket.seqnum = NOTUSED;
      ackpacket.acknum = -1;
      strncpy(ackpacket.payload, packet.payload, 20);
      ComputeChecksum(&ackpacket);
      tolayer3(B, ackpacket);
   }
}

void print_info()
{
   printf("***********************************************************************************************************\n");
   printf("                                        Packet Transmission Summary                                           \n");
   printf("***********************************************************************************************************\n");

   printf("From Sender to Receiever: \n");
   printf("total sent pkts:  %10d \n", packet_sent);
   printf("total correct pkts:  %7d \n", packet_correct);
   printf("total resent pkts:  %8d \n", packet_resent);
   printf("total lost pkts:  %10d \n", packet_lost);
   printf("total corrupted pkts:  %5d \n", packet_corrupt);
   printf("Goodput of the GBN: %.3f bps \n", (32 * 10000 * 4) / currenttime());
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
   base = 0;
   nextseqnum = 0;
   buffront = 0;
   bufrear = 0;
   msgnum = 0;
   winfront = 0;
   winrear = 0;
   pktnum = 0;
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
   expectedseqnum = 0;
};

/***************** SECTION III: NETWORK EMULATION CODE ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again,
you defeinitely should not have to modify
******************************************************************/

struct event
{
   float evtime;       /* event time */
   int evtype;         /* event type code */
   int eventity;       /* entity where event occurs */
   struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
};
struct event *evlist = NULL; /* the event list */
void insertevent(struct event *);
/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = -1;  /* for my debugging */
int nsim = 0;    /* number of messages from 5 to 4 so far */
int nsimmax = 0; /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

char pattern[40]; /*channel pattern string*/
int npttns = 0;
int cp = -1; /*current pattern*/
char pttnchars[3] = {'o', '-', 'x'};
enum pttns
{
   OK = 0,
   CORRUPT,
   LOST
};

int main(void)
{
   struct event *eventptr;
   struct msg msg2give;
   struct pkt pkt2give;

   int i, j;

   init();
   A_init();
   B_init();

   while (1)
   {
      eventptr = evlist; /* get next event to simulate */
      if (eventptr == NULL)
         goto terminate;

      evlist = evlist->next; /* remove this event from event list */
      if (evlist != NULL)
         evlist->prev = NULL;

      if (TRACE >= 2)
      {
         printf("\nEVENT time: %f,", eventptr->evtime);
         printf("  type: %d", eventptr->evtype);
         if (eventptr->evtype == 0)
            printf(", timerinterrupt  ");
         else if (eventptr->evtype == 1)
            printf(", fromlayer5 ");
         else
            printf(", fromlayer3 ");
         printf(" entity: %d\n", eventptr->eventity);
      }
      time = eventptr->evtime; /* update time to next event time */
      // if (nsim==nsimmax)
      //	break;                        /* all done with simulation */

      if (eventptr->evtype == FROM_LAYER5)
      {
         generate_next_arrival();
         j = nsim % 26;
         for (i = 0; i < 20; i++)
            msg2give.data[i] = 97 + j;

         if (TRACE > 2)
         {
            printf("          MAINLOOP: data given to student: ");
            for (i = 0; i < 20; i++)
               printf("%c", msg2give.data[i]);
            printf("\n");
         }

         if (eventptr->eventity == A)
            A_output(msg2give);
         else
         {
         }
      }
      else if (eventptr->evtype == FROM_LAYER3)
      {
         pkt2give.seqnum = eventptr->pktptr->seqnum;
         pkt2give.acknum = eventptr->pktptr->acknum;
         pkt2give.checksum = eventptr->pktptr->checksum;
         for (i = 0; i < 20; i++)
            pkt2give.payload[i] = eventptr->pktptr->payload[i];

         if (eventptr->eventity == A) /* deliver packet by calling */
            A_input(pkt2give);        /* appropriate entity */
         else
            B_input(pkt2give);

         free(eventptr->pktptr); /* free the memory for packet */
      }
      else if (eventptr->evtype == TIMER_INTERRUPT)
      {
         if (eventptr->eventity == A)
            A_timerinterrupt();
         else
         {
         }
         // B_timerinterrupt();
      }
      else
      {
         printf("INTERNAL PANIC: unknown event type \n");
      }
      free(eventptr);
   }

terminate:
   printf("Simulator terminated.\n");
   print_info();
}

void init() /* initialize the simulator */
{
   printf("***********************************************************************************************************\n");
   printf("         IERG3310 lab1: Reliable Data Transfer Protocol-GBN, implemented by 1155145396           \n");
   printf("***********************************************************************************************************\n");

   float jimsrand();
   // printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: \n");
   // fscanf(fp,"%d",&nsimmax);
   scanf("%d", &nsimmax);
   printf("Enter time between messages from sender's layer5 [ > 0.0]:\n");
   // fscanf(fp,"%f",&lambda);
   scanf("%f", &lambda);
   printf("Enter channel pattern string\n");
   // fscanf(fp, "%s",pattern);
   scanf("%s", pattern);
   npttns = strlen(pattern);
   // printf("%d patterns: %s\n",npttns,pattern);

   printf("Enter sender's window size\n");
   scanf("%d", &WINDOWSIZE);

   winbuf = (struct pkt *)malloc(sizeof(struct pkt) * WINDOWSIZE);

   // printf("Enter TRACE:\n");
   // fscanf(fp,"%d",&TRACE);
   // scanf("%d",&TRACE);

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time = 0.0;              /* initialize time to 0.0 */
   generate_next_arrival(); /* initialize event list */

   printf("***********************************************************************************************************\n");
   printf("                                        Packet Transmission Log                                            \n");
   printf("***********************************************************************************************************\n");
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
   double x, log(), ceil();
   struct event *evptr;
   // char *malloc();

   if (nsim >= nsimmax)
      return;

   if (TRACE > 2)
      printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda;

   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime = time + x;
   evptr->evtype = FROM_LAYER5;
   if (BIDIRECTIONAL)
      evptr->eventity = B;
   else
      evptr->eventity = A;
   insertevent(evptr);
   nsim++;
}

void insertevent(p) struct event *p;
{
   struct event *q, *qold;

   if (TRACE > 2)
   {
      printf("            INSERTEVENT: time is %lf\n", time);
      printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
   }
   q = evlist; /* q points to front of list in which p struct inserted */
   if (q == NULL)
   { /* list is empty */
      evlist = p;
      p->next = NULL;
      p->prev = NULL;
   }
   else
   {
      for (qold = q; q != NULL && p->evtime >= q->evtime; q = q->next)
         qold = q;
      if (q == NULL)
      { /* end of list */
         qold->next = p;
         p->prev = qold;
         p->next = NULL;
      }
      else if (q == evlist)
      { /* front of list */
         p->next = evlist;
         p->prev = NULL;
         p->next->prev = p;
         evlist = p;
      }
      else
      { /* middle of list */
         p->next = q;
         p->prev = q->prev;
         q->prev->next = p;
         q->prev = p;
      }
   }
}

void printevlist()
{
   struct event *q;

   printf("--------------\nEvent List Follows:\n");
   for (q = evlist; q != NULL; q = q->next)
   {
      printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
   }
   printf("--------------\n");
}

/********************** SECTION IV: Student-callable ROUTINES ***********************/

/* get the current time of the system*/
float currenttime()
{
   return time;
}

/* called by students routine to cancel a previously-started timer */
int stoptimer(AorB)
int AorB; /* A or B is trying to stop timer */
{
   struct event *q;

   if (TRACE > 2)
      printf("          STOP TIMER: stopping timer at %f\n", time);
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q = evlist; q != NULL; q = q->next)
      if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
      {
         /* remove this event */
         if (q->next == NULL && q->prev == NULL)
            evlist = NULL;         /* remove first and only event on list */
         else if (q->next == NULL) /* end of list - there is one in front */
            q->prev->next = NULL;
         else if (q == evlist)
         { /* front of list - there must be event after */
            q->next->prev = NULL;
            evlist = q->next;
         }
         else
         { /* middle of list */
            q->next->prev = q->prev;
            q->prev->next = q->next;
         }
         free(q);
         return 0;
      }
   printf("Warning: unable to cancel your timer. It wasn't running.\n");
   return 0;
};

int starttimer(int AorB, float increment)
{

   struct event *q;
   struct event *evptr;
   // char *malloc();

   if (TRACE > 2)
      printf("          START TIMER: starting timer at %f\n", time);
   /* be nice: check to see if timer is already started, if so, then  warn */
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q = evlist; q != NULL; q = q->next)
      if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
      {
         printf("Warning: attempt to start a timer that is already started\n");
         return 0;
      }

   /* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime = time + increment;
   evptr->evtype = TIMER_INTERRUPT;

   evptr->eventity = AorB;
   insertevent(evptr);
   return 0;
}

/************************** TOLAYER3 ***************/
void tolayer3(AorB, packet) int AorB; /* A or B is trying to stop timer */
struct pkt packet;
{
   struct pkt *mypktptr;
   struct event *evptr;
   // char *malloc();
   float jimsrand();
   int i;

   cp++;

   ntolayer3++;

   /* simulate losses: */
   if (pattern[cp % npttns] == pttnchars[LOST])
   {
      nlost++;
      if (TRACE > 0)
         printf("          TOLAYER3: packet being lost\n");
      return;
   }

   /* make a copy of the packet student just gave me since he/she may decide */
   /* to do something with the packet after we return back to him/her */
   mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
   mypktptr->seqnum = packet.seqnum;
   mypktptr->acknum = packet.acknum;
   mypktptr->checksum = packet.checksum;
   for (i = 0; i < 20; i++)
      mypktptr->payload[i] = packet.payload[i];
   if (TRACE > 2)
   {
      printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
             mypktptr->acknum, mypktptr->checksum);
      for (i = 0; i < 20; i++)
         printf("%c", mypktptr->payload[i]);
      printf("\n");
   }

   /* create future event for arrival of packet at the other side */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtype = FROM_LAYER3;        /* packet will pop out from layer3 */
   evptr->eventity = (AorB + 1) % 2;   /* event occurs at other entity */
   evptr->pktptr = mypktptr;           /* save ptr to my copy of packet */
   evptr->evtime = time + RTT / 2 - 1; /* hard code the delay on channel */

   /* simulate corruption: */
   if (pattern[cp % npttns] == pttnchars[CORRUPT])
   {
      ncorrupt++;
      mypktptr->payload[0] = 'Z'; /* corrupt payload */
      mypktptr->seqnum = 999999;
      mypktptr->acknum = 999999;
      if (TRACE > 0)
         printf("          TOLAYER3: packet being corrupted\n");
   }

   if (TRACE > 2)
      printf("          TOLAYER3: scheduling arrival on other side\n");
   insertevent(evptr);
}

void tolayer5(AorB, datasent) int AorB;
char datasent[20];
{
   int i;
   if (TRACE > 2)
   {
      printf("          TOLAYER5: data received: ");
      for (i = 0; i < 20; i++)
         printf("%c", datasent[i]);
      printf("\n");
   }
}
