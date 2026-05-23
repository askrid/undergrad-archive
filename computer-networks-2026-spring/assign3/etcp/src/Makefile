# TODO: Make this Makefile.in pretty

.PHONY: clean libccp

### TARGET ###
etcp_LIB_DIR=../lib
etcp_LIB=libetcp.a
etcp_HDR_DIR=../include
etcp_HDR = etcp_api.h etcp_epoll.h

### GCC ###
GCC=gcc

### FLAGS ###
ifeq ($(shell uname -m),x86_64)
GCC_OPT = -m64
else
GCC_OPT =
endif

GCC_OPT += -Wall -fPIC -fgnu89-inline -Werror -g -O3
GCC_OPT += -D__USRLIB__


### LIBRARIES AND INCLUDES ###
INC_DIR=./include
INC= -I$(INC_DIR)

### SOURCE CODE ###
SRCS = core.c tcp_flow.c config.c api.c eventpoll.c socket.c\
	   util.c eth_in.c ip_in.c tcp_in.c eth_out.c ip_out.c tcp_out.c \
	   arp.c timer.c addr.c etcp_hash.c memory_manager.c \
	   receive_buffer.c send_buffer.c flow_queue.c \
	   cc_trace.c io_engine.c pcap_module.c


OBJS = $(patsubst %.c,%.o,$(SRCS))
DEPS = $(patsubst %.c,.%.d,$(SRCS))

ifeq ($V,) # no echo
	export MSG=@echo
	export HIDE=@
else
	export MSG=@\#
	export HIDE=
endif


all: default


default: $(OBJS) $(etcp_HDR)
	mkdir -p $(etcp_LIB_DIR)
	ar rvs $(etcp_LIB_DIR)/$(etcp_LIB) $(OBJS)


$(OBJS): %.o: %.c Makefile
	$(MSG) "   CC $<"
	$(HIDE) $(GCC) $(CFLAGS) $(GCC_OPT) $(INC) -c $< -o $@
$(DEPS): .%.d: %.c Makefile
	$(HIDE) $(GCC) $(GCC_OPT) $(INC) -MM $(CFLAGS) $< > $@

-include $(DEPS)

$(etcp_HDR):
	cp $(INC_DIR)/$@ $(etcp_HDR_DIR)/$@

clean: clean-library
	$(MSG) "   CLEAN *.o's"
	$(HIDE) rm -f *.o *~ core
	$(MSG) "   CLEAN *.d's"
	$(HIDE) rm -f .*.d

clean-library:
	$(MSG) "   CLEAN *.a"
	$(HIDE) rm -f $(etcp_LIB_DIR)/*
	$(MSG) "   CLEAN *.h"
	$(HIDE) rm -f $(etcp_HDR_DIR)/*

distclean: clean
	rm -f Makefile
