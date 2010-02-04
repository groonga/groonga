/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define __USE_XOPEN
#include "lib/ctx.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

/*
#define DEBUG_FTP
*/

#define FTPUSER "anonymous"
#define FTPPASSWD "grntest"
#define FTPSERVER "ftp.groonga.org"
#define FTPBUF 20000
#define DEFAULT_PORT 10041
#define DEFAULT_DEST "localhost"


static grn_critical_section grntest_cs;

static int grntest_stop_flag = 0;
static int grntest_detail_on = 0;
static int grntest_alloctimes = 0;
#define TMPFILE "_grntest.tmp"

FILE *grntest_logfp;

#define OS_LINUX64   "LINUX64"
#define OS_LINUX32   "LINUX32"
#define OS_WINDOWS64 "WINDOWS64"
#define OS_WINDOWS32 "WINDOWS32"

#ifdef WIN32
typedef SOCKET ftpsocket;
#define FTPERROR INVALID_SOCKET 
#define ftpclose closesocket
#define GROONGA_PATH "groonga.exe"
#else
typedef int ftpsocket;
#define ftpclose close
#define FTPERROR -1
#endif /* WIN32 */

char *grntest_osinfo;

#ifdef WIN32
#include <Windows.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#endif /* WIN32 */


grn_obj *grntest_db = NULL;

#define MAX_CON_JOB 10
#define MAX_CON 64

#define MAX_COMMAND 1024
#define BUF_LEN 1024
#define MAX_PATH_LEN 256
#define MAX_COMMAND_LEN 10000
#define LOGBUF_LEN 10000

#define J_DO_LOCAL  1  /* do_local */
#define J_DO_GQTP   2  /* do_gqtp */
#define J_REP_LOCAL 3  /* rep_local */
#define J_REP_GQTP  4  /* rep_gqtp */

static char grntest_username[BUF_LEN];
static char grntest_scriptname[BUF_LEN];
static char grntest_date[BUF_LEN];
static char grntest_serverhost[BUF_LEN];
static char grntest_log_tmpbuf[BUF_LEN];

struct commandtable {
  char *command[MAX_COMMAND];
  int num;
};

struct job {
  char jobname[BUF_LEN];
  char commandfile[BUF_LEN];
  int qnum;
  int jobtype;
  int concurrency;
  int ntimes;
  int done;
  long long int max;
  long long int min;
};

struct task {
  char *file;
  struct commandtable *table;
  int jobtype;
  int ntimes;
  int qnum;
  int job_id;
  long long int max;
  long long int min;
};

static struct task grntest_task[MAX_CON];
static struct job grntest_job[MAX_CON];
static int grntest_jobdone;
static int grntest_jobnum;
static grn_ctx grntest_ctx[MAX_CON];

grn_obj grntest_starttime, grntest_jobs_start;

static
int
grntest_atoi(const char *str, const char *end, const char **rest)
{
  while (grn_isspace(str, GRN_ENC_UTF8) == 1) {
    str++;
  }
  return grn_atoi(str, end, rest);
}
  
static
int
report_p(int jobtype)
{
  if (jobtype == J_REP_LOCAL) {
    return 1;
  }
  if (jobtype == J_REP_GQTP) {
    return 1;
  }
  return 0;
}

static
int
gqtp_p(int jobtype)
{
  if (jobtype == J_DO_GQTP) {
    return 1;
  }
  if (jobtype == J_REP_GQTP) {
    return 1;
  }
  return 0;
}

static
int
error_exit_in_thread(intptr_t code)
{
  fprintf(stderr, "Fatal error! Check script file!\n");
  fflush(stderr);
  CRITICAL_SECTION_ENTER(grntest_cs);
  grntest_stop_flag = 1;
  CRITICAL_SECTION_LEAVE(grntest_cs);
#ifdef WIN32
  _endthreadex(code);
#else
  pthread_exit((void *)code);
#endif /* WIN32 */
  return 0;
}


static
int
escape_command(unsigned char *in, int ilen,  unsigned char *out, int olen)
{
  int i = 0, j = 0;

  while (i < ilen) {
    if (j >= olen) {
      fprintf(stderr, "too many escapse:%s\n", in);
      exit(1);
    }

    if ((in[i] == '\\') || (in[i] == '\"') || (in[i] == '/') || (in[i] == '\b') ||
        (in[i] == '\f') || (in[i] == '\n') || (in[i] == '\r') || (in[i] == '\t')) {
      out[j] = 0x5C;
      j++;
      out[j] = in[i];
      j++;
      i++;
    } else {
      out[j] = in[i];
      j++;
      i++;
    }
  }

  out[j] = '\0';
  return j;
}

static
int
report_load_command(grn_ctx *ctx, char *ret, int task_id, long long int start_time, 
                    grn_obj *end_time)
{
  int i, len;
  long long int start, end;
  char rettmp[BUF_LEN];

  if ((ret[0] == '[') && (ret[1] == '[')) {
    i = 2;
    len = 1;
    while (ret[i] != ']') {
      i++;
      len++;
      if (ret[i] == '\0') {
        fprintf(stderr, "Error results:load\n");
        error_exit_in_thread(3);
      }
    }
    len++;
    strncpy(rettmp, &ret[1], len);
    rettmp[len] = '\0';
  } else {
    strcpy(rettmp, ret);
  }

  start = start_time - GRN_TIME_VALUE(&grntest_starttime);
  end = GRN_TIME_VALUE(end_time) - GRN_TIME_VALUE(&grntest_starttime);
  fprintf(grntest_logfp, "[%d, \"load\", %lld, %lld, %s],\n",  
          task_id,  start, end, rettmp);
  fflush(grntest_logfp);
  return 0;
}

static
int
report_command(grn_ctx *ctx, char *command, char *ret, int task_id, 
               grn_obj *start_time, grn_obj *end_time)
{
  int i, len, clen;
  long long int start, end;
  char rettmp[BUF_LEN];
  char command_escaped[MAX_COMMAND_LEN * 2];

  if ((ret[0] == '[') && (ret[1] == '[')) {
    i = 2;
    len = 1;
    while (ret[i] != ']') {
      i++;
      len++;
      if (ret[i] == '\0') {
        fprintf(stderr, "Error results:command=[%s]\n", command);
        error_exit_in_thread(3);
      }
    }
    len++;
    strncpy(rettmp, &ret[1], len);
    rettmp[len] = '\0';
  } else {
    strcpy(rettmp, ret);
  }

  start = GRN_TIME_VALUE(start_time) - GRN_TIME_VALUE(&grntest_starttime);
  end = GRN_TIME_VALUE(end_time) - GRN_TIME_VALUE(&grntest_starttime);
  clen = strlen(command);
  escape_command(command, clen, command_escaped, clen * 2);
  fprintf(grntest_logfp, "[%d, \"%s\", %lld, %lld, %s],\n",  
          task_id, command_escaped, start, end, rettmp);
  fflush(grntest_logfp);
  return 0;
}

static
int
output_result_final(grn_ctx *ctx, int qnum)
{
  grn_obj end_time;
  long long int latency, self;
  double sec, qps;

  GRN_TIME_INIT(&end_time, 0);
  GRN_TIME_NOW(ctx, &end_time);

  latency = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&grntest_starttime);
  self = latency;
  sec = self / (double)1000000;
  qps = (double)qnum / sec;
  fprintf(grntest_logfp, 
         "{\"total\": %lld, \"qps\": %f}]\n", latency, qps);
  grn_obj_close(ctx, &end_time);
  return 0;
}

static
int
output_sysinfo(char *sysinfo)
{
  fprintf(grntest_logfp, "[%s\n", sysinfo);
  return 0;
}
  

static
int
shutdown_server(grn_ctx *ctx)
{
  int ret;
  ret = grn_ctx_connect(ctx, grntest_serverhost, DEFAULT_PORT, 0);
  if (ret) {
    fprintf(stderr, "Cannot connect groonga server(shutdown):ret=%d\n", ret);
    exit(1);
  }
  grn_ctx_send(ctx, "shutdown", 8, 0);
  return 0;
}

static
int
error_command(grn_ctx *ctx, char *command, int task_id)
{
  fprintf(stderr, "error!:command=%s task_id = %d\n", command, task_id);
  fflush(stderr);
  error_exit_in_thread(1);
  return 0;
}

static
int
do_load_command(grn_ctx *ctx, char *command, int type, int task_id, 
                long long int *load_start)
{
  char *res;
  int res_len, flags, ret;
  grn_obj start_time, end_time;

  if (*load_start == 0) {
    GRN_TIME_INIT(&start_time, 0);
    GRN_TIME_NOW(ctx, &start_time);
    *load_start = GRN_TIME_VALUE(&start_time);
    grn_obj_close(ctx, &start_time);
  }

  grn_ctx_send(ctx, command, strlen(command), 0);
/* fix me. 
   when command fails, ctx->rc is not 0 in local mode!
  if (ctx->rc) {
    fprintf(stderr, "ctx_send:rc=%d:command:%s\n", ctx->rc, command);
    error_exit_in_thread(1);
  }
*/
  do {
    grn_ctx_recv(ctx, &res, &res_len, &flags);
    if (ctx->rc) {
      fprintf(stderr, "ctx_recv:rc=%d\n", ctx->rc);
      error_exit_in_thread(1);
      return 0;
    }
    if (res_len) {
      long long int self;
      GRN_TIME_INIT(&end_time, 0);
      GRN_TIME_NOW(ctx, &end_time);

      self = GRN_TIME_VALUE(&end_time) - *load_start;

      if (grntest_task[task_id].max < self) {
        grntest_task[task_id].max = self; 
      }
      if (grntest_task[task_id].min > self) {
        grntest_task[task_id].min = self;
      }

      if (report_p(grntest_task[task_id].jobtype)) {
        unsigned char tmpbuf[BUF_LEN];

        if (res_len < BUF_LEN) {
          strncpy(tmpbuf, res, res_len);
          tmpbuf[res_len] = '\0';
        } else {
          strncpy(tmpbuf, res, BUF_LEN - 2);
          tmpbuf[BUF_LEN -2] = '\0';
        }
        report_load_command(ctx, tmpbuf, task_id, *load_start, &end_time);
      }
      grn_obj_close(ctx, &end_time);
      ret = 1;
      break;
    } else {
      ret = 0;
      break;
    }
  } while ((flags & GRN_CTX_MORE));

  return ret;
}

static
int
do_command(grn_ctx *ctx, char *command, int type, int task_id)
{
  char *res;
  int res_len, flags;
  grn_obj start_time, end_time;

  GRN_TIME_INIT(&start_time, 0);
  GRN_TIME_NOW(ctx, &start_time);

  grn_ctx_send(ctx, command, strlen(command), 0);
/* fix me. 
   when command fails, ctx->rc is not 0 in local mode!
  if (ctx->rc) {
    fprintf(stderr, "ctx_send:rc=%d:command:%s\n", ctx->rc, command);
    error_exit_in_thread(1);
  }
*/

  do {
    grn_ctx_recv(ctx, &res, &res_len, &flags);
    if (ctx->rc) {
      fprintf(stderr, "ctx_recv:rc=%d\n", ctx->rc);
      error_exit_in_thread(1);
      return 0;
    }
    if (res_len) {
      long long int self;
      GRN_TIME_INIT(&end_time, 0);
      GRN_TIME_NOW(ctx, &end_time);

      self = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&start_time);

      if (grntest_task[task_id].max < self) {
        grntest_task[task_id].max = self; 
      }
      if (grntest_task[task_id].min > self) {
        grntest_task[task_id].min = self;
      }

      if (report_p(grntest_task[task_id].jobtype)) {
        unsigned char tmpbuf[BUF_LEN];

        if (res_len < BUF_LEN) {
          strncpy(tmpbuf, res, res_len);
          tmpbuf[res_len] = '\0';
        } else {
          strncpy(tmpbuf, res, BUF_LEN - 2);
          tmpbuf[BUF_LEN -2] = '\0';
        }
        report_command(ctx, command, tmpbuf, task_id, &start_time, &end_time);
      }
      grn_obj_close(ctx, &end_time);
      break;
    } else {
      error_command(ctx, command, task_id);
    }
  } while ((flags & GRN_CTX_MORE));

  grn_obj_close(ctx, &start_time);

  return 0;
}

static
int
comment_p(char *command)
{
  if (command[0] == '#') {
    return 1;
  }
  return 0;
}

static
int
load_command_p(char *command)
{
  int i = 0;

  while (grn_isspace(&command[i], GRN_ENC_UTF8) == 1) {
    i++;
  }
  if (command[i] == '\0') {
    return 0;
  }
  if (!strncmp(&command[i], "load", 4)) {
    return 1;
  }
  return 0;
}

static
int
worker_sub(intptr_t task_id)
{
  int i, load_mode;
  grn_obj end_time;
  long long int latency, self;
  double sec, qps;
  long long int load_start;

  grntest_task[task_id].max = 0LL;
  grntest_task[task_id].min = 9223372036854775807LL;
  grntest_task[task_id].qnum = 0;

  for (i = 0; i < grntest_task[task_id].ntimes; i++) {
    if (grntest_task[task_id].file != NULL) {
      FILE *fp;
      char tmpbuf[MAX_COMMAND_LEN];
      fp = fopen(grntest_task[task_id].file, "r");
      if (!fp) {
        fprintf(stderr, "Cannot open %s\n",grntest_task[task_id].file);
        error_exit_in_thread(1);
      } 
      tmpbuf[MAX_COMMAND_LEN-2] = '\0';
      load_mode = 0;
      load_start = 0LL;
      while (fgets(tmpbuf, MAX_COMMAND_LEN, fp) != NULL) {
        if (tmpbuf[MAX_COMMAND_LEN-2] != '\0') {
          fprintf(stderr, "Too long commmand in %s\n",grntest_task[task_id].file);
          error_exit_in_thread(1);
        }
        tmpbuf[strlen(tmpbuf)-1] = '\0';
        if (comment_p(tmpbuf)) {
          continue;
        }
        if (load_command_p(tmpbuf)) {
          load_mode = 1;
        }
        if (load_mode == 1) {
          if (do_load_command(&grntest_ctx[task_id], tmpbuf, 
                              grntest_task[task_id].jobtype,
                              task_id, &load_start)) {
            grntest_task[task_id].qnum++;
            load_mode = 0;
            load_start = 0LL;
          }
          continue;
        }
        do_command(&grntest_ctx[task_id], tmpbuf, 
                     grntest_task[task_id].jobtype,
                     task_id);
        grntest_task[task_id].qnum++;
      }
      fclose(fp);
    } else {
      int line;
      if (grntest_task[task_id].table == NULL) {
        fprintf(stderr, "Fatal error!:check script file!\n");
        error_exit_in_thread(1);
      }
      load_mode = 0;
      for (line = 0; line < grntest_task[task_id].table->num; line++) {
        if (load_command_p(grntest_task[task_id].table->command[line])) {
          load_mode = 1;
        }
        if (load_mode == 1) {
          if (do_load_command(&grntest_ctx[task_id], 
                              grntest_task[task_id].table->command[line], 
                              grntest_task[task_id].jobtype, task_id, &load_start)) {
            load_mode = 0;
            load_start = 0LL;
            grntest_task[task_id].qnum++;
          }
          continue;
        }
        do_command(&grntest_ctx[task_id], 
                   grntest_task[task_id].table->command[line], 
                   grntest_task[task_id].jobtype, task_id);
        grntest_task[task_id].qnum++;
      } 
    }
  }

  GRN_TIME_INIT(&end_time, 0);
  GRN_TIME_NOW(&grntest_ctx[task_id], &end_time);
  latency = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&grntest_starttime);
  self = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&grntest_jobs_start);

  CRITICAL_SECTION_ENTER(grntest_cs);
  if (grntest_job[grntest_task[task_id].job_id].max < grntest_task[task_id].max) {
    grntest_job[grntest_task[task_id].job_id].max = grntest_task[task_id].max;
  }
  if (grntest_job[grntest_task[task_id].job_id].min > grntest_task[task_id].min) {
    grntest_job[grntest_task[task_id].job_id].min = grntest_task[task_id].min;
  }

  grntest_job[grntest_task[task_id].job_id].qnum += grntest_task[task_id].qnum;
  grntest_job[grntest_task[task_id].job_id].done++;
  if (grntest_job[grntest_task[task_id].job_id].done == 
      grntest_job[grntest_task[task_id].job_id].concurrency) {
    char tmpbuf[BUF_LEN];
    sec = self / (double)1000000;
    qps = (double)grntest_job[grntest_task[task_id].job_id].qnum/ sec;
    grntest_jobdone++;
    sprintf(tmpbuf, 
            "{\"job\": \"%s\", \"latency\": %lld, \"self\": %lld, \"qps\": %f, \"min\": %lld, \"max\": %lld}",
            grntest_job[grntest_task[task_id].job_id].jobname, latency, self, qps,
            grntest_job[grntest_task[task_id].job_id].min,
            grntest_job[grntest_task[task_id].job_id].max);
    if (grntest_jobdone < grntest_jobnum) {
      strcat(tmpbuf, ",");
    }
    strcat(grntest_log_tmpbuf, tmpbuf);
    if (grntest_log_tmpbuf[LOGBUF_LEN - 2] != '\0') {
      error_exit_in_thread(1);
    }
    if (grntest_jobdone == grntest_jobnum) {
      if (grntest_detail_on) {
        fseek(grntest_logfp, -2, SEEK_CUR);
        fprintf(grntest_logfp, "], \n");
      }
      fprintf(grntest_logfp, "\"summary\" :[");
      fprintf(grntest_logfp, "%s", grntest_log_tmpbuf);
      fprintf(grntest_logfp, "]");
      fflush(grntest_logfp);
    }
  }
  grn_obj_close(&grntest_ctx[task_id], &end_time);
  CRITICAL_SECTION_LEAVE(grntest_cs);

  return 0;
}

#ifdef WIN32
static
int
__stdcall 
worker(void *val)
{
  worker_sub((intptr_t) val);
  return 0;
}
#else
static
void *
worker(void *val)
{
  worker_sub((intptr_t) val);
  return NULL;
}
#endif /* WIN32 */

#ifdef WIN32
int
thread_main(grn_ctx *ctx, int num)
{
  int  i;
  int  ret;
  HANDLE pthread[MAX_CON];

  for (i = 0; i < num; i++) {
    pthread[i] = (HANDLE)_beginthreadex(NULL, 0, worker, (void *)i,
                                        0, NULL);
    if (pthread[i]== (HANDLE)0) {
       fprintf(stderr, "thread failed:%d\n", i);
       error_exit_in_thread(1);
    }
  }

  ret = WaitForMultipleObjects(num, pthread, TRUE, INFINITE);
  if (ret == WAIT_TIMEOUT) {
     fprintf(stderr, "timeout\n");
     error_exit_in_thread(1);
  }

  for (i = 0; i < num; i++) {
    CloseHandle(pthread[i]);
  }
  return 0;
}
#else
int
thread_main(grn_ctx *ctx, int num)
{
  intptr_t i;
  int ret;
  pthread_t pthread[MAX_CON];

  for (i = 0; i < num; i++) {
    ret = pthread_create(&pthread[i], NULL, worker, (void *)i);
    if (ret) {
      fprintf(stderr, "Cannot create thread:ret=%d\n", ret);
      error_exit_in_thread(1);
    }
  }
  
  for (i = 0; i < num; i++) {
    ret = pthread_join(pthread[i], NULL);
    if (ret) {
      fprintf(stderr, "Cannot join thread:ret=%d\n", ret);
      error_exit_in_thread(1);
    }
  }
  return 0;
}
#endif

static
int
error_exit(grn_ctx *ctx, int ret)
{
  fflush(stderr);
  shutdown_server(ctx);
  grn_ctx_fin(ctx);
  grn_fin();
  exit(ret);
}

