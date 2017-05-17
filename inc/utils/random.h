/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * random.h - Simple Random Number generator.
 *
 * Copyright (c) 2009 Sudhi Herle <sw at herle.net>
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
 * Creation date: Thu Apr 30 22:11:14 2009
 */

#ifndef __UTILS_RANDOM_H_1241147474__
#define __UTILS_RANDOM_H_1241147474__ 1

#include <string.h>
#include <stdlib.h>
#include <string>

namespace putils
{


class randgen
{
public:
    randgen(const char* name) : m_name(name) { }
    virtual ~randgen();


    std::string& name() { return m_name; }

    virtual unsigned long generate_long() = 0;
 

    virtual double generate_float();

    // Generate in the range [-INT_MAX -- INT_MAX]
    virtual int generate_int();

    virtual bool generate_bool();

    virtual unsigned char generate_byte();
    virtual void* generate_bits(void* buf, size_t bufsz);

    unsigned long range(long st, long en)
    {
        unsigned long n = en + 1 - st;
        unsigned long r = generate_long() % n;

        return st + r;
    }

protected:
    std::string m_name;
};


/*
 * System random number generator
 * Uses /dev/urandom on Unix/Posix systems and Crypto provider on
 * Win32 systems.
 */

class sys_rand : public randgen
{
public:
    sys_rand();
    virtual ~sys_rand();

    virtual unsigned long generate_long();
    virtual void* generate_bits(void* buf, size_t bufsz);

protected:
    union
    {
        void* ptr;
        int fd;
    };
    void* h1; // for win32
    void* h2; // for win32
};


/*
 * This is the famous Mersenne Twister random number generator.
 *
 *  Makato Matsumoto and Takuji Nishimura,
 *  "Mersenne Twister: A 623-Dimensionally Equidistributed Uniform
 *   Pseudo-Random Number Generator",
 *  ACM Transactions on Modeling and Computer Simulation,
 *  Vol. 8, No. 1, January 1998, pp 3--30.
 */
class mt_rand :  public randgen
{
public:
    mt_rand(unsigned long seed=0x4897b0af);
    virtual ~mt_rand();

    virtual unsigned long generate_long();
 

protected:
    void mix();

protected:
    enum { N_PERIOD = 624 };
    unsigned long m_mt[N_PERIOD]; /* the array for the state vector  */
    unsigned long m_mag01[2];
    int m_mti;             /* index */
};


/*
 * A Pseudo Random Number Generator of type Multiply with Carry.
 * Based on the k=2 PRNG described in
 *    http://www.ms.uky.edu/~mai/RandomNumber
 */
class rand_marsaglia : public randgen
{
public:
    rand_marsaglia(unsigned long z=362436069, unsigned long w=521288629,
                   bool random = true);
    virtual ~rand_marsaglia();

    virtual unsigned long generate_long();

private:
    unsigned long m_z,
                  m_w,
                  m_zm,
                  m_wm;
};



}; // namespace putils

#endif /* ! __UTILS_RANDOM_H_1241147474__ */

/* EOF */
