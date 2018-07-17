/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
conn_mem * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ipv6/uip.h"
#include "net/routing/rpl-classic/rpl.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/etimer.h"
#include "sys/timer.h"
#include "sys/clock.h"
#include "sys/node-id.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "lib/random.h"

#include "net/netstack.h"
#include "dev/button-sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if COOJA_MOTE
#include "platform/cooja/dev/cooja-radio.h"
#else
#include "dev/cc2420/cc2420.h"
#endif

#define MAX_TRIES 0


#define PACKET_LEN       50

#define MAX_PAYLOAD_SIZE 150

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define UDP_EXAMPLE_ID  190

#define ACT_PERIOD (5*PERIOD)

#ifndef SIM_POWER
#define SIM_POWER 31
#endif

enum stream{
  cbr,
  request
};

typedef struct udptimer {
  struct udptimer *next;
  struct timer backoff;
  uip_ipaddr_t *ip;
  enum stream stream;
  uint32_t seq;
} udptimer_t;

typedef struct rtx {
  struct rtx *next;
  struct ctimer timer;
  uint32_t seq;
  uip_ipaddr_t ip;
  uint8_t times;
  char msg[MAX_PAYLOAD_SIZE];
} rtx_t;

static struct uip_udp_conn *server_conn;
//static uip_ipaddr_t *my_ipaddr;

static struct etimer periodic;
static struct etimer stats_timer;

uint8_t actuators[] = ACTUATORS;

LIST(udp_timers);
MEMB(tmem, udptimer_t, 120);

LIST(rtxs);
MEMB(rtxmem, rtx_t, 30);

PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
clock_time_t 
random_period(){
  clock_time_t t = CLOCK_SECOND*ACT_PERIOD*(random_rand()*1.0/RANDOM_RAND_MAX);
  return t; 
}
/*---------------------------------------------------------------------------*/
void
retransmit (void* ptr){
  rtx_t *r = (rtx_t*) ptr;
  PRINTF("{type: 'request' , action: 'Rtx', object:%s, from: 1, to: %d}\n",
         r->msg,
         //my_ipaddr->u8[15],
         r->ip.u8[15]
         );
  
  uip_udp_packet_sendto(server_conn,r->msg,strlen(r->msg),
                           &r->ip,UIP_HTONS(UDP_CLIENT_PORT));
  r->times += 1;
  if(r->times < MAX_TRIES)
    ctimer_reset(&r->timer);
  
  return;
}
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
#if REVERSE == 0 || REVERSE == 2
  char *appdata;

  if(uip_newdata()) {
    appdata = (char *)uip_appdata;
    appdata[uip_datalen()] = 0;
    if(strstr(appdata,"cbr")!=NULL){
      PRINTF("\n{type: 'cbr', action: 'Recv', object:{sender:%d, message: %s}, from:%d, to:%d}\n",
        UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1],
        appdata,
        UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1],
        UIP_IP_BUF->destipaddr.u8[sizeof(UIP_IP_BUF->destipaddr.u8) - 1]
      );
    }else if(strstr(appdata,"request")!=NULL){
      char str[90];
      strcpy(str,appdata);
      uip_clear_buf();
      uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
      uip_udp_packet_send(server_conn, str, strlen(str));
      uip_create_unspecified(&server_conn->ripaddr);
    }
  }
#else
  char *str;
  char *id_str;
  uint32_t id;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    if(strstr(str,"request")!=NULL){
      PRINTF("\n{type: 'request' , action: 'Recv', object:{sender:%d, message:%s}, from: 1, to: %d}\n",
         UIP_IP_BUF->destipaddr.u8[15],
         str,
         //UIP_IP_BUF->destipaddr.u8[15],
         //my_ipaddr->u8[15],
         UIP_IP_BUF->srcipaddr.u8[15]);
         /*Cancel retransmission*/
		id_str = strstr(str,"id:");
		if(id_str != NULL){
		  id_str += 4;//"id: [id]" change offset to [id]
		  id_str = strtok(id_str,",");//"insert \0 on first comma"
		  id = atoi(id_str);
		  PRINTF("type:'r %d\n",id);
		  rtx_t* r = list_head(rtxs);
		  while(r != NULL){
			rtx_t* act_r = r;
			r = list_item_next(r);
			if(UIP_IP_BUF->srcipaddr.u8[15] == act_r->ip.u8[15] && act_r->seq == id){
			  list_remove(rtxs,act_r);
			  ctimer_stop(&act_r->timer);
			  memb_free(&rtxmem,act_r);    
			}
		  }
		}
    }else if(strstr(str,"cbr")!=NULL){
      PRINTF("\n{type: 'cbr', action: 'Recv', object:{sender:%d, message: %s}, from:%d, to:%d}\n",
        UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1],
        str,
        UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1],
        UIP_IP_BUF->destipaddr.u8[sizeof(UIP_IP_BUF->destipaddr.u8) - 1]
      );
    }
    
  }
