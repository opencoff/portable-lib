/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * strmatch.h -- Collection of string matching algorithm (template).
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
 * This file implements
 *
 *    - a linear-time string-matching algorithm due to Knuth,
 *      Morris, and Pratt [1].
 *      Inspired by code from Thomas Graf <tgraf@suug.ch> [2],
 *      converted to C++ template by Sudhi Herle.
 *
 *    - a linear-time string matching algorithm due to Rabin-Karp [3]
 *
 *    - a linear-time string matching algorithm due to Boyer-Moore [3]
 *
 *
 *   [1] Cormen, Leiserson, Rivest, Stein
 *       Introduction to Algorithms, 2nd Edition, MIT Press
 *
 *   [2] Patch to LKML -- lib/ts_kmp.c
 *   [3] http://www-igm.univ-mlv.fr/~lecroq/string/node1.html
 */

#ifndef __STRMATCH_H_1119564978__
#define __STRMATCH_H_1119564978__ 1


#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string>


// Template class that defines a KMP search algorithm instance
template<typename T> class kmp_search
{
public:
    typedef std::char_traits<T>  traits_type;
    typedef typename traits_type::char_type char_type;

    kmp_search(const T * pat, int patlen);
    ~kmp_search();

    // Match 'textlen' bytes of 'text'. Return all the matching
    // offsets in 'text' as a vector<int>. If there are no matches,
    // vector::size() will be 0 (zero).
    std::vector<int>  matchall(const T * text, int textlen);

    // Return the pattern length
    int patlen() const { return m_patlen; }

    // Return ptr to pattern
    const T* pat() const { return m_pat; }


protected:
    void calc_prefix_table();


    // Match 'textlen' bytes of 'text' and return true if text
    // contains 'pat', false otherwise.  If the match succeeds,
    // return the offset of the match in pair::second.
    //
    // NOTE: This function finds only _one_ successful match.
    std::pair<bool, int> match(const T * text, int textlen);



protected:
    T *              m_pat;
    int              m_patlen;
    std::vector<int> m_prefix_tab;

private:
    kmp_search();   // disallow
    kmp_search& operator=(const kmp_search&);   // disallow
};



template<typename T> void
kmp_search<T>::calc_prefix_table()
{
    int k, q;

    for (k = 0, q = 1; q < m_patlen; q++)
    {
        while (k > 0 && !traits_type::eq(m_pat[k], m_pat[q]))
            k = m_prefix_tab[k-1];
        if ( traits_type::eq(m_pat[k], m_pat[q]) )
            k++;
        m_prefix_tab[q] = k;
    }
}


template<typename T>
kmp_search<T>::kmp_search(const T * pat, int patlen)
    : m_patlen(patlen),
      m_prefix_tab(patlen)
{
    if ( patlen < 0 )
        throw std::length_error("patlen is negative");

    m_pat = new T[patlen];
    memcpy(m_pat, pat, sizeof(char_type) * patlen);
    calc_prefix_table();
}


template<typename T>
kmp_search<T>::~kmp_search()
{
    delete[] m_pat;
}


template<typename T> std::pair<bool, int>
kmp_search<T>::match(const T * text, int textlen)
{
    if ( textlen < 0 || !text )
        throw std::length_error("textlen is negative");

    std::pair<bool, int> r(false, -1);

    int i,
        q = 0;

    for (i = 0; i < textlen; i++)
    {
        while (q > 0 && !traits_type::eq(m_pat[q], text[i]))
            q = m_prefix_tab[q - 1];
        if ( traits_type::eq(m_pat[q], text[i]) )
            q++;
        if (q == m_patlen)
        {
            r.first  = true;
            r.second = i + 1 - m_patlen;
            break;
        }
    }

    return r;
}

template<typename T> std::vector<int>
kmp_search<T>::matchall(const  T * text, int textlen)
{
    std::vector<int> r;

    int off = 0;
    while(textlen > 0)
    {
        std::pair<bool, int> m = match(text, textlen);
        if ( !m.first )
            break;

        int consume  = m.second + m_patlen; // nbytes to consume
        int matchoff = off + m.second;      // offset relative to start of text

        text        += consume;
        textlen     -= consume;
        off         += consume;

        r.push_back(matchoff);
    }
    return r;
}