static
int
get_sysinfo(char *path, char *result, int olen)
{
  char tmpbuf[256];

#ifdef WIN32
  int cinfo[4];
  ULARGE_INTEGER dinfo;
  char cpustring[64];
  SYSTEM_INFO sinfo;
  MEMORYSTATUSEX minfo;
  OSVERSIONINFO osinfo;

  strcpy(result, "{");

  sprintf(tmpbuf, "\"script\": \"%s.scr\",\n", grntest_scriptname);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"user\": \"%s\",\n", grntest_username);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"date\": \"%s\",\n", grntest_date);
  strcat(result, tmpbuf);

  memset(cpustring, 0, 64);
  __cpuid(cinfo, 0x80000002);
  memcpy(cpustring, cinfo, 16);
  __cpuid(cinfo, 0x80000003);
  memcpy(cpustring+16, cinfo, 16);
  __cpuid(cinfo, 0x80000004);
  memcpy(cpustring+32, cinfo, 16);

  sprintf(tmpbuf, "  \"CPU\": \"%s\",\n", cpustring);
  strcat(result, tmpbuf);

  if (sizeof(int *) == 8 ) {
    grntest_osinfo = OS_WINDOWS64;
    sprintf(tmpbuf, "  \"BIT\": 64,\n");
    strcat(result, tmpbuf);
  } else {
    grntest_osinfo = OS_WINDOWS32;
    sprintf(tmpbuf, "  \"BIT\": 32,\n");
    strcat(result, tmpbuf);
  }

  GetSystemInfo(&sinfo);
  sprintf(tmpbuf, "  \"CORE\": %d,\n", sinfo.dwNumberOfProcessors);
  strcat(result, tmpbuf);

  minfo.dwLength = sizeof(minfo);
  GlobalMemoryStatusEx(&minfo);
  sprintf(tmpbuf, "  \"RAM\": \"%I64dMByte\",\n", 
          minfo.ullTotalPhys/(1024*1024));
  strcat(result, tmpbuf);

  GetDiskFreeSpaceEx(NULL, NULL, &dinfo, NULL);
  sprintf(tmpbuf, "  \"HDD\": \"%I64dKBytes\",\n", dinfo.QuadPart/1024 );
  strcat(result, tmpbuf);

  osinfo.dwOSVersionInfoSize = sizeof(osinfo); GetVersionEx(&osinfo);
  sprintf(tmpbuf, "  \"OS\": \"Windows %d.%d\",\n", osinfo.dwMajorVersion,
  osinfo.dwMinorVersion);
  strcat(result, tmpbuf);

  sprintf(tmpbuf, "  \"VERSION\": \"%s\"\n", grn_get_version());
  strcat(result, tmpbuf);
  strcat(result, "}");

#else /* linux only */
  FILE *fp;
  int ret;
  int cpunum = 0;
  int minfo = 0;
  char cpustring[256];
  struct utsname ubuf;
  struct statvfs vfsbuf;

  strcpy(result, "{");

  sprintf(tmpbuf, "\"script\": \"%s.scr\",\n", grntest_scriptname);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"user\": \"%s\",\n", grntest_username);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"date\": \"%s\",\n", grntest_date);
  strcat(result, tmpbuf);

  fp = fopen("/proc/cpuinfo", "r");
  if (!fp) {
    fprintf(stderr, "Cannot open cpuinfo\n");
    exit(1);
  }
  while (fgets(tmpbuf, 256, fp) != NULL) {
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    if (!strncmp(tmpbuf, "model name\t: ", 13)) {
      strcpy(cpustring, &tmpbuf[13]);
    }
    if (!strncmp(tmpbuf, "processor\t: ", 12)) {
      cpunum = grntest_atoi(&tmpbuf[12], &tmpbuf[12] + 20, NULL);
    }
  }
  fclose(fp);
  sprintf(tmpbuf, "  \"CPU\": %s\",\n", cpustring);
  strcat(result, tmpbuf);

  if (sizeof(int *) == 8 ) {
    grntest_osinfo = OS_LINUX64;
    sprintf(tmpbuf, "  \"BIT\": 64,\n");
    strcat(result, tmpbuf);
  } else {
    grntest_osinfo = OS_LINUX32;
    sprintf(tmpbuf, "  \"BIT\": 32,\n");
    strcat(result, tmpbuf);
  }

  sprintf(tmpbuf, "  \"CORE\": %d,\n", cpunum + 1);
  strcat(result, tmpbuf);

  fp = fopen("/proc/meminfo", "r");
  if (!fp) {
    fprintf(stderr, "Cannot open meminfo\n");
    exit(1);
  }
  while (fgets(tmpbuf, 256, fp) != NULL) {
    tmpbuf[strlen(tmpbuf)-1] = '\0';
    if (!strncmp(tmpbuf, "MemTotal:", 9)) {
      minfo = grntest_atoi(&tmpbuf[10], &tmpbuf[10] + 40, NULL);
    }
  }
  fclose(fp);
  sprintf(tmpbuf, "  \"RAM\": \"%dMBytes\",\n", minfo/1024);
  strcat(result, tmpbuf);

  ret = statvfs(path, &vfsbuf);
  if (ret) {
    fprintf(stderr, "Cannot access %s\n", path);
    exit(1);
  }

  sprintf(tmpbuf, "  \"HDD\": \"%ldKBytes\",\n", vfsbuf.f_blocks * 4);
  strcat(result, tmpbuf);

  uname(&ubuf);
  sprintf(tmpbuf, "  \"OS\": \"%s %s\",\n", ubuf.sysname, ubuf.release);
  strcat(result, tmpbuf);

  sprintf(tmpbuf, "  \"VERSION\": \"%s\"\n", grn_get_version());
  strcat(result, tmpbuf);

  strcat(result, "},");
#endif /* WIN32 */
  if (strlen(result) >= olen) {
    fprintf(stderr, "buffer overrun in get_sysinfo!\n");
    exit(1);
  }

  return 0;
}

static
int
start_server(char *dbpath, int r)
{
  int ret;
#ifdef WIN32
  char tmpbuf[BUF_LEN];

  PROCESS_INFORMATION pi;
  STARTUPINFO si;

  if (strlen(dbpath) > BUF_LEN - 100) {
    fprintf(stderr, "too long dbpath!\n");
    exit(1); 
  }

  strcpy(tmpbuf, GROONGA_PATH);
  strcat(tmpbuf, " -s ");
  strcat(tmpbuf, dbpath);
  memset(&si, 0, sizeof(si));
  si.cb=sizeof(si);
  ret = CreateProcess(NULL, tmpbuf, NULL, NULL, FALSE,
		      0, NULL, NULL, &si, &pi);

  if (ret == 0) {
    fprintf(stderr, "Cannot start groonga server:error=%d\n", GetLastError());
    exit(1);
  }

#else
  pid_t pid;
  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "Cannot start groonga server:Cannot fork\n");
    exit(1);
  }
  if (pid == 0) {
    ret = execlp("groonga", "groonga", "-s", dbpath, (char*)NULL);
    if (ret == -1) {
      fprintf(stderr, "Cannot start groonga server:errno=%d\n", errno);
      exit(1);
    }
  }
#endif /* WIN32 */ 

  return 0; 
}

