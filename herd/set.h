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

#ifndef HERD_SET_H
#define HERD_SET_H

#include "mce.h"

typedef struct herd_set Set;


/*
 * Set allocator
 */
Set* set_new(u32 initial_size);

/*
 * Set deallocator
 */
void set_delete(Set *set, void (*f)(void*));

/*
 * Add one element to set. Return new number of elements in the set,
 * or -1 if an error occured.
 */
int set_add(Set *set, const char *name, void *data);

/*
 * Get one element in set. Return true (non-zero) if element was
 * found. If data pointer is non null, write found data element to it.
 */
#ifdef WIN32
int set_lookup(Set *set, const char *name, void *data);
#else /* WIN32 */
int set_lookup(Set *set, const char *name, void **data);
#endif /* WIN32 */
/*
 * Remove element of given name. Return true (non-zero) if element was
 * found and deleted.
 */
int set_remove(Set *set, const char *name, void (*f)(void*));

/*
 * Return number of elements in set
 */
int set_size(Set *set);

#endif