/*
 * Boyer-Moore algorithm (+Turbo mode)
 *  http://www-igm.univ-mlv.fr/~lecroq/string/node15.html
 */
template<typename T> class boyer_moore_search
{
public:
    typedef std::char_traits<T>             traits_type;
    typedef typename traits_type::char_type char_type;
#define _bm_alphabet_size       (1 << (sizeof(char_type) * 8))

    boyer_moore_search(const T * pat, int patlen);
    ~boyer_moore_search();


    // Match 'textlen' bytes of 'text'. Return all the matching
    // offsets in 'text' as a vector<int>. If there are no matches,
    // vector::size() will be 0 (zero).
    std::vector<int>  matchall(const T * text, int textlen);

    // Turbo mode:
    //  - remembers the "factor of text" that matched a suffix of
    //    the pattern during the last attempt
    //  - potentially jumps over this "factor"
    std::vector<int>  turbo_matchall(const T * text, int textlen);

    // Return the pattern length
    int patlen() const { return m_patlen; }

    // Return ptr to pattern
    const T* pat() const { return m_pat; }


protected:
    void calc_suffixes(std::vector<int>& suff);
    void calc_bad_char();
    void calc_good_shift();



protected:
    T *              m_pat;
    int              m_patlen;
    std::vector<int> m_bad_char,
                     m_good_shift;

private:
    boyer_moore_search();   // disallow
    boyer_moore_search& operator=(const boyer_moore_search&);   // disallow
};


template<typename T> void boyer_moore_search<T>::calc_bad_char()
{
    int i,
        m1 = m_patlen - 1;

    for (i = 0; i < _bm_alphabet_size; ++i)
        m_bad_char[i] = m_patlen;

    for (i = 0; i < m1; i++)
        m_bad_char[m_pat[i]] = m1 - i;
}

template<typename T> void boyer_moore_search<T>::calc_suffixes(std::vector<int>& suff)
{
    int f = 0, g, i;

    g       = m_patlen - 1;
    suff[g] = m_patlen;

    for (i = m_patlen - 2; i >= 0; --i)
    {
        int j = i + m_patlen - 1 - f;
        if (i > g && suff[j] < (i - g))
            suff[i] = suff[j];
        else
        {
            int q;

            if (i < g)
                g = i;

            f = i;
            q = m_patlen - 1 - f;
            while (g >= 0 && traits_type::eq(m_pat[g], m_pat[g + q]))
                --g;
            suff[i] = f - g;
        }
    }
}


template<typename T> void boyer_moore_search<T>::calc_good_shift()
{
    int i, j;
    std::vector<int> suff(m_patlen);

    calc_suffixes(suff);

    for (i = 0; i < m_patlen; ++i)
        m_good_shift[i] = m_patlen;

    j = 0;
    for (i = m_patlen - 1; i >= 0; --i)
    {
        if (suff[i] == (i + 1))
        {
            int k = m_patlen - 1 - i;
            for (; j < k; ++j)
            {
                if (m_good_shift[j] == m_patlen)
                    m_good_shift[j] = k;
            }
        }
    }
    for (i = 0; i <= m_patlen - 2; ++i)
        m_good_shift[m_patlen - 1 - suff[i]] = m_patlen - 1 - i;
}

template<typename T>
boyer_moore_search<T>::boyer_moore_search(const T * pat, int patlen)
    : m_patlen(patlen),
      m_bad_char(_bm_alphabet_size),
      m_good_shift(patlen)
{
    if ( patlen < 0 )
        throw std::length_error("patlen is negative");

    m_pat = new T[patlen];
    memcpy(m_pat, pat, sizeof(char_type) * patlen);

    calc_good_shift();
    calc_bad_char();
}


template<typename T>
boyer_moore_search<T>::~boyer_moore_search()
{
    delete[] m_pat;
}


