
CC ?= gcc

#########################################################

APP := raspi-atcmd-cli

CFLAGS = -pthread -Wall -Wno-unused-function
LFLAGS = -lpthread

SRCS = \
	raspi-spi.c \
	raspi-uart.c \
	raspi-hif.c \
	nrc-hspi.c \
	nrc-atcmd.c \
	nrc-iperf.c \
	main.c

#########################################################

all: $(APP)

$(APP): $(SRCS)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	@rm -vf $(APP)


