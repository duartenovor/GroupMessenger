#CC= arm-linux-gcc
#CC=gcc

CFLAGS = -lrt
OBJS = client.elf
HEAD = ../Parser/ucmd.h

all:
	$(CC_C) -pthread -o daemon.elf daemon.c ../Parser/ucmd.c $(CFLAGS);\
	$(CC_C) -pthread -o client.elf client.c $(CFLAGS)

transfer_client: 
	scp daemon.elf client.elf root@$(IP):/$(FOLDER)

.PHONY: clean
clean:
	rm -rf *.elf
