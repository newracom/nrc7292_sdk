
CC ?= gcc

#########################################################

APP := raspi-atcmd-remote

CFLAGS = -Wall -Wno-unused-function -DCONFIG_VERBOSE
LFLAGS =

SRCS = main.c

#########################################################

all: $(APP)

$(APP): $(SRCS)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	@rm -vf $(APP)


