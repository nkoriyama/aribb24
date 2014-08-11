/*****************************************************************************
 * aribsub.c : ARIB STD-B24 bitstream parser
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#include "png.h"
#include "md5.h"
#include "aribb24/parser.h"

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

char* get_arib_data_dir( arib_parser_t *p_parser )
{
    const char *psz_arib_base_dir = p_parser->psz_arib_base_dir;
    if( psz_arib_base_dir == NULL )
    {
        return NULL;
    }

    char *psz_arib_data_dir;
    if( asprintf( &psz_arib_data_dir, "%s"DIR_SEP"data", psz_arib_base_dir ) < 0 )
    {
        psz_arib_data_dir = NULL;
    }

    return psz_arib_data_dir;
}

void create_arib_basedir( arib_parser_t *p_parser )
{
    const char *psz_arib_base_dir = p_parser->psz_arib_base_dir;
    if( psz_arib_base_dir == NULL )
    {
        return;
    }

    struct stat st;
    if( stat( psz_arib_base_dir, &st ) )
    {
        if( mkdir( psz_arib_base_dir, 0777) != 0 )
        {
            // ERROR
        }
    }
}

void create_arib_datadir( arib_parser_t *p_parser )
{
    create_arib_basedir( p_parser );
    char *psz_arib_data_dir = get_arib_data_dir( p_parser );
    if( psz_arib_data_dir == NULL )
    {
        return;
    }

    struct stat st;
    if( stat( psz_arib_data_dir, &st ) )
    {
        if( mkdir( psz_arib_data_dir, 0777) == 0 )
        {
            // ERROR
        }
    }

    free( psz_arib_data_dir );
}

static void load_drcs_conversion_table( arib_parser_t *p_parser )
{
    create_arib_basedir( p_parser );
    const char *psz_arib_base_dir = p_parser->psz_arib_base_dir;
    if( psz_arib_base_dir == NULL )
    {
        return;
    }

    char* psz_conv_file;
    if( asprintf( &psz_conv_file, "%s"DIR_SEP"drcs_conv.ini", psz_arib_base_dir ) < 0 )
    {
        psz_conv_file = NULL;
    }
    if( psz_conv_file == NULL )
    {
        return;
    }

    FILE *fp = fopen( psz_conv_file, "r" );
    free( psz_conv_file );
    if( fp == NULL )
    {
        return;
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
            p_parser->p_drcs_conv = p_next;
        }
        else
        {
            p_drcs_conv->p_next = p_next;
        }
        p_drcs_conv = p_next;
        p_drcs_conv->p_next = NULL;
    }

    fclose( fp );
}

FILE* open_image_file( arib_parser_t* p_parser, const char *psz_hash )
{
    FILE* fp = NULL;
    create_arib_datadir( p_parser );

    char *psz_arib_data_dir = get_arib_data_dir( p_parser );
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

    struct stat st;
    if( stat( psz_image_file, &st ) )
    {
        fp = fopen( psz_image_file, "wb" );
        if( fp == NULL )
        {
            // ERROR
        }
    }

    free( psz_image_file );
    return fp;
}

char* get_drcs_pattern_data_hash(
        arib_parser_t *p_parser,
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

void save_drcs_pattern_data_image(
        arib_parser_t *p_parser,
        const char* psz_hash,
        int i_width, int i_height,
        int i_depth, const int8_t* p_patternData )
{
    FILE *fp = open_image_file( p_parser, psz_hash );
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
}

void save_drcs_pattern(
        arib_parser_t *p_parser,
        int i_width, int i_height,
        int i_depth, const int8_t* p_patternData )
{
    char* psz_hash = get_drcs_pattern_data_hash( p_parser,
            i_width, i_height, i_depth, p_patternData );

    strncpy( p_parser->drcs_hash_table[p_parser->i_drcs_num], psz_hash, 32 );
    p_parser->drcs_hash_table[p_parser->i_drcs_num][32] = '\0';

    p_parser->i_drcs_num++;

    save_drcs_pattern_data_image( p_parser, psz_hash,
            i_width, i_height, i_depth, p_patternData );

    free( psz_hash );
}

void parse_data_unit_statement_body( arib_parser_t *p_parser, bs_t *p_bs,
                                           uint8_t i_data_unit_parameter,
                                           uint32_t i_data_unit_size )
{
    char* p_data_unit_data_byte = (char*) calloc(
            i_data_unit_size + 1, sizeof(char) );
    if( p_data_unit_data_byte == NULL )
    {
        return;
    }
    for( uint32_t i = 0; i < i_data_unit_size; i++ )
    {
        p_data_unit_data_byte[i] = bs_read( p_bs, 8 );
        p_parser->i_data_unit_size += 1;
    }
    p_data_unit_data_byte[i_data_unit_size] = '\0';

    memcpy( p_parser->psz_subtitle_data + p_parser->i_subtitle_data_size,
            p_data_unit_data_byte, i_data_unit_size );
    p_parser->i_subtitle_data_size += i_data_unit_size;

    free( p_data_unit_data_byte );
}

void parse_data_unit_DRCS( arib_parser_t *p_parser, bs_t *p_bs,
                                  uint8_t i_data_unit_parameter,
                                  uint32_t i_data_unit_size )
{
#ifdef ARIBSUB_GEN_DRCS_DATA
    if( p_parser->p_drcs_data != NULL )
    {
        for( int i = 0; i < p_parser->p_drcs_data->i_NumberOfCode; i++ )
        {
            drcs_code_t* p_drcs_code = &p_parser->p_drcs_data->p_drcs_code[i];
            for( int j = 0; j < p_drcs_code->i_NumberOfFont ; j++ )
            {
                drcs_font_data_t *p_drcs_font_data =  &p_drcs_code->p_drcs_font_data[j];
                free( p_drcs_font_data->p_drcs_pattern_data );
                free( p_drcs_font_data->p_drcs_geometric_data );
            }
            free( p_drcs_code->p_drcs_font_data );
        }
        free( p_parser->p_drcs_data->p_drcs_code );
        free( p_parser->p_drcs_data );
    }
    p_parser->p_drcs_data = (drcs_data_t*) calloc( 1, sizeof(drcs_data_t) );
    if( p_parser->p_drcs_data == NULL )
    {
        return;
    }
#endif //ARIBSUB_GEN_DRCS_DATA

    int8_t i_NumberOfCode = bs_read( p_bs, 8 );
    p_parser->i_data_unit_size += 1;

#ifdef ARIBSUB_GEN_DRCS_DATA
    p_parser->p_drcs_data->i_NumberOfCode = i_NumberOfCode;
    p_parser->p_drcs_data->p_drcs_code = (drcs_code_t*) calloc(
            i_NumberOfCode, sizeof(drcs_code_t) );
    if( p_parser->p_drcs_data->p_drcs_code == NULL )
    {
        return;
    }
#endif //ARIBSUB_GEN_DRCS_DATA

    for( int i = 0; i < i_NumberOfCode; i++ )
    {
        bs_skip( p_bs, 16 ); /* i_character_code */
        p_parser->i_data_unit_size += 2;
        uint8_t i_NumberOfFont = bs_read( p_bs, 8 );
        p_parser->i_data_unit_size += 1;

