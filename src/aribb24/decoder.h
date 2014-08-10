/*****************************************************************************
 * decorder.h : ARIB STD-B24 JIS 8bit character code decoder
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

#ifndef ARIBB24_DECODER_H
#define ARIBB24_DECODER_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if 0
/*****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.3.1 Caption management data
 *****************************************************************************/
typedef struct
{
} arib_caption_management_data_t;

/*****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.3.2 Caption statement data
 *****************************************************************************/
typedef struct
{
} arib_caption_statement_data_t;

/*****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.2 Structure of data group
 *****************************************************************************/
typedef struct
{
    uint8_t  i_data_group_id;
    uint8_t  i_data_group_version;
    uint8_t  i_data_group_link_number;
    uint8_t  i_last_data_group_link_number;
    uint16_t i_data_group_size;

    arib_caption_management_data_t *caption_management_data;
    arib_caption_statement_data_t *caption_statement_data;
} arib_data_group_t;

#endif

//#define ARIBSUB_GEN_DRCS_DATA
#ifdef ARIBSUB_GEN_DRCS_DATA
typedef struct drcs_geometric_data_s
{
    int8_t      i_regionX;
    int8_t      i_regionY;
    int16_t     i_geometricData_length;

    int8_t      *p_geometricData;
} drcs_geometric_data_t;

typedef struct drcs_pattern_data_s
{
    int8_t      i_depth;
    int8_t      i_width;
    int8_t      i_height;

    int8_t      *p_patternData;
} drcs_pattern_data_t;

typedef struct  drcs_font_data_s
{
    int8_t                i_fontId;
    int8_t                i_mode;

    drcs_pattern_data_t   *p_drcs_pattern_data;
    drcs_geometric_data_t *p_drcs_geometric_data;
} drcs_font_data_t;

typedef struct drc_code_s
{
    int16_t            i_CharacterCode;
    int8_t             i_NumberOfFont;
    drcs_font_data_t   *p_drcs_font_data;
} drcs_code_t;

typedef struct drcs_data_s
{
    int8_t      i_NumberOfCode;

    drcs_code_t *p_drcs_code;
} drcs_data_t;
#endif //ARIBSUB_GEN_DRCS_DATA

typedef struct arib_buf_region_s
{
    char *p_start;
    char *p_end;

    int i_foreground_color;
    int i_background_color;

    int i_foreground_alpha;
    int i_background_alpha;

    int i_planewidth;
    int i_planeheight;

    int i_width;
    int i_height;

    int i_fontwidth;
    int i_fontheight;

    int i_verint;
    int i_horint;

    int i_charleft;
    int i_charbottom;

    int i_veradj;
    int i_horadj;

    struct arib_buf_region_s *p_next;
} arib_buf_region_t;


/*****************************************************************************
 * ARIB STD-B24 JIS 8bit character code decoder
 *****************************************************************************/
typedef struct arib_decoder_s
{
    const unsigned char *buf;
    size_t count;
    char *ubuf;
    size_t ucount;
    int (**handle_gl)(struct arib_decoder_s *, int);
    int (**handle_gl_single)(struct arib_decoder_s *, int);
    int (**handle_gr)(struct arib_decoder_s *, int);
    int (*handle_g0)(struct arib_decoder_s *, int);
    int (*handle_g1)(struct arib_decoder_s *, int);
    int (*handle_g2)(struct arib_decoder_s *, int);
    int (*handle_g3)(struct arib_decoder_s *, int);
    int kanji_ku;

    int i_control_time;

    int i_color_map;
    int i_foreground_color;
    int i_foreground_color_prev;
    int i_background_color;
    int i_foreground_alpha;
    int i_background_alpha;

    int i_planewidth;
    int i_planeheight;

    int i_width;
    int i_height;
    int i_left;
    int i_top;

    int i_fontwidth;
    int i_fontwidth_cur;
    int i_fontheight;
    int i_fontheight_cur;

    int i_horint;
    int i_horint_cur;
    int i_verint;
    int i_verint_cur;

    int i_charwidth;
    int i_charheight;

    int i_right;
    int i_bottom;

    int i_charleft;
    int i_charbottom;

    int i_drcs_num;
    unsigned int drcs_conv_table[188];

    bool b_use_private_conv;

    bool b_replace_ellipsis;

    arib_buf_region_t *p_region;
    bool b_need_next_region;
} arib_decoder_t;

void arib_initialize_decoder( arib_decoder_t* decoder );

void arib_initialize_decoder_a_profile( arib_decoder_t* decoder );

void arib_initialize_decoder_c_profile( arib_decoder_t* decoder );

void arib_finalize_decoder( arib_decoder_t* decoder );

int arib_decode_buffer( arib_decoder_t* decoder,
                         const unsigned char *buf, size_t count,
                         char *ubuf, int ucount );

#endif