static
int
parse_line(char *buf, int start, int end, int num)
{
  int i, j, flag = 0;
  char tmpbuf[BUF_LEN];

  grntest_job[num].concurrency = 1;
  grntest_job[num].ntimes = 1;
  grntest_job[num].done = 0;
  grntest_job[num].qnum = 0;
  grntest_job[num].max = 0LL;
  grntest_job[num].min = 9223372036854775807LL;

  strncpy(grntest_job[num].jobname, &buf[start], end - start);
  grntest_job[num].jobname[end - start] = '\0';
  i = start;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      i++;
      continue;
    }
    if (!strncmp(&buf[i], "do_local", 8)) {
      grntest_job[num].jobtype = J_DO_LOCAL;
      i = i + 8;
      break;
    }
    if (!strncmp(&buf[i], "do_gqtp", 7)) {
      grntest_job[num].jobtype = J_DO_GQTP;
      i = i + 7;
      break;
    }
    if (!strncmp(&buf[i], "rep_local", 9)) {
      grntest_job[num].jobtype = J_REP_LOCAL;
      i = i + 9;
      break;
    }
    if (!strncmp(&buf[i], "rep_gqtp", 8)) {
      grntest_job[num].jobtype = J_REP_GQTP;
      i = i + 8;
      break;
    }
    flag = 1;
    i++;
  }
  if (i == end) {
    if (flag) {
      return 3;
    }
    return 1;
  }

  if (grn_isspace(&buf[i], GRN_ENC_UTF8) != 1) {
    return 4;
  }
  i++;

  while (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
    i++;
    continue;
  }
  j = 0;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      break;
    }
    grntest_job[num].commandfile[j] = buf[i];
    i++;
    j++;
    if (j > 255) {
      return 5;
    }
  }
  grntest_job[num].commandfile[j] = '\0';
  if (i == end) {
   return 0;
  }
  while (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
    i++;
  }

  if (i == end) {
   return 0;
  }

  j = 0;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      break;
    }
    tmpbuf[j] = buf[i];
    i++;
    j++;
    if (j > 16) {
      return 6;
    }
  }
  tmpbuf[j] ='\0';
  grntest_job[num].concurrency = grntest_atoi(tmpbuf, tmpbuf + j, NULL);
  if (grntest_job[num].concurrency == 0) {
    return 7;
  }

  while (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
    i++;
  }
  if (i == end) {
   return 0;
  }

  j = 0;
  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      break;
    }
    tmpbuf[j] = buf[i];
    i++;
    j++;
    if (j > 16) {
      return 8;
    }
  }
  tmpbuf[j] ='\0';
  grntest_job[num].ntimes = grntest_atoi(tmpbuf, tmpbuf + j, NULL);
  if (grntest_job[num].ntimes == 0) {
    return 9;
  }
  if (i == end) {
    return 0;
  }

  while (i < end) {
    if (grn_isspace(&buf[i], GRN_ENC_UTF8) == 1) {
      i++;
      continue;
    }
    return 10;
  }
  return 0;
}

static
int
get_jobs(grn_ctx *ctx, char *buf, int line)
{
  int i, len, start, end, ret;
  int jnum = 0;

  len = strlen(buf);
  i = 0;
  while (i < len) {
   if ((buf[i] == '#') || (buf[i] == '\r') || (buf[i] == '\n')) {
     buf[i] = '\0';
     len = i;
     break;
   }
   i++;
  }

  i = 0;
  start = 0;
  while (i < len) {
    if (buf[i] == ';') {
      end = i;
      ret = parse_line(buf, start, end, jnum);
      if (ret) {
        if (ret > 1) {
          fprintf(stderr, "Syntax error:line=%d:ret=%d:%s\n", line, ret, buf);
          error_exit(ctx, 1);
        }
      } else {
        jnum++;
      }
      start = end + 1;
    }
    i++;
  }
  end = len;
  ret = parse_line(buf, start, end, jnum);
  if (ret) {
    if (ret > 1) {
      fprintf(stderr, "Syntax error:line=%d:ret=%d:%s\n", line, ret, buf);
      error_exit(ctx, 1);
    }
  } else {
    jnum++;
  }
  return jnum;
}

static
int
make_task_table(grn_ctx *ctx, int jobnum)
{
  int i, j, len, line;
  int tid = 0;
  FILE *fp;
  struct commandtable *ctable;
  char tmpbuf[MAX_COMMAND_LEN];

  for (i = 0; i < jobnum; i++) {
    if (grntest_job[i].concurrency == 1) {
      grntest_task[tid].file = grntest_job[i].commandfile;
      grntest_task[tid].table = NULL;
      grntest_task[tid].ntimes = grntest_job[i].ntimes;
      grntest_task[tid].jobtype = grntest_job[i].jobtype;
      grntest_task[tid].job_id = i;
      tid++;
      continue;
    }
    for (j = 0; j < grntest_job[i].concurrency; j++) {
      if (j == 0) {
        ctable = malloc(sizeof(struct commandtable));
        grntest_alloctimes++;
        if (!ctable) {
          fprintf(stderr, "Cannot alloc commandtable\n");
          error_exit(ctx, 1);
        }
        fp = fopen(grntest_job[i].commandfile, "r");
        if (!fp) {
          fprintf(stderr, "Cannot alloc commandfile:%s\n",
                   grntest_job[i].commandfile);
          error_exit(ctx, 1);
        }
        line = 0;
        tmpbuf[MAX_COMMAND_LEN-2] = '\0';
        while (fgets(tmpbuf, MAX_COMMAND_LEN, fp) != NULL) {
          if (tmpbuf[MAX_COMMAND_LEN-2] != '\0') {
            tmpbuf[MAX_COMMAND_LEN-2] = '\0';
            fprintf(stderr, "Too long command in %s\n", 
                   grntest_job[i].commandfile);
            fprintf(stderr, "line =%d:%s\n", line + 1, tmpbuf);
            error_exit(ctx, 1);
          }
          len = strlen(tmpbuf);
          len--;
          tmpbuf[len] = '\0';
          if (comment_p(tmpbuf)) {
            continue;
          }
          ctable->command[line] = strdup(tmpbuf);
          grntest_alloctimes++;
          if (ctable->command[line] == NULL) {
            fprintf(stderr, "Cannot alloc commandfile:%s\n",
                    grntest_job[i].commandfile);
            error_exit(ctx, 1);
          }
          ctable->num = line;
          line++;
          if (line >= MAX_COMMAND) {
            fprintf(stderr, "Too many commands in %s\n", 
                   grntest_job[i].commandfile);
            error_exit(ctx, 1);
          }
        }
        ctable->num = line;
      }
      grntest_task[tid].file = NULL;
      grntest_task[tid].table = ctable;
      grntest_task[tid].ntimes = grntest_job[i].ntimes;
      grntest_task[tid].jobtype = grntest_job[i].jobtype;
      grntest_task[tid].job_id = i;
      tid++;
    }
  }
  return tid;
}

/*
static
int
print_commandlist(int task_id)
{
  int i;

  for (i = 0; i < grntest_task[task_id].table->num; i++) {
    printf("%s\n", grntest_task[task_id].table->command[i]);
  }
  return 0;
}
*/

