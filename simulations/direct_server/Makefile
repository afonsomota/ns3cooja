all: udp-server
CONTIKI=/home/user/contiki-ng

MODULES += os/services/rpl-border-router

MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

ifdef WITH_COMPOWER
APPS+=powertrace
CFLAGS+= -DCONTIKIMAC_CONF_COMPOWER=1 -DWITH_COMPOWER=1 -DQUEUEBUF_CONF_NUM=4
endif

ifdef TARGET
ifeq ($(TARGET),cooja)
CFLAGS+=-DCOOJA_MOTE=1
endif
endif

ifdef SERVER_REPLY
CFLAGS+=-DSERVER_REPLY=$(SERVER_REPLY)
endif
ifdef PERIOD
CFLAGS+=-DPERIOD=$(PERIOD)
endif
ifdef REVERSE
CFLAGS+=-DREVERSE=$(REVERSE)
endif


ifdef NBR_TABLE_CONF_MAX_NEIGHBORS	
CFLAGS+=-DNBR_TABLE_CONF_MAX_NEIGHBORS=$(NBR_TABLE_CONF_MAX_NEIGHBORS)
endif

ifdef TIMEOUT	
CFLAGS+=-DTIMEOUT=$(TIMEOUT)
endif

ifdef TRAFFIC	
CFLAGS+=-DTRAFFIC=$(TRAFFIC)
endif

ifdef ACTUATORS
CFLAGS+=-DACTUATORS=$(ACTUATORS)
endif

ifdef SEED
CFLAGS+=-DSEED=$(SEED)
endif

ifdef SIM_POWER
CFLAGS+=-DSIM_POWER=$(SIM_POWER)
endif

ifdef USE_UNI_LINKS
CFLAGS+=-DUSE_UNI_LINKS=$(USE_UNI_LINKS)
endif

ifdef WITHOUT_ACK
CFLAGS+=-DWITHOUT_ACK=$(WITHOUT_ACK)
endif

ifeq ($(MAKE_WITH_NON_STORING),1)
CFLAGS += -DWITH_NON_STORING=1
endif

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

touch:
	touch $(CONTIKI)/os/net/*

udp-server-custom.z1: lock clean touch udp-server.z1
	mv udp-server.z1 $(PROJDIR)/udp-server-custom.z1
	sh unlock.sh

udp-client-max.z1: lock touch udp-client.z1
	mv udp-client.z1 $(PROJDIR)/udp-client-max.z1
	sh unlock.sh

udp-client-min.z1: lock touch udp-client.z1
	mv udp-client.z1 $(PROJDIR)/udp-client-min.z1
	sh unlock.sh

udp-client-mid.z1: lock touch udp-client.z1
	mv udp-client.z1 $(PROJDIR)/udp-client-mid.z1
	sh unlock.sh

udp-server-custom.cooja: clean touch udp-server.cooja
	mv udp-server.cooja $(PROJDIR)/udp-server-custom.cooja

udp-client-max.cooja: touch udp-client.cooja
	mv udp-client.cooja $(PROJDIR)/udp-client-max.cooja

udp-client-min.cooja: touch udp-client.cooja
	mv udp-client.cooja $(PROJDIR)/udp-client-min.cooja

udp-client-mid.cooja: touch udp-client.cooja
	mv udp-client.cooja $(PROJDIR)/udp-client-mid.cooja
