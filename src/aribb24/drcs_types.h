#ifndef DRCS_TYPES_H
#define DRCS_TYPES_H 1

#include <stdint.h>

typedef struct drcs_conversion_s
{
    char                     hash[32+1];
    unsigned int             code;

    struct drcs_conversion_s *p_next;
} drcs_conversion_t ;


#define ARIBSUB_GEN_DRCS_DATA
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

#endif