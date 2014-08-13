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

/* If building or using aribb24 as a DLL, define ARIBB24_DLL.
 */
/* TODO: define ARIBB24_BUILD_DLL when building this library as DLL.
 */
#if defined _WIN32 || defined __CYGWIN__
  #ifdef ARIBB24_DLL
    #ifdef ARIBB24_BUILD_DLL
      #ifdef __GNUC__
        #define ARIB_API __attribute__ ((dllexport))
      #else
        #define ARIB_API __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define ARIB_API __attribute__ ((dllimport))
      #else
        #define ARIB_API __declspec(dllimport)
      #endif
    #endif
  #else
    #if __GNUC__ >= 4
      #define ARIB_API __attribute__ ((visibility ("default")))
    #else
      #define ARIB_API
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define ARIB_API __attribute__ ((visibility ("default")))
  #else
    #define ARIB_API
  #endif
#endif

typedef struct arib_instance_private_t arib_instance_private_t;
typedef struct arib_instance_t 
{
    bool b_generate_drcs;
    bool b_use_private_conv;
    bool b_replace_ellipsis;
    arib_instance_private_t *p;
} arib_instance_t;
typedef struct arib_parser_t arib_parser_t;
typedef struct arib_decoder_t arib_decoder_t;
typedef void(* arib_messages_callback_t)(void *, const char *);

ARIB_API arib_instance_t * arib_instance_new( void * );
ARIB_API void arib_instance_destroy( arib_instance_t * ); 

ARIB_API void arib_set_base_path( arib_instance_t *, const char * );

ARIB_API arib_parser_t * arib_get_parser( arib_instance_t * );
ARIB_API arib_decoder_t * arib_get_decoder( arib_instance_t * );

ARIB_API void arib_register_messages_callback( arib_instance_t *,
                                      arib_messages_callback_t );

#endif
