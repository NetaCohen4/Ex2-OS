CC=gcc
CFLAGS=-Wall -Wextra -std=c99
TARGETS=molecule_supplier atom_supplier molecule_requester


all: $(TARGETS)

molecule_supplier: molecule_supplier.c
	$(CC) $(CFLAGS) -o molecule_supplier molecule_supplier.c

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o atom_supplier atom_supplier.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o molecule_requester molecule_requester.c

clean:
	rm -f $(TARGETS)