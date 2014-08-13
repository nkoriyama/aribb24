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

#include "aribb24.h"

#include <stdint.h>
#include <time.h>

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

ARIB_API void arib_initialize_decoder( arib_decoder_t* decoder );

ARIB_API void arib_initialize_decoder_a_profile( arib_decoder_t* decoder );

ARIB_API void arib_initialize_decoder_c_profile( arib_decoder_t* decoder );

ARIB_API void arib_finalize_decoder( arib_decoder_t* decoder );

ARIB_API size_t arib_decode_buffer( arib_decoder_t* decoder,
                                    const unsigned char *buf, size_t count,
                                    char *ubuf, size_t ucount );

ARIB_API time_t arib_decoder_get_time( arib_decoder_t *decoder );

ARIB_API const arib_buf_region_t * arib_decoder_get_regions( arib_decoder_t * ); 

#endif
