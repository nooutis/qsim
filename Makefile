# =========================================================================
# Makefile per Progetto Sistemi Operativi II (a.a. 2025-26)
# =========================================================================

# Variabili del compilatore e flag
CC = gcc
CFLAGS = -g -Wall -Wextra -pthread -O2
# LDFLAGS per GSL, libreria matematica e thread
LDFLAGS = $(shell gsl-config --libs) -lm -lpthread

# File e target
TARGET = qsim
OBJ = main.o parser.o thread.o memory.o measurement.o

# Dichiarazione dei target che non sono file reali
.PHONY: all clean

# -------------------------------------------------------------------------
# TARGET PRINCIPALI
# -------------------------------------------------------------------------

# Il target 'all' è il primo, quindi viene eseguito di default
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

# -------------------------------------------------------------------------
# COMPILAZIONE DEI MODULI (Oggetti)
# -------------------------------------------------------------------------

main.o: main.c data.h parser.h thread.h memory.h measurement.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c parser.h data.h
	$(CC) $(CFLAGS) -c parser.c

thread.o: thread.c thread.h data.h
	$(CC) $(CFLAGS) -c thread.c

memory.o: memory.c memory.h data.h
	$(CC) $(CFLAGS) -c memory.c

measurement.o: measurement.c measurement.h data.h
	$(CC) $(CFLAGS) -c measurement.c
# -------------------------------------------------------------------------
# UTILITY
# -------------------------------------------------------------------------

# Pulisce i file temporanei e l'eseguibile
clean:
	rm -f $(OBJ) $(TARGET)

