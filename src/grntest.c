#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "lib/ctx.h"
#include <time.h>
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


static grn_critical_section CS;

static int STOP_FLAG = 0;
static int Detail_On = 0;
#define TMPFILE "_grntest.tmp"

FILE *LogFp;

#define OS_LINUX64   "LINUX64"
#define OS_LINUX32   "LINUX32"
#define OS_WINDOWS64 "WINDOWS64"
#define OS_WINDOWS32 "WINDOWS32"

#ifdef WIN32
typedef SOCKET ftpsocket;
#define FTPERROR INVALID_SOCKET 
#define ftpclose closesocket
#define GROONGA_PATH "c:\\Windows\\System32\\groonga.exe"
#else
typedef int ftpsocket;
#define ftpclose close
#define FTPERROR -1
#endif /* WIN32 */

char *OSInfo;

#ifdef WIN32
#include <Windows.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/timeb.h>
//#pragma comment(lib, "../../groonga/lib/libgroonga.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#endif /* WIN32 */

FILE *LOG_FP;

grn_obj *DB = NULL;

#define MAX_CON_JOB 10
#define MAX_CON 64

#define MAX_COMMAND 1024
#define BUF_LEN 1024
#define MAX_COMMAND_LEN 10000

#define J_DO_LOCAL  1  /* do_local */
#define J_DO_GQTP   2  /* do_gqtp */
#define J_REP_LOCAL 3  /* rep_local */
#define J_REP_GQTP  4  /* rep_gqtp */

static char UserName[BUF_LEN];
static char ScriptName[BUF_LEN];
static char Date[BUF_LEN];
static char ServerHost[BUF_LEN];

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

static struct task TaskTable[MAX_CON];
static struct job JobTable[MAX_CON];
static int JobDoneNum;
static int JobNum;
static grn_ctx CtxTable[MAX_CON];

grn_obj StartTime, Jobs_Start;

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
report_command(grn_ctx *ctx, char *command, char *ret, int task_id, 
               grn_obj *start_time, grn_obj *end_time)
{
  int i;
  long long int start, end;
  int hash_ret = 0; 
  char rettmp[BUF_LEN];

  if (ret[0] == '[') {
    i = 1;
    while (ret[i] == '[') {
     i++;
    }
    strcpy(rettmp, &ret[i]);
    i = 0;
    while (rettmp[i] != '\0') {
      if (rettmp[i] == ']') {
        rettmp[i] = '\0';
        break;
      }
      i++;
    }
  }
  else {
    if (ret[0] == '{') {
      hash_ret = 1;
    }
    strcpy(rettmp, ret);
  }

  start = GRN_TIME_VALUE(start_time) - GRN_TIME_VALUE(&StartTime);
  end = GRN_TIME_VALUE(end_time) - GRN_TIME_VALUE(&StartTime);
  if (hash_ret) {
    fprintf(LogFp, "[%d, \"%s\", %lld, %lld, %s], \n",  
            task_id, command, start, end, rettmp);
  }
  else {
    fprintf(LogFp, "[%d, \"%s\", %lld, %lld, \"%s\"], \n",  
            task_id, command, start, end, rettmp);
  }
  fflush(LogFp);
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

  latency = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&StartTime);
  self = latency;
  sec = self / (double)1000000;
  qps = (double)qnum / sec;
  fprintf(LogFp, 
         "{\"total\": %lld, \"qps\": %f}]\n", latency, qps);
  grn_obj_close(ctx, &end_time);
  return 0;
}

static
int
output_sysinfo(char *sysinfo)
{
  fprintf(LogFp, "[%s\n", sysinfo);
  return 0;
}
  
static
int
error_exit_in_thread(intptr_t code)
{
  fprintf(stderr, "Fatal error! Check script file!\n");
  fflush(stderr);
  CRITICAL_SECTION_ENTER(CS);
  STOP_FLAG = 1;
  CRITICAL_SECTION_LEAVE(CS);
#ifdef WIN32
  _endthreadex(code);
#else
  pthread_exit((void *)code);
#endif /* WIN32 */
}