/* return num of query */
static
int
do_jobs(grn_ctx *ctx, int jobnum, int line)
{
  int i, j, task_num, ret, qnum = 0,thread_num = 0;

  for (i = 0; i < jobnum; i++) {
/*
printf("%d:type =%d:file=%s:con=%d:ntimes=%d\n", i, grntest_job[i].jobtype,
        grntest_job[i].commandfile, JobTable[i].concurrency, JobTable[i].ntimes);

*/
    thread_num = thread_num + grntest_job[i].concurrency;
  }

  if (thread_num >= MAX_CON) {
    fprintf(stderr, "Too many threads requested(MAX=64):line=%d\n", line);
    error_exit(ctx, 1);
  }

  task_num = make_task_table(ctx, jobnum);
  if (task_num != thread_num) {
    fprintf(stderr, "Logical error\n");
    error_exit(ctx, 9);
  }
  
  grntest_detail_on = 0;
  for (i = 0; i < task_num; i++) {
    grn_ctx_init(&grntest_ctx[i], 0);
    if (gqtp_p(grntest_task[i].jobtype)) {
      ret = grn_ctx_connect(&grntest_ctx[i], grntest_serverhost, DEFAULT_PORT, 0);
      if (ret) {
        fprintf(stderr, "Cannot connect groonga server:ret=%d\n", ret);
        error_exit(ctx, 1);
      }
    } else {
      grn_ctx_use(&grntest_ctx[i], grntest_db);
    }
    if (report_p(grntest_task[i].jobtype)) {
      grntest_detail_on++;
    }
  }
  if (grntest_detail_on) {
    fprintf(grntest_logfp, "\"detail\": [\n");
    fflush(grntest_logfp);
  }

  grntest_log_tmpbuf[0] = '\0';
  grntest_log_tmpbuf[LOGBUF_LEN-2] = '\0';
  thread_main(ctx, task_num);

  for (i = 0; i < task_num; i++) {
    grn_ctx_fin(&grntest_ctx[i]);
    qnum = qnum + grntest_task[i].qnum;
  }

  i = 0;
  while (i < task_num) {
    int job_id; 
    if (grntest_task[i].table != NULL) {
      job_id = grntest_task[i].job_id;
      for (j = 0; j < grntest_task[i].table->num; j++) {
        free(grntest_task[i].table->command[j]);
        grntest_alloctimes--;
      }
      free(grntest_task[i].table); 
      grntest_alloctimes--;
      while (job_id == grntest_task[i].job_id) {
        i++;
      }
    } else {
        i++;
    }
  }
  return qnum;
}

/* return num of query */
static
int
do_script(grn_ctx *ctx, char *sfile)
{
  int line = 0;
  int job_num;
  int qnum, qnum_total = 0;
  FILE *fp;
  char buf[BUF_LEN];

  fp = fopen(sfile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open script file:%s\n", sfile);
    error_exit(ctx, 1);
  }
  buf[BUF_LEN-2] = '\0';
  while (fgets(buf, BUF_LEN, fp) != NULL) {
    line++;
    if (buf[BUF_LEN-2] != '\0') {
      fprintf(stderr, "Too long line in script file:%d\n", line);
      error_exit(ctx, 1);
    }
    grntest_jobdone = 0;
    job_num = get_jobs(ctx, buf, line);
    grntest_jobnum = job_num;

    if (job_num > 0) {
      GRN_TIME_INIT(&grntest_jobs_start, 0);
      GRN_TIME_NOW(ctx, &grntest_jobs_start);
      fprintf(grntest_logfp, "{\"jobs\": \"%s\",\n", buf);
      qnum = do_jobs(ctx, job_num, line);
      fprintf(grntest_logfp, "},\n");
      qnum_total = qnum_total + qnum;

      grn_obj_close(ctx, &grntest_jobs_start);
    }
    if (grntest_stop_flag) {
      fprintf(stderr, "Error:Quit\n");
      break;
    }
  }
  fclose(fp);
  return qnum_total;
}

static
int
start_local(grn_ctx *ctx, char *dbpath)
{
  grntest_db = grn_db_open(ctx, dbpath);
  if (!grntest_db) {
    grntest_db = grn_db_create(ctx, dbpath, NULL);
  }
  if (!grntest_db) {
    fprintf(stderr, "Cannot open db:%s\n", dbpath);
    exit(1);
  }
  return 0;
}

static
int
check_server(grn_ctx *ctx)
{
  int ret, retry = 0;
  while (1) {
    ret = grn_ctx_connect(ctx, grntest_serverhost, DEFAULT_PORT, 0);
    if (ret == GRN_CONNECTION_REFUSED) {
      sleep(1);
      retry++;
      if (retry > 5) {
        fprintf(stderr, "Cannot connect groonga server(retry):ret=%d\n", 
                         ret);
        return 1;
      }
      continue;
    }
    if (ret) {
      fprintf(stderr, "Cannot connect groonga server:ret=%d\n", ret);
      return 1;
    }
    break;
  }
  return 0;
}

#define MODE_LIST 1
#define MODE_GET  2
#define MODE_PUT  3
#define MODE_TIME 4

static
int
check_response(char *buf)
{
  if (buf[0] == '1') {
    return 1;
  }
  if (buf[0] == '2') {
    return 1;
  }
  if (buf[0] == '3') {
    return 1;
  }
  return 0;
}

static
int
read_response(ftpsocket socket, char *buf)
{
  int ret;
  ret = recv(socket, buf, BUF_LEN - 1, 0);
  if (ret == -1) {
    fprintf(stderr, "recv error:3\n");
    exit(1);
  }
  buf[ret] ='\0';
#ifdef DEBUG_FTP
  fprintf(stderr, "recv:%s", buf);
#endif
  return ret;
}

static
int
put_file(ftpsocket socket, char *filename)
{
  FILE *fp;
  int c, ret, size = 0;
  char buf[1];

  fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "LOCAL:no such file:%s\n", filename);
    return 0;
  }

  while ((c = fgetc(fp)) != EOF) {
    buf[0] = c;
    ret = send(socket, buf, 1, 0);
    if (ret == -1) {
      fprintf(stderr, "send error\n");
      exit(1);
    }
    size++;
  }
  fclose(fp);
  return size;
}

static
int
ftp_list(ftpsocket data_socket)
{
  int ret;
  char buf[BUF_LEN];

  while (1) {
    ret = recv(data_socket, buf, BUF_LEN - 2, 0);
    if (ret == 0) {
      fflush(stdout);
      return 0;
    }
    buf[ret] = '\0';
    fprintf(stdout, "%s", buf);
  }
  
  return 0;
}

static
int
get_file(ftpsocket socket, char *filename, int size)
{
  FILE *fp;
  int ret, total;
  char buf[FTPBUF];

  fp = fopen(filename, "wb");
  if (!fp) {
    fprintf(stderr, "Cannot open %s\n",  filename);
    return -1;
  }

  total = 0;
  while (total != size) {
    ret = recv(socket, buf, FTPBUF, 0);
    if (ret == -1) {
      fprintf(stderr, "recv error:2:ret=%d:size=%d:total\n", ret, size);
      return -1;
    }
    if (ret == 0) {
      break;
    }
    fwrite(buf, ret, 1, fp);
    total = total + ret;
  }

  fclose(fp);
  return size;
}

static
int
write_to_server(ftpsocket socket, char *buf)
{
#ifdef DEBUG_FTP
  fprintf(stderr, "send:%s", buf);
#endif
  send(socket, buf, strlen(buf), 0);
  return 0;
}

