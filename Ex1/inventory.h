#ifndef INVENTORY_H
#define INVENTORY_H

typedef struct {
    unsigned int hydrogen;
    unsigned int oxygen;
    unsigned int carbon;
} Inventory;

void init_inventory(Inventory *inv, unsigned int h, unsigned int o, unsigned int c);
void add_atoms(Inventory *inv, const char *type, unsigned int amount);
int deliver_molecule(Inventory *inv, const char *molecule, unsigned int amount);
void print_inventory(Inventory *inv);

#endif