static
int
shutdown_server(grn_ctx *ctx)
{
  int ret;
  ret = grn_ctx_connect(ctx, ServerHost, DEFAULT_PORT, 0);
  if (ret) {
    fprintf(stderr, "Cannot connect groonga server(shutdown):ret=%d\n", ret);
    exit(1);
  }
  grn_ctx_send(ctx, "shutdown", 8, 0);
  return 0;
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
/*
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

      if (TaskTable[task_id].max < self) {
        TaskTable[task_id].max = self; 
      }
      if (TaskTable[task_id].min > self) {
        TaskTable[task_id].min = self;
      }

      if (report_p(TaskTable[task_id].jobtype)) {
        unsigned char tmpbuf[BUF_LEN];

        if (res_len < BUF_LEN) {
          strncpy(tmpbuf, res, res_len);
          tmpbuf[res_len] = '\0';
        }
        else {
          strncpy(tmpbuf, res, BUF_LEN - 2);
          tmpbuf[BUF_LEN -2] = '\0';
        }
        report_command(ctx, command, tmpbuf, task_id, &start_time, &end_time);
      }
      break;
    }
  } while ((flags & GRN_CTX_MORE));

  grn_obj_close(ctx, &start_time);
  grn_obj_close(ctx, &end_time);

  return 0;
}

static
int
worker_sub(intptr_t task_id)
{
  int i;
  grn_obj end_time;
  long long int latency, self;
  double sec, qps;

  TaskTable[task_id].max = 0LL;
  TaskTable[task_id].min = 9223372036854775807LL;
  TaskTable[task_id].qnum = 0;

  for (i = 0; i < TaskTable[task_id].ntimes; i++) {
    if (TaskTable[task_id].file != NULL) {
      FILE *fp;
      char tmpbuf[MAX_COMMAND_LEN];
      fp = fopen(TaskTable[task_id].file, "r");
      if (!fp) {
        fprintf(stderr, "Cannot open %s\n",TaskTable[task_id].file);
        error_exit_in_thread(1);
      } 
      tmpbuf[MAX_COMMAND_LEN-2] = '\0';
      while (fgets(tmpbuf, MAX_COMMAND_LEN, fp) != NULL) {
        if (tmpbuf[MAX_COMMAND_LEN-2] != '\0') {
          fprintf(stderr, "Too long commmand in %s\n",TaskTable[task_id].file);
          error_exit_in_thread(1);
        }
        tmpbuf[strlen(tmpbuf)-1] = '\0';
        do_command(&CtxTable[task_id], tmpbuf, 
                   TaskTable[task_id].jobtype,
                   task_id);
        TaskTable[task_id].qnum++;
      }
      fclose(fp);
    }
    else {
      int line;
      if (TaskTable[task_id].table == NULL) {
        fprintf(stderr, "Fatal error!:check script file!\n");
        error_exit_in_thread(1);
      }
      for (line = 0; line < TaskTable[task_id].table->num; line++) {
        do_command(&CtxTable[task_id], 
                   TaskTable[task_id].table->command[line], 
                   TaskTable[task_id].jobtype, task_id);
        TaskTable[task_id].qnum++;
      } 
    }
  }

  GRN_TIME_INIT(&end_time, 0);
  GRN_TIME_NOW(&CtxTable[task_id], &end_time);
  latency = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&StartTime);
  self = GRN_TIME_VALUE(&end_time) - GRN_TIME_VALUE(&Jobs_Start);

  CRITICAL_SECTION_ENTER(CS);
  if (JobTable[TaskTable[task_id].job_id].max < TaskTable[task_id].max) {
    JobTable[TaskTable[task_id].job_id].max = TaskTable[task_id].max;
  }
  if (JobTable[TaskTable[task_id].job_id].min > TaskTable[task_id].min) {
    JobTable[TaskTable[task_id].job_id].min = TaskTable[task_id].min;
  }

  JobTable[TaskTable[task_id].job_id].qnum += TaskTable[task_id].qnum;
  JobTable[TaskTable[task_id].job_id].done++;
  if (JobTable[TaskTable[task_id].job_id].done == 
      JobTable[TaskTable[task_id].job_id].concurrency) {
    sec = self / (double)1000000;
    qps = (double)JobTable[TaskTable[task_id].job_id].qnum/ sec;
    JobDoneNum++;
    if (JobDoneNum == 1) {
      if (Detail_On) {
        fseek(LogFp, -2, SEEK_CUR);
        fprintf(LogFp, "], \n");
      }
      fprintf(LogFp, "\"summary\" :[");
    }
    fprintf(LogFp, 
            "{\"job\": \"%s\", \"latency\": %lld, \"self\": %lld, \"qps\": %f, \"min\": %lld, \"max\": %lld}",
            JobTable[TaskTable[task_id].job_id].jobname, latency, self, qps,
            JobTable[TaskTable[task_id].job_id].min,
            JobTable[TaskTable[task_id].job_id].max);
    if (JobDoneNum < JobNum) {
      fprintf(LogFp, ", ");
    }
    else {
      fprintf(LogFp, "]");
    }
    fflush(LogFp);
  }
  grn_obj_close(&CtxTable[task_id], &end_time);
  CRITICAL_SECTION_LEAVE(CS);

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
get_sysinfo(char *path, char *result)
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

  sprintf(tmpbuf, "\"script\": \"%s.scr\",\n", ScriptName);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"user\": \"%s\",\n", UserName);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"date\": \"%s\",\n", Date);
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
    OSInfo = OS_WINDOWS64;
    sprintf(tmpbuf, "  \"BIT\": 64,\n");
    strcat(result, tmpbuf);
  }
  else {
    OSInfo = OS_WINDOWS32;
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
  osinfo.dwMinorVersion); strcat(result, tmpbuf);

  sprintf(tmpbuf, "  \"VERSION\": \"%s\"\n", grn_get_version());
  strcat(result, tmpbuf);
  strcat(result, "}");

#else /* linux only */
#include <sys/utsname.h>
#include <sys/statvfs.h>
  FILE *fp;
  int ret;
  int cpunum;
  int minfo;
  char cpustring[256];
  struct utsname ubuf;
  struct statvfs vfsbuf;

  strcpy(result, "{");

  sprintf(tmpbuf, "\"script\": \"%s.scr\",\n", ScriptName);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"user\": \"%s\",\n", UserName);
  strcat(result, tmpbuf);
  sprintf(tmpbuf, "  \"date\": \"%s\",\n", Date);
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
      cpunum = atoi(&tmpbuf[12]);
    }
  }
  fclose(fp);
  sprintf(tmpbuf, "  \"CPU\": %s\",\n", cpustring);
  strcat(result, tmpbuf);

  if (sizeof(int *) == 8 ) {
    OSInfo = OS_LINUX64;
    sprintf(tmpbuf, "  \"BIT\": 64,\n");
    strcat(result, tmpbuf);
  }
  else {
    OSInfo = OS_LINUX32;
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
      minfo = atoi(&tmpbuf[10]);
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
  char tmpbuf[16];

  JobTable[num].concurrency = 1;
  JobTable[num].ntimes = 1;
  JobTable[num].done = 0;
  JobTable[num].qnum = 0;
  JobTable[num].max = 0LL;
  JobTable[num].min = 9223372036854775807LL;

  strncpy(JobTable[num].jobname, &buf[start], end - start);
  JobTable[num].jobname[end - start] = '\0';
  i = start;
  while (i < end) {
    if (isspace(buf[i])) {
      i++;
      continue;
    }
    if (!strncmp(&buf[i], "do_local", 8)) {
      JobTable[num].jobtype = J_DO_LOCAL;
      i = i + 8;
      break;
    }
    if (!strncmp(&buf[i], "do_gqtp", 7)) {
      JobTable[num].jobtype = J_DO_GQTP;
      i = i + 7;
      break;
    }
    if (!strncmp(&buf[i], "rep_local", 9)) {
      JobTable[num].jobtype = J_REP_LOCAL;
      i = i + 9;
      break;
    }
    if (!strncmp(&buf[i], "rep_gqtp", 8)) {
      JobTable[num].jobtype = J_REP_GQTP;
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

  if (!isspace(buf[i])) {
    return 4;
  }
  i++;

  while (isspace(buf[i])) {
    i++;
    continue;
  }
  j = 0;
  while (i < end) {
    if (isspace(buf[i])) {
      break;
    }
    JobTable[num].commandfile[j] = buf[i];
    i++;
    j++;
    if (j > 255) {
      return 5;
    }
  }
  JobTable[num].commandfile[j] = '\0';
  if (i == end) {
   return 0;
  }
  while (isspace(buf[i])) {
    i++;
    continue;
  }
  if (i == end) {
   return 0;
  }
  j = 0;
  while (i < end) {
    if (isspace(buf[i])) {
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
  JobTable[num].concurrency = atoi(tmpbuf);
  if (JobTable[num].concurrency == 0) {
    return 7;
  }

  while (isspace(buf[i])) {
    i++;
    continue;
  }
  if (i == end) {
   return 0;
  }
  j = 0;
  while (i < end) {
    if (isspace(buf[i])) {
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
  JobTable[num].ntimes = atoi(tmpbuf);
  if (JobTable[num].ntimes == 0) {
    return 9;
  }
  if (i == end) {
    return 0;
  }

  while (i < end) {
    if (isspace(buf[i])) {
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
      }
      else {
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
  }
  else {
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
    if (JobTable[i].concurrency == 1) {
      TaskTable[tid].file = JobTable[i].commandfile;
      TaskTable[tid].table = NULL;
      TaskTable[tid].ntimes = JobTable[i].ntimes;
      TaskTable[tid].jobtype = JobTable[i].jobtype;
      TaskTable[tid].job_id = i;
      tid++;
      continue;
    }
    for (j = 0; j < JobTable[i].concurrency; j++) {
      if (j == 0) {
        ctable = malloc(sizeof(struct commandtable));
        if (!ctable) {
          fprintf(stderr, "Cannot alloc commandtable\n");
          error_exit(ctx, 1);
        }
        fp = fopen(JobTable[i].commandfile, "r");
        if (!fp) {
          fprintf(stderr, "Cannot alloc commandfile:%s\n",
                   JobTable[i].commandfile);
          error_exit(ctx, 1);
        }
        line = 0;
        tmpbuf[MAX_COMMAND_LEN-2] = '\0';
        while (fgets(tmpbuf, MAX_COMMAND_LEN, fp) != NULL) {
          if (tmpbuf[MAX_COMMAND_LEN-2] != '\0') {
            tmpbuf[MAX_COMMAND_LEN-2] = '\0';
            fprintf(stderr, "Too long command in %s\n", 
                   JobTable[i].commandfile);
            fprintf(stderr, "line =%d:%s\n", line + 1, tmpbuf);
            error_exit(ctx, 1);
          }
          len = strlen(tmpbuf);
          len--;
          tmpbuf[len] = '\0';
          ctable->command[line] = strdup(tmpbuf);
          if (ctable->command[line] == NULL) {
            fprintf(stderr, "Cannot alloc commandfile:%s\n",
                    JobTable[i].commandfile);
            error_exit(ctx, 1);
          }
          ctable->num = line;
          line++;
          if (line >= MAX_COMMAND) {
            fprintf(stderr, "Too many commands in %s\n", 
                   JobTable[i].commandfile);
            error_exit(ctx, 1);
          }
        }
        ctable->num = line;
      }
      TaskTable[tid].file = NULL;
      TaskTable[tid].table = ctable;
      TaskTable[tid].ntimes = JobTable[i].ntimes;
      TaskTable[tid].jobtype = JobTable[i].jobtype;
      TaskTable[tid].job_id = i;
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

  for (i = 0; i < TaskTable[task_id].table->num; i++) {
    printf("%s\n", TaskTable[task_id].table->command[i]);
  }
  return 0;
}
*/

/* return num of query */
static
int
do_jobs(grn_ctx *ctx, int jobnum, int line)
{
  int i, task_num, ret, qnum = 0;
  int thread_num = 0;

  for (i = 0; i < jobnum; i++) {
/*
printf("%d:type =%d:file=%s:con=%d:ntimes=%d\n", i, JobTable[i].jobtype,
        JobTable[i].commandfile, JobTable[i].concurrency, JobTable[i].ntimes);

*/
    thread_num = thread_num + JobTable[i].concurrency;
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
  
  Detail_On = 0;
  for (i = 0; i < task_num; i++) {
    grn_ctx_init(&CtxTable[i], 0);
    if (gqtp_p(TaskTable[i].jobtype)) {
      ret = grn_ctx_connect(&CtxTable[i], ServerHost, DEFAULT_PORT, 0);
      if (ret) {
        fprintf(stderr, "Cannot connect groonga server:ret=%d\n", ret);
        error_exit(ctx, 1);
      }
    }
    else {
      grn_ctx_use(&CtxTable[i], DB);
    }
    if (report_p(TaskTable[i].jobtype)) {
      Detail_On++;
    }
  }
  if (Detail_On) {
    fprintf(LogFp, "\"detail\" :[\n");
  }

  thread_main(ctx, task_num);


  for (i = 0; i < task_num; i++) {
    grn_ctx_fin(&CtxTable[i]);
    qnum = qnum + TaskTable[i].qnum;
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
    JobDoneNum = 0;
    job_num = get_jobs(ctx, buf, line);
    JobNum = job_num;

    if (job_num > 0) {
      GRN_TIME_INIT(&Jobs_Start, 0);
      GRN_TIME_NOW(ctx, &Jobs_Start);
      fprintf(LogFp, "{\"jobs\": \"%s\",\n", buf);
      qnum = do_jobs(ctx, job_num, line);
      fprintf(LogFp, "},\n");
      qnum_total = qnum_total + qnum;

      grn_obj_close(ctx, &Jobs_Start);
    }
    if (STOP_FLAG) {
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
  DB = grn_db_open(ctx, dbpath);
  if (!DB) {
    DB = grn_db_create(ctx, dbpath, NULL);
  }
  if (!DB) {
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
    ret = grn_ctx_connect(ctx, ServerHost, DEFAULT_PORT, 0);
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
  ret = recv(socket, buf, BUF_LEN, 0);
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
  size = atoi(buf);
  
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
      sprintf(send_mesg, "LIST\r\n");
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
    strcpy(retval, get_ftp_date(buf));
  }

  switch (mode) {
    case MODE_LIST:
      read_response(data_socket, buf);
      fprintf(stderr, "%s\n", buf);
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
          atoi(argv[3]), argv[4], val);
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
  }
  else {
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
printf("commandfile=[%s]:buf=%s\n", JobTable[i].commandfile, buf);
*/
        fnum = cache_file(filelist, JobTable[i].commandfile, fnum);
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

  strcpy(ServerHost, DEFAULT_DEST);
  if (argc > 4) {
    if (!strncmp(argv[3], "-dest", 5)) {
      strcpy(ServerHost, argv[4]);
      remote_mode = 1;
    }
  }

  grn_init();
  CRITICAL_SECTION_INIT(CS);
  
  grn_ctx_init(&context, 0);
  grn_set_default_encoding(GRN_ENC_UTF8);

  start_local(&context, argv[2]);
  if (!remote_mode) {
    start_server(argv[2], 0);
  }

  get_sysinfo(argv[2], sysinfo);

  if (check_server(&context)) {
    goto exit;
  }

  sync_script(&context, argv[1]);
  get_scriptname(argv[1], ScriptName, ".scr");
  get_username(UserName);

  GRN_TIME_INIT(&StartTime, 0);
  GRN_TIME_NOW(&context, &StartTime);
  sec = (time_t)(GRN_TIME_VALUE(&StartTime)/1000000);
  get_date(Date, &sec);
  
  sprintf(log, "%s-%s-%lld.log", ScriptName, 
          UserName, GRN_TIME_VALUE(&StartTime));

  LogFp = fopen(log, "w+b");
  if (!LogFp) {
    fprintf(stderr, "Cannot open logfile:%s\n", log);
    shutdown_server(&context);
    goto exit;
  }
  get_sysinfo(argv[2], sysinfo);
  output_sysinfo(sysinfo);

  qnum = do_script(&context, argv[1]);
  output_result_final(&context, qnum);
  fclose(LogFp);

  if (!remote_mode) {
    shutdown_server(&context);
  }

  ftp_sub(FTPUSER, FTPPASSWD, FTPSERVER, log, 3, 
          "report", NULL);
exit:
  CRITICAL_SECTION_FIN(CS);
  grn_obj_close(&context, &StartTime);
  grn_obj_close(&context, DB);
  grn_ctx_fin(&context);
  grn_fin();
  return 0;
}
