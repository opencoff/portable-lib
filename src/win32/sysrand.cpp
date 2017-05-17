/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sysrand.cpp - Win32 implementation of system random number
 * generator interface.
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Mon Sep 19 14:37:57 2005
 */


#include <windows.h>
#include <wincrypt.h>

#include "syserror.h"
#include "utils/sysrand.h"
#include "utils/random.h"

namespace putils {

typedef BOOL (WINAPI CryptAcquireContextA_F)(HCRYPTPROV *phProv,
                  LPCSTR pszContainer, LPCSTR pszProvider, DWORD dwProvType,
                  DWORD dwFlags );
typedef BOOL (WINAPI CryptGenRandom_F)(HCRYPTPROV hProv, DWORD dwLen,
                                          BYTE *pbBuffer );


sys_rand::sys_rand()
    : randgen("sysrand")
{
    CryptGenRandom_F * rand_fp  = NULL;
    HCRYPTPROV crypto_provider  = 0;

    HINSTANCE dll = GetModuleHandle("advapi32.dll");

    /*
     * Win32 crypto provider is in advapi32.dll.
     * One could've linked this into the application. But, that
     * would mean the user of this library has to know to do
     * that. Doing it this way is easier..
     */
    if (dll == NULL)
        throw dll_not_found("advapi32.dll");

    CryptAcquireContextA_F * crypt_ctxt =
        (CryptAcquireContextA_F*)GetProcAddress(dll, "CryptAcquireContextA");

    if (crypt_ctxt == NULL)
        throw dll_symbol_not_found("advapi32.dll", "CryptAcquireContextA");

    rand_fp = (CryptGenRandom_F*)GetProcAddress(dll, "CryptGenRandom");
    if (rand_fp == NULL)
        throw dll_symbol_not_found("advapi32.dll", "CryptGenRandom");

    if (! (*crypt_ctxt)(&crypto_provider, NULL, NULL,
                PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        throw sys_exception(geterror(), "Can't acquire crypt context");


    ptr = (void*)rand_fp;
    h1  = (void*)crypto_provider;
    h2  = (void*)dll;
}

sys_rand::~sys_rand()
{
    HINSTANCE dll = (HINSTANCE)h2;

    // XXX Do we call CryptReleaseContextA()?

    ::FreeLibrary(dll);
}

unsigned long
sys_rand::generate_long()
{
    union
    {
        unsigned long v;
        unsigned char buf[sizeof(unsigned long)];
    } u;

    generate_bits(u.buf, sizeof u.buf);
    return u.v;
}


void *
sys_rand::generate_bits(void* buf, size_t nbytes)
{
    HCRYPTPROV crypto_provider  = (HCRYPTPROV)h1;
    CryptGenRandom_F * rand_fp  = (CryptGenRandom_F*)ptr;

    if (! (*rand_fp)(crypto_provider, nbytes, (unsigned char *)buf) )
        throw sys_exception(geterror(), "System random generator failed");

    return buf;
}

} // namespace putils

/*
 * Return random bytes
 */
extern "C" void *
sys_randbytes(void * buf, int nbytes)
{
    static sys_rand r;

    return r.generate_bits(buf, nbytes);
}


/*
 * Return random u32
 */
extern "C" unsigned long
sys_randlong()
{
    static sys_rand r;

    return r.generate_long();
}

