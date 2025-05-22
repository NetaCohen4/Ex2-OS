#include <stdio.h>
#include <string.h>
#include "inventory.h"

void init_inventory(Inventory *inv, unsigned int h, unsigned int o, unsigned int c) {
    inv->hydrogen = h;
    inv->oxygen = o;
    inv->carbon = c;
}

void add_atoms(Inventory *inv, const char *type, unsigned int amount) {
    if (strcmp(type, "HYDROGEN") == 0)
        inv->hydrogen += amount;
    else if (strcmp(type, "OXYGEN") == 0)
        inv->oxygen += amount;
    else if (strcmp(type, "CARBON") == 0)
        inv->carbon += amount;
}

int deliver_molecule(Inventory *inv, const char *molecule, unsigned int amount) {
    unsigned int h = 0, o = 0, c = 0;
    if (strcmp(molecule, "WATER") == 0) { h=2; o=1; }
    else if (strcmp(molecule, "CARBON DIOXIDE") == 0) { c=1; o=2; }
    else if (strcmp(molecule, "ALCOHOL") == 0) { c=2; h=6; o=1; }
    else if (strcmp(molecule, "GLUCOSE") == 0) { c=6; h=12; o=6; }
    else return 0;

    if (inv->hydrogen < h*amount || inv->oxygen < o*amount || inv->carbon < c*amount)
        return 0;

    inv->hydrogen -= h * amount;
    inv->oxygen   -= o * amount;
    inv->carbon   -= c * amount;
    return 1;
}

void print_inventory(Inventory *inv) {
    printf("Hydrogen: %u, Oxygen: %u, Carbon: %u\n", inv->hydrogen, inv->oxygen, inv->carbon);
}
