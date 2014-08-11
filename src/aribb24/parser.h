/*****************************************************************************
 * parser.h : ARIB STD-B24 bitstream parser
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

#ifndef ARIBB24_PARSER_H
#define ARIBB24_PARSER_H 1

#include "aribb24/aribb24.h"
#include "aribb24/decoder.h"
#include "aribb24/bits.h"

#define DEBUG_ARIBSUB 1

/****************************************************************************
 * Local structures
 ****************************************************************************/

typedef struct drcs_conversion_s {
    char                     hash[32+1];
    unsigned int             code;

    struct drcs_conversion_s *p_next;
} drcs_conversion_t ;

struct arib_parser_t
{
    arib_instance_t  *p_instance;

    /* Decoder internal data */
#if 0
    arib_data_group_t data_group;
#endif
    uint32_t          i_data_unit_size;
    int               i_subtitle_data_size;
    unsigned char     *psz_subtitle_data;

    char              *psz_fontname;
    bool              b_ignore_ruby;
    bool              b_use_coretext;
    bool              b_ignore_position_adjustment;
    bool              b_replace_ellipsis;

#ifdef ARIBSUB_GEN_DRCS_DATA
    drcs_data_t       *p_drcs_data;
#endif //ARIBSUB_GEN_DRCS_DATA

    int               i_drcs_num;
    char              drcs_hash_table[188][32 + 1];

    drcs_conversion_t *p_drcs_conv;
};

void arib_parse_pes( arib_parser_t *, const void *p_data, size_t i_data );


#endif