static
int
get_port(char *buf, char *host, int *port)
{
  int ret,d1,d2,d3,d4,d5,d6;
  ret = sscanf(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
         &d1, &d2, &d3, &d4, &d5, &d6);
  if (ret != 6) {
    fprintf(stderr, "Cannot enter passsive mode\n");
    return 0;
  }

  *port = d5 * 256 + d6;
  sprintf(host, "%d.%d.%d.%d", d1, d2, d3, d4);
  return 1;
}

static
ftpsocket
open_socket(char *host, int port)
{
  ftpsocket sock;
  struct hostent *servhost;
  struct sockaddr_in server;
  u_long inaddr;
  int ret;

  servhost = gethostbyname(host);
  if (servhost == NULL){
    fprintf(stderr, "Bad hostname [%s]\n", host);
    return -1;
  }
  inaddr = *(u_long*)(servhost->h_addr_list[0]);

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr = *(struct in_addr*)&inaddr;
   
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    fprintf(stderr, "socket error\n");
    return -1;
  }
  ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
  if (ret == -1) {
    fprintf(stderr, "connect error\n");
    return -1;
  }
  return sock;
}

static
char *
get_ftp_date(char *buf)
{
  while (*buf !=' ') {
    buf++;
    if (*buf == '\0') {
      return NULL;
    }
  }
  buf++;
  
  return buf;
}

static
int
get_size(char *buf)
{
  int size;

  while (*buf !='(') {
    buf++;
    if (*buf == '\0') {
      return 0;
    }
  }
  buf++;
  size = grntest_atoi(buf, buf + strlen(buf), NULL);
  
  return size;
}

int
ftp_sub(char *user, char *passwd, char *host, char *filename, 
         int mode, char *cd_dirname, char *retval)
{
  int size = 0;
  int status = 0;
  ftpsocket command_socket, data_socket;
  int data_port;
  char data_host[BUF_LEN];
  char send_mesg[BUF_LEN]; 
  char buf[BUF_LEN];
#ifdef WIN32
  WSADATA ws;

  WSAStartup(MAKEWORD(2,0), &ws);
#endif /* WIN32 */

  if ((filename != NULL) && (strlen(filename) >= MAX_PATH_LEN)) {
    fprintf(stderr, "too long filename\n");
    exit(1);
  }

  if ((cd_dirname != NULL) && (strlen(cd_dirname) >= MAX_PATH_LEN)) {
    fprintf(stderr, "too long dirname\n");
    exit(1);
  }

  command_socket = open_socket(host, 21);
  if (command_socket == FTPERROR) {
    return 0;
  }

  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send username */
  sprintf(send_mesg, "USER %s\r\n", user);
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send passwd */
  sprintf(send_mesg, "PASS %s\r\n", passwd);
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send TYPE I */
  sprintf(send_mesg, "TYPE I\r\n");
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  /* send PASV */
  sprintf(send_mesg, "PASV\r\n");
  write_to_server(command_socket, send_mesg);
  read_response(command_socket, buf);
  if (!check_response(buf)) {
    goto exit;
  }

  if (!get_port(buf, data_host, &data_port)) {
    goto exit;
  }

  data_socket = open_socket(data_host, data_port);
  if (data_socket == FTPERROR) {
    goto exit;
  }

  if (cd_dirname) {
    sprintf(send_mesg, "CWD %s\r\n", cd_dirname);
    write_to_server(command_socket, send_mesg);
  }

  read_response(command_socket, buf);
  if (!check_response(buf)) {
    ftpclose(data_socket);
    goto exit;
  }

  switch (mode) {
    case MODE_LIST:
      sprintf(send_mesg, "LIST \r\n");
      write_to_server(command_socket, send_mesg);
      break;
    case MODE_PUT:
      sprintf(send_mesg, "STOR %s\r\n", filename);
      write_to_server(command_socket, send_mesg);
      break;
    case MODE_GET:
      sprintf(send_mesg, "RETR %s\r\n", filename);
      write_to_server(command_socket, send_mesg);
      break;
    case MODE_TIME:
      sprintf(send_mesg, "MDTM %s\r\n", filename);
      write_to_server(command_socket, send_mesg);
      break;
    default:
      fprintf(stderr, "invalid mode\n");
      ftpclose(data_socket);
      goto exit;
  }

  read_response(command_socket, buf);
  if (!check_response(buf)) {
    ftpclose(data_socket);
    goto exit;
  }
  if (!strncmp(buf, "150", 3)) {
    size = get_size(buf);
  }
  if (!strncmp(buf, "213", 3)) {
    retval[BUF_LEN-2] = '\0';
    strcpy(retval, get_ftp_date(buf));
    if (retval[BUF_LEN-2] != '\0' ) {
      fprintf(stderr, "buffer over run in ftp\n");
      exit(1);
    }
  }

  switch (mode) {
    case MODE_LIST:
      ftp_list(data_socket);
      break;
    case MODE_GET:
      if (get_file(data_socket, filename, size) == -1) {
        ftpclose(data_socket);
        goto exit;
      }
      fprintf(stderr, "get:%s\n", filename);
      break;
    case MODE_PUT:
      if (put_file(data_socket, filename) == -1) {
        ftpclose(data_socket);
        goto exit;
      }
      fprintf(stderr, "put:%s\n", filename);
      break;
    default:
      break;
  }

  ftpclose(data_socket);
  if ((mode == MODE_GET) || (mode == MODE_PUT)) {
    read_response(command_socket, buf);
  }
  write_to_server(command_socket, "QUIT\n");
  status = 1;
exit:
  ftpclose(command_socket);

#ifdef WIN32
  WSACleanup();
#endif
  return status;
}

static
int
ftp_main(int argc, char **argv)
{
  char val[BUF_LEN];
  val[0] = '\0';
  ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, argv[2], 
          grntest_atoi(argv[3], argv[3] + strlen(argv[3]), NULL), argv[4], val);
  if (val[0] != '\0') {
    printf("val=%s\n", val);
  }
  return 0;
}

static
int
get_username(char *name)
{
#ifdef WIN32
  strcpy(name, getenv("USERNAME"));
#else
  strcpy(name, getenv("USER"));
#endif
  return 0;
}

static
int
get_date(char *date, time_t *sec)
{
#ifdef WIN32
  struct tm tmbuf;
  struct tm *tm = &tmbuf;
  localtime_s(tm, sec);
#else
  struct tm *tm = localtime(sec);
#endif /* WIN32 */
#ifdef WIN32
  strftime(date, 128, "%Y-%m-%d %H:%M:%S", tm);
#else
  strftime(date, 128, "%F %T", tm);
#endif /* WIN32 */
  return 1;
}

static
int
get_scriptname(char *path, char *name, char *suffix)
{
  int slen = strlen(suffix);
  int len = strlen(path);

  if (len >= BUF_LEN) {
    fprintf(stderr, "too long script name\n");
    exit(1);
  }
  if (slen > len) {
    fprintf(stderr, "too long suffux\n");
    exit(1);
  }

  strcpy(name, path);
  if (strncmp(&name[len-slen], suffix, slen)) {
    name[0] = '\0';
    return 0;
  }
  name[len-slen] = '\0';
  return 1;
}

