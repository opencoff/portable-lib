
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utils/strmatch.h"

using namespace std;

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

    assert(v1.size() == 3);
    assert(v1[0] == 19);
    assert(v1[1] == 28);
    assert(v1[2] == 37);

    assert(v2.size() == 3);
    assert(v2[0] == 19);
    assert(v2[1] == 28);
    assert(v2[2] == 37);

    assert(v3.size() == 3);
    assert(v3[0] == 19);
    assert(v3[1] == 28);
    assert(v3[2] == 37);

    return 0;
}
