/*
 *  Copyright 2009-2011 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "bytecode.h"
#include "parsedata.h"
#include "fsmrun.h"
#include <iostream>
#include <assert.h>

using std::cout;

void TypeRef::resolve( ParseData *pd ) const
{
	/* Look for the production's associated region. */
	Namespace *nspace = nspaceQual->getQual( pd );

	if ( nspace == 0 )
		error(loc) << "do not have namespace for resolving reference" << endp;
	
	if ( repeatType == RepeatRepeat ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String repeatName( 128, "_repeat_%s", typeName.data );

	  	SymbolMapEl *inDict = nspace->symbolMap.find( repeatName );
	    if ( inDict == 0 )
			pd->makeRepeatProd( nspace, repeatName, nspaceQual, typeName );
	}
	else if ( repeatType == RepeatList ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String listName( 128, "_list_%s", typeName.data );

		SymbolMapEl *inDict = nspace->symbolMap.find( listName );
	    if ( inDict == 0 )
			pd->makeListProd( nspace, listName, nspaceQual, typeName );
	}
	else if ( repeatType == RepeatOpt ) {
		/* If the factor is an opt, create the opt element and link the factor
		 * to it. */
		String optName( 128, "_opt_%s", typeName.data );

		SymbolMapEl *inDict = nspace->symbolMap.find( optName );
	    if ( inDict == 0 )
			pd->makeOptProd( nspace, optName, nspaceQual, typeName );
	}
}

void LangTerm::resolve( ParseData *pd ) const
{
	/* FIXME: implementation missing here. */
	switch ( type ) {
		case ConstructType: {
			typeRef->resolve( pd );
			break;
		}
		case VarRefType:
		case MethodCallType:
		case NumberType:
		case StringType:
		case MatchType:
		case NewType:
		case TypeIdType:
		case SearchType:
		case NilType:
		case TrueType:
		case FalseType:
		case ParseType:
		case ParseStopType:
		case MakeTreeType:
		case MakeTokenType:
		case EmbedStringType:
			break;
	}
}

void LangVarRef::resolve( ParseData *pd ) const
{

}

void LangExpr::resolve( ParseData *pd ) const
{
	switch ( type ) {
		case BinaryType: {
			left->resolve( pd );
			right->resolve( pd );
			break;
		}
		case UnaryType: {
			right->resolve( pd );
			break;
		}
		case TermType: {
			term->resolve( pd );
			break;
		}
	}
}

void LangStmt::resolveAccumItems( ParseData *pd ) const
{
	/* Assign bind ids to the variables in the replacement. */
	for ( ReplItemList::Iter item = *accumText->list; item.lte(); item++ ) {
		varRef->resolve( pd );

		switch ( item->type ) {
		case ReplItem::FactorType:
			break;
		case ReplItem::InputText:
			break;
		case ReplItem::ExprType:
			item->expr->resolve( pd );
			break;
		}
	}
}

void LangStmt::resolve( ParseData *pd ) const
{
	switch ( type ) {
		case PrintType: 
		case PrintXMLACType:
		case PrintXMLType:
		case PrintStreamType: {
			/* Push the args backwards. */
			for ( ExprVect::Iter pex = exprPtrVect->last(); pex.gtb(); pex-- )
				(*pex)->resolve( pd );
			break;
		}
		case ExprType: {
			/* Evaluate the exrepssion, then pop it immediately. */
			expr->resolve( pd );
			break;
		}
		case IfType: {
			/* Evaluate the test. */
			expr->resolve( pd );

			/* Analyze the if true branch. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->resolve( pd );

			if ( elsePart != 0 )
				elsePart->resolve( pd );
			break;
		}
		case ElseType: {
			break;
		}
		case RejectType:
			break;
		case WhileType: {
			expr->resolve( pd );

			/* Compute the while block. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->resolve( pd );
			break;
		}
		case AssignType: {
			/* Evaluate the exrepssion. */
			expr->resolve( pd );
			break;
		}
		case ForIterType: {
			/* Evaluate and push the arguments. */
			langTerm->resolve( pd );

			/* Compile the contents. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->resolve( pd );

			break;
		}
		case ReturnType: {
			/* Evaluate the exrepssion. */
			expr->resolve( pd );
			break;
		}
		case BreakType: {
			break;
		}
		case YieldType: {
			/* take a reference and yield it. Immediately reset the referece. */
			varRef->resolve( pd );
			break;
		}
		case AccumType: {
			//for ( )
			break;
		}
	}
}

void CodeBlock::resolve( ParseData *pd ) const
{
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->resolve( pd );
}

void ParseData::resolveFunction( Function *func )
{
	CodeBlock *block = func->codeBlock;
	block->resolve( this );
}

void ParseData::resolveUserIter( Function *func )
{
	CodeBlock *block = func->codeBlock;
	block->resolve( this );
}

void ParseData::resolvePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;
	block->resolve( this );
}

void ParseData::resolveRootBlock()
{
	CodeBlock *block = rootCodeBlock;
	block->resolve( this );
}

void ParseData::resolveTranslateBlock( KlangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;
	block->resolve( this );
}

void ParseData::resolveReductionCode( Definition *prod )
{
	CodeBlock *block = prod->redBlock;
	block->resolve( this );
}

void ParseData::resolveParseTree()
{
	/* Compile functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ ) {
		if ( f->isUserIter )
			resolveUserIter( f );
		else
			resolveFunction( f );
	}

	/* Compile the reduction code. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			resolveReductionCode( prod );
	}

	/* Compile the token translation code. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 )
			resolveTranslateBlock( lel );
	}

	/* Compile preeof blocks. */
	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			resolvePreEof( r );
	}

	/* Compile the init code */
	resolveRootBlock( );
}