#ifdef WIN32
static
int
get_tm_from_serverdate(char *serverdate, struct tm *tm)
{
  int res;
  int year, month, day, hour, minute, second;

  res = sscanf(serverdate, "%4d%2d%2d%2d%2d%2d", 
               &year, &month, &day, &hour, &minute, &second);

/*
  printf("%d %d %d %d %d %d\n", year, month, day, hour, minute, second);
*/

  tm->tm_sec = second;
  tm->tm_min = minute;
  tm->tm_hour = hour;
  tm->tm_mday = day;
  tm->tm_mon = month - 1;
  tm->tm_year = year - 1900;
  tm->tm_isdst = -1;

  return 0;
}
#endif /* WIN32 */



static
int
sync_sub(grn_ctx *ctx, char *filename)
{
  int ret;
  char serverdate[BUF_LEN];
#ifdef WIN32
  struct _stat statbuf;
#else
  struct stat statbuf;
#endif /* WIN32 */
  time_t st, lt;
  struct tm stm;

  ret = ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, filename, MODE_TIME, "data", 
               serverdate);
  if (ret == 0) {
    fprintf(stderr, "[%s] does not exist in server\n", filename);
    return 0;
  }
#ifdef WIN32
  get_tm_from_serverdate(serverdate, &stm);
#else
  strptime(serverdate, "%Y%m%d %H%M%S", &stm);
#endif /* WIN32 */

  /* fixme! needs timezone info */
  st = mktime(&stm) + 3600 * 9;
  lt = st;

#ifdef WIN32
  ret = _stat(filename, &statbuf);
#else
  ret = stat(filename, &statbuf);
#endif /* WIN32 */

  if (!ret) {
    lt = statbuf.st_mtime;
    if (lt < st) {
      fprintf(stderr, "newer [%s] exists in server\n", filename);
      fflush(stderr);
      ret = ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, filename, MODE_GET, "data", 
                    NULL);
      return ret;
    }
  } else {
    fprintf(stderr, "[%s] does not exist in local\n", filename);
    fflush(stderr);
    ret = ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, filename, MODE_GET, "data", 
                  NULL);
    return ret;
  }
  return 0;
}

static
int
cache_file(char **flist, char *file, int fnum)
{
  int i;

  for (i = 0; i < fnum; i++) {
    if (!strcmp(flist[i], file) ) {
      return fnum;
    }
  }
  flist[fnum] = strdup(file);
  fnum++;
  if (fnum >= BUF_LEN) {
    fprintf(stderr, "too many uniq commands file!\n");
    exit(1);
  }
  return fnum;
}

static
int
sync_datafile(grn_ctx *ctx, char *sfile)
{
  int line = 0;
  int fnum = 0;
  int i, job_num;
  FILE *fp;
  char buf[BUF_LEN];
  char *filelist[BUF_LEN];

  fp = fopen(sfile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open script file:%s\n", sfile);
    error_exit(ctx, 1);
  }
  buf[BUF_LEN-2] = '\0';
  while (fgets(buf, BUF_LEN, fp) != NULL) {
    line++;
    if (buf[BUF_LEN-2] != '\0') {
      fprintf(stderr, "Too long line in script file:%d\n", line);
      error_exit(ctx, 1);
    }
    job_num = get_jobs(ctx, buf, line);

    if (job_num > 0) {
      for (i = 0; i < job_num; i++) {
/*
printf("commandfile=[%s]:buf=%s\n", grntest_job[i].commandfile, buf);
*/
        fnum = cache_file(filelist, grntest_job[i].commandfile, fnum);
      }
    }
  }
  for (i = 0; i < fnum; i++) {
    if (sync_sub(ctx, filelist[i])) {
      fprintf(stderr, "updated!:%s\n", filelist[i]);
      fflush(stderr);
    }
    free(filelist[i]);
  } 
  fclose(fp);
  return fnum;
}

static
int
sync_script(grn_ctx *ctx, char *filename)
{
  int ret, filenum;

  ret = sync_sub(ctx, filename);
  if (!ret) {
    return 0;
  }

  fprintf(stderr, "updated!:%s\n", filename);
  fflush(stderr);
  filenum = sync_datafile(ctx, filename);
  return 1;
}

int
main(int argc, char **argv)
{
  int qnum;
  grn_ctx context;
  char sysinfo[BUF_LEN];
  char log[BUF_LEN];
  time_t sec;
  int remote_mode = 0;

  if ((argc > 4) && (!strcmp(argv[1], "-ftp"))) {
    ftp_main(argc, argv);
    return 0;
  }

  if ((argc > 1) && (!strncmp(argv[1], "-dir", 4))) {
    ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, NULL, 1, "data", 
             NULL);
    return 0;
  }

  if (argc < 3) {
    fprintf(stderr, "Usage:%s script db [-dest <ip/hostname>]\n", argv[0]);
    fprintf(stderr, " Type %s -dir to get script list. \n", argv[0]);
    exit(1);
  }

  strcpy(grntest_serverhost, DEFAULT_DEST);
  if (argc > 4) {
    if (!strncmp(argv[3], "-dest", 5)) {
      strcpy(grntest_serverhost, argv[4]);
      remote_mode = 1;
    }
  }

  grn_init();
  CRITICAL_SECTION_INIT(grntest_cs);
  
  grn_ctx_init(&context, 0);
  grn_set_default_encoding(GRN_ENC_UTF8);

  start_local(&context, argv[2]);
  if (!remote_mode) {
    start_server(argv[2], 0);
  }

  if (check_server(&context)) {
    goto exit;
  }

  sync_script(&context, argv[1]);
  get_scriptname(argv[1], grntest_scriptname, ".scr");
  get_username(grntest_username);

  GRN_TIME_INIT(&grntest_starttime, 0);
  GRN_TIME_NOW(&context, &grntest_starttime);
  sec = (time_t)(GRN_TIME_VALUE(&grntest_starttime)/1000000);
  get_date(grntest_date, &sec);
  
  sprintf(log, "%s-%s-%lld.log", grntest_scriptname, 
          grntest_username, GRN_TIME_VALUE(&grntest_starttime));

  grntest_logfp = fopen(log, "w+b");
  if (!grntest_logfp) {
    fprintf(stderr, "Cannot open logfile:%s\n", log);
    shutdown_server(&context);
    goto exit;
  }

  get_sysinfo(argv[2], sysinfo, BUF_LEN);
  output_sysinfo(sysinfo);

  qnum = do_script(&context, argv[1]);
  output_result_final(&context, qnum);
  fclose(grntest_logfp);

  if (!remote_mode) {
    shutdown_server(&context);
  }

  ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, log, 3, 
          "report", NULL);
exit:
  CRITICAL_SECTION_FIN(grntest_cs);
  grn_obj_close(&context, &grntest_starttime);
  grn_obj_close(&context, grntest_db);
  grn_ctx_fin(&context);
  grn_fin();
/*
  fprintf(stderr, "grntest_alloctimes=%d\n", grntest_alloctimes);
*/
  return 0;
}
