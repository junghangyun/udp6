#CROSS = powerpc-linux-gnu-
CROSS = arm-linux-gnueabihf-
LIBS = -lrt -lpthread

SRC = udp6.c
TARGET = udp6

$(TARGET):
	$(CROSS)gcc -o $@ $(SRC) $(LIBS)
clean:
	rm -rf $(TARGET)
	rm -rf *.o

