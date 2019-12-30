/*
 * test harness for testing hash tables
 *
 * Reads from stdin or a file and builds a hash table. Does
 * self-test of hash table functions and prints out some timining
 * numbers.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/time.h>

#include "getopt_long.h"
#include "error.h"
#include "utils/hashtab.h"
#include "utils/arena.h"
#include "utils/utils.h"
#include "fast/vect.h"
#include "utils/hashfunc.h"
#include "utils/siphash.h"

extern uint32_t arc4random(void);


// This is what we store in the hash table.
struct datum
{
    char* key;
    char* val;
};
typedef struct datum datum;

// input from file is stored here
VECT_TYPEDEF(input_vect, datum*);

static struct option lopt[] =
{
    {"logsize", required_argument, 0, 's'},
    {"fill", required_argument, 0, 'f'},
    {"hash",  required_argument, 0, 'H'},
    {"help",  no_argument, 0, 'h'},
    {0, 0,0, 0}
} ;

static char sopt[] = "s:f:H:h" ;

static void Usage(int) ;
static void test_hash_table(input_vect*, hash_table_policy*);
static void read_data(input_vect* inp, FILE* fp, memmgr* mem);
static void store_data(input_vect *, hash_table_t) ;
static void query_data(input_vect *, hash_table_t) ;
static void remove_data(input_vect *, hash_table_t) ;
static void iterator_test(input_vect *, hash_table_t, int sorted);


static hash_func_f* str2hash(const char* name);

static int str_keyval_cmp(const void*, const void*);


// fold a 64 bit value into a 32 bit one. Use all 64 bits to do the
// folding.
static inline uint32_t
fold32(uint64_t v)
{
    return 0xffffffff & (v - (v >> 32));
}




/*
 * List of hash functions
 */
static uint32_t hash_murmur3(const void*);
static uint32_t hash_hsieh(const void*);
static uint32_t hash_fnv(const void*);
static uint32_t hash_city(const void*);
static uint32_t hash_sip(const void*);
static uint32_t hash_yorrike(const void*);
static uint32_t hash_fast(const void*);

struct hash_func_pair
{
    const char* name;
    hash_func_f* fp;
};
typedef struct hash_func_pair hash_func_pair;


static const hash_func_pair Hashnames [] =
{
    {"hsieh",   hash_hsieh},
    {"murmur",  hash_murmur3},
    {"fnv",     hash_fnv},
    {"city",    hash_city},
    {"siphash", hash_sip},
    {"fast",    hash_fast},
    {"yorrike", hash_yorrike},
    {"default", hash_sip},
    {0, 0}
};

static int
str_keyval_cmp(const void* lhs, const void* rhs)
{
    const datum* a = (const datum*)lhs;
    const datum* b = (const datum*)rhs;
    return strcmp(a->key, b->key);
}

static hash_func_f*
str2hash(const char* name)
{
    const hash_func_pair* p = &Hashnames[0];

    for (; p->name; ++p) {
        if (0 == strcasecmp(name, p->name))
            return p->fp;
    }

    return 0;
}

/*
 * Perf measurement
 */
struct perf
{
    uint64_t c;
    uint64_t t;
};
typedef struct perf perf;

static inline perf
perfinit()
{
    perf p = { 0, 0 };
    return p;
}

static inline perf
perfstart()
{
    perf p;
    p.t = timenow();
    p.c = sys_cpu_timestamp();
    return p;
}

static inline void
perfaccum(perf* dest, perf p0)
{
    perf n = perfstart();

    dest->c += (n.c - p0.c);
    dest->t += (n.t - p0.t);
}


int
main(int argc, char *argv[])
{
    int logsize=0, htmaxfill=0 ;
    int c;
    FILE *fp = 0;
    char *filename ;
    hash_table_policy pol;
    hash_func_f* hashfunc;
    arena_t arena;
    input_vect vv;

    program_name = argv[0] ;

    hashfunc = str2hash("default");

    while ( (c=getopt_long(argc, argv, sopt, lopt, 0)) != EOF ) {
        switch(c)
        {
            case 0 :
                break ;

            case 's' :
                logsize = atoi(optarg) ;
                break ;
            case 'H':
                hashfunc = str2hash(optarg);
                if (!hashfunc)
                    error(1, 0, "Can't find hash function '%s'", optarg);
                break;
            case 'f' :
                htmaxfill = atoi(optarg) ;
                break ;
            case 'h':
            default :
                Usage(1) ;
        }
    }

    if (optind >= argc)
        Usage(1) ;  /* File name not present */

    VECT_INIT(&vv, 1048576);
    memset(&pol, 0, sizeof pol);
    arena_new(&arena, 128*1024);

    arena_memmgr(&pol.mem, arena);

    /* What ever is left in argv[optind] is our file name */
    for (; optind < argc; ++optind) {
        filename = argv[optind] ;
        fp = 0 == strcmp(filename, "-") ? stdin : fopen(filename, "r");
        if (!fp)
            error(1, errno, "Unable to open %s\n", filename) ;
        setvbuf(stdout, 0, _IONBF, 0) ;
        setvbuf(fp, 0, _IOFBF, 256 * 1024) ;
        read_data(&vv, fp, &pol.mem);
        if (fp != stdin) fclose(fp);
    }


    pol.hash    = hashfunc;
    pol.cmp     = str_keyval_cmp;
    pol.dtor    = 0;
    pol.fillmax = htmaxfill;
    pol.logsize = logsize;

    lockmgr_new_empty(&pol.lock);
    test_hash_table(&vv, &pol);


    /* since we are using the arena memory manager, we don't have to
     * bother with deleting the hash table at all.
     * Just delete the enclosing 'arena'.
     */
    arena_delete(arena);

    fflush(stdout) ;
    fflush(stderr) ;
    return 0 ;
}

