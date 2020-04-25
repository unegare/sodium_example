CC ?= gcc
CFLAGS ?= -Wall -Wpedantic -O2
LDFLAGS += -lsodium
TARGET = a.out
LDPATH += -L/usr/local/lib
IDPATH += -I/usr/local/include

.PHONY: all
all: $(TARGET)

$(TARGET): ex_001_file_enc_dec.o
	$(CC) -o $@ $? $(LDPATH) $(LDFLAGS) 
	strip ${TARGET}

ex_001_file_enc_dec.o: ex_001_file_enc_dec.c
	$(CC) $(CFLAGS) -c $< $(IDPATH)

.PHONY: clean
clean:
	rm -f ex_001_file_enc_dec.o $(TARGET)
