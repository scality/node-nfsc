/* copied from rfc 1813 */

%#define LM_MAXSTRLEN   1024
%#define MAXNAMELEN     LM_MAXSTRLEN+1

typedef unsigned hyper _uint64;
typedef unsigned int _uint32;
typedef int _int32;

enum nlmstat4 {
  NLM4_GRANTED = 0,
  NLM4_DENIED = 1,
  NLM4_DENIED_NOLOCKS = 2,
  NLM4_BLOCKED = 3,
  NLM4_DENIED_GRACE_PERIOD = 4,
  NLM4_DEADLCK = 5,
  NLM4_ROFS = 6,
  NLM4_STALE_FH = 7,
  NLM4_FBIG = 8,
  NLM4_FAILED = 9
};

enum    fsh_mode4 {
  FSM4_DN  = 0,    /* deny none */
  FSM4_DR  = 1,    /* deny read */
  FSM4_DW  = 2,    /* deny write */
  FSM4_DRW = 3     /* deny read/write */
};

enum    fsh_access4 {
  FSA4_NONE = 0,   /* for completeness */
  FSA4_R    = 1,   /* read only */
  FSA4_W    = 2,   /* write only */
  FSA4_RW   = 3    /* read/write */
};

struct nlm_holder4 {
  bool     exclusive;
  _int32    svid;
  netobj   oh;
  _uint64   l_offset;
  _uint64   l_len;
};

struct nlm_lock4 {
  string   caller_name<LM_MAXSTRLEN>;
  netobj   fh;
  netobj   oh;
  _int32    svid;
  _uint64   l_offset;
  _uint64   l_len;
};

struct nlm_share4 {
  string      caller_name<LM_MAXSTRLEN>;
  netobj      fh;
  netobj      oh;
  fsh_mode4   mode;
  fsh_access4 access;
};

/*
 * res
 */

struct nlm_stat4 {
  nlmstat4 stat;
};

struct nlm_res4 {
  netobj cookie;
  nlm_stat4 stat;
};

union nlm_testrply4 switch (nlmstat4 stat) {
 case NLM4_DENIED:
   struct nlm_holder4 holder;
 default:
   void;
 };

struct nlm_testres4 {
  netobj cookie;
  nlm_testrply4 stat;
};

struct  nlm_shareres4 {
  netobj  cookie;
  nlmstat4       stat;
  int     sequence;
};

/*
 * args
 */

struct nlm_testargs4 {
  netobj cookie;          
  bool exclusive;
  struct nlm_lock4 alock;
};

struct nlm_lockargs4 {
  netobj cookie;
  bool block;
  bool exclusive;
  struct nlm_lock4 alock;
  bool reclaim;
  int state;
};

struct nlm_cancelargs4 {
  netobj cookie;          
  bool block;
  bool exclusive;
  struct nlm_lock4 alock;
};

struct nlm_unlockargs4 {
  netobj cookie;          
  struct nlm_lock4 alock;
};

struct  nlm_shareargs4 {
  netobj  cookie;
  nlm_share4       share;
  bool    reclaim;
};

struct  nlm_notify4 {
  string name<MAXNAMELEN>;
  long state;
};

/*
 * program
 */

program NLM_PROGRAM {

  version NLM_V4 {

    void NLMPROC4_NULL(void)                  = 0;
    
    nlm_testres4 NLMPROC4_TEST(nlm_testargs4)         = 1;
        
    nlm_res4 NLMPROC4_LOCK(nlm_lockargs4)         = 2;
    
    nlm_res4 NLMPROC4_CANCEL(nlm_cancelargs4)       = 3;
    
    nlm_res4 NLMPROC4_UNLOCK(nlm_unlockargs4)     = 4;
    
    nlm_res4 NLMPROC4_GRANTED(nlm_testargs4)      = 5;
    
    void NLMPROC4_TEST_MSG(nlm_testargs4)     = 6;
    
    void NLMPROC4_LOCK_MSG(nlm_lockargs4)     = 7;
    
    void NLMPROC4_CANCEL_MSG(nlm_cancelargs4)   = 8;
    
    void NLMPROC4_UNLOCK_MSG(nlm_unlockargs4) = 9;
    
    void NLMPROC4_GRANTED_MSG(nlm_testargs4) = 10;
    
    void NLMPROC4_TEST_RES(nlm_testres4)     = 11;
    
    void NLMPROC4_LOCK_RES(nlm_res4)         = 12;
    
    void NLMPROC4_CANCEL_RES(nlm_res4)       = 13;
    
    void NLMPROC4_UNLOCK_RES(nlm_res4)       = 14;
    
    void NLMPROC4_GRANTED_RES(nlm_res4)      = 15;
    
    nlm_shareres4 NLMPROC4_SHARE(nlm_shareargs4)      = 20;
    
    nlm_shareres4 NLMPROC4_UNSHARE(nlm_shareargs4)    = 21;
    
    nlm_res4 NLMPROC4_NM_LOCK(nlm_lockargs4)     = 22;
    
    void NLMPROC4_FREE_ALL(nlm_notify4)      = 23;
    
  } = 4;
  
} = 100021;