#endif
}
/*---------------------------------------------------------------------------*/
//static void
//print_local_addresses(void)
//{
//  int i;
//  uint8_t state;
//
//  PRINTF("Server IPv6 addresses: ");
//  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
//    state = uip_ds6_if.addr_list[i].state;
//    if(state == ADDR_TENTATIVE || state == ADDR_PREFERRED) {
//      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
//      PRINTF("\n");
//      /* hack to make address "final" */
//      my_ipaddr = &uip_ds6_if.addr_list[i].ipaddr;
//      if (state == ADDR_TENTATIVE) {
//	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
//      }
//    }
//  }
//}
/*---------------------------------------------------------------------------*/
static void
send_message_to_client(udptimer_t* t)
{
  uip_ipaddr_t *ip = (uip_ipaddr_t*) t->ip;
  char buf[80];
  char stream[20];

  if(t->stream == cbr){
    strcpy(stream,"cbr");
  }else if(t->stream == request){
    strcpy(stream,"request");
  }else{
    stream[0]=0;
  }

  t->seq += 1;
  sprintf(buf, "{id: %d, stream: '%s'}", t->seq,stream);
  printf("\n{type: '%s' , action: 'Send', object:%s, from: 1, to: %d}\n",
         stream,
         buf,
         //my_ipaddr->u8[15],
         ip->u8[15]
         );
  
  uip_udp_packet_sendto(server_conn,buf,PACKET_LEN,
                           ip,UIP_HTONS(UDP_CLIENT_PORT));
  
  if(sizeof(actuators) > 0 && MAX_TRIES > 0){
    rtx_t *r = (rtx_t*) memb_alloc(&rtxmem);
    if(r==NULL) return;
    r->times = 0;
    r->ip = *(t->ip);
    r->seq = t->seq;
    strcpy(r->msg,buf);
    ctimer_set(&r->timer,(clock_time_t)CLOCK_SECOND*TIMEOUT/1000,retransmit,r);
    list_add(rtxs,r);
  }
  return;  
}
/*---------------------------------------------------------------------------*/
udptimer_t*
udptimer_lookup(uip_ipaddr_t *ipaddr)
{
  udptimer_t* t = list_head(udp_timers);
  while(t != NULL){
    if(uip_ipaddr_cmp(t->ip,ipaddr)){
      return t;
    }
    t = list_item_next(t);
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
trigger_client_send(void* ptr)
{
  clock_time_t next_trigger = clock_time() + ACT_PERIOD*CLOCK_SECOND;
  uip_ds6_route_t *r = uip_ds6_route_head();
  udptimer_t* t;
  while(r!=NULL){
    if(r->length != 128){
      continue;
    }
    int i;
    uint8_t is_actuator = 0;
    for(i = 0; i < sizeof(actuators); i++){
      if((&r->ipaddr)->u8[15] == actuators[i]){
        is_actuator = 1;
      }
    }
    if(!is_actuator && sizeof(actuators)>0){
      r = uip_ds6_route_next(r);
      continue;
    }
    t = udptimer_lookup(&r->ipaddr);
    if(t == NULL){
      t = memb_alloc(&tmem);
      if(t==NULL){
        PRINTF("IMPORTANT: Could not alloc udptimer\n");
        continue;
      }
      t->ip = &r->ipaddr;
      t->seq = 0;
      uint8_t target_id = t->ip->u8[15];
      if(target_id%2){
        t->stream = cbr;
      }else{
        t->stream = request;
      }
/*
#ifdef TRAFFIC
#if TRAFFIC == CBR
     t->stream = cbr;
#elif TRAFFIC == REQUEST
     t->stream = request;
#endif
#endif
*/
      if(sizeof(actuators) != 0) t->stream = request;
      list_push(udp_timers,t);
      //timer_set(&t->backoff,(random_rand() % PERIOD)*CLOCK_SECOND);
      timer_set(&t->backoff,/*ACT_PERIOD*CLOCK_SECOND - (clock_time() % (ACT_PERIOD*CLOCK_SECOND)) +*/ random_period());
      //PRINTF("IMPORTANT: Added udp target. List size: %d\n",list_length(udp_timers));
    }
    if(timer_expired(&t->backoff)){
      send_message_to_client(t);
      //clock_time_t backoff = (random_rand() % PERIOD)*CLOCK_SECOND;
      clock_time_t backoff = random_period();
      backoff += ACT_PERIOD*CLOCK_SECOND/2; //- (clock_time() % (ACT_PERIOD*CLOCK_SECOND));
      timer_set(&t->backoff,backoff);
    }else if(clock_time()+timer_remaining(&t->backoff) < next_trigger){
      next_trigger = clock_time()+timer_remaining(&t->backoff);
    }
    r = uip_ds6_route_next(r);
  }
  etimer_set(&periodic, next_trigger - clock_time());
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
//  uip_ipaddr_t ipaddr;
//  struct uip_ds6_addr *root_if;
#ifdef WITH_COMPOWER
  static struct etimer powertrace;
#endif

  PROCESS_BEGIN();

  PROCESS_PAUSE();

#ifdef SEED
  random_init(node_id);
#endif

#if COOJA_MOTE
radio_set_txpower(100 * SIM_POWER / 31);
#else
cc2420_set_txpower(SIM_POWER);
#endif


  PRINTF("UDP server started. nbr:%d routes:%d\n",
         NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

#if UIP_CONF_ROUTER

//  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
//  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
//  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
//  root_if = uip_ds6_addr_lookup(&ipaddr);
//  if(root_if != NULL) {
//    //rpl_dag_t *dag;
//    //dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
//    uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
//    //rpl_set_prefix(dag, &ipaddr, 64);
//    NETSTACK_ROUTING.root_set_prefix(&ipaddr,NULL);
//    NETSTACK_ROUTING.root_start();
//    PRINTF("created a new RPL dag\n");
//  } else {
//    PRINTF("failed to create a new RPL DAG\n");
//  }
#endif /* UIP_CONF_ROUTER */
  
//  print_local_addresses();

  list_init(udp_timers);
  memb_init(&tmem);
  
  list_init(rtxs);
  memb_init(&rtxmem);

  /* The data sink runs with a 100% duty cycle in order to ensure high 
     packet reception rates. */
  NETSTACK_MAC.off();

  server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
  if(server_conn == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));

  PRINTF("Created a server connection with remote address ");
  PRINT6ADDR(&server_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n", UIP_HTONS(server_conn->lport),
         UIP_HTONS(server_conn->rport));

#if REVERSE
  printf("REVERSE\n");
  etimer_set(&periodic, START_INTERVAL);
#endif 

  etimer_set(&stats_timer, 60*CLOCK_SECOND);

/*
#ifdef WITH_COMPOWER
  powertrace_sniff(POWERTRACE_ON);
  etimer_set(&powertrace,30*CLOCK_SECOND);
#endif*/

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    } else if(ev == PROCESS_EVENT_TIMER){
#if REVERSE
      if(etimer_expired(&periodic)){
        trigger_client_send(NULL);
      }
#endif
      if(etimer_expired(&stats_timer)){
        printf("\n{type: 'Stats', key: 'NeighborCount', value: %d}\n",uip_ds6_nbr_num());
      }
    }
#ifdef WITH_COMPOWER
    if(etimer_expired(&powertrace)){
      etimer_reset(&powertrace);
      powertrace_print("#P");
    }
#endif
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
