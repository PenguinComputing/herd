/*
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * Copyright 2007-2009 Sun Microsystems, Inc. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the only version 2 of the GNU General
 * Public License, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included in the COPYING file that accompanied this file).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <stdlib.h>
#include <string.h>

#include "set.h"

#define SET_INITIAL_SIZE 128

typedef struct set_elem_str
{
    char* name;
    void* data;
    struct set_elem_str* next;

} set_elem_t;

struct herd_set {
    u32          table_size;
    set_elem_t** table;
    u32          count;
};

// Simple string hash function.
static u32 hashfunction(const char* str)
{
    u32 val = 5381;

    int c;
    while ((c = (int)(*str++)))
	val = ((val << 5) + val) + c;

    return val;
}

// Internal lookup routine.
static set_elem_t** look_for(Set* set, const char* name)
{
    set_elem_t** ptrholder;
    set_elem_t* ptr;

    int pos = hashfunction(name) % set->table_size;

    ptrholder = &(set->table[pos]);

    while ((ptr = *ptrholder)) {
	if (strcmp(name, ptr->name) == 0)
	    return ptrholder;
	ptrholder = &(ptr->next);
    }

    return NULL;
}

Set* set_new(u32 initial_size)
{
    Set* set = (Set*)malloc(sizeof(Set));
    if (set)
	memset(set, 0, sizeof(Set));

    if (!initial_size)
	initial_size = SET_INITIAL_SIZE;

    set->table_size = initial_size;
    set->table = (set_elem_t**)malloc(sizeof(set_elem_t*) * initial_size);
    memset(set->table, 0, sizeof(set_elem_t*) * initial_size);

    return set;
}


void set_delete(Set *set, void (*f)(void*))
{
    int i;

    for (i = 0; i < set->table_size; i++) {
	set_elem_t *ptr, *next;

	ptr = set->table[i];
	while (ptr) {
	    next = ptr->next;
	    if (ptr->name)
		free(ptr->name);
	    if (f)
		(*f)(ptr->data);
	    free(ptr);
	    ptr = next;
	}
    }
    free(set->table);
    free(set);
}

int set_add(Set* set, const char* name, void* data)
{
    set_elem_t* ptr;
    int pos;

    if (!set || !name)
	return -1;

    pos = hashfunction(name) % set->table_size;

    ptr = (set_elem_t*)malloc(sizeof(set_elem_t));
    ptr->name = strdup(name);
    ptr->data = data;
    ptr->next = set->table[pos];
    set->table[pos] = ptr;
    set->count++;

    return set->count;
}

int set_lookup(Set* set, const char* name, void** data)
{
    set_elem_t** ptrholder;

    if (!name || !set)
	return 0;

    ptrholder = look_for(set, name);
    if (!ptrholder)
	return 0;

    if (data)
	*data = (*ptrholder)->data;

    return 1;
}

int set_remove(Set* set, const char* name, void (*f)(void*))
{
    set_elem_t **ptrholder, *ptr, *pnext;
    void* data;

    if (!name || !set)
	return 0;

    ptrholder = look_for(set, name);

    if (!ptrholder)
	return 0;

    ptr = *ptrholder;
    pnext = ptr->next;
    data = ptr->data;

    free(ptr->name);
    free(ptr);

    *ptrholder = pnext;
    if (f)
	(*f)(data);

    set->count--;
    return 1;
}

int set_size(Set* set)
{
    if (set)
	return set->count;

    return 0;
}
