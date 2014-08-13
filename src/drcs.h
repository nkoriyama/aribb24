/*****************************************************************************
 * drcs.h : ARIB STD-B24 JIS 8bit character code decoder
 *****************************************************************************
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

#ifndef DRCS_PRIVATE_H
#define DRCS_PRIVATE_H 1

#include <stdint.h>

typedef struct drcs_conversion_s
{
    char                     hash[32+1];
    unsigned int             code;

    struct drcs_conversion_s *p_next;
} drcs_conversion_t ;


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

bool apply_drcs_conversion_table( arib_instance_t * );
bool load_drcs_conversion_table( arib_instance_t * );
void save_drcs_pattern( arib_instance_t *, int, int, int, const int8_t* );

#endif