void
test_hash_table(input_vect* input, hash_table_policy* pol)
{
    hash_table_t tb;
    hash_table_stat st;
    int err;


    err = hash_table_new(&tb, pol);
    if (err < 0)
        error(1, -err, "Can't create hash table");

    /* Populate phase */
    store_data(input, tb) ;

    // Shuffle after storing. So, next set of tests are randomized.
    VECT_SHUFFLE(input, arc4random);

    /* Verify phase */
    query_data(input, tb) ;

    iterator_test(input, tb, 0);
    iterator_test(input, tb, 1);

    /* Get hash table statistics - before we delete all nodes */
    hash_table_stats(tb, &st);

    /* Lastly, test the remove() interface */
    remove_data(input, tb);

    /* Print stats. */
    printf ("Statistics:\n"
            "   Nodes:      %lu (exp %4.2f/bucket, actual %4.2f/bucket)\n"
            "   Buckets:    %lu, occupied %lu (%4.2f%%)\n"
            "   Chain len:  max %lu avg %4.2f\n"
            "   Ops:\n"
            "     Splits:   %d\n"
            "     Inserts:  %d\n"
            "     Lookups:  %d (failed %d)\n"
            ,
            st.nodes, hash_table_density(&st), hash_table_actual_density(&st),
            st.size, st.fill, hash_table_fill_percent(&st),
            st.maxchainlen, hash_table_actual_density(&st),
            st.splits,
            st.inserts,
            st.lookups, st.failed_lookups
            );

    /*
     * have to get rid of all the objects we created - esp. mutexes
     * and such.
     */
    hash_table_delete(tb);
}


static datum*
read_keyval(FILE* fp, datum* kp, memmgr* mem)
{
    char buf[1024];

    char* ptr;
    char* kptr,
        * vptr;

    fgets(buf, sizeof buf, fp);
    if (feof(fp)) return 0;

    size_t n = strlen(buf);
    if (buf[n-1] == '\n') buf[--n] = 0;
    if (buf[n-1] == '\r') buf[--n] = 0;


#define strsave(mem, b)  \
    ({ size_t xxn = strlen(b)+1;\
       char*  xxs = (char*)memmgr_alloc(mem, xxn);\
       memcpy(xxs, b, xxn);\
       xxs;\
     })

    ptr  = strchr(buf, ' ');
    if (ptr) {
        *ptr = 0;
        kptr = buf;
        vptr = ptr+1;
    } else {
        vptr = kptr = buf;
    }

    kp->key = strsave(mem, kptr);
    kp->val = strsave(mem, vptr);
    return kp;
}

static void
read_data(input_vect* inp, FILE* fp, memmgr* mem)
{
    do {
        datum* z = (datum*)memmgr_alloc(mem, sizeof *z);

        if (!read_keyval(fp, z, mem))
            break;

        VECT_PUSH_BACK(inp, z);
    } while(1);

    printf("Read %zu records into array\n", VECT_LEN(inp));
}

#define _d(x)   ((double)(x))
static void
show_timing(const char* prefix, uint64_t n, perf* p)
{
    double ave   = _d(p->c) / _d(n);
    double speed = 1000.0 * _d(n) / _d(p->t);


    fprintf(stderr, "%s %" PRIu64 " records; %7.3f cy/op %6.2f M ops/sec\n",
            prefix, n, ave, speed);
}

static void
remove_data(input_vect *inp, hash_table_t tb)
{
    size_t i;
    datum** pd;
    perf p = perfinit();
    perf p0;

    VECT_FOR_EACHi(inp, i, pd) {
        datum* d  = *pd;
        void* ret = 0;
        int err;

        p0 = perfstart();
        err = hash_table_remove(tb, d, &ret);
        perfaccum(&p, p0);

        if (err != 0)
            error(1, -err, "** Fatal error while removing rec %d key %s!\n",
                    i, d->key);
    }
    
    show_timing("Removed ", VECT_LEN(inp), &p);

}


#if 0
static void
dumper(void* cookie, const void* x)
{
    (void)cookie;
    datum* d = (datum*)x;
    printf("   DUMP: %s => %s\n", d->key, d->val);
}
#endif

