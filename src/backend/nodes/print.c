/*-------------------------------------------------------------------------
 *
 * print.c
 *	  various print routines (used mostly for debugging)
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /cvsroot/pgsql/src/backend/nodes/print.c,v 1.47 2001/03/22 03:59:32 momjian Exp $
 *
 * HISTORY
 *	  AUTHOR			DATE			MAJOR EVENT
 *	  Andrew Yu			Oct 26, 1994	file creation
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/printtup.h"
#include "nodes/print.h"
#include "optimizer/clauses.h"
#include "parser/parsetree.h"
#include "utils/lsyscache.h"

static char *plannode_type(Plan *p);

/*
 * print
 *	  print contents of Node to stdout
 */
void
print(void *obj)
{
	char	   *s;

	s = nodeToString(obj);
	printf("%s\n", s);
	fflush(stdout);
}

/*
 * pretty print hack extraordinaire.  -ay 10/94
 */
void
pprint(void *obj)
{
	char	   *s;
	int			i;
	char		line[80];
	int			indentLev;
	int			j;

	s = nodeToString(obj);

	indentLev = 0;
	i = 0;
	for (;;)
	{
		for (j = 0; j < indentLev * 3; j++)
			line[j] = ' ';
		for (; j < 75 && s[i] != '\0'; i++, j++)
		{
			line[j] = s[i];
			switch (line[j])
			{
				case '}':
					if (j != indentLev * 3)
					{
						line[j] = '\0';
						printf("%s\n", line);
						line[indentLev * 3] = '\0';
						printf("%s}\n", line);
					}
					else
					{
						line[j] = '\0';
						printf("%s}\n", line);
					}
					indentLev--;
					j = indentLev * 3 - 1;		/* print the line before :
												 * and resets */
					break;
				case ')':
					line[j + 1] = '\0';
					printf("%s\n", line);
					j = indentLev * 3 - 1;
					break;
				case '{':
					indentLev++;
					/* !!! FALLS THROUGH */
				case ':':
					if (j != 0)
					{
						line[j] = '\0';
						printf("%s\n", line);
						/* print the line before : and resets */
						for (j = 0; j < indentLev * 3; j++)
							line[j] = ' ';
					}
					line[j] = s[i];
					break;
			}
		}
		line[j] = '\0';
		if (s[i] == '\0')
			break;
		printf("%s\n", line);
	}
	if (j != 0)
		printf("%s\n", line);
	fflush(stdout);
	return;
}

/*
 * print_rt
 *	  print contents of range table
 */
void
print_rt(List *rtable)
{
	List	   *l;
	int			i = 1;

	printf("resno\trelname(refname)\trelid\tinFromCl\n");
	printf("-----\t----------------\t-----\t--------\n");
	foreach(l, rtable)
	{
		RangeTblEntry *rte = lfirst(l);

		if (rte->relname)
			printf("%d\t%s (%s)\t%u",
				   i, rte->relname, rte->eref->relname, rte->relid);
		else
			printf("%d\t[subquery] (%s)\t",
				   i, rte->eref->relname);
		printf("\t%s\t%s\n",
			   (rte->inh ? "inh" : ""),
			   (rte->inFromCl ? "inFromCl" : ""));
		i++;
	}
}


/*
 * print_expr
 *	  print an expression
 */
void
print_expr(Node *expr, List *rtable)
{
	if (expr == NULL)
	{
		printf("<>");
		return;
	}

	if (IsA(expr, Var))
	{
		Var		   *var = (Var *) expr;
		char	   *relname,
				   *attname;

		switch (var->varno)
		{
			case INNER:
				relname = "INNER";
				attname = "?";
				break;
			case OUTER:
				relname = "OUTER";
				attname = "?";
				break;
			default:
				{
					RangeTblEntry *rte;

					Assert(var->varno > 0 &&
						   (int) var->varno <= length(rtable));
					rte = rt_fetch(var->varno, rtable);
					relname = rte->eref->relname;
					attname = get_rte_attribute_name(rte, var->varattno);
				}
				break;
		}
		printf("%s.%s", relname, attname);
	}
	else if (IsA(expr, Expr))
	{
		Expr	   *e = (Expr *) expr;

		if (is_opclause(expr))
		{
			char	   *opname;

			print_expr((Node *) get_leftop(e), rtable);
			opname = get_opname(((Oper *) e->oper)->opno);
			printf(" %s ", ((opname != NULL) ? opname : "(invalid operator)"));
			print_expr((Node *) get_rightop(e), rtable);
		}
		else
			printf("an expr");
	}
	else
		printf("not an expr");
}

/*
 * print_pathkeys -
 *	  pathkeys list of list of PathKeyItems
 */
