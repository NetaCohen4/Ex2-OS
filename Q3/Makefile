CC=gcc
CFLAGS=-Wall -Wextra -std=c99
TARGETS=bar_drinks atom_supplier molecule_requester


all: $(TARGETS)

bar_drinks: bar_drinks.c
	$(CC) $(CFLAGS) -o bar_drinks bar_drinks.c

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o atom_supplier atom_supplier.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o molecule_requester molecule_requester.c

clean:
	rm -f $(TARGETS)