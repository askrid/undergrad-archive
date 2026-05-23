TARGETS = ETCP_server ETCP_client
CC=gcc -g -O3 -Wall -Werror -fgnu89-inline
CFLAGS=-g -O2

# Add arch-specific optimization
ifeq ($(shell uname -m),x86_64)
LIBS += -m64
endif

# etcp library and header 
etcp_FLD    =../../etcp/
etcp_INC    =-I${etcp_FLD}/include
etcp_LIB    =-L${etcp_FLD}/lib
etcp_TARGET = ${etcp_LIB}/libetcp.a

INC = -I./include/ ${etcp_INC}
LIBS = ${etcp_LIB} -letcp -lpthread  -lrt

# pcap-specific variables
LIBS += -lpcap

ifeq ($V,) # no echo
	export MSG=@echo
	export HIDE=@
else
	export MSG=@\#
	export HIDE=
endif

all: ETCP_server ETCP_client

ETCP_server.o: ETCP_server.c
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< ${CFLAGS} ${INC}

ETCP_server: ETCP_server.o ${etcp_FLD}/lib/libetcp.a
	$(MSG) "   LD $<"
	$(HIDE) ${CC} $< ${UTIL_OBJ} -L${etcp_FLD}/lib -letcp -lpthread  -lrt ${LIBS} -o $@

ETCP_client.o: ETCP_client.c
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< ${CFLAGS} ${INC}

ETCP_client: ETCP_client.o ${etcp_FLD}/lib/libetcp.a
	$(MSG) "   LD $<"
	$(HIDE) ${CC} $< ${UTIL_OBJ} -L${etcp_FLD}/lib -letcp -lpthread  -lrt ${LIBS} -o $@

clean:
	$(MSG) "   CLEAN $(TARGETS)"
	$(HIDE) rm -f *~ *.o ${TARGETS} log_*

distclean: clean
	rm -rf Makefile
