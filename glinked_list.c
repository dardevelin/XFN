/*
 * glinked_list.c
 * This file is part of dbsglinked
 *
 * Copyright (C) 2012 - Darcy Brás da Silva
 *
 * dbsglinked is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * dbsglinked is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dbsglinked. If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "glinked_list.h"

void libglinked_init_list(libglinked_list_t *list, 
    void *(*node_allocator)(size_t), void(*node_deallocator)(void*))
{
	list->count = 0;
	if(node_allocator != NULL)
		list->node_allocator = node_allocator;
	else
		list->node_allocator = LIBGLINKED_DEFAULT_ALLOCATOR;
	
	if(node_deallocator != NULL)
		list->node_deallocator = node_deallocator;
	else
		list->node_deallocator = LIBGLINKED_DEFAULT_DEALLOCATOR;
	
	list->head = NULL;
	return;
}

libglinked_node_t *libglinked_create_node(libglinked_list_t *list,
    void *data, void(*dalloc)(void*))
{
	libglinked_node_t *ptrnode;
	
	if (data == NULL )
		return NULL;

	if( (ptrnode = list->node_allocator( sizeof(libglinked_node_t)))==NULL )
	{
		fprintf(stderr,"libglinked_create_node malloc failed to alloc node_t\n");
		return NULL;
	}

	ptrnode->data = data;
	ptrnode->data_deallocator = dalloc;
	ptrnode->next = NULL;

	return ptrnode;
}

void libglinked_delete_node(libglinked_list_t *list, libglinked_node_t *node)
{
	if( node != NULL )
	{
		if( node->data != NULL && node->data_deallocator != NULL )
			node->data_deallocator(node->data);
		
		list->node_deallocator(node);
		return;
	}
}

libglinked_node_t *libglinked_enqueue_node(libglinked_list_t *list,
    libglinked_node_t *node)
{
	libglinked_node_t *ptrnode = list->head;
	
	if( node == NULL )
		return NULL; // nothing to enqueue
	
	if( list->head == NULL )
	{
		list->head = node;
		list->count++;
		return node;
	}

	while(ptrnode->next != NULL )
		ptrnode = ptrnode->next;

	ptrnode->next = node;
	list->count++;
	return ptrnode;
}

libglinked_node_t *libglinked_push_node(libglinked_list_t *list,
    libglinked_node_t *node)
{

	if( node == NULL )
		return NULL; //nothing to push
	
	if( list->head == NULL )
	{
		list->head = node;
		list->count++;
		return node;
	}

	node->next = list->head;
	list->head = node;
	list->count++;

	return node;
}

libglinked_node_t *libglinked_dequeue_node(libglinked_list_t *list)
{
	libglinked_node_t *ptrnode = list->head;
	
	if( list->head == NULL )
		return NULL; // nothing to dequeue

	list->head = list->head->next;
	list->count--;
	return ptrnode;
}

libglinked_node_t *libglinked_pop_node(libglinked_list_t *list)
{
	libglinked_node_t *ptrnode = list->head;

	if( list->head == NULL )
		return NULL; // nothing to pop
	
	list->head = list->head->next;
	list->count--;
	return ptrnode;
}

void libglinked_delete_list(libglinked_list_t *list )
{
	while(list->head != NULL )
	{
		libglinked_delete_node(list,libglinked_dequeue_node(list));
	}
	
}

size_t libglinked_get_num_items(libglinked_list_t *list)
{
	return list->count;
}

void libglinked_show_node(libglinked_node_t *node, void(*f)(void*))
{
	f(node->data);
}

void libglinked_show_list(libglinked_list_t *list, void(*f)(void*))
{
	libglinked_node_t *ptrnode = list->head;

	while(ptrnode != NULL )
	{
		libglinked_show_node(ptrnode,f);
		ptrnode = ptrnode->next;
	}
}

void libglinked_reverse_list(libglinked_list_t *list)
{
	libglinked_node_t *head = list->head,
		              *prev = NULL, 
		              *next = NULL;

    while( head )
    {
        //set head as last node
        list->head = head;
        //save the next ptr in PtrNext
        next = head->next;
        //change next to previous
        head->next = prev;
        //set current as previous
        prev = head;
        //set current as Next
        head = next;
    }
    
    next = list->head;
    list->head = next;
}

void *libglinked_foreach_node(libglinked_list_t * list, void * state,
    void *(*action)(void * ,void *, void *),
    void * param)
{
	libglinked_node_t *ptrnode;
	void *action_ret = NULL;

	if(action == NULL)
		return NULL; // no action to run, fail


	for(ptrnode = list->head; ptrnode != NULL; ptrnode = ptrnode->next)
	{
		action_ret = action(state, ptrnode->data, param);
	}

	return action_ret;
}

libglinked_node_t *libglinked_find_node(libglinked_list_t *list, void *key,
    bool(*cmp)(void *, void*))
{
	libglinked_node_t *ptrnode;

	if(cmp == NULL)
		return NULL; //no cmp to run, fail

	for(ptrnode = list->head; ptrnode != NULL; ptrnode = ptrnode->next)
	{
		if(true == cmp(ptrnode->data, key))
			break;
	}

	return ptrnode;
}

libglinked_node_t *libglinked_remove_node(libglinked_list_t *list, void *key,
    bool(*cmp)(void *, void *))
{
	libglinked_node_t *ptrnode;
	libglinked_node_t *ret = NULL;
	
    //check if the list isn't empty
	if(list->head == NULL)
		return NULL;

	if(cmp == NULL)
		return NULL; //no cmp to trigger, fail

	//check if is the first node
	ptrnode = list->head;
	if( true == cmp(ptrnode->data, key) )
	{
		ret = list->head;
		list->head = list->head->next;
		if( (list->count - 1) >= 0 )
			--list->count;
		
		return ret;
	}
	
	for(ptrnode = list->head; ptrnode->next != NULL; ptrnode = ptrnode->next)
	{
		if(true == cmp(ptrnode->next->data, key))
			break;
	}

	if(ptrnode->next == NULL)
		return NULL; //not found
	
	// save the node that we are removing
	ret = ptrnode->next;
	// keep the list linked
	ptrnode->next = ret->next;
	list->count--;
	return ret;
}

libglinked_list_t *libglinked_split_list(libglinked_list_t *list, 
    libglinked_list_t *nlist, void *key,
	bool(*cmp)(void *,void*))
{
	libglinked_node_t *ptrnode;
	size_t i=0;	

	if(list->count < 2)
		return NULL; // no possible split, fail

	if(cmp == NULL)
		return NULL; //no action to trigger, fail
	
	for(ptrnode=list->head; ptrnode->next != NULL; ptrnode=ptrnode->next,i++)
	{
		if(true == cmp(ptrnode->next->data, key))
			break;
	}

	if(ptrnode->next == NULL)
		return NULL; //not found

	libglinked_init_list(nlist, list->node_allocator, list->node_deallocator);
	nlist->head  = ptrnode->next;
	nlist->count = list->count - i-1;
	list->count  = i+1;
	ptrnode->next = NULL;
	
	return nlist;
}

static void libglinked_invalidate_list(libglinked_list_t *list)
{
	list->head = NULL;
	list->count = 0;
	list->node_allocator = NULL;
	list->node_deallocator = NULL;
}

libglinked_list_t *libglinked_join_list(libglinked_list_t *list,
    libglinked_list_t *slist, void *key, bool(*cmp)(void *,void*))
{
	libglinked_node_t *ptrnode;
	libglinked_node_t *sptrnode;
	bool end_join = false;

	if(slist->head == NULL)
		return NULL; // second list empty, nothing to join

	if(key == NULL && cmp == NULL)
		end_join = true;

	if( (key == NULL && cmp != NULL) || (cmp == NULL && key != NULL ) )
		return NULL; //invalid option, or both null, or both !null, fail

	if(list->head == NULL)
	{
		list = slist;
		libglinked_invalidate_list(slist);
	}

	if(list->node_allocator != slist->node_allocator
	   || list->node_deallocator != slist->node_deallocator)
		return NULL; //incompatible lists, fail

	if(end_join == true)
	{
		//get to the end of list
		for(ptrnode=list->head; ptrnode->next != NULL; ptrnode=ptrnode->next)
			;

		//append second list
		ptrnode->next = slist->head;
		//update items count
		list->count += slist->count;
		//invalidate slist
		libglinked_invalidate_list(slist);
		
		return list;
	}

	//set ptrnode to key position
	for(ptrnode=list->head; ptrnode->next != NULL; ptrnode=ptrnode->next)
	{
		if(true == cmp(ptrnode->data, key))
			break;
	}

	//got to end of slist
	for(sptrnode=slist->head; sptrnode->next != NULL; sptrnode=sptrnode->next)
		;

	//join second half of list to the end of slist
	sptrnode->next = ptrnode->next;
	//link the first half of list to the second list
	ptrnode->next = slist->head;

	//update list count, 
	list->count += slist->count;

	libglinked_invalidate_list(slist);

	return list;
}
