/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * errno.c - Map win32 error to posix error.
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
 * Creation date: Sat Jan 21 18:09:15 2006
 */

#include <windows.h>
#include "win32/os.h"

struct error_map_type
{
    unsigned long   winerr; /* win32 error */
    int             perr;   /* POSIX errno */
};
typedef struct error_map_type error_map_type;

static const error_map_type    Errmap[] = 
{
    /*   1 */       {   ERROR_INVALID_FUNCTION          ,   EINVAL          }
    /*   2 */   ,   {   ERROR_FILE_NOT_FOUND            ,   ENOENT          }
    /*   3 */   ,   {   ERROR_PATH_NOT_FOUND            ,   ENOENT          }
    /*   4 */   ,   {   ERROR_TOO_MANY_OPEN_FILES       ,   EMFILE          }
    /*   5 */   ,   {   ERROR_ACCESS_DENIED             ,   EACCES          }
    /*   6 */   ,   {   ERROR_INVALID_HANDLE            ,   EBADF           }
    /*   7 */   ,   {   ERROR_ARENA_TRASHED             ,   ENOMEM          }
    /*   8 */   ,   {   ERROR_NOT_ENOUGH_MEMORY         ,   ENOMEM          }
    /*   9 */   ,   {   ERROR_INVALID_BLOCK             ,   ENOMEM          }
    /*  10 */   ,   {   ERROR_BAD_ENVIRONMENT           ,   E2BIG           }
    /*  11 */   ,   {   ERROR_BAD_FORMAT                ,   ENOEXEC         }
    /*  12 */   ,   {   ERROR_INVALID_ACCESS            ,   EINVAL          }
    /*  13 */   ,   {   ERROR_INVALID_DATA              ,   EINVAL          }
    /*  14 */   ,   {   ERROR_OUTOFMEMORY               ,   ENOMEM          }
    /*  15 */   ,   {   ERROR_INVALID_DRIVE             ,   ENOENT          }
    /*  16 */   ,   {   ERROR_CURRENT_DIRECTORY         ,   EINVAL          }
    /*  17 */   ,   {   ERROR_NOT_SAME_DEVICE           ,   EXDEV           }
    /*  18 */   ,   {   ERROR_NO_MORE_FILES             ,   ENOENT          }
    /*  19 */   ,   {   ERROR_WRITE_PROTECT             ,   EROFS           }
    /*  20 */   ,   {   ERROR_BAD_UNIT                  ,   ENXIO           }
    /*  21 */   ,   {   ERROR_NOT_READY                 ,   EBUSY           }
    /*  22 */   ,   {   ERROR_BAD_COMMAND               ,   EIO             }
    /*  23 */   ,   {   ERROR_CRC                       ,   EIO             }
    /*  24 */   ,   {   ERROR_BAD_LENGTH                ,   EIO             }
    /*  25 */   ,   {   ERROR_SEEK                      ,   EIO             }
    /*  26 */   ,   {   ERROR_NOT_DOS_DISK              ,   EIO             }
    /*  27 */   ,   {   ERROR_SECTOR_NOT_FOUND          ,   ENXIO           }
    /*  28 */   ,   {   ERROR_OUT_OF_PAPER              ,   EBUSY           }
    /*  29 */   ,   {   ERROR_WRITE_FAULT               ,   EIO             }
    /*  30 */   ,   {   ERROR_READ_FAULT                ,   EIO             }
    /*  31 */   ,   {   ERROR_GEN_FAILURE               ,   EIO             }
    /*  32 */   ,   {   ERROR_SHARING_VIOLATION         ,   EAGAIN          }
    /*  33 */   ,   {   ERROR_LOCK_VIOLATION            ,   EACCES          }
    /*  34 */   ,   {   ERROR_WRONG_DISK                ,   ENXIO           }
    /*  35 */   ,   {   35                              ,   ENFILE          }
    /*  36 */   ,   {   ERROR_SHARING_BUFFER_EXCEEDED   ,   ENFILE          }
    /*  37 */   ,   {   ERROR_HANDLE_EOF                ,   EINVAL          }
    /*  38 */   ,   {   ERROR_HANDLE_DISK_FULL          ,   ENOSPC          }
    /*  39 */   ,   {   0                               ,   0               }
    /*  40 */   ,   {   0                               ,   0               }
    /*  41 */   ,   {   0                               ,   0               }
    /*  42 */   ,   {   0                               ,   0               }
    /*  43 */   ,   {   0                               ,   0               }
    /*  44 */   ,   {   0                               ,   0               }
    /*  45 */   ,   {   0                               ,   0               }
    /*  46 */   ,   {   0                               ,   0               }
    /*  47 */   ,   {   0                               ,   0               }
    /*  48 */   ,   {   0                               ,   0               }
    /*  49 */   ,   {   0                               ,   0               }
    /*  50 */   ,   {   ERROR_NOT_SUPPORTED             ,   ENOSYS          }
    /*  51 */   ,   {   0                               ,   0               }
    /*  52 */   ,   {   0                               ,   0               }
    /*  53 */   ,   {   ERROR_BAD_NETPATH               ,   ENOENT          }
    /*  54 */   ,   {   0                               ,   0               }
    /*  55 */   ,   {   0                               ,   0               }
    /*  56 */   ,   {   0                               ,   0               }
    /*  57 */   ,   {   0                               ,   0               }
    /*  58 */   ,   {   0                               ,   0               }
    /*  59 */   ,   {   0                               ,   0               }
    /*  60 */   ,   {   0                               ,   0               }
    /*  61 */   ,   {   0                               ,   0               }
    /*  62 */   ,   {   0                               ,   0               }
    /*  63 */   ,   {   0                               ,   0               }
    /*  64 */   ,   {   0                               ,   0               }
    /*  65 */   ,   {   ERROR_NETWORK_ACCESS_DENIED     ,   EACCES          }
    /*  66 */   ,   {   0                               ,   0               }
    /*  67 */   ,   {   ERROR_BAD_NET_NAME              ,   ENOENT          }
    /*  68 */   ,   {   0                               ,   0               }
    /*  69 */   ,   {   0                               ,   0               }
    /*  70 */   ,   {   0                               ,   0               }
    /*  71 */   ,   {   0                               ,   0               }
    /*  72 */   ,   {   0                               ,   0               }
    /*  73 */   ,   {   0                               ,   0               }
    /*  74 */   ,   {   0                               ,   0               }
    /*  75 */   ,   {   0                               ,   0               }
    /*  76 */   ,   {   0                               ,   0               }
    /*  77 */   ,   {   0                               ,   0               }
    /*  78 */   ,   {   0                               ,   0               }
    /*  79 */   ,   {   0                               ,   0               }
    /*  80 */   ,   {   ERROR_FILE_EXISTS               ,   EEXIST          }
    /*  81 */   ,   {   0                               ,   0               }
    /*  82 */   ,   {   ERROR_CANNOT_MAKE               ,   EACCES          }
    /*  83 */   ,   {   ERROR_FAIL_I24                  ,   EACCES          }
    /*  84 */   ,   {   0                               ,   0               }
    /*  85 */   ,   {   0                               ,   0               }
    /*  86 */   ,   {   0                               ,   0               }
    /*  87 */   ,   {   ERROR_INVALID_PARAMETER         ,   EINVAL          }
    /*  88 */   ,   {   0                               ,   0               }
    /*  89 */   ,   {   ERROR_NO_PROC_SLOTS             ,   EAGAIN          }
    /*  90 */   ,   {   0                               ,   0               }
    /*  91 */   ,   {   0                               ,   0               }
    /*  92 */   ,   {   0                               ,   0               }
    /*  93 */   ,   {   0                               ,   0               }
    /*  94 */   ,   {   0                               ,   0               }
    /*  95 */   ,   {   0                               ,   0               }
    /*  96 */   ,   {   0                               ,   0               }
    /*  97 */   ,   {   0                               ,   0               }
    /*  98 */   ,   {   0                               ,   0               }
    /*  99 */   ,   {   0                               ,   0               }
    /* 100 */   ,   {   0                               ,   0               }
    /* 101 */   ,   {   0                               ,   0               }
    /* 102 */   ,   {   0                               ,   0               }
    /* 103 */   ,   {   0                               ,   0               }
    /* 104 */   ,   {   0                               ,   0               }
    /* 105 */   ,   {   0                               ,   0               }
    /* 106 */   ,   {   0                               ,   0               }
    /* 107 */   ,   {   0                               ,   0               }
    /* 108 */   ,   {   ERROR_DRIVE_LOCKED              ,   EACCES          }
    /* 109 */   ,   {   ERROR_BROKEN_PIPE               ,   EPIPE           }
    /* 110 */   ,   {   0                               ,   0               }
    /* 111 */   ,   {   ERROR_BUFFER_OVERFLOW           ,   ENAMETOOLONG    }
    /* 112 */   ,   {   ERROR_DISK_FULL                 ,   ENOSPC          }
    /* 113 */   ,   {   0                               ,   0               }
    /* 114 */   ,   {   ERROR_INVALID_TARGET_HANDLE     ,   EBADF           }
    /* 115 */   ,   {   0                               ,   0               }
    /* 116 */   ,   {   0                               ,   0               }
    /* 117 */   ,   {   0                               ,   0               }
    /* 118 */   ,   {   0                               ,   0               }
    /* 119 */   ,   {   0                               ,   0               }
    /* 120 */   ,   {   0                               ,   0               }
    /* 121 */   ,   {   0                               ,   0               }
    /* 122 */   ,   {   0                               ,   0               }
    /* 123 */   ,   {   ERROR_INVALID_NAME              ,   ENOENT          }
    /* 124 */   ,   {   ERROR_INVALID_HANDLE            ,   EINVAL          }
    /* 125 */   ,   {   0                               ,   0               }
    /* 126 */   ,   {   0                               ,   0               }
    /* 127 */   ,   {   0                               ,   0               }
    /* 128 */   ,   {   ERROR_WAIT_NO_CHILDREN          ,   ECHILD          }
    /* 129 */   ,   {   ERROR_CHILD_NOT_COMPLETE        ,   ECHILD          }
    /* 130 */   ,   {   ERROR_DIRECT_ACCESS_HANDLE      ,   EBADF           }
    /* 131 */   ,   {   ERROR_NEGATIVE_SEEK             ,   EINVAL          }
    /* 132 */   ,   {   ERROR_SEEK_ON_DEVICE            ,   EACCES          }
    /* 133 */   ,   {   0                               ,   0               }
    /* 134 */   ,   {   0                               ,   0               }
    /* 135 */   ,   {   0                               ,   0               }
    /* 136 */   ,   {   0                               ,   0               }
    /* 137 */   ,   {   0                               ,   0               }
    /* 138 */   ,   {   0                               ,   0               }
    /* 139 */   ,   {   0                               ,   0               }
    /* 140 */   ,   {   0                               ,   0               }
    /* 141 */   ,   {   0                               ,   0               }
    /* 142 */   ,   {   0                               ,   0               }
    /* 143 */   ,   {   0                               ,   0               }
    /* 144 */   ,   {   0                               ,   0               }
    /* 145 */   ,   {   ERROR_DIR_NOT_EMPTY             ,   ENOTEMPTY       }
    /* 146 */   ,   {   0                               ,   0               }
    /* 147 */   ,   {   0                               ,   0               }
    /* 148 */   ,   {   0                               ,   0               }
    /* 149 */   ,   {   0                               ,   0               }
    /* 150 */   ,   {   0                               ,   0               }
    /* 151 */   ,   {   0                               ,   0               }
    /* 152 */   ,   {   0                               ,   0               }
    /* 153 */   ,   {   0                               ,   0               }
    /* 154 */   ,   {   0                               ,   0               }
    /* 155 */   ,   {   0                               ,   0               }
    /* 156 */   ,   {   0                               ,   0               }
    /* 157 */   ,   {   0                               ,   0               }
    /* 158 */   ,   {   ERROR_NOT_LOCKED                ,   EACCES          }
    /* 159 */   ,   {   0                               ,   0               }
    /* 160 */   ,   {   0                               ,   0               }
    /* 161 */   ,   {   ERROR_BAD_PATHNAME              ,   ENOENT          }
    /* 162 */   ,   {   0                               ,   0               }
    /* 163 */   ,   {   0                               ,   0               }
    /* 164 */   ,   {   ERROR_MAX_THRDS_REACHED         ,   EAGAIN          }
    /* 165 */   ,   {   0                               ,   0               }
    /* 166 */   ,   {   0                               ,   0               }
    /* 167 */   ,   {   ERROR_LOCK_FAILED               ,   EACCES          }
    /* 168 */   ,   {   0                               ,   0               }
    /* 169 */   ,   {   0                               ,   0               }
    /* 170 */   ,   {   0                               ,   0               }
    /* 171 */   ,   {   0                               ,   0               }
    /* 172 */   ,   {   0                               ,   0               }
    /* 173 */   ,   {   0                               ,   0               }
    /* 174 */   ,   {   0                               ,   0               }
    /* 175 */   ,   {   0                               ,   0               }
    /* 176 */   ,   {   0                               ,   0               }
    /* 177 */   ,   {   0                               ,   0               }
    /* 178 */   ,   {   0                               ,   0               }
    /* 179 */   ,   {   0                               ,   0               }
    /* 180 */   ,   {   0                               ,   0               }
    /* 181 */   ,   {   0                               ,   0               }
    /* 182 */   ,   {   0                               ,   0               }
    /* 183 */   ,   {   ERROR_ALREADY_EXISTS            ,   EEXIST          }
    /* 184 */   ,   {   0                               ,   0               }
    /* 185 */   ,   {   0                               ,   0               }
    /* 186 */   ,   {   0                               ,   0               }
    /* 187 */   ,   {   0                               ,   0               }
    /* 188 */   ,   {   0                               ,   0               }
    /* 189 */   ,   {   0                               ,   0               }
    /* 190 */   ,   {   0                               ,   0               }
    /* 191 */   ,   {   0                               ,   0               }
    /* 192 */   ,   {   0                               ,   0               }
    /* 193 */   ,   {   0                               ,   0               }
    /* 194 */   ,   {   0                               ,   0               }
    /* 195 */   ,   {   0                               ,   0               }
    /* 196 */   ,   {   0                               ,   0               }
    /* 197 */   ,   {   0                               ,   0               }
    /* 198 */   ,   {   0                               ,   0               }
    /* 199 */   ,   {   0                               ,   0               }

    /* 206 */   ,   {   ERROR_FILENAME_EXCED_RANGE      ,   ENAMETOOLONG    }

    /* 215 */   ,   {   ERROR_NESTING_NOT_ALLOWED       ,   EAGAIN          }

    /* 267 */   ,   {   ERROR_DIRECTORY                 ,   ENOTDIR         }

#ifdef EINPROGRESS
    /* 997 */   ,   {   ERROR_IO_PENDING                ,   EINPROGRESS     }
#else /* ? EINPROGRESS */
    /* 997 */   ,   {   ERROR_IO_PENDING                ,   EBUSY           }
#endif /* EINPROGRESS */

    /* 1816 */  ,   {   ERROR_NOT_ENOUGH_QUOTA          ,   ENOMEM          }
};

/* Return the size of a initialized array of any type */
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(a[0]))


/*
 * Map win32 error to a posix error.
 */
int
map_win32_error(unsigned long winerr)
{
    const error_map_type* map = &Errmap[0],
                        * end = map + ARRAY_SIZE(Errmap);
    for(; map < end; ++map)
    {
        if(winerr == map->winerr)
            return map->perr;
    }

    return EINVAL;
}

/* EOF */
