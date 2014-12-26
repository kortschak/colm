#include <colm/program.h>
#include <colm/struct.h>

#include <stdlib.h>
#include <string.h>

struct colm_tree *colm_get_global( Program *prg, long pos )
{
	return colm_struct_get_field( prg->global, pos );
}

static struct colm_struct *colm_struct_new_size( Program *prg, int size )
{
	size_t memsize = sizeof(struct colm_struct) + ( sizeof(Tree*) * size );
	struct colm_struct *item = (struct colm_struct*) malloc( memsize );
	memset( item, 0, memsize );

	if ( prg->heap.head == 0 ) {
		prg->heap.head = prg->heap.tail = item;
		item->prev = item->next = 0;
	}
	else {
		item->prev = prg->heap.tail;
		item->next = 0;
		prg->heap.tail->next = item;
		prg->heap.tail = item;
	}
	
	return item;
}

struct colm_struct *colm_struct_new( Program *prg, int id )
{
	struct colm_struct *s = colm_struct_new_size( prg, prg->rtd->selInfo[id].size );
	s->id = id;
	return s;
}

struct colm_struct *colm_struct_inbuilt( Program *prg, int size, void *destructor )
{
	struct colm_struct *s = colm_struct_new_size( prg, size );
	s->id = -1;
	return s;
}

void colm_struct_delete( Program *prg, Tree **sp, struct colm_struct *el )
{
	if ( el->id >= 0 ) { 
		short *t = prg->rtd->selInfo[el->id].trees;
		int i, len = prg->rtd->selInfo[el->id].treesLen;
		for ( i = 0; i < len; i++ ) {
			Tree *tree = colm_struct_get_field( el, t[i] );
			treeDownref( prg, sp, tree );
		}
	}
	free( el );
}