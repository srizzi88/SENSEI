/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Public (application) include file for PROJ.4 API, and constants.
 * Author:   Frank Warmerdam, <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam <warmerdam@pobox.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

/* General projections header file */
#ifndef PROJ_API_H
#define PROJ_API_H

#include "svtk_libproj_mangle.h"
#include "svtklibproj_export.h"

/* standard inclusions */
#include <math.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This version number should be updated with every release!  The format of
 * PJ_VERSION is
 *
 * * Before version 4.10.0: PJ_VERSION=MNP where M, N, and P are the major,
 *   minor, and patch numbers; e.g., PJ_VERSION=493 for version 4.9.3.
 *
 * * Version 4.10.0 and later: PJ_VERSION=MMMNNNPP later where MMM, NNN, PP
 *   are the major, minor, and patch numbers (the minor and patch numbers
 *   are padded with leading zeros if necessary); e.g., PJ_VERSION=401000
 *   for version 4.10.0.
 */
#define PJ_VERSION 493

/* pj_init() and similar functions can be used with a non-C locale */
/* Can be detected too at runtime if the symbol pj_atof exists */
#define PJ_LOCALE_SAFE 1

extern char const pj_release[]; /* global release id string */

#define RAD_TO_DEG	57.295779513082321
#define DEG_TO_RAD	.017453292519943296


extern int pj_errno;	/* global error return code */

#if !defined(PROJECTS_H)
    typedef struct { double u, v; } projUV;
    typedef struct { double u, v, w; } projUVW;
    typedef void *projPJ;
    #define projXY projUV
    #define projLP projUV
    #define projXYZ projUVW
    #define projLPZ projUVW
    typedef void *projCtx;
#else
    typedef PJ *projPJ;
    typedef projCtx_t *projCtx;
#   define projXY	XY
#   define projLP       LP
#   define projXYZ      XYZ
#   define projLPZ      LPZ
#endif

/* file reading api, like stdio */
typedef int *PAFile;
typedef struct projFileAPI_t {
    PAFile  (*FOpen)(projCtx ctx, const char *filename, const char *access);
    size_t  (*FRead)(void *buffer, size_t size, size_t nmemb, PAFile file);
    int     (*FSeek)(PAFile file, long offset, int whence);
    long    (*FTell)(PAFile file);
    void    (*FClose)(PAFile);
} projFileAPI;

/* procedure prototypes */

svtklibproj_EXPORT
projXY pj_fwd(projLP, projPJ);
svtklibproj_EXPORT
projLP pj_inv(projXY, projPJ);

svtklibproj_EXPORT
projXYZ pj_fwd3d(projLPZ, projPJ);
svtklibproj_EXPORT
projLPZ pj_inv3d(projXYZ, projPJ);

svtklibproj_EXPORT
int pj_transform( projPJ src, projPJ dst, long point_count, int point_offset,
                  double *x, double *y, double *z );
svtklibproj_EXPORT
int pj_datum_transform( projPJ src, projPJ dst, long point_count, int point_offset,
                        double *x, double *y, double *z );
svtklibproj_EXPORT
int pj_geocentric_to_geodetic( double a, double es,
                               long point_count, int point_offset,
                               double *x, double *y, double *z );
svtklibproj_EXPORT
int pj_geodetic_to_geocentric( double a, double es,
                               long point_count, int point_offset,
                               double *x, double *y, double *z );
svtklibproj_EXPORT
int pj_compare_datums( projPJ srcdefn, projPJ dstdefn );
svtklibproj_EXPORT
int pj_apply_gridshift( projCtx, const char *, int,
                        long point_count, int point_offset,
                        double *x, double *y, double *z );
svtklibproj_EXPORT
void pj_deallocate_grids(void);
svtklibproj_EXPORT
void pj_clear_initcache(void);
svtklibproj_EXPORT
int pj_is_latlong(projPJ);
svtklibproj_EXPORT
int pj_is_geocent(projPJ);
svtklibproj_EXPORT
void pj_get_spheroid_defn(projPJ defn, double *major_axis, double *eccentricity_squared);
svtklibproj_EXPORT
void pj_pr_list(projPJ);
svtklibproj_EXPORT
void pj_free(projPJ);
svtklibproj_EXPORT
void pj_set_finder( const char *(*)(const char *) );
svtklibproj_EXPORT
void pj_set_searchpath ( int count, const char **path );
svtklibproj_EXPORT
projPJ pj_init(int, char **);
svtklibproj_EXPORT
projPJ pj_init_plus(const char *);
svtklibproj_EXPORT
projPJ pj_init_ctx( projCtx, int, char ** );
svtklibproj_EXPORT
projPJ pj_init_plus_ctx( projCtx, const char * );
svtklibproj_EXPORT
char *pj_get_def(projPJ, int);
svtklibproj_EXPORT
projPJ pj_latlong_from_proj( projPJ );
svtklibproj_EXPORT
void *pj_malloc(size_t);
svtklibproj_EXPORT
void pj_dalloc(void *);
svtklibproj_EXPORT
void *pj_calloc (size_t n, size_t size);
svtklibproj_EXPORT
void *pj_dealloc (void *ptr);
svtklibproj_EXPORT
char *pj_strerrno(int);
svtklibproj_EXPORT
int *pj_get_errno_ref(void);
svtklibproj_EXPORT
const char *pj_get_release(void);
svtklibproj_EXPORT
void pj_acquire_lock(void);
svtklibproj_EXPORT
void pj_release_lock(void);
svtklibproj_EXPORT
void pj_cleanup_lock(void);

svtklibproj_EXPORT
projCtx pj_get_default_ctx(void);
svtklibproj_EXPORT
projCtx pj_get_ctx( projPJ );
svtklibproj_EXPORT
void pj_set_ctx( projPJ, projCtx );
svtklibproj_EXPORT
projCtx pj_ctx_alloc(void);
svtklibproj_EXPORT
void    pj_ctx_free( projCtx );
svtklibproj_EXPORT
int pj_ctx_get_errno( projCtx );
svtklibproj_EXPORT
void pj_ctx_set_errno( projCtx, int );
svtklibproj_EXPORT
void pj_ctx_set_debug( projCtx, int );
svtklibproj_EXPORT
void pj_ctx_set_logger( projCtx, void (*)(void *, int, const char *) );
svtklibproj_EXPORT
void pj_ctx_set_app_data( projCtx, void * );
svtklibproj_EXPORT
void *pj_ctx_get_app_data( projCtx );
svtklibproj_EXPORT
void pj_ctx_set_fileapi( projCtx, projFileAPI *);
svtklibproj_EXPORT
projFileAPI *pj_ctx_get_fileapi( projCtx );

svtklibproj_EXPORT
void pj_log( projCtx ctx, int level, const char *fmt, ... );
svtklibproj_EXPORT
void pj_stderr_logger( void *, int, const char * );

/* file api */
svtklibproj_EXPORT
projFileAPI *pj_get_default_fileapi();

svtklibproj_EXPORT
PAFile pj_ctx_fopen(projCtx ctx, const char *filename, const char *access);
svtklibproj_EXPORT
size_t pj_ctx_fread(projCtx ctx, void *buffer, size_t size, size_t nmemb, PAFile file);
svtklibproj_EXPORT
int    pj_ctx_fseek(projCtx ctx, PAFile file, long offset, int whence);
svtklibproj_EXPORT
long   pj_ctx_ftell(projCtx ctx, PAFile file);
svtklibproj_EXPORT
void   pj_ctx_fclose(projCtx ctx, PAFile file);
svtklibproj_EXPORT
char  *pj_ctx_fgets(projCtx ctx, char *line, int size, PAFile file);

svtklibproj_EXPORT
PAFile pj_open_lib(projCtx, const char *, const char *);

svtklibproj_EXPORT
int pj_run_selftests (int verbosity);


#define PJ_LOG_NONE        0
#define PJ_LOG_ERROR       1
#define PJ_LOG_DEBUG_MAJOR 2
#define PJ_LOG_DEBUG_MINOR 3

#ifdef __cplusplus
}
#endif

#endif /* ndef PROJ_API_H */

