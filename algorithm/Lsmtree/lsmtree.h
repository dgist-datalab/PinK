#ifndef __LSM_HEADER__
#define __LSM_HEADER__
#include <pthread.h>
#include <limits.h>
#include "level.h"
#include "skiplist.h"
#include "bloomfilter.h"
#include "cache.h"
#include "level.h"
#include "page.h"
#include "../../include/settings.h"
#include "../../include/utils/rwlock.h"
#include "../../interface/queue.h"
#include "../../include/container.h"
#include "../../include/settings.h"
#include "../../include/lsm_settings.h"
#include "../../include/utils/dl_sync.h"
#include "../../include/types.h"
#include "../../include/sem_lock.h"
#include "../../include/data_struct/redblack.h"
#include "../../interface/interface.h"
//#define ONESEGMENT (DEFKEYINHEADER*DEFVALUESIZE)
#ifdef DVALUE
	#define CONVPPA(_ppa) _ppa/NPCINPAGE
#else
	#define CONVPPA(_ppa) _ppa
#endif

#define HEADERR MAPPINGR
#define HEADERW MAPPINGW
#define GCHR GCMR
#define GCHW GCMW
#define SDATAR 9
#define RANGER 10
#define OLDDATA 13
#define BGREAD 15
#define BGWRITE 16
#define TESTREAD 17

//lower type, algo type
typedef struct level level;
typedef struct run run_t;
typedef struct level_ops level_ops;
typedef struct htable htable;

enum READTYPE{
	NOTFOUND,FOUND,CACHING,FLYING
};
enum comp_opt_type{
	NON,PIPE,HW,MIXEDCOMP
};
enum LSMTYPE{
	NOUSE,ONLYCACHE,FIXEDFILTER,ONLYFILTER,ONLYFIXEDFILTER,PINASYM,LASTFILTER
};

#ifdef MULTILEVELREAD
enum GETREQTYPE{
	GET_INIT,GET_DONE
};
#define SETGETREQTYPE (TYPE,LEVEL) ((LEVEL<<2)|TYPE)
#define GETREQTYPE (VALUE) (value&0x7)
#define GETREQLEVELN (VALUE) (value>>2)
#endif

#define SETNOCPY(a)		(a|=1)
#define SETGCOPT(a)		(a|=2)
#define SETHWREAD(a)	(a|=4)
#define SETCOMPOPT(a,b)	(a=(b<<8)|a)
#define SETLSMTYPE(a,b) (a=(b<<16)|a)
#define SETFILTER(a,b)	(a=(b<<3)|a)

#define ISNOCPY(a)		(a&1)
#define ISGCOPT(a)		(a&2)
#define ISHWREAD(a)		(a&4)
#define GETFILTER(a)	((a>>3)&3)	
#define GETCOMPOPT(a)	((a>>8)&0xff)
#define GETLSMTYPE(a)	((a>>16)&0xff)

#ifdef MULTILEVELREAD
typedef struct multi_req_params{
	int target_runs;
	int return_runs;
	int overlap_cnt;
	int *target_ppas;
}mreq_params;
#endif

typedef struct req_params{
	int datas[4];
	run_t *entry;
}rparams;

typedef struct lsm_params{
	//dl_sync lock;
	uint8_t lsm_type;
	ppa_t ppa;
	void *entry_ptr;
	PTR test;
	PTR* target;
	value_set* value;
	htable * htable_ptr;
	fdriver_lock_t *lock;
}lsm_params;

typedef struct lsm_sub_req{
	KEYT key;
	value_set *value;
	request *parents;
	run_t *ent;
	uint8_t status;
}lsm_sub_req;

typedef struct lsm_range_params{
	uint8_t lsm_type;
	int now;
	int not_found;
	int max;
	uint32_t now_level;
	uint8_t status;
	char **mapping_data;
	fdriver_lock_t global_lock;
	lsm_sub_req *children;
}lsm_range_params;


typedef struct lsmtree_setting_parmaters{
	uint8_t LEVELN;
	uint8_t LEVELCACHING;
	uint16_t VALUESIZE;
	uint32_t ONESEGMENT;
	uint32_t KEYNUM;
	uint32_t HEADERNUM;
	float caching_size;
	float *bf_fprs;

	uint64_t total_memory;
	uint64_t level_list_memory;
	uint64_t bf_memory;
	uint64_t cache_memory;
	uint64_t pin_memory;
	int64_t remain_memory;
}lsp;

