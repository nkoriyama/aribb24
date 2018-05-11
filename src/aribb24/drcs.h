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

#include "aribb24.h"

bool apply_drcs_conversion_table( arib_instance_t * );
bool load_drcs_conversion_table( arib_instance_t * );
void save_drcs_pattern( arib_instance_t *, int, int, int, const int8_t* );
ARIB_API char* get_drcs_pattern_data_hash(arib_instance_t *, int , int, int, const int8_t* );
// save_drcs_pattern_data_image はどうする？ 勝手に上がる気が...？

#endif
