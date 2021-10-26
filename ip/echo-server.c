/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of and a contribution to the lwIP TCP/IP stack.
 *
 * Credits go to Adam Dunkels (and the current maintainers) of this software.
 *
 * Christiaan Simons rewrote this file to get a more stable echo example.
 */

/**
 * @file
 * TCP echo server example using raw API.
 *
 * Echos all bytes sent by connecting client,
 * and passively closes when client is done.
 *
 */


#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"
#include "raft_types.h"
#include "core/process.h"
#include "core/printf.h"
#include "core/time.h"
#include "core/initfunc.h"
#include "core/thread.h"
#include "core/types.h"

#if LWIP_TCP

char *raft_state = "Leader";
int term = 0;
int LeaderId = 0;
char *logmsg = "hello";
char log[100];
int LeaderCommit = 0;
int index = 0;

int SQLite_Result = 0;

static struct tcp_pcb *echo_pcb;

enum echo_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

struct echo_state
{
  u8_t state;
  u8_t retries;
  struct tcp_pcb *pcb;
  /* pbuf (chain) to recycle */
  struct pbuf *p;
};


static err_t echo_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t echo_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void echo_error(void *arg, err_t err);
static err_t echo_poll(void *arg, struct tcp_pcb *tpcb);
static err_t echo_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void echo_send(struct tcp_pcb *tpcb, struct echo_state *es);
static void echo_close(struct tcp_pcb *tpcb, struct echo_state *es);

static int
Result_handler(int m, int c)
{
SQLite_Result = 1;
return 0;
}

void
echo_server_init (int port)
{
  echo_pcb = tcp_new();

  
  if (echo_pcb != NULL)
  {
    err_t err;

    err = tcp_bind(echo_pcb, IP_ADDR_ANY, port);
    if (err == ERR_OK)
    {
      echo_pcb = tcp_listen(echo_pcb);
      tcp_accept(echo_pcb, echo_accept);
    }
    else 
    {
      /* abort? output diagnostic? */
    }
  }
  else
  {
    /* abort? output diagnostic? */
  }
}


static err_t
echo_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  struct echo_state *es;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  /* commonly observed practive to call tcp_setprio(), why? */
  tcp_setprio(newpcb, TCP_PRIO_MIN);
  

  es = (struct echo_state *)mem_malloc(sizeof(struct echo_state));

  if (es != NULL)
  {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    es->retries = 0;
    es->p = NULL;

    /* pass newly allocated es to our callbacks */
    tcp_arg(newpcb, es);
    tcp_recv(newpcb, echo_recv);
    tcp_err(newpcb, echo_error);
    tcp_poll(newpcb, echo_poll, 0);
    ret_err = ERR_OK;
  }
  else
  {
    ret_err = ERR_MEM;
  }
  return ret_err;  
}

