/*****************************************************************************
 * aribb24.c : ARIB STD-B24 JIS 8bit character code decoder
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

#ifndef ARIBB24_MAIN_C
#define ARIBB24_MAIN_C 1

#include "aribb24/aribb24.h"
#include "aribb24_private.h"

#include "aribb24/parser.h"
#include "parser_private.h"
#include "aribb24/decoder.h"
#include "decoder_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

arib_instance_t * arib_instance_new( void *p_opaque )
{
    arib_instance_t *p_instance = calloc( 1, sizeof(*p_instance) );
    if ( !p_instance )
        return NULL;
    p_instance->p = calloc( 1, sizeof(*(p_instance->p)) );
    if (!p_instance->p)
    {
        free( p_instance );
        return NULL;
    }
    p_instance->p->p_opaque = p_opaque;
    return p_instance;
}

void arib_instance_destroy( arib_instance_t *p_instance )
{
    if ( p_instance->p->p_decoder )
        arib_decoder_free( p_instance->p->p_decoder ); 
    if ( p_instance->p->p_parser )
        arib_parser_free( p_instance->p->p_parser ); 
    free( p_instance );
}

void arib_register_messages_callback( arib_instance_t *p_arib_instance,
                                      arib_messages_callback_t pf_callback )
{
    p_arib_instance->p->pf_messages = pf_callback;
}

void arib_set_base_path( arib_instance_t *p_instance, const char *psz_path )
{
    if ( p_instance->p->psz_base_path )
        free( p_instance->p->psz_base_path );
    p_instance->p->psz_base_path = psz_path ? strdup( psz_path ): NULL;
}

arib_parser_t * arib_get_parser( arib_instance_t *p_instance )
{
    if ( !p_instance->p->p_parser )
        p_instance->p->p_parser = arib_parser_new( p_instance );
    return p_instance->p->p_parser;
}

arib_decoder_t * arib_get_decoder( arib_instance_t *p_instance )
{
    if ( !p_instance->p->p_decoder )
        p_instance->p->p_decoder = arib_decoder_new( p_instance );
    return p_instance->p->p_decoder;
}

#endif
