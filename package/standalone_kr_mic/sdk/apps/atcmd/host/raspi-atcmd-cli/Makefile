
CC ?= gcc

#########################################################

APP := raspi-atcmd-cli

CFLAGS = -pthread -Wall -Wno-unused-function 
LFLAGS = -lpthread

SRCS = main.c \
	raspi-hif.c \
	raspi-spi.c \
	raspi-uart.c \
	nrc-hspi.c

#########################################################

all: $(APP)

$(APP): $(SRCS)
	$(CC) -g -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	@rm -vf $(APP)


