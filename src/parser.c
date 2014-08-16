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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>

#include "aribb24/aribb24.h"
#include "aribb24/parser.h"
#include "aribb24/bits.h"
#include "aribb24_private.h"
#include "parser_private.h"

struct arib_parser_t
{
    arib_instance_t  *p_instance;

    /* Decoder internal data */
#if 0
    arib_data_group_t data_group;
#endif
    uint32_t          i_data_unit_size;
    size_t            i_subtitle_data_size;
    unsigned char     *psz_subtitle_data;

#ifdef ARIBSUB_GEN_DRCS_DATA
    drcs_data_t       *p_drcs_data;
#endif //ARIBSUB_GEN_DRCS_DATA
};

static void parse_data_unit_statement_body( arib_parser_t *p_parser, bs_t *p_bs,
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

static void parse_data_unit_DRCS( arib_parser_t *p_parser, bs_t *p_bs,
                                  uint8_t i_data_unit_parameter,
                                  uint32_t i_data_unit_size )
{
    p_parser->p_instance->p->i_drcs_num = 0;
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
                save_drcs_pattern( p_parser->p_instance, i_width, i_height, i_depth + 2,
                                   p_drcs_pattern_data->p_patternData );
#else
                save_drcs_pattern( p_parser->p_instance, i_width, i_height, i_depth + 2,
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

static void parse_data_unit_others( arib_parser_t *p_parser, bs_t *p_bs,
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
static void parse_data_unit( arib_parser_t *p_parser, bs_t *p_bs )
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
static void parse_caption_management_data( arib_parser_t *p_parser, bs_t *p_bs )
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
static void parse_caption_statement_data( arib_parser_t *p_parser, bs_t *p_bs )
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
static void parse_data_group( arib_parser_t *p_parser, bs_t *p_bs )
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

arib_parser_t * arib_parser_new( arib_instance_t *p_instance )
{
    arib_parser_t *p_parser = calloc( 1, sizeof(*p_parser) );
    if ( !p_parser )
       return NULL;
    p_parser->p_instance = p_instance;
    arib_log( p_parser->p_instance, "arib parser was created" );
    if ( p_instance->p->psz_base_path )
    {
        load_drcs_conversion_table( p_parser->p_instance );
        arib_log( p_parser->p_instance, "could not load drcs conversion table" );
    }
    return p_parser;
}

void arib_parser_free( arib_parser_t *p_parser )
{
    arib_log( p_parser->p_instance, "arib parser was destroyed" );
    free( p_parser->psz_subtitle_data );
    free( p_parser );
}

const unsigned char * arib_parser_get_data( arib_parser_t *p_parser, size_t *pi_size )
{
    *pi_size = p_parser->i_subtitle_data_size;
    return p_parser->psz_subtitle_data;
}
