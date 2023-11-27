#ifndef INIFILE_H
#define INIFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ini_property {
    char key[100];
    char value[100];

    struct ini_property *prev_property;
    struct ini_property *next_property;
};

struct ini_section {
    char name[100];
    struct ini_property *properties;

    struct ini_section *prev_section;
    struct ini_section *next_section;
};


struct ini_section *read_ini(char *filename);
struct ini_section *rewind_ini_section(struct ini_section *sections);
struct ini_section *get_ini_section(struct ini_section *sections, char *name);
struct ini_property *get_ini_property(struct ini_section *section, char *key);

#endif