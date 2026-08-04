CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
ROOT := ..
INCLUDES := \
 -I$(ROOT)/wsounddna89/include \
 -I$(ROOT)/wsoundreceiver89/include \
 -I$(ROOT)/wsoundprojectile89/include \
 -I$(ROOT)/wsoundprop89/include \
 -I$(ROOT)/wsoundroom89/include \
 -I$(ROOT)/wsoundaction89/include \
 -I$(ROOT)/wsoundimpact89/include \
 -I$(ROOT)/wsoundricochet89/include \
 -I$(ROOT)/wsoundcombatbus89/include
LIBS := \
 $(ROOT)/wsounddna89/libwsounddna89.a \
 $(ROOT)/wsoundreceiver89/libwsoundreceiver89.a \
 $(ROOT)/wsoundprojectile89/libwsoundprojectile89.a \
 $(ROOT)/wsoundprop89/libwsoundprop89.a \
 $(ROOT)/wsoundroom89/libwsoundroom89.a \
 $(ROOT)/wsoundaction89/libwsoundaction89.a \
 $(ROOT)/wsoundimpact89/libwsoundimpact89.a \
 $(ROOT)/wsoundricochet89/libwsoundricochet89.a \
 $(ROOT)/wsoundcombatbus89/libwsoundcombatbus89.a
all: showcase
showcase: wsound_world_showcase.c $(LIBS)
	$(CC) $(CFLAGS) $(INCLUDES) wsound_world_showcase.c $(LIBS) -o showcase
	cd $(ROOT) && demo/showcase
clean:
	rm -f showcase