static err_t
echo_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct echo_state *es;
  err_t ret_err;
  u64 start, end;

  LWIP_ASSERT("arg != NULL",arg != NULL);
  es = (struct echo_state *)arg;
  if (p == NULL)
  {
    /* remote host closed connection */
    es->state = ES_CLOSING;
    if(es->p == NULL)
    {
       /* we're done sending, close it */
       echo_close(tpcb, es);
    }
    else
    {
      /* we're not done yet */
      tcp_sent(tpcb, echo_sent);
      echo_send(tpcb, es);
    }
    ret_err = ERR_OK;
  }
  else if(err != ERR_OK)
  {
    /* cleanup, for unkown reason */
    if (p != NULL)
    {
      es->p = NULL;
      pbuf_free(p);
    }
    ret_err = err;
  }
  else if(es->state == ES_ACCEPTED)
  {
  
  /* first data chunk in p->payload */
    es->state = ES_RECEIVED;
    /* store reference to incoming pbuf (chain) */
    es->p = p;

    tcp_sent(tpcb, echo_sent);

  //  printf("%llu\n",get_cpu_time());
         /* char配列にTCPペイロードと長さをコピー */
      unsigned char str[255];
  //    printf("memset\n");
      memset(str, 0, 255);
//      printf("memcpy\n");
      memcpy(str, p -> payload, p -> len);
char *str2;
int start = 0;
int count = 0;
for(int i=0;str[i]!='\0';i++){
if(strcmp(",",&str[i],1)==0){
if(count == 0){
term = (unsigned char)str2;
start = 0;
count++;
i++;
}else if(count == 1){
LeaderId = (unsigned char)str2;
start = 0;
count++;
i++;
}else if(count == 2){
index = (unsigned char)str2;
start = 0;
count++;
i++;
}else if(count == 3){
logmsg = str2;
start = 0;
count++;
i++;
}
}else{
str2[start]=str[i];
start++;
}
}
printf("%d,%d,%d,%s\n",term,LeaderId,index,logmsg);
//      printf("str -> %s\n", str);
      /* メッセージバッファを用意 */
      struct msgbuf mbuf;
    //  printf("setmsgbuf\n");
      setmsgbuf(&mbuf, str, sizeof str,0);

      /* SQL実行文の送信 */
  int sqlite;
  sqlite = newprocess("sqliteexample2");

      //        printf("newprocess = %d\n",sqlite);
  //      printf("msgsendbuf\n");
//        start = get_cpu_time();
        msgsendbuf(sqlite, 0, &mbuf, 1);
//  	end = get_cpu_time();
//	printf("%lld\n", end-start);
   //    printf("msgsendbufend\n");
//	msgregister("SQLite_Result",Result_handler);
	msgclose(sqlite);
      //  printf("msgclosed\n");


    /* install send completion notifier */
//    tcp_sent(tpcb, echo_sent);
    echo_send(tpcb, es);
    ret_err = ERR_OK;
  }
  else if (es->state == ES_RECEIVED)
  {

    /* read some more data */
    if(es->p == NULL)
    {
      es->p = p;
  
      tcp_sent(tpcb, echo_sent);


     /* char配列にTCPペイロードと長さをコピー */
      unsigned char str[32];
//      printf("memset\n");
      memset(str, 0, 32);
//      printf("memcpy\n");
      memcpy(str, p -> payload, p -> len);
//      printf("str -> %s\n", str);
      /* メッセージバッファを用意 */
      struct msgbuf mbuf;
//      printf("setmsgbuf\n");
      setmsgbuf(&mbuf, str, sizeof str,0);

      /* SQL実行文の送信 */      
int sqlite;
       sqlite = newprocess("sqliteexample2");
//        printf("newprocess = %d\n",sqlite);
//	printf("msgsendbuf\n");
//        start = get_cpu_time();
        msgsendbuf(sqlite, 0, &mbuf, 1);
//        end = get_cpu_time();
//        printf("%lld\n", end-start);
//	msgsendbuf(sqlite, 0, &mbuf, 1);
//	printf("msgsendbufend\n");
	msgclose(sqlite);
//	printf("msgclosed\n");

        echo_send(tpcb, es);
      
    }
    else
    {
      struct pbuf *ptr;

      /* chain pbufs to the end of what we recv'ed previously  */
      ptr = es->p;
      pbuf_chain(ptr,p);
    }
    ret_err = ERR_OK;
  }
  else if(es->state == ES_CLOSING)
  {
    /* odd case, remote side closing twice, trash data */
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  else
  {
    /* unkown es->state, trash data  */
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

static void
echo_error(void *arg, err_t err)
{
  struct echo_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct echo_state *)arg;
  if (es != NULL)
  {
    mem_free(es);
  }
}

static err_t
echo_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct echo_state *es;

  es = (struct echo_state *)arg;
  if (es != NULL)
  {
    if (es->p != NULL)
    {
      /* there is a remaining pbuf (chain)  */
      tcp_sent(tpcb, echo_sent);
      echo_send(tpcb, es);
    }
    else
    {
      /* no remaining pbuf (chain)  */
      if(es->state == ES_CLOSING)
      {
        echo_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

static err_t
echo_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct echo_state *es;

  LWIP_UNUSED_ARG(len);

  es = (struct echo_state *)arg;
  es->retries = 0;
  
  if(es->p != NULL)
  {
    /* still got pbufs to send */
    tcp_sent(tpcb, echo_sent);
    echo_send(tpcb, es);
  }
  else
  {
    /* no more pbufs to send */
    if(es->state == ES_CLOSING)
    {
      echo_close(tpcb, es);
    }
  }
  return ERR_OK;
}

static void
echo_send(struct tcp_pcb *tpcb, struct echo_state *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;
//  err_t wr_err2 = ERR_OK;

  while ((wr_err == ERR_OK) &&
         (es->p != NULL) && 
         (es->p->len <= tcp_sndbuf(tpcb)))
  {
  ptr = es->p;
//if(SQLite_Result == 0){
  /* enqueue data for transmission */
//  ptr->payload = "SUCCESS";
//  ptr->len = 8;
  wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
//  wr_err = tcp_write(tpcb, "SUCCESS", 7, 1);
//}else{
//  ptr->payload = "FAILED";
//  ptr->len = 7;
//  wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
//  wr_err = tcp_write(tpcb, "FAILED", 6, 1);
//  SQLite_Result = 0;
//}
  if (wr_err == ERR_OK)
  {
     u16_t plen;
      u8_t freed;

     plen = ptr->len;
     /* continue with next pbuf in chain (if any) */
     es->p = ptr->next;
     if(es->p != NULL)
     {
       /* new reference! */
       pbuf_ref(es->p);
     }
     /* chop first pbuf from chain */
      do
      {
        /* try hard to free pbuf */
        freed = pbuf_free(ptr);
      }
      while(freed == 0);
     /* we can read more data now */
     tcp_recved(tpcb, plen);
   }
   else if(wr_err == ERR_MEM)
   {
      /* we are low on memory, try later / harder, defer to poll */
     es->p = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}

static void
echo_close(struct tcp_pcb *tpcb, struct echo_state *es)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  
  if (es != NULL)
  {
    mem_free(es);
  }  
  tcp_close(tpcb);
}
/*
static void heartbeat_thread (void *arg) { 
u64 start,cur;

start = get_time();

    for (;;) {  
	schedule();
      	cur = get_time();
	if(cur - start >100){
	printf("100\n");
	start = cur;
	}	
    }  
	thread_exit();
} 

static void heartbeat_kernel_init (void) { 
    thread_new (heartbeat_thread, NULL, VMM_STACKSIZE); 
}  

INITFUNC ("config1", heartbeat_kernel_init);
*/
#endif /* LWIP_TCP */