#ifdef ARIBSUB_GEN_DRCS_DATA
        drcs_code_t *p_drcs_code = &p_parser->p_drcs_data->p_drcs_code[i];
        p_drcs_code->i_CharacterCode = i_CharacterCode;
        p_drcs_code->i_NumberOfFont = i_NumberOfFont;
        p_drcs_code->p_drcs_font_data = (drcs_font_data_t*) calloc(
                i_NumberOfFont, sizeof(drcs_font_data_t) );
        if( p_drcs_code->p_drcs_font_data == NULL )
        {
            return;
        }
#else
#endif //ARIBSUB_GEN_DRCS_DATA

        for( int j = 0; j < i_NumberOfFont; j++ )
        {
            bs_skip( p_bs, 4 ); /* i_fontID */
            uint8_t i_mode = bs_read( p_bs, 4 );
            p_parser->i_data_unit_size += 1;

#ifdef ARIBSUB_GEN_DRCS_DATA
            drcs_font_data_t* p_drcs_font_data =
                &p_drcs_code->p_drcs_font_data[j];
            p_drcs_font_data->i_fontId = i_fontId;
            p_drcs_font_data->i_mode = i_mode;
            p_drcs_font_data->p_drcs_pattern_data = NULL;
            p_drcs_font_data->p_drcs_geometric_data = NULL;
#else
#endif //ARIBSUB_GEN_DRCS_DATA

            if( i_mode == 0x00 || i_mode == 0x01 )
            {
                uint8_t i_depth = bs_read( p_bs, 8 );
                p_parser->i_data_unit_size += 1;
                uint8_t i_width = bs_read( p_bs, 8 );
                p_parser->i_data_unit_size += 1;
                uint8_t i_height = bs_read( p_bs, 8 );
                p_parser->i_data_unit_size += 1;

                int i_bits_per_pixel = ceil( sqrt( ( i_depth + 2 ) ) );

#ifdef ARIBSUB_GEN_DRCS_DATA
                drcs_pattern_data_t* p_drcs_pattern_data =
                    p_drcs_font_data->p_drcs_pattern_data =
                    (drcs_pattern_data_t*) calloc(
                            1, sizeof(drcs_pattern_data_t) );
                if( p_drcs_pattern_data == NULL )
                {
                    return;
                }
                p_drcs_pattern_data->i_depth = i_depth;
                p_drcs_pattern_data->i_width = i_width;
                p_drcs_pattern_data->i_height = i_height;
                p_drcs_pattern_data->p_patternData = (int8_t*) calloc(
                            i_width * i_height * i_bits_per_pixel / 8,
                            sizeof(int8_t) );
                if( p_drcs_pattern_data->p_patternData == NULL )
                {
                    return;
                }
#else
                int8_t *p_patternData = (int8_t*) calloc(
                            i_width * i_height * i_bits_per_pixel / 8,
                            sizeof(int8_t) );
                if( p_patternData == NULL )
                {
                    return;
                }
#endif //ARIBSUB_GEN_DRCS_DATA

                for( int k = 0; k < i_width * i_height * i_bits_per_pixel / 8; k++ )
                {
                    uint8_t i_patternData = bs_read( p_bs, 8 );
                    p_parser->i_data_unit_size += 1;
#ifdef ARIBSUB_GEN_DRCS_DATA
                    p_drcs_pattern_data->p_patternData[k] = i_patternData;
#else
                    p_patternData[k] = i_patternData;
#endif //ARIBSUB_GEN_DRCS_DATA
                }

#ifdef ARIBSUB_GEN_DRCS_DATA
                save_drcs_pattern( p_parser, i_width, i_height, i_depth + 2,
                                   p_drcs_pattern_data->p_patternData );
#else
                save_drcs_pattern( p_parser, i_width, i_height, i_depth + 2,
                                   p_patternData );
                free( p_patternData );
#endif //ARIBSUB_GEN_DRCS_DATA
            }
            else
            {
                bs_skip( p_bs, 8 ); /* i_regionX */
                p_parser->i_data_unit_size += 1;
                bs_skip( p_bs, 8 ); /* i_regionY */
                p_parser->i_data_unit_size += 1;
                uint16_t i_geometricData_length = bs_read( p_bs, 16 );
                p_parser->i_data_unit_size += 2;

#ifdef ARIBSUB_GEN_DRCS_DATA
                drcs_geometric_data_t* p_drcs_geometric_data =
                    p_drcs_font_data->p_drcs_geometric_data =
                    (drcs_geometric_data_t*) calloc(
                            1, sizeof(drcs_geometric_data_t) );
                if( p_drcs_geometric_data == NULL )
                {
                    return;
                }
                p_drcs_geometric_data->i_regionX = i_regionX;
                p_drcs_geometric_data->i_regionY = i_regionY;
                p_drcs_geometric_data->i_geometricData_length = i_geometricData_length;
                p_drcs_geometric_data->p_geometricData = (int8_t*)
                    calloc( i_geometricData_length, sizeof(int8_t) );
                if( p_drcs_geometric_data->p_geometricData == NULL )
                {
                    return;
                }
#else
#endif //ARIBSUB_GEN_DRCS_DATA

                for( int k = 0; k < i_geometricData_length ; k++ )
                {
                    bs_skip( p_bs, 8 ); /* i_geometric_data */
                    p_parser->i_data_unit_size += 1;

#ifdef ARIBSUB_GEN_DRCS_DATA
                    p_drcs_geometric_data->p_geometricData[k] = i_geometricData;
#else
#endif //ARIBSUB_GEN_DRCS_DATA
                }
            }
        }
    }
}

