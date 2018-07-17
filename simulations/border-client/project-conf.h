/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
 * All rights reserved.
 *
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
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#include "contiki.h"

//#define ENERGEST_CONF_ON 1

//#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#undef UIP_CONF_MAX_ROUTES

#ifndef ACTUATORS
#define ACTUATORS {}
#endif

#define RPL_CONF_STATS 1

#ifndef TIMEOUT
#define TIMEOUT 2000
#endif

#define UIP_CONF_IPV6_CHECKS 0

#ifdef COOJA_MOTE
#if COOJA_MOTE == 1
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     125
#endif
#define UIP_CONF_MAX_ROUTES   125
#endif
#else
#define NBR_TABLE_CONF_MAX_NEIGHBORS     15
#define UIP_CONF_MAX_ROUTES   23
#define COOJA_MOTE 0
#endif

#define TCPIP_CONF_ANNOTATE_TRANSMISSIONS 0

#ifndef WITH_NON_STORING
#define WITH_NON_STORING 0 /* Set this to run with non-storing mode */
#endif /* WITH_NON_STORING */

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nullrdc_driver

#define NETSTACK_CONF_MAC csma_driver
#define UIP_CONF_UIP_DS6_NOTIFICATIONS 1

#undef UIP_CONF_ND6_SEND_NA
#define UIP_CONF_ND6_SEND_NA 1

#define NULLRDC_CONF_802154_AUTOACK_HW  0

#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0


/* Define as minutes */
#define RPL_CONF_DEFAULT_LIFETIME_UNIT   60

/* 10 minutes lifetime of routes */
#define RPL_CONF_DEFAULT_LIFETIME        10

#define RPL_CONF_DEFAULT_ROUTE_INFINITE_LIFETIME 1

#ifndef PERIOD
#define PERIOD 60
#endif

#define RPL_CONF_STATS 1

#define START_INTERVAL          (2 * 60 * CLOCK_SECOND)
#define STOP_INTERVAL          (9 * 60 * CLOCK_SECOND)
#define SEND_INTERVAL           (PERIOD * CLOCK_SECOND)
#define SEND_TIME               (random_rand() % (SEND_INTERVAL))


#ifndef REVERSE
#define REVERSE 0
#endif

#ifndef SERVER_REPLY
#define SERVER_REPLY 0
#endif

#endif
