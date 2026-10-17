/* V559 C ORIGINAL: real SQLite public sqlite3_str, version 3.46.1. */
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static sqlite3_mem_methods original;
static int armed, nth, once, attempts, rejected;
static long long outstanding;

static int should_fail(void){
  if(!armed) return 0;
  ++attempts;
  if(nth>0 && attempts>=nth && (!once || attempts==nth)){
    ++rejected;
    return 1;
  }
  return 0;
}
static void *fi_malloc(int n) {
  if(should_fail()) return NULL;
  void *p=original.xMalloc(n);
  if(p) ++outstanding;
  return p;
}
static void fi_free(void *p) {
  if(p) --outstanding;
  original.xFree(p);
}
static void *fi_realloc(void *p,int n) {
  if(should_fail()) return NULL;
  void *q=original.xRealloc(p,n);
  if(q && !p) ++outstanding;
  return q;
}
static int fi_size(void *p){return original.xSize(p);}
static int fi_round(int n){return original.xRoundup(n);}
static int fi_init(void *p){return original.xInit ? original.xInit(p) : SQLITE_OK;}
static void fi_shutdown(void *p){if(original.xShutdown)original.xShutdown(p);}

static void arm(int n, int one){ nth=n;once=one;attempts=0;rejected=0;armed=1; }
static void disarm(void){armed=0;}

static void run(const char *case_name,int limit,int fail_at,int fail_once,
                int expect_code,int expect_null,int expected_reject){
  const long long before=outstanding;
  sqlite3 *db=NULL;
  if(limit>0){
    assert(sqlite3_open(":memory:",&db)==SQLITE_OK);
    assert(db);
    int actual=sqlite3_limit(db,SQLITE_LIMIT_LENGTH,limit);
    (void)actual;
  }
  arm(fail_at,fail_once);
  sqlite3_str *s=sqlite3_str_new(db);
  assert(s); /* also nonnull for constructor OOM singleton */
  int first_status=sqlite3_str_errcode(s);
  if(strcmp(case_name,"empty")!=0){
    sqlite3_str_append(s,"ab",2);
    sqlite3_str_append(s,"cd",2);
    /* Validate sticky failure; should not allocate further after error. */
    char large[128]; memset(large,'X',sizeof(large));
    sqlite3_str_append(s,large,(int)sizeof(large));
  }
  int status=sqlite3_str_errcode(s);
  int length=sqlite3_str_length(s);
  int attempted=attempts;
  int observed_reject=rejected;
  char *owned=sqlite3_str_finish(s);
  const int was_null=(owned==NULL);
  disarm();
  if(status!=expect_code || was_null!=expect_null || observed_reject!=expected_reject){
    fprintf(stderr,"FAIL %s st=%d expected=%d len=%d null=%d wanted=%d attempts=%d rejects=%d wanted=%d\n",
            case_name,status,expect_code,length,was_null,expect_null,attempted,observed_reject,expected_reject);
    exit(2);
  }
  if(owned){
    assert(strlen(owned)==132);
    assert(memcmp(owned,"abcd",4)==0);
    for(int i=4;i<132;i++) assert(owned[i]=='X');
    sqlite3_free(owned);
  }
  if(db) assert(sqlite3_close(db)==SQLITE_OK);
  if(outstanding!=before){
    fprintf(stderr,"LEAK? case %s before=%lld after=%lld\n",case_name,before,outstanding);
    exit(3);
  }
  /* C constructor OOM returns usable singleton; independently demonstrate
     a subsequent successful builder with no fault injection (host recovered). */
  sqlite3_str *later=sqlite3_str_new(NULL);
  assert(later);
  sqlite3_str_append(later,"Z",1);
  assert(sqlite3_str_errcode(later)==SQLITE_OK);
  char *z=sqlite3_str_finish(later);
  assert(z && strcmp(z,"Z")==0);
  sqlite3_free(z);
  assert(outstanding==before);
  printf("C_CASE %s code=%d null=%d attempts=%d rejected=%d initial=%d len=%d live_delta=%lld later=OK\n",
         case_name,status,was_null,attempted,observed_reject,first_status,length,outstanding-before);
}

int main(void){
  assert(strcmp(sqlite3_libversion(),"3.46.1")==0);
  assert(sqlite3_config(SQLITE_CONFIG_GETMALLOC,&original)==SQLITE_OK);
  sqlite3_mem_methods m=original;
  m.xMalloc=fi_malloc;m.xFree=fi_free;m.xRealloc=fi_realloc;
  m.xSize=fi_size;m.xRoundup=fi_round;m.xInit=fi_init;m.xShutdown=fi_shutdown;
  assert(sqlite3_config(SQLITE_CONFIG_MALLOC,&m)==SQLITE_OK);
  assert(sqlite3_initialize()==SQLITE_OK);
  run("success",0,0,1,SQLITE_OK,0,0);
  run("empty",0,0,1,SQLITE_OK,1,0);
  run("limit",32,0,1,SQLITE_TOOBIG,1,0);
  run("oom_new_once",0,1,1,SQLITE_NOMEM,1,1);
  run("oom_grow_once",0,2,1,SQLITE_NOMEM,1,1);
  run("oom_realloc_once",0,3,1,SQLITE_NOMEM,1,1);
  run("oom_new_persistent",0,1,0,SQLITE_NOMEM,1,1);
  run("oom_grow_persistent",0,2,0,SQLITE_NOMEM,1,1);
  run("oom_realloc_persistent",0,3,0,SQLITE_NOMEM,1,1);
  assert(sqlite3_shutdown()==SQLITE_OK);
  printf("C_SQLITE_STR_DONE sqlite_version=%s outstanding=%lld\n",sqlite3_libversion(),outstanding);
  return 0;
}