void parse_data_unit_others( arib_parser_t *p_parser, bs_t *p_bs,
                                    uint8_t i_data_unit_parameter,
                                    uint32_t i_data_unit_size )
{
    for( uint32_t i = 0; i < i_data_unit_size; i++ )
    {
        bs_skip( p_bs, 8 );
        p_parser->i_data_unit_size += 1;
    }
}

/*****************************************************************************
 * parse_data_unit
 *****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.4 Structure of data unit
 *****************************************************************************/
void parse_data_unit( arib_parser_t *p_parser, bs_t *p_bs )
{
    uint8_t i_unit_separator = bs_read( p_bs, 8 );
    p_parser->i_data_unit_size += 1;
    if( i_unit_separator != 0x1F )
    {
        return;
    }
    uint8_t i_data_unit_parameter = bs_read( p_bs, 8 );
    p_parser->i_data_unit_size += 1;
    uint32_t i_data_unit_size = bs_read( p_bs, 24 );
    p_parser->i_data_unit_size += 3;
    if( i_data_unit_parameter == 0x20 )
    {
        parse_data_unit_statement_body( p_parser, p_bs,
                                       i_data_unit_parameter,
                                       i_data_unit_size );
    }
    else if( i_data_unit_parameter == 0x30 ||
             i_data_unit_parameter == 0x31 )
    {
        parse_data_unit_DRCS( p_parser, p_bs,
                              i_data_unit_parameter,
                              i_data_unit_size );
    }
    else
    {
        parse_data_unit_others( p_parser, p_bs,
                                i_data_unit_parameter,
                                i_data_unit_size );
    }
}

