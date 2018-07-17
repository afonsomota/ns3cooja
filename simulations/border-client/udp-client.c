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
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */


#include "contiki.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ipv6/uip.h"
#include "net/routing/rpl-classic/rpl-private.h"
#include "net/routing/rpl-classic/rpl.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-udp-packet.h"
#include "sys/ctimer.h"

#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif

#include <stdio.h>
#include <string.h>

#include "lib/random.h"
#include "net/ipv6/uip-ds6-route.h"

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678


#define UDP_EXAMPLE_ID  190

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_FULL
#include "net/ipv6/uip-debug.h"

#ifndef PERIOD
#define PERIOD 60
#endif

#ifndef SIM_POWER
#define SIM_POWER 31
#endif

#if COOJA_MOTE
#include "platform/cooja/dev/cooja-radio.h"
#else
#include "dev/cc2420/cc2420.h"
#endif

#include "sys/node-id.h"

#define MAX_PAYLOAD_LEN		100
#define PACKET_LEN          50

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;
static uint8_t power;
//static uip_ipaddr_t * my_ipaddr;
char tstream[20];

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
#if REVERSE==0
static int seq_id;
static int reply;
#endif

static void
tcpip_handler(void)
{
#if REVERSE==0
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    reply++;
    //printf("DATA recv '%s' (s:%d, r:%d)\n", str, seq_id, reply);
    printf("\n{type: 'request', action: 'Recv', object:{sender:%d, message:%s}, from: %d, to: %d}\n",
           UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1],str,node_id,server_ipaddr.u8[15]);
  }
#else
  char *appdata;
  char str[90];
  if(uip_newdata()) {
    appdata = (char *)uip_appdata;
    appdata[uip_datalen()] = 0;
    if(strstr(appdata,"cbr")){
      printf("\n{type: 'cbr', action: 'Recv', object:{sender:%d, message:%s}, from: %d, to: %d}\n",
           UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1],appdata,UIP_IP_BUF->srcipaddr.u8[15],node_id);
    }else if(strstr(appdata,"request")){
      strcpy(str,appdata);
      uip_clear_buf();
      uip_udp_packet_sendto(client_conn, str, strlen(str),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/
#if REVERSE==0
static void
send_packet(void *ptr)
{
  char buf[MAX_PAYLOAD_LEN];
  
  //rpl_print_neighbor_list();
/*
#ifdef SERVER_REPLY
  uint8_t num_used = 0;
  uip_ds6_nbr_t *nbr;

  nbr = nbr_table_head(ds6_neighbors);
  while(nbr != NULL) {
    nbr = nbr_table_next(ds6_neighbors, nbr);
    num_used++;
  }

  if(seq_id > 0) {
    ANNOTATE("#A r=%d/%d,color=%s,n=%d %d\n", reply, seq_id,
             reply >= 0.65*seq_id ? (reply==seq_id? "GREEN" :"ORANGE") : "RED", uip_ds6_route_num_routes(), num_used);
  }
#endif*/ /* SERVER_REPLY */

  seq_id++;
  sprintf(buf, "{id: %d, stream: '%s', nodeTime: %d}", seq_id,tstream,/*clock_seconds()*/0);
  printf("\n{type: '%s', action: 'Send', object:%s, from: %d, to: %d}\n",
         tstream,buf,node_id,server_ipaddr.u8[15]);
//  PRINT6ADDR(my_ipaddr);
//  PRINTF("', to: '");
//  PRINT6ADDR(&server_ipaddr);
//  PRINTF("'}");
  uip_udp_packet_sendto(client_conn, buf, PACKET_LEN,
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
#endif
/*---------------------------------------------------------------------------*/
//static void
//print_local_addresses(void)
//{
//  int i;
//  uint8_t state;
//
//  printf("Client IPv6 addresses: ");
//  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
//    state = uip_ds6_if.addr_list[i].state;
//    if(uip_ds6_if.addr_list[i].isused &&
//       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
//      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
//      printf("\n");
//      my_ipaddr = &uip_ds6_if.addr_list[i].ipaddr;
//      /* hack to make address "final" */
//      if (state == ADDR_TENTATIVE) {
//	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
//      }
//    }
//  }
//}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
//  uip_ipaddr_t ipaddr;

//  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
//  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
//  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

//  uip_ip6addr(&server_ipaddr, 0xf2a1,0, 0, 0, 0x027b, 0x007b, 0x007b, 0x007b);
//  uip_ip6addr(&server_ipaddr, 0xfd00,0, 0, 0, 0x0201, 0x0001, 0x0001, 0x0001);
  uip_ip6addr(&server_ipaddr, 0xfd33,0, 0, 0, 0x0233, 0x0033, 0x0033, 0x0033);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic;
  static struct etimer periodic_end;
  static struct etimer stats_timer;
  static struct etimer powertrace;
#if REVERSE==0
//  static struct ctimer backoff_timer;
#endif

#if COOJA_MOTE
power = 100 * SIM_POWER / 31;
radio_set_txpower(power);
#else
cc2420_set_txpower(SIM_POWER);
#endif

  PROCESS_BEGIN();

  PROCESS_PAUSE();
  
  random_init(node_id);

//  if(random_rand()>RANDOM_RAND_MAX/2){
    strcpy(tstream,"cbr");
//  }else{
//    strcpy(tstream,"request");
//  }

  set_global_address();

  printf("UDP client process started nbr:%d routes:%d\n",
         NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

  //print_local_addresses();

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL); 
  if(client_conn == NULL) {
    printf("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT)); 

  printf("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  printf(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  /* initialize serial line */
  //uart1_set_input(serial_line_input_byte);
  //serial_line_init();

#ifdef WITH_COMPOWER
  powertrace_sniff(POWERTRACE_ON);
#endif

  etimer_set(&periodic, START_INTERVAL + SEND_TIME);
  etimer_set(&periodic_end, STOP_INTERVAL);
  etimer_set(&powertrace, 30*CLOCK_SECOND);
  etimer_set(&stats_timer, 60*CLOCK_SECOND);
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }


    if(etimer_expired(&periodic)) {
      etimer_set(&periodic, SEND_INTERVAL);
#if REVERSE==0
      if (!etimer_expired(&periodic_end))
        send_packet(NULL);
//        ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
#endif
    }
    if (etimer_expired(&stats_timer)){
//      printf("{type: 'Stats', key: 'ParentSwitch', value: %d}\n",rpl_stats.parent_switch);
//      printf("{type: 'Stats', key: 'GlobalRepairs', value: %d}\n",rpl_stats.global_repairs);
      etimer_reset(&stats_timer);
    }
#if WITH_COMPOWER
    if(etimer_expired(&powertrace)){
      etimer_reset(&powertrace);
      powertrace_print("#P");
    }
#endif
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
