#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "error.h"
#include "utils/utils.h"
#include "fast/list.h"

struct elem
{
    DL_LINK(elem) link;
    int val;
    int refcount;
};
typedef struct elem elem;

DL_HEAD_TYPEDEF(elem_head, elem);


static elem_head Head;

static int
rmelem(elem_head* head)
{
    int ndeletes = 0;
    elem* n = DL_FIRST(head);

    printf("BEGIN GC CYCLE\n");
    while (n)
    {
        elem* next = DL_NEXT(n, link);

        sleep(1);
        if (--n->refcount == 0)
        {
            elem* prev = DL_PREV(n, link);

            DL_REMOVE(head, n, link);

            next = prev ? DL_NEXT(prev, link) : 0;

            printf("   ** Element %d @ %p is now gone\n",
                    n->val, n);
            DEL(n);
            ++ndeletes;
        }
        printf(" [n=%p, next=%p]\n", n, next);
        n = next;
    }

    return ndeletes;
}

static void
dump_list(elem_head* head, const char* msg)
{
    elem* n;
    printf(msg);

    DL_FOREACH(head, n, link)
    {
        printf("  %p: %d ref=%d\n", n, n->val, n->refcount);
    }
}

int
main()
{
    elem* n;

    DL_INIT(&Head);

#if 1
    n = NEWZ(elem);
    n->val = 10;
    n->refcount = 2;
    DL_INSERT_HEAD(&Head, n, link);

    n = NEWZ(elem);
    n->val = 11;
    n->refcount = 1;
    DL_INSERT_HEAD(&Head, n, link);


    n = NEWZ(elem);
    n->val = 12;
    n->refcount = 3;
    DL_INSERT_HEAD(&Head, n, link);
#endif


    dump_list(&Head, "INITIAL STATE\n");
    while (rmelem(&Head))
    {
        dump_list(&Head, "POST GC CYCLE\n");
    }

    return 0;
}
