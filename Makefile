CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = artemis
OBJS = main.o artemis.o validaciones.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c artemis.h
	$(CC) $(CFLAGS) -c main.c

artemis.o: artemis.c artemis.h validaciones.h
	$(CC) $(CFLAGS) -c artemis.c

validaciones.o: validaciones.c validaciones.h
	$(CC) $(CFLAGS) -c validaciones.c

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe
