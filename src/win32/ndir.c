/* vim: ts=4:sw=4:expandtab:
 *
 * ndir.c - POSIX dirent.h implementation for win32 systems.
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
 * Notice:
 * =======
 * Everything non trivial in this code is from: @(#)msd_dir.c 1.4
 * 87/11/06.  A public domain implementation of BSD directory routines
 * for MS-DOS.  Written by Michael Rendell ({uunet,utai}michael@garfield),
 * August 1897
 *
 * Portability, ANSI-fication, rentrancy, and Win2k/WinXP changes
 * done by  Sudhi Herle<sw at herle.net> (c) 2005
 */


#include <io.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fast/vect.h"

#include <dirent.h>


// Define a vector of strings to hold the directory contents
VECT_TYPEDEF(dirlist_vect, char *);

struct _dirdesc
{
    dirlist_vect    dd_list;

    /* Current offset within the vector */
    int dd_off;

    /* Hold a copy here for re-entrancy purposes */
    struct dirent dd_de;
};

/* find ALL files! */
#define ATTRIBUTES	(_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_SUBDIR)


static char *
string_dup(char * s)
{
    size_t l = strlen (s) + 1;
    char * n = (char *) malloc(l);
    if ( n )
        memcpy(n, s, l);

    return n;
}


DIR *
opendir(const char *name)
{
    struct _finddata_t find_buf;
    DIR *dirp;
    char name_buf[_MAX_PATH + 1];
    char *slash = "";
    long hFile;

    if (!name)
        name = "";
    else if (*name)
    {
        const char *s;
        int l = strlen(name);

        assert(l < _MAX_PATH);

        s = name + l - 1;
        if ( !(l == 2 && *s == ':') && *s != '\\' && *s != '/')
            slash = "/";	/* save to insert slash between path and "*.*" */
    }


    strcat(strcat(strcpy(name_buf, name), slash), "*.*");

    // printf ("name=%s namebuf=%s\n", name, name_buf);

    dirp = (DIR *) malloc(sizeof (DIR));
    if ( !dirp )
        return 0;


    VECT_INIT(&dirp->dd_list, 128, char *);
    dirp->dd_off = 0;

    if ((hFile = _findfirst(name_buf, &find_buf)) < 0)
    {
        closedir(dirp);
        return 0;
    }

    do
    {
        char * entry = string_dup(find_buf.name);
        if ( !entry )
            goto _err;

        VECT_PUSH_BACK(&dirp->dd_list, entry, char *);
    } while (!_findnext(hFile, &find_buf));

    _findclose(hFile);

    return dirp;

_err:
    _findclose (hFile);
    closedir (dirp);
    return 0;
}


void
closedir(DIR *dirp)
{
    int i = 0;

    for (i = 0; i < VECT_SIZE(&dirp->dd_list); ++i)
    {
        char * e = VECT_ELEM(&dirp->dd_list, i);
        free (e);
    }
    VECT_FINI(&dirp->dd_list);
    free(dirp);
}


struct dirent *
readdir(DIR *dirp)
{
    struct dirent * de = &dirp->dd_de;
    char * nm;
    size_t len;

    if ( VECT_SIZE(&dirp->dd_list) == 0 ||
         VECT_SIZE(&dirp->dd_list) <= dirp->dd_off )
        return 0;

    nm  = VECT_ELEM(&dirp->dd_list, dirp->dd_off);
    len = strlen (nm);

    assert(len < sizeof (de->d_name));

    memcpy(de->d_name, nm, len+1);

    de->d_ino    = 0;
    de->d_namlen = de->d_reclen = len;
    dirp->dd_off++;

    return de;
}


void
seekdir(DIR *dirp, long off)
{
    if ( VECT_SIZE(&dirp->dd_list) > 0   &&
         VECT_SIZE(&dirp->dd_list) > off )
        dirp->dd_off = (int)off;
}


long
telldir(DIR *dirp)
{
    return dirp->dd_off;
}

/* EOF */
