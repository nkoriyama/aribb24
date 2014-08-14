/*****************************************************************************
 * drcs.c : ARIB STD-B24 DRCS conversion tables handling
 *****************************************************************************
 * Copyright (C) 2014 Naohiro KORIYAMA
 *
 * Authors:  Naohiro KORIYAMA <nkoriyama@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef HAVE_PNG
  #include "png.h"
#endif
#include "aribb24/aribb24.h"
#include "aribb24/bits.h"
#include "aribb24_private.h"
#include "drcs.h"
#include "md5.h"

#if defined( _WIN32 ) || defined( __SYMBIAN32__ ) || defined( __OS2__ )
#   define mkdir(a,b) mkdir(a)
#endif

#if defined( _WIN32 ) || defined( __SYMBIAN32__ ) || defined( __OS2__ )
#   define DIR_SEP_CHAR '\\'
#   define DIR_SEP "\\"
#   define PATH_SEP_CHAR ';'
#   define PATH_SEP ";"
#else
#   define DIR_SEP_CHAR '/'
#   define DIR_SEP "/"
#   define PATH_SEP_CHAR ':'
#   define PATH_SEP ":"
#endif

#if defined( _WIN32 ) || defined( __SYMBIAN32__ ) || defined( __OS2__ )
#   define S_IRGRP 0
#   define S_IWGRP 0
#   define S_IXGRP 0
#   define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#   define S_IROTH 0
#   define S_IWOTH 0
#   define S_IXOTH 0
#   define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#endif

static char* get_arib_data_dir( arib_instance_t *p_instance )
{
    const char *psz_arib_base_path = p_instance->p->psz_base_path;
    if( psz_arib_base_path == NULL )
    {
        return NULL;
    }

    char *psz_arib_data_dir;
    if( asprintf( &psz_arib_data_dir, "%s"DIR_SEP"data", psz_arib_base_path ) < 0 )
    {
        psz_arib_data_dir = NULL;
    }

    return psz_arib_data_dir;
}

static bool create_arib_basedir( arib_instance_t *p_instance )
{
    const char *psz_arib_base_path = p_instance->p->psz_base_path;
    if( psz_arib_base_path == NULL )
    {
        return false;
    }

    struct stat st;
    if( stat( psz_arib_base_path, &st ) )
    {
        if( mkdir( psz_arib_base_path, 0700) != 0 )
        {
            return false;
        }
    }
    return true;
}

static bool create_arib_datadir( arib_instance_t *p_instance )
{
    create_arib_basedir( p_instance );
    char *psz_arib_data_dir = get_arib_data_dir( p_instance );
    if( psz_arib_data_dir == NULL )
    {
        return false;
    }

    struct stat st;
    if( stat( psz_arib_data_dir, &st ) )
    {
        if( mkdir( psz_arib_data_dir, 0700) == 0 )
        {
            free( psz_arib_data_dir );
            return false;
        }
    }

    free( psz_arib_data_dir );
    return true;
}

bool apply_drcs_conversion_table( arib_instance_t *p_instance )
{
    for( int i = 0; i < p_instance->p->i_drcs_num; i++ )
    {
        unsigned int uc = 0;
        drcs_conversion_t *p_drcs_conv = p_instance->p->p_drcs_conv;
        while( p_drcs_conv != NULL )
        {
            if( strcmp( p_drcs_conv->hash, p_instance->p->drcs_hash_table[i] ) == 0 )
            {
                uc = p_drcs_conv->code;
                break;
            }
            p_drcs_conv = p_drcs_conv->p_next;
        }
#ifdef DEBUG_ARIBSUB
        if( uc )
        {
            arib_log( p_instance, "Mapping [%s=U+%04x] will be used.",
                      p_instance->p->drcs_hash_table[i], uc );
        }
        else
        {
            arib_log( p_instance, "Mapping for hash[%s] is not found.",
                      p_instance->p->drcs_hash_table[i] );
        }
#endif
        p_instance->p->drcs_conv_table[i] = uc;
    }
    return true;
}

bool load_drcs_conversion_table( arib_instance_t *p_instance )
{
    if ( !create_arib_basedir( p_instance ) )
        return false;
    const char *psz_arib_base_path = p_instance->p->psz_base_path;
    if( psz_arib_base_path == NULL )
    {
        return false;
    }

    char* psz_conv_file;
    if( asprintf( &psz_conv_file, "%s"DIR_SEP"drcs_conv.ini", psz_arib_base_path ) < 0 )
    {
        psz_conv_file = NULL;
    }
    if( psz_conv_file == NULL )
    {
        return false;
    }

    FILE *fp = fopen( psz_conv_file, "r" );
    free( psz_conv_file );
    if( fp == NULL )
    {
        return false;
    }

    drcs_conversion_t *p_drcs_conv = NULL;
    char buf[256] = { 0 };
    while( fgets( buf, 256, fp ) != 0 )
    {
        if( buf[0] == ';' || buf[0] == '#' ) // comment
        {
            continue;
        }

        char* p_ret = strchr( buf, '\n' );
        if( p_ret != NULL )
        {
            *p_ret = '\0';
        }

        char *p_eq = strchr( buf, '=' );
        if( p_eq == NULL || p_eq - buf != 32 )
        {
            continue;
        }
        char *psz_code = strstr( buf, "U+" );
        if( psz_code == NULL || strlen( psz_code ) < 2 || strlen( psz_code ) > 8 )
        {
            continue;
        }

        char hash[32 + 1];
        strncpy( hash, buf, 32 );
        hash[32] = '\0';
        unsigned long code = strtoul( psz_code + 2, NULL, 16 );
        if( code > 0x10ffff )
        {
            continue;
        }

        drcs_conversion_t *p_next = (drcs_conversion_t*) calloc(
                1, sizeof(drcs_conversion_t) );
        if( p_next == NULL )
        {
            continue;
        }
        strncpy( p_next->hash, hash, 32 );
        p_next->hash[32] = '\0';
        p_next->code = code;
        if( p_drcs_conv == NULL )
        {
            p_instance->p->p_drcs_conv = p_next;
        }
        else
        {
            p_drcs_conv->p_next = p_next;
        }
        p_drcs_conv = p_next;
        p_drcs_conv->p_next = NULL;
    }

    fclose( fp );
    return true;
}

static FILE* open_image_file( arib_instance_t* p_instance, const char *psz_hash )
{
    FILE* fp = NULL;
    if ( !create_arib_datadir( p_instance ) )
        return NULL;

    char *psz_arib_data_dir = get_arib_data_dir( p_instance );
    if( psz_arib_data_dir == NULL )
    {
        return NULL;
    }

    char* psz_image_file;
    if( asprintf( &psz_image_file, "%s"DIR_SEP"%s.png", psz_arib_data_dir, psz_hash ) < 0 )
    {
        psz_image_file = NULL;
    }
    free( psz_arib_data_dir );
    if( psz_image_file == NULL )
    {
        return NULL;
    }

    int flags = O_CREAT | O_EXCL | O_WRONLY;
#if defined( _WIN32 ) || defined( __SYMBIAN32__ ) || defined( __OS2__ )
    flags |= O_BINARY;
#endif
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int fd = open( psz_image_file, flags, mode );
    if ( fd != -1 )
    {
        fp = fdopen( fd, "wb" );
        if( fp == NULL )
        {
            arib_log( p_instance, "Failed creating image file %s", psz_image_file );
            close( fd );
        }
    }

    free( psz_image_file );
    return fp;
}

static char* get_drcs_pattern_data_hash(
        arib_instance_t *p_instance,
        int i_width, int i_height,
        int i_depth, const int8_t* p_patternData )
{
    int i_bits_per_pixel = ceil( sqrt( ( i_depth ) ) );
    struct md5_s md5;
    InitMD5( &md5 );
    AddMD5( &md5, p_patternData, i_width * i_height * i_bits_per_pixel / 8 );
    EndMD5( &md5 );
    return psz_md5_hash( &md5 );
}

static void save_drcs_pattern_data_image(
        arib_instance_t *p_instance,
        const char* psz_hash,
        int i_width, int i_height,
        int i_depth, const int8_t* p_patternData )
{
#ifdef HAVE_PNG
    FILE *fp = open_image_file( p_instance, psz_hash );
    if( fp == NULL )
    {
        return;
    }

    png_structp png_ptr = png_create_write_struct(
            PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    if( png_ptr == NULL )
    {
        goto png_create_write_struct_failed;
    }
    png_infop info_ptr = png_create_info_struct( png_ptr );
    if( info_ptr == NULL )
    {
        goto png_create_info_struct_failed;
    }

    if( setjmp( png_jmpbuf( png_ptr ) ) )
    {
        goto png_failure;
    }

    png_set_IHDR( png_ptr,
                  info_ptr,
                  i_width,
                  i_height,
                  1,
                  PNG_COLOR_TYPE_PALETTE,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT );

    png_bytepp pp_image;
    pp_image = png_malloc( png_ptr, i_height * sizeof(png_bytep) );
    for( int j = 0; j < i_height; j++ )
    {
        pp_image[j] = png_malloc( png_ptr, i_width * sizeof(png_byte) );
    }

#if defined( __ANDROID__ )
    int i_bits_per_pixel = ceil( log( i_depth ) / log( 2 ) );
#else
    int i_bits_per_pixel = ceil( log2( i_depth ) );
#endif

    bs_t bs;
    bs_init( &bs, p_patternData, i_width * i_height * i_bits_per_pixel / 8 );

    for( int j = 0; j < i_height; j++ )
    {
        for( int i = 0; i < i_width; i++ )
        {
            uint8_t i_pxl = bs_read( &bs, i_bits_per_pixel );
            pp_image[j][i] = i_pxl ? 1 : 0;
        }
    }

    png_byte trans_values[1];
    trans_values[0] = (png_byte)0;
    png_set_tRNS( png_ptr, info_ptr, trans_values, 1, NULL );

    int colors[][3] =
    {
        {255, 255, 255}, /* background */
        {  0,   0,   0}, /* foreground */
    };
    png_color palette[2];
    for( int i = 0; i < 2; i++ )
    {
        palette[i].red = colors[i][0];
        palette[i].green = colors[i][1];
        palette[i].blue = colors[i][2];
    }
    png_set_PLTE( png_ptr, info_ptr, palette, 2 );

    png_init_io( png_ptr, fp );
    png_write_info( png_ptr, info_ptr );
    png_set_packing( png_ptr );
    png_write_image( png_ptr, pp_image );
    png_write_end( png_ptr, info_ptr );

    for( int j = 0; j < i_height; j++ )
    {
        png_free( png_ptr, pp_image[j] );
    }
    png_free( png_ptr, pp_image );

png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct( &png_ptr, &info_ptr );
png_create_write_struct_failed:
    fclose( fp );
#endif
}

void save_drcs_pattern(
        arib_instance_t *p_instance,
        int i_width, int i_height,
        int i_depth, const int8_t* p_patternData )
{
    char* psz_hash = get_drcs_pattern_data_hash( p_instance,
            i_width, i_height, i_depth, p_patternData );

    strncpy( p_instance->p->drcs_hash_table[p_instance->p->i_drcs_num], psz_hash, 32 );
    p_instance->p->drcs_hash_table[p_instance->p->i_drcs_num][32] = '\0';

    p_instance->p->i_drcs_num++;

    save_drcs_pattern_data_image( p_instance, psz_hash,
            i_width, i_height, i_depth, p_patternData );

    free( psz_hash );
}

