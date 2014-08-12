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

#include "aribb24.h"

#define DEBUG_ARIBSUB 1

/****************************************************************************
 * Local structures
 ****************************************************************************/

ARIB_API void arib_parse_pes( arib_parser_t *, const void *p_data, size_t i_data );
ARIB_API const unsigned char * arib_parser_get_data( arib_parser_t *, size_t * );


#endif