static void
store_data(input_vect *inp, hash_table_t tb)
{
    size_t i;
    datum** pd;
    perf p = perfinit();
    perf p0;

    VECT_FOR_EACHi(inp, i, pd) {
        datum* d = *pd;
        int err ;

        p0 = perfstart();
        err = hash_table_insert(tb, d);
        perfaccum(&p, p0);

        //printf("   Insert: %s [%zu] => %p\n", d->key, i, d);
        if (err != 0) {
            if (err == -EEXIST)
                error(1, 0, "** Duplicate  rec %d key :%s:\n", i, d->key) ;
            else
                error(1, 0, "** Fatal error while inserting rec %d key %s!\n", i, d->key);
        }
    }

    show_timing("Inserted", VECT_LEN(inp), &p);

    //hash_table_apply(tb, dumper, 0);
}


void
query_data(input_vect *inp, hash_table_t tb)
{
    size_t i;
    datum** pd;

    perf p = perfinit();
    perf p0;

    VECT_FOR_EACHi(inp, i, pd) {
        datum* d = *pd;
        void * ret = 0;
        int err;

        p0 = perfstart();
        err = hash_table_lookup(tb, d, &ret);
        perfaccum(&p, p0);

        if (!err)
            error(0, 0, "** Can't find rec %zu key %s\n", i, d->key);

        if (ret) {
            datum* x = (datum*)ret;
            //if (0 != strcmp(x->key, d->key))
            if (d != x)
                error(1, 0, "** fatal query error; rec %zu; exp %s, saw %s\n", i, d->key, x->key);
        }
    }

    show_timing("Queried ", VECT_LEN(inp), &p);
}


#if 0
static int
find_in_vect(input_vect* inp, datum* d)
{
    datum** px;
    VECT_FOR_EACH(inp, px) {
        datum* x = *px;

        if (0 == strcmp(x->key, d->key)) return 1;
    }

    return 0;
}
#endif


static void
iterator_test(input_vect* inp, hash_table_t tb, int sorted)
{
    hash_table_iter_t iter;
    size_t n = 0;
    char buf[64];

    if (hash_table_iter_new(&iter, tb, sorted) < 0)
        error(1, 0, "Can't create sorted iter");


    snprintf(buf, sizeof buf, "%s-iter ", sorted ? "sorted" : "vanilla");

    perf p  = perfinit();
    perf p0 = perfstart();

    hash_table_iter_first(iter);

    do
    {
        void * pair = hash_table_iter_item(iter);
        if (!pair) break;

        // Every element must be in the input vector
        //if (!find_in_vect(inp, (datum*)pair)) break;

        n++;
    } while (hash_table_iter_next(iter) != EOF);

    perfaccum(&p, p0);

    if ( n != VECT_LEN(inp) )
        error (0, 0, "Iterator returned '%d' items; expected '%d'\n", n, VECT_LEN(inp));

    hash_table_iter_delete(iter);

    show_timing(buf, VECT_LEN(inp), &p);
}


static void
Usage(int s)
{
    printf("\
Usage: %s [OPTIONS] file [file ..]\n\
       file                     Input file to read data from\n\
\n\
       OPTIONS:\n\
          --logsize=num -s num  Set hash table size to `1 << num' nodes\n\
          --fill=num, -f num    Set fill percent to `num' %%\n\
          --hash=NAME, -H NAME  Use hash function 'NAME' instead of default\n\
                                NAME must be one of 'default', 'hsieh', 'murmur', \n\
                                'fnv', 'city', 'siphash[*]', 'fast', 'yorrike'\n\
", program_name) ;

    exit(s) ;
}

/* -- Various hash functions -- */
static uint32_t
hash_murmur3(const void* x)
{
    const datum* p = (const datum*)x;
    return murmur3_hash_32(p->key, strlen((const char*)p->key), 0x58718AFD);
}

static uint32_t
hash_hsieh(const void* x)
{
    const datum* p = (const datum*)x;
    return hsieh_hash(p->key, strlen((const char*)p->key), 0);
}

static uint32_t
hash_fnv(const void* x)
{
    const datum* p = (const datum*)x;
    return fnv_hash(p->key, strlen((const char*)p->key), 0);
}

static uint32_t
hash_fast(const void* x)
{
    const datum* p = (const datum*)x;
    return fasthash32(p->key, strlen((const char*)p->key), 0);
}

static uint32_t
hash_city(const void* x)
{
    const datum* p = (const datum*)x;
    return fold32(city_hash(p->key, strlen((const char*)p->key), 0));
}

static uint32_t
hash_sip(const void* x)
{
    const datum* p = (const datum*)x;
    uint64_t key[2] = { 0x5555555555555555,  ~0x5555555555555555 };
    return fold32(siphash24(p->key, strlen((const char*)p->key), &key[0]));
}

static uint32_t
hash_yorrike(const void* x)
{
    const datum* p = (const datum*)x;
    return yorrike_hash32(p->key, strlen(p->key), 0);
}

/* EOF */
