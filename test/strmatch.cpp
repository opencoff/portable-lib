
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utils/strmatch.h"

using namespace std;

static void
print_vect(const char* s, vector<int>& v)
{
    for (vector<int>::size_type i = 0; i < v.size(); ++i)
    {
        printf("%s: match at pos %d\n", s, v[i]);
    }
}

int
main()
{
    const char * pat = "chicken";
	const char *d = "We dance the funky chicken, chicken, chicken";


    kmp_search<char> kmp(pat, strlen(pat));
    boyer_moore_search<char> bm(pat, strlen(pat));
    rabin_karp_search<char> rk(pat, strlen(pat));

    vector<int> v1 = kmp.matchall(d, strlen(d));
    vector<int> v2 = bm.matchall(d, strlen(d));
    vector<int> v3 = bm.turbo_matchall(d, strlen(d));
    vector<int> v4 = rk.matchall(d, strlen(d));

    print_vect("KMP", v1);
    print_vect("BM",  v2);
    print_vect("TBM", v3);
    print_vect("RK", v4);

    return 0;
}
