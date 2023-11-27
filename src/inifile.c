#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "inifile.h"

#define MAXLINE 100

void strmap(char* szString)
{
    uint8_t i=0;
    uint8_t c=0;
    uint8_t len = strlen(szString);

    for (i=0; i<len; i++)
    {
        c = szString[i];
        if (c >= 64 && c<64+27)
            szString[i]+=128;
        else if (c > 96 && c<96+27)
            szString[i]-=32;
    }    

}

struct ini_section *read_ini(char *filename)
{
    static char buf[MAXLINE];

    static char val1[100];
    static char val2[100];

    struct ini_section *prev_section = NULL;
    struct ini_section *curr_section = NULL;

    struct ini_property *prev_property = NULL;
    struct ini_property *curr_property = NULL;

    FILE *fp = fopen(filename, "r");
    if (!fp)
        return 0;

    while(fgets(buf, MAXLINE, fp) != NULL)
    {
        if (buf[0] == 0x0A)
            strcpy(buf, buf+1);
        if (buf[0] == '#') // comment? ignore line
            continue;

        if(sscanf(buf, "[%[^]]]", val1) > 0)
        {
            // Start of section
            prev_section = curr_section;
            curr_section = (struct ini_section *) malloc(sizeof(struct ini_section));

            //printf("load: %s\r\n", val1);
            strmap(val1);
            //printf("case: %s\r\n", val1);

            strncpy(curr_section->name, val1, sizeof(val1));
            curr_section->properties = NULL;
            curr_section->prev_section = prev_section;
            curr_section->next_section = NULL;

            if(prev_section != NULL)
            {
                prev_section->next_section = curr_section;
            }

            prev_property = NULL;
            curr_property = NULL;
        }
        else if(sscanf(buf, "%[^=]=%s", val1, val2) > 0)
        {
            // Key = Value
            if(curr_section != NULL)
            {
                prev_property = curr_property;

                curr_property = (struct ini_property *) malloc(sizeof(struct ini_property));

                strmap(val1);
                strmap(val2);
                strncpy(curr_property->key, val1, sizeof(val1));
                strncpy(curr_property->value, val2, sizeof(val2));
                curr_property->prev_property = prev_property;
                curr_property->next_property = NULL;

                if(prev_property != NULL)
                {
                    prev_property->next_property = curr_property;
                }

                if(curr_section->properties == NULL)
                {
                    curr_section->properties = curr_property;
                }
            }
        }
    }

    fclose(fp);

    while(curr_section->prev_section != NULL)
    {
        curr_section = curr_section->prev_section;
    }

    return curr_section;
}

struct ini_section *rewind_ini_section(struct ini_section *sections)
{
    struct ini_section *curr_section = sections;
    
    while(curr_section->prev_section != NULL)
    {
        curr_section = curr_section->prev_section;
    }
    
    return curr_section;
}

struct ini_section *get_ini_section(struct ini_section *sections, char *name)
{
    struct ini_section *retVal = NULL;
    struct ini_section *curr_section = rewind_ini_section(sections);

    while(curr_section != NULL)
    {
        if(strcmp(curr_section->name, name) == 0)
        {
            retVal = curr_section;
            break;
        }

        curr_section = curr_section->next_section;
    }

    return retVal;
}

struct ini_property *get_ini_property(struct ini_section *section, char *key)
{
    struct ini_property *retVal = NULL;
    struct ini_property *properties = section->properties;
    struct ini_property *curr_property = properties;

    while(curr_property != NULL)
    {
        //printf("strcmp: %d, len: %d\r\n",strcmp(curr_property->key, key), strlen(curr_property->key), strlen(key));
        //printf("   %02X, %02X\r\n",curr_property->key[8], key[8]);

        if(strcmp(curr_property->key, key) == 0)
        {
            //printf("found\r\n");
            retVal = curr_property;
            break;
        }

        curr_property = curr_property->next_property;
    }

    return retVal;
}

void delete_ini_section(struct ini_section *section)
{
    struct ini_property *curr_property = section->properties;

    while(curr_property != NULL)
    {
        struct ini_property *next_property = curr_property->next_property;
        
        free(curr_property);

        curr_property = next_property;
    }

    if(section->prev_section != NULL)
    {
        section->prev_section->next_section = section->next_section;
    }

    if(section->next_section != NULL)
    {
        section->next_section->prev_section = section->prev_section;
    }

    free(section);
    section = NULL;
}

void delete_all_ini_sections(struct ini_section *sections)
{
    struct ini_section *curr_section = rewind_ini_section(sections);

    while(curr_section != NULL)
    {
        struct ini_section *next_section = curr_section->next_section;
    
        delete_ini_section(curr_section);

        curr_section = next_section;
    }
}

