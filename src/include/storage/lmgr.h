/*-------------------------------------------------------------------------
 *
 * lmgr.h
 *	  POSTGRES lock manager definitions.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: lmgr.h,v 1.30 2001/03/22 04:01:07 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef LMGR_H
#define LMGR_H

#include "storage/lock.h"
#include "utils/rel.h"

/* These are the valid values of type LOCKMODE: */

/* NoLock is not a lock mode, but a flag value meaning "don't get a lock" */
#define NoLock					0

#define AccessShareLock			1		/* SELECT */
#define RowShareLock			2		/* SELECT FOR UPDATE */
#define RowExclusiveLock		3		/* INSERT, UPDATE, DELETE */
#define ShareLock				4		/* CREATE INDEX */
#define ShareRowExclusiveLock	5		/* like EXCLUSIVE MODE, allows
										 * SHARE ROW */
#define ExclusiveLock			6		/* blocks ROW SHARE/SELECT...FOR
										 * UPDATE */
#define AccessExclusiveLock		7		/* ALTER TABLE, DROP TABLE,
										 * VACUUM, and unqualified LOCK
										 * TABLE */
extern LOCKMETHOD LockTableId;


extern LOCKMETHOD InitLockTable(int maxBackends);
extern void RelationInitLockInfo(Relation relation);

/* Lock a relation */
extern void LockRelation(Relation relation, LOCKMODE lockmode);
extern void UnlockRelation(Relation relation, LOCKMODE lockmode);

extern void LockRelationForSession(LockRelId *relid, LOCKMODE lockmode);
extern void UnlockRelationForSession(LockRelId *relid, LOCKMODE lockmode);

/* Lock a page (mainly used for indices) */
extern void LockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode);
extern void UnlockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode);

/* Lock an XID (used to wait for a transaction to finish) */
extern void XactLockTableInsert(TransactionId xid);
extern void XactLockTableWait(TransactionId xid);

#endif	 /* LMGR_H */