typedef struct lsmtree_levelsize_params{
	float size_factor;
	float last_size_factor;
	bool* size_factor_change;//true: it will be changed size

	//keynum_in_header real;
	uint32_t keynum_in_header;
	uint32_t keynum_in_header_cnt;
	uint32_t result_padding;

	//for average value in data segment
	double avg_of_length;
	uint32_t length_cnt;
}llp;

typedef struct lsmtree_monitor_info{
	/*bench info!*/
	uint32_t data_gc_cnt;
	uint32_t header_gc_cnt;
	uint32_t compaction_cnt;
	uint32_t last_compaction_cnt;
	uint32_t zero_compaction_cnt;
	uint32_t __header_read_cnt;
	uint32_t channel_overlap_cnt;
#ifdef PREFIXCHECK
	uint32_t pr_check_cnt;
#endif
	uint32_t check_cnt;
}lmi;

/*
	setup values 32bit
	0 byte : 
		0bit nocpy,
		1bit gc_opt,
		2bit hw_read,
	  3,4bit filtertype,
	  5~7bit reserved
	1 byte compaction option
	2 byte lsm_type
	3 byte reserved
 */
typedef struct lsmtree{
	uint32_t setup_values;
	uint8_t LEVELN;
	uint8_t LEVELCACHING;
	uint8_t result_padding;
	//level opertaion sets//
	struct queue *re_q;
	pthread_mutex_t *level_lock;
	pthread_mutex_t memlock;
	pthread_mutex_t templock;
	struct skiplist *memtable;
	struct skiplist *temptable;
	level **disk;
	level *c_level;
	level_ops *lop;
	lower_info* li;
#ifdef DVALUE
	pthread_mutex_t data_lock;
	ppa_t data_ppa; //for one data caching for read
	value_set* data_value;
#endif
	struct cache* lsm_cache;

	/*for gc*/
	bool gc_started;
	struct skiplist *gc_list;
	bool gc_compaction_flag;
	blockmanager *bm;
	struct lsm_block *active_block;
	bool delayed_header_trim;
	uint32_t delayed_trim_ppa;
	//^ UINT_MAX is nothing to do //this is for nocpy, when header_gc triggered in compactioning

#ifdef EMULATOR
	Redblack rb_ppa_key;
#endif
	bool debug_flag;
}lsmtree;

uint32_t lsm_argument_set(int argc, char **argv);
uint32_t lsm_create(lower_info *, blockmanager *, algorithm *);
uint32_t __lsm_create_normal(lower_info *, algorithm *);
void lsm_setup_params();
void lsm_destroy(lower_info*, algorithm*);
uint32_t lsm_get(request *const);
uint32_t lsm_set(request *const);
uint32_t lsm_multi_get(request *const, int num);
uint32_t lsm_proc_re_q();
uint32_t lsm_remove(request *const);

uint32_t __lsm_get(request *const);
uint8_t lsm_find_run(KEYT key, run_t **,run_t *,struct keyset **, int *level, int *run);
uint32_t __lsm_range_get(request *const);

void* lsm_end_req(struct algo_req*const);
void* lsm_mget_end_req(struct algo_req *const);
bool lsm_kv_validcheck(uint8_t *, int idx);
void lsm_kv_validset(uint8_t *,int idx);

htable *htable_copy(htable *);
htable *htable_assign(char*,bool);
htable *htable_dummy_assign();
void htable_free(htable*);
void htable_print(htable*,ppa_t);
algo_req *lsm_get_req_factory(request*,uint8_t);
void htable_check(htable *in,KEYT lpa,ppa_t ppa,char *);

uint32_t lsm_multi_set(request *const, uint32_t num);
uint32_t lsm_range_get(request *const);
uint32_t lsm_memory_size();
uint32_t lsm_simul_put(ppa_t ppa, KEYT key);
//copy the value
uint32_t lsm_test_read(ppa_t ppa, char *data);
level *lsm_level_resizing(level *target, level *src);
KEYT* lsm_simul_get(ppa_t ppa); //copy the value
void lsm_simul_del(ppa_t ppa);
float diff_get_sizefactor(uint32_t keynum_in_header);
#endif
