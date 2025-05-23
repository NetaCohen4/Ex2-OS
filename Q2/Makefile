CC=gcc
CFLAGS=-Wall -Wextra -std=c99
TARGETS=atom_warehouse atom_supplier requester_molecule


all: $(TARGETS)

atom_warehouse: atom_warehouse.c
	$(CC) $(CFLAGS) -o atom_warehouse atom_warehouse.c

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o atom_supplier atom_supplier.c

requester_molecule: requester_molecule.c
	$(CC) $(CFLAGS)  -D_POSIX_C_SOURCE=200112L -o requester_molecule requester_molecule.c

clean:
	rm -f $(TARGETS)