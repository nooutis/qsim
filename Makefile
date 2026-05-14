# =========================================================================
# Makefile per Progetto Sistemi Operativi II (a.a. 2025-26)
# =========================================================================

CC = gcc

GSL_CFLAGS = $(shell gsl-config --cflags)
GSL_LIBS = $(shell gsl-config --libs)

CFLAGS = -Wall -Wextra -pthread -O2 $(GSL_CFLAGS)
LDFLAGS = $(GSL_LIBS) -lm -lpthread

TARGET = qsim
OBJ = main.o parser.o thread.o memory.o measurement.o data.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

# -------------------------------------------------------------------------
# COMPILAZIONE DEI MODULI
# -------------------------------------------------------------------------

main.o: main.c data.h parser.h thread.h memory.h measurement.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c parser.h data.h memory.h
	$(CC) $(CFLAGS) -c parser.c

thread.o: thread.c measurement.c thread.h data.h measurement.h
	$(CC) $(CFLAGS) -c thread.c

memory.o: memory.c memory.h data.h
	$(CC) $(CFLAGS) -c memory.c

measurement.o: measurement.c measurement.h data.h
	$(CC) $(CFLAGS) -c measurement.c

data.o: data.c memory.h data.h
	$(CC) $(CFLAGS) -c data.c

clean:
	rm -f $(OBJ) $(TARGET)