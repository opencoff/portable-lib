/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * configfile.h - Pythonic config file processing.
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef ___INC_UTILS_CONFIGFILE_H_6480698_1441264996__
#define ___INC_UTILS_CONFIGFILE_H_6480698_1441264996__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "utils/gstring.h"
#include "fast/list.h"
#include "fast/vect.h"

struct node;

struct keyval
{
    gstr key;
    gstr val;
};
typedef struct keyval keyval;


// Array of key-value pairs. 
VECT_TYPEDEF(kv_vect, keyval);

// Linked list of child nodes;
DL_HEAD_TYPEDEF(node_head, node);


// A node in the config "tree"
struct node
{
    // Internal node data
    int         col;
    int         master; // bool

    DL_ENTRY(node) link;

    // Child nodes
    node_head  subtree;

    // List of key-value pairs
    kv_vect    values;

    gstr       name;
};
typedef struct node node;



struct configfile
{
    node*       root;

    // file name
    const char* name;

};
typedef struct configfile configfile;


int  config_open(configfile* cf, const char* name);

int  config_close(configfile* cf);

void config_dump(configfile* cf);


// Open a directory "/config/a/b" and return the entries in it
node* config_get_section(configfile* cf, const char* path);

// Starting form 'start' find a node that ends in 'path'
node* config_get_subsection(node* start, const char* path);



// Starting from the root, find the leaf that ends in 'path'
const char* config_get_entry(configfile* cf, const char* path);

// Starting with 'start' find the leaf that ends in 'path'
const char* config_section_get_entry(node* start, const char* path);


// -- Parse values --

int  config_parse_int(const char*,  long* val);
int  config_parse_uint(const char*, unsigned long* val);
int  config_parse_bool(const char*, int* val);

#define config_parse_size(s, v)             config_parse_uint(s,v)
#define config_parse_ipv4(s,i)              parse_ipv4(i, s)
#define config_parse_ipv4_and_mask(s,i,m)   parse_ipv4_and_mask(i,m,s)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___INC_UTILS_CONFIGFILE_H_6480698_1441264996__ */

/* EOF */