/*****************************************************************************
 * parse_caption_management_data
 *****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.3.1 Caption management data
 *****************************************************************************/
void parse_caption_management_data( arib_parser_t *p_parser, bs_t *p_bs )
{
    uint8_t i_TMD = bs_read( p_bs, 2 );
    bs_skip( p_bs, 6 ); /* Reserved */
    if( i_TMD == 0x02 /* 10 */ )
    {
        bs_skip( p_bs, 32 ); /* OTM << 4 */
        bs_skip( p_bs, 4 ); /* OTM & 15 */
        bs_skip( p_bs, 4 ); /* Reserved */
    }
    uint8_t i_num_languages = bs_read( p_bs, 8 );
    for( int i = 0; i < i_num_languages; i++ )
    {
        bs_skip( p_bs, 3 ); /* i_language_tag */
        bs_skip( p_bs, 1 ); /* Reserved */
        uint8_t i_DMF = bs_read( p_bs, 4 );
        if( i_DMF == 0x0C /* 1100 */ ||
            i_DMF == 0x0D /* 1101 */ ||
            i_DMF == 0x0E /* 1110 */ )
        {
            bs_skip( p_bs, 8 ); /* i_DC */
        }
        bs_skip( p_bs, 24 ); /* i_ISO_639_language_code */
        bs_skip( p_bs, 4 ); /* i_Format */
        bs_skip( p_bs, 2 ); /* i_TCS */
        bs_skip( p_bs, 2 ); /* i_rollup_mode */
    }
    uint32_t i_data_unit_loop_length = bs_read( p_bs, 24 );
    free( p_parser->psz_subtitle_data );
    p_parser->i_data_unit_size = 0;
    p_parser->i_subtitle_data_size = 0;
    p_parser->psz_subtitle_data = NULL;
    if( i_data_unit_loop_length > 0 )
    {
        p_parser->psz_subtitle_data = (unsigned char*) calloc(
                i_data_unit_loop_length + 1, sizeof(unsigned char) );
    }
    while( p_parser->i_data_unit_size < i_data_unit_loop_length )
    {
        parse_data_unit( p_parser, p_bs );
    }
}