template<typename T> std::vector<int>
boyer_moore_search<T>::matchall(const T * text, int textlen)
{
    if ( textlen < 0 || !text )
        throw std::length_error("textlen is negative");

    std::vector<int> r;

    int i,
        j = 0,
        n = textlen - m_patlen;
          
    while (j <= n)
    {
        for(i = m_patlen-1; i >= 0 && traits_type::eq(m_pat[i], text[i+j]); --i)
            ;

        // match found
        if (i < 0)
        {
            r.push_back(j);
            j += m_good_shift[0];
        }
        else
            j += std::max(m_good_shift[i], m_bad_char[text[i+j]] - m_patlen + 1 + i);
    }

    return r;
}

template<typename T> std::vector<int>
boyer_moore_search<T>::turbo_matchall(const T * text, int textlen)
{
    if ( textlen < 0 || !text )
        throw std::length_error("textlen is negative");

    std::vector<int> r;

    int i, j,
        u, v, turbo_shift, shift, bc_shift,
        n = textlen - m_patlen;

          
    j = u = 0;
    shift = m_patlen;
    while (j <= n)
    {
        i = m_patlen - 1;

        while(i >= 0 && traits_type::eq(m_pat[i], text[i+j]))
        {
            --i;
            if (u != 0 && i == (m_patlen-1-shift))
                i -= u;
        }

        // match found
        if (i < 0)
        {
            r.push_back(j);
            shift = m_good_shift[0];
            u     = m_patlen - shift;
        }
        else
        {
            v           = m_patlen - 1 - i;
            turbo_shift = u - v;
            bc_shift    = m_bad_char[text[i+j]] - m_patlen + 1 + i;
            shift       = std::max(turbo_shift, bc_shift);
            shift       = std::max(shift, m_good_shift[i]);
            if (shift == m_good_shift[i])
                u = std::min(m_patlen - shift, v);
            else
            {
                if (turbo_shift < bc_shift)
                    shift = std::max(shift, u+1);
                u = 0;
            }
        }
        j += shift;
    }

    return r;
}



/*
 * Rabin-Karp Algorithm implementation.
 */
template<typename T> class rabin_karp_search
{
public:
    typedef std::char_traits<T>  traits_type;
    typedef typename traits_type::char_type char_type;

    rabin_karp_search(const T * pat, int patlen);
    ~rabin_karp_search();

    // Match 'textlen' bytes of 'text'. Return all the matching
    // offsets in 'text' as a vector<int>. If there are no matches,
    // vector::size() will be 0 (zero).
    std::vector<int>  matchall(const T * text, int textlen);

    // Return the pattern length
    int patlen() const { return m_patlen; }

    // Return ptr to pattern
    const T* pat() const { return m_pat; }


protected:
    T *              m_pat;
    int              m_patlen;

private:
    rabin_karp_search();   // disallow
    rabin_karp_search& operator=(const rabin_karp_search&);   // disallow
};


template<typename T>
rabin_karp_search<T>::rabin_karp_search(const T * pat, int patlen)
    : m_patlen(patlen)
{
    if ( patlen < 0 )
        throw std::length_error("patlen is negative");

    m_pat = new T[patlen];
    memcpy(m_pat, pat, sizeof(char_type) * patlen);
}


template<typename T>
rabin_karp_search<T>::~rabin_karp_search()
{
    delete[] m_pat;
}

template<typename T> std::vector<int>
rabin_karp_search<T>::matchall(const T * text, int textlen)
{
#define _rehash(a,b,h)   ((((h) - ((a) * d)) << 1) + (b))
    int d, i, j, q,
        h_pat, h_txt;


    std::vector<int> r;

    // compute d = 2^(m-1)
    d = 1 << (m_patlen-1);
    for (h_pat = h_txt = i = 0; i < m_patlen; ++i)
    {
        h_pat = (h_pat << 1) + m_pat[i];
        h_txt = (h_txt << 1) + text[i];
    }

    j = 0;
    q = textlen - m_patlen;
    while (j <= q)
    {
        if (h_pat == h_txt && 0 == memcmp(m_pat, text+j, m_patlen * sizeof(char_type)))
        {
            r.push_back(j);
        }
        h_txt = _rehash(text[j], text[j+m_patlen], h_txt);
        ++j;
    }

    return r;
}

#endif /* ! __STRMATCH_H_1119564978__ */

/* EOF */
