
CC ?= gcc

#########################################################

APP := raspi-atcmd-cli

SRCS := \
	raspi-spi.c \
	raspi-uart.c \
	raspi-eirq.c \
	raspi-hif.c \
	nrc-hspi.c \
	nrc-atcmd.c \
	nrc-iperf.c \
	main.c

CFLAGS += -pthread -Wall -Wno-unused-function
LDFLAGS += -lpthread -lz

#########################################################

all: $(APP)

$(APP): $(SRCS)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	@rm -vf $(APP)


