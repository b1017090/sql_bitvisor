/*
 * Copyright (c) 2007, 2008 University of Tsukuba
 * Copyright (c) 2009 Igel Co., Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Tsukuba nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <time.h>
#include <lib_lineinput.h>
#include <lib_printf.h>
#include <lib_stdlib.h>
#include <lib_string.h>
#include <lib_syscalls.h>
#include <lib_storage_io.h>
#include "assert.h"
#include "sqlite3.h"
#include "vvfs.h"


struct stordata {
	int id;
	int dev;
	long long start;
	long long size;
	long long *lastblk[2];
};

struct revdata {
	long long size;
	v_rw_t *rw;
	void *rw_param;
	long long *lastblk[2];
};

static int waitd;
static char waitflag;

int
sqlite3_os_init (void)
{
	return SQLITE_OK;
}

int
sqlite3_os_end (void)
{
	return SQLITE_OK;
}

static int
callback (void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for(i = 0; i < argc; i++)
		printf ("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	printf ("\n");
	return 0;
}

static int
mem_rw (void *rw_param, void *rbuf, const void *wbuf, int nblk, long long lba)
{
	static char storage1[524288];
	static char storage2[524288];
	char *storage = rw_param ? storage2 : storage1;
	int size = rw_param ? sizeof storage2 : sizeof storage1;
	if (!nblk)
		return 0;	/* Flush */
	if (lba >= size >> 12)
		return -1;
	if (!rbuf && !wbuf && nblk == 1) /* Set last LBA */
		return 0;
	if (lba + nblk > size >> 12)
		nblk = (size >> 12) - lba;
	if (rbuf) {
		memcpy (rbuf, &storage[lba << 12], nblk << 12);
		return nblk;
	}
	if (wbuf) {
		memcpy (&storage[lba << 12], (void *)wbuf, nblk << 12);
		return nblk;
	}
	return -1;
}

static void
callback_rw (void *data, int len)
{
	int *p = data;
	if (len > 0)
		*p = 1;
	waitflag = 1;
}

static int
stor_rw (void *rw_param, void *rbuf, const void *wbuf, int nblk, long long lba)
{
	struct stordata *p = rw_param;
	if (!nblk)
		return 0;	/* Flush: not implemented */
	if (lba >= p->size)
		return -1;
	if (!rbuf && !wbuf && nblk == 1) { /* Set last LBA */
		if (lba + 1 + *p->lastblk[1] + 1 < p->size) {
			*p->lastblk[0] = lba;
			return 0;
		}
		return -1;
	}
	if (lba + nblk > p->size)
		nblk = p->size - lba;
	int ok = 0;
	if (rbuf) {
		waitflag = 0;
		if (storage_io_aread (p->id, p->dev, rbuf, nblk << 12,
				      (lba << 12) + p->start,
				      callback_rw, &ok) < 0)
			return -1;
	} else if (wbuf) {
		waitflag = 0;
		if (storage_io_awrite (p->id, p->dev, (void *)wbuf, nblk << 12,
				       (lba << 12) + p->start,
				       callback_rw, &ok) < 0)
			return -1;
	}
	if (rbuf || wbuf) {
		struct msgbuf mbuf;
		setmsgbuf (&mbuf, &waitflag, sizeof waitflag, 0);
		msgsendbuf (waitd, 0, &mbuf, 1);
		if (ok)
			return nblk;
	}
	return -1;
}

static int
rev_rw (void *rw_param, void *rbuf, const void *wbuf, int nblk, long long lba)
{
	struct revdata *r = rw_param;
	if (rbuf || wbuf) {
		for (int i = 0; i < nblk; i++) {
			long long r_lba = r->size - lba - nblk + i;
			void *r_rbuf = NULL;
			const void *r_wbuf = NULL;
			if (rbuf)
				r_rbuf = rbuf + ((nblk - i - 1) << 12);
			if (wbuf)
				r_wbuf = wbuf + ((nblk - i - 1) << 12);
			int ret = r->rw (r->rw_param, r_rbuf, r_wbuf, 1,
					 r_lba);
			if (ret < 0)
				return ret;
		}
		return nblk;
	}
	if (!rbuf && !wbuf && nblk == 1) { /* Set last LBA */
		if (lba + 1 + *r->lastblk[1] + 1 < r->size) {
			*r->lastblk[0] = lba;
			return 0;
		}
		return -1;
	}
	return r->rw (r->rw_param, rbuf, wbuf, nblk, lba);
}

static void
callback_size (void *data, long long size)
{
	long long *p = data;
	*p = size;
	waitflag = 1;
}