/*****************************************************************************
 * parse_caption_data
 *****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.3.2 Caption statement data
 *****************************************************************************/
void parse_caption_statement_data( arib_parser_t *p_parser, bs_t *p_bs )
{
    uint8_t i_TMD = bs_read( p_bs, 2 );
    bs_skip( p_bs, 6 ); /* Reserved */
    if( i_TMD == 0x01 /* 01 */ || i_TMD == 0x02 /* 10 */ )
    {
        bs_skip( p_bs, 32 ); /* STM << 4 */
        bs_skip( p_bs, 4 ); /* STM & 15 */
        bs_skip( p_bs, 4 ); /* Reserved */
    }
    uint32_t i_data_unit_loop_length = bs_read( p_bs, 24 );
    free( p_parser->psz_subtitle_data );
    p_parser->i_subtitle_data_size = 0;
    p_parser->psz_subtitle_data = NULL;
    if( i_data_unit_loop_length > 0 )
    {
        p_parser->psz_subtitle_data = (unsigned char*) calloc(
                i_data_unit_loop_length + 1, sizeof(unsigned char) );
    }
    while( p_parser->i_data_unit_size < i_data_unit_loop_length )
    {
        parse_data_unit( p_parser, p_bs );
    }
}

/*****************************************************************************
 * parse_data_group
 *****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.2 Structure of data group
 *****************************************************************************/
void parse_data_group( arib_parser_t *p_parser, bs_t *p_bs )
{
    uint8_t i_data_group_id = bs_read( p_bs, 6 );
    bs_skip( p_bs, 2 ); /* i_data_group_version */
    bs_skip( p_bs, 8 ); /* i_data_group_link_number */ 
    bs_skip( p_bs, 8 ); /* i_last_data_group_link_number */
    bs_skip( p_bs, 16 ); /* i_data_group_size */

    if( i_data_group_id == 0x00 || i_data_group_id == 0x20 )
    {
        parse_caption_management_data( p_parser, p_bs );
    }
    else
    {
        parse_caption_statement_data( p_parser, p_bs );
    }
}

/*****************************************************************************
 * arib_parse_pes
 *****************************************************************************
 * ARIB STD-B24 VOLUME3 Chapter 5 Independent PES transmission protocol
 *****************************************************************************/
void arib_parse_pes( arib_parser_t *p_parser, const void *p_data, size_t i_data )
{
    bs_t bs;
    bs_init( &bs, p_data, i_data );
    uint8_t i_data_group_id = bs_read( &bs, 8 );
    if( i_data_group_id != 0x80 && i_data_group_id != 0x81 )
    {
        return;
    }
    uint8_t i_private_stream_id = bs_read( &bs, 8 );
    if( i_private_stream_id != 0xFF )
    {
        return;
    }
    bs_skip( &bs, 4 ); /* reserved */
    uint8_t i_PES_data_packet_header_length= bs_read( &bs, 4 );

     /* skip PES_data_private_data_byte */
    bs_skip( &bs, i_PES_data_packet_header_length );

    parse_data_group( p_parser, &bs );
}

arib_parser_t * arib_parser_new( void *p_opaque, const char *psz_arib_basedir )
{
    arib_parser_t *p_parser = calloc( 1, sizeof(*p_parser) );
    if ( !p_parser )
       return NULL;
    p_parser->p_opaque = p_opaque;
    p_parser->psz_arib_base_dir = psz_arib_basedir ? strdup( psz_arib_basedir ) : NULL;
    if ( p_parser->psz_arib_base_dir )
        load_drcs_conversion_table( p_parser );
    return p_parser;
}

void arib_parser_free( arib_parser_t *p_parser )
{
    free( p_parser->psz_arib_base_dir );
    free( p_parser );
}
