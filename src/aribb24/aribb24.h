/*****************************************************************************
 * aribb24.h : ARIB STD-B24 JIS 8bit character code decoder
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

#ifndef ARIBB24_MAIN_H
#define ARIBB24_MAIN_H 1

#include <stdbool.h>

typedef struct arib_instance_private_t arib_instance_private_t;
typedef struct arib_instance_t 
{
    bool b_generate_drcs;
    arib_instance_private_t *p;
} arib_instance_t;
typedef struct arib_parser_t arib_parser_t;
typedef struct arib_decoder_t arib_decoder_t;
typedef void(* arib_messages_callback_t)(void *, const char *);

arib_instance_t * arib_instance_new( void * );
void arib_instance_destroy( arib_instance_t * ); 

void arib_set_base_path( arib_instance_t *, const char * );

arib_parser_t * arib_get_parser( arib_instance_t * );
arib_decoder_t * arib_get_decoder( arib_instance_t * );

void arib_register_messages_callback( arib_instance_t *,
                                      arib_messages_callback_t );

#endif
