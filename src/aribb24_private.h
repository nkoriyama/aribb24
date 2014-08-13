/*****************************************************************************
 * aribb24_private.h : ARIB STD-B24 JIS 8bit character code decoder
 *****************************************************************************
 * Copyright (C) 2014 François Cartegnie
 *
 * Authors:  François Cartegnie <fcvlcdev@free.fr>
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

#ifndef ARIBB24_PRIVATE_H
#define ARIBB24_PRIVATE_H 1

#include "drcs.h"

struct arib_instance_private_t
{
    void *p_opaque;
    arib_messages_callback_t pf_messages;
    arib_decoder_t *p_decoder;
    arib_parser_t *p_parser;
    char *psz_base_path;
    char *psz_last_error;

    drcs_conversion_t *p_drcs_conv;
    int i_drcs_num;
    unsigned int drcs_conv_table[188];
    char drcs_hash_table[188][32 + 1];
};

void arib_log( arib_instance_t *, const char *, ... );

#endif