int
_start (int m, int c, struct msgbuf *buf, int bufcnt)
//sqltest(int a, char buf)
{
/*
if(bufcnt < 1){
exitprocess(0);
	return -1;
}*/
static char heap[1048576] __attribute__ ((aligned (8)));
	sqlite3_config (SQLITE_CONFIG_HEAP, heap, sizeof heap, 32);
	if (sqlite3_initialize () != SQLITE_OK) {
		printf ("sqlite3_initialize failed\n");
		return 1;
	}
	sqlite3_vfs_register (v_vfs(), 1);
	printf ("Memory(m) or storage_io(s)? ");
//	static char buf[256];
	int id = storage_io_init ();
	printf ("id %d\n", id);
	printf ("Number of devices %d\n", storage_io_get_num_devices (id));
	printf ("Device number? ");
//	buf[0]=1;
      //  lineinput (buf, sizeof buf);
	char *p;
	int dev = 1;
	printf("%d\n",dev);
	printf ("Start LBA? ");
	//buf[0]=500109310;
	//lineinput (buf, sizeof buf);
	long start = 500109310;
	printf("%ld\n",start);
	printf ("End LBA? ");
	//buf[0]=500117502;
	//lineinput (buf, sizeof buf);
	long end = 500117502;
	printf("%ld\n",end);
	if (end < start)
	printf ("Device %d LBA %ld-%ld Storage size ", dev, start, end);
	waitd = msgopen ("wait");
	if (waitd < 0) {
		printf ("msgopen \"wait\" failed\n");
		exitprocess (1);
		//return 1;
	}

	long long size = 0;
	waitflag = 0;
	if (storage_io_aget_size (id, dev, callback_size, &size) < 0) {
		printf ("storage_io_aget_size failed\n");
		exitprocess (1);
		//return 1;
	}

	struct msgbuf mbuf;
	setmsgbuf (&mbuf, &waitflag, sizeof waitflag, 0);
	msgsendbuf (waitd, 0, &mbuf, 1);
	printf ("%lld (may be incorrect)\n", size);
	//printf ("Continue(y/n)? ");
	//buf[0]="y";
	//lineinput (buf, sizeof buf);
		long long lastblk1 = 0;
		long long lastblk2 = 0;
		struct stordata s = {
			.id = id,
			.dev = dev,
			.start = start << 9,
			.size = (end - start + 1) >> 3,
			.lastblk = {
				&lastblk1,
				&lastblk2,
			},
		};
		struct cat_data *sc = cat_new (stor_rw, &s, 12);
		v_register ("a", 12, cat_rw, sc);
		struct revdata r = {
			.size = s.size,
			.rw = cat_rw,
			.rw_param = sc,
			.lastblk = {
				&lastblk2,
				&lastblk1,
			},
		};
		struct cat_data *rc = cat_new (rev_rw, &r, 12);
		v_register ("a-journal", 12, cat_rw, rc);


	static char *sqlbuf;
	sqlbuf = buf[0].base;
//        sqlbuf = buf;
	sqlite3 *db;
	int time,time2;
	time = msgopen("timecount");
	time2 = msgopen("timecount2");
	//	msgsendint(time,0);
	if (sqlite3_open ("a", &db) != SQLITE_OK) {
		printf ("Can't open database: %s\n", sqlite3_errmsg (db));
		sqlite3_close (db);
		return 1;
	}
		//static char buf[256];
		//printf ("sqliteexample> ");
		//lineinput (buf, 256);
//		if (!buf[0]) {
//			sqlite3_close (db);
//			return 0;
//		}
		char *zErrMsg = NULL;
                int rc2 = sqlite3_exec (db, "create table foo(no INT)", callback, 0, &zErrMsg);

//		u64 start1,end1;
//		start1=get_cpu_time();
//		for(int i=0;i<500;i++){
		msgsendint(time,0);
		rc2 = sqlite3_exec (db, sqlbuf, callback, 0, &zErrMsg);
		if (rc2 != SQLITE_OK) {
                printf ("SQL error: %s\n", zErrMsg);
                sqlite3_free (zErrMsg);
                }

		msgsendint(time2,0);
//		}
//		rc2 = sqlite3_exec (db, "COMMIT", callback, 0, &zErrMsg);
//		exitprocess(0);
//		msgsendint(time, 0);
//		if (rc2 != SQLITE_OK) {
//			printf ("SQL error: %s\n", zErrMsg);
//			sqlite3_free (zErrMsg);
//		}
		//exitprocess(0);
		msgclose(time);
		sqlite3_close(db);
//		end1=get_cpu_time();
//		printf("%lld\n",end1-start1);
		exitprocess(0);
		return 0;
//	exitprocess(0);
}

/*int
_start (int a1, int a2)
{
//	if(sqltest(a2)==0){
//	exitprocess(0);
	msgregister("sqlitemsg", sqltest);
	return 0;
//	}
}*/
