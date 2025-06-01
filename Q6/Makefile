CC=gcc
CFLAGS=-Wall -Wextra -std=c99
CLIENTFLAG = -D_POSIX_C_SOURCE=200112L
TARGETS=drinks_bar atom_supplier molecule_requester test


all: $(TARGETS)

drinks_bar: drinks_bar.c
	$(CC) $(CFLAGS) -o drinks_bar drinks_bar.c

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o atom_supplier atom_supplier.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o molecule_requester molecule_requester.c

test: test.c
	$(CC) $(CFLAGS) -o test test.c


clean:
	rm -f $(TARGETS)