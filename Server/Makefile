#CC= arm-linux-gcc
#CC=gcc

CFLAGS = -lrt
HEAD = ../Parser/ucmd.h

all :
	$(CC_S) -pthread -o server.elf server.c ../Parser/ucmd.c $(CFLAGS)

transfer_server: 
	scp server.elf root@$(IP):/$(FOLDER)

.PHONY: clean
clean:
	rm -rf *.elf