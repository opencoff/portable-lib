/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * rotatefile.h - Simple API to rotate a file.
 *
 * Copyright (c) 2007 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef __ROTATEFILE_H_1186176198__
#define __ROTATEFILE_H_1186176198__ 1


#include <string>
#include <exception>


namespace putils
{



/**
 * Unconditionally rotate a file and keep the last 'nsaved' copies.
 * Flags is currently unused. It can be used to communicate things
 * like "compress" etc.
 *
 * @param filename  File to rotate
 * @param nsaved    Number of backups to save
 * @param flags     Behavior modification flags; bitwise OR of the
 *                  ROTATE_xxx bit definitions above
 */
int rotate_filename(const std::string& filename, int nsaved, unsigned int flags = 0);


/**
 * Rotate a file if it exceeds size_mb MBytes and keep the last 'nsaved' copies.
 * Flags is currently unused. It can be used to communicate things
 * like "compress" etc.
 *
 * @param filename  File to rotate
 * @param size_mb   File size in Mega bytes (1024*1024 bytes) beyond
 *                  which the file will be rotated.
 * @param nsaved    Number of backups to save
 * @param flags     Behavior modification flags; bitwise OR of the
 *                  ROTATE_xxx bit definitions above
 */
int rotate_filename_by_size(const std::string& filename, int nsaved,
                    unsigned long size_mb, unsigned int flags = 0);

}

#endif /* ! __ROTATEFILE_H_1186176198__ */

/* EOF */