void
print_pathkeys(List *pathkeys, List *rtable)
{
	List	   *i,
			   *k;

	printf("(");
	foreach(i, pathkeys)
	{
		List	   *pathkey = lfirst(i);

		printf("(");
		foreach(k, pathkey)
		{
			PathKeyItem *item = lfirst(k);

			print_expr(item->key, rtable);
			if (lnext(k))
				printf(", ");
		}
		printf(") ");
		if (lnext(i))
			printf(", ");
	}
	printf(")\n");
}

/*
 * print_tl
 *	  print targetlist in a more legible way.
 */
void
print_tl(List *tlist, List *rtable)
{
	List	   *tl;

	printf("(\n");
	foreach(tl, tlist)
	{
		TargetEntry *tle = lfirst(tl);

		printf("\t%d %s\t", tle->resdom->resno, tle->resdom->resname);
		if (tle->resdom->reskey != 0)
			printf("(%d):\t", tle->resdom->reskey);
		else
			printf("    :\t");
		print_expr(tle->expr, rtable);
		printf("\n");
	}
	printf(")\n");
}

/*
 * print_slot
 *	  print out the tuple with the given TupleTableSlot
 */
void
print_slot(TupleTableSlot *slot)
{
	if (!slot->val)
	{
		printf("tuple is null.\n");
		return;
	}
	if (!slot->ttc_tupleDescriptor)
	{
		printf("no tuple descriptor.\n");
		return;
	}

	debugtup(slot->val, slot->ttc_tupleDescriptor, NULL);
}

static char *
plannode_type(Plan *p)
{
	switch (nodeTag(p))
	{
			case T_Plan:
			return "PLAN";
		case T_Result:
			return "RESULT";
		case T_Append:
			return "APPEND";
		case T_Scan:
			return "SCAN";
		case T_SeqScan:
			return "SEQSCAN";
		case T_IndexScan:
			return "INDEXSCAN";
		case T_TidScan:
			return "TIDSCAN";
		case T_SubqueryScan:
			return "SUBQUERYSCAN";
		case T_Join:
			return "JOIN";
		case T_NestLoop:
			return "NESTLOOP";
		case T_MergeJoin:
			return "MERGEJOIN";
		case T_HashJoin:
			return "HASHJOIN";
		case T_Material:
			return "MATERIAL";
		case T_Sort:
			return "SORT";
		case T_Agg:
			return "AGG";
		case T_Unique:
			return "UNIQUE";
		case T_SetOp:
			return "SETOP";
		case T_Limit:
			return "LIMIT";
		case T_Hash:
			return "HASH";
		case T_Group:
			return "GROUP";
		default:
			return "UNKNOWN";
	}
}

/*
   prints the ascii description of the plan nodes
   does this recursively by doing a depth-first traversal of the
   plan tree.  for SeqScan and IndexScan, the name of the table is also
   printed out

*/
void
print_plan_recursive(Plan *p, Query *parsetree, int indentLevel, char *label)
{
	int			i;
	char		extraInfo[100];

	if (!p)
		return;
	for (i = 0; i < indentLevel; i++)
		printf(" ");
	printf("%s%s :c=%.2f..%.2f :r=%.0f :w=%d ", label, plannode_type(p),
		   p->startup_cost, p->total_cost,
		   p->plan_rows, p->plan_width);
	if (IsA(p, Scan) ||IsA(p, SeqScan))
	{
		RangeTblEntry *rte;

		rte = rt_fetch(((Scan *) p)->scanrelid, parsetree->rtable);
		StrNCpy(extraInfo, rte->relname, NAMEDATALEN);
	}
	else if (IsA(p, IndexScan))
	{
		RangeTblEntry *rte;

		rte = rt_fetch(((IndexScan *) p)->scan.scanrelid, parsetree->rtable);
		StrNCpy(extraInfo, rte->relname, NAMEDATALEN);
	}
	else
		extraInfo[0] = '\0';
	if (extraInfo[0] != '\0')
		printf(" ( %s )\n", extraInfo);
	else
		printf("\n");
	print_plan_recursive(p->lefttree, parsetree, indentLevel + 3, "l: ");
	print_plan_recursive(p->righttree, parsetree, indentLevel + 3, "r: ");

	if (IsA(p, Append))
	{
		List	   *lst;
		int			whichplan = 0;
		Append	   *appendplan = (Append *) p;

		foreach(lst, appendplan->appendplans)
		{
			Plan	   *subnode = (Plan *) lfirst(lst);

			/*
			 * I don't think we need to fiddle with the range table here,
			 * bjm
			 */
			print_plan_recursive(subnode, parsetree, indentLevel + 3, "a: ");

			whichplan++;
		}
	}
}

/* print_plan
  prints just the plan node types */

void
print_plan(Plan *p, Query *parsetree)
{
	print_plan_recursive(p, parsetree, 0, "");
}
