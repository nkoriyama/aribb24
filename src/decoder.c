/*****************************************************************************
 * decoder.c : ARIB STD-B24 JIS 8bit character code decoder
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "aribb24/decoder.h"
#include "aribb24_private.h"
#include "decoder_private.h"
#include "convtable.h"
#include "decoder_macro.h"
#include "drcs.h"

#if 0
/*****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.3.1 Caption management data
 *****************************************************************************/
typedef struct
{
} arib_caption_management_data_t;

/*****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.3.2 Caption statement data
 *****************************************************************************/
typedef struct
{
} arib_caption_statement_data_t;

/*****************************************************************************
 * ARIB STD-B24 VOLUME 1 Part 3 Chapter 9.2 Structure of data group
 *****************************************************************************/
typedef struct
{
    uint8_t  i_data_group_id;
    uint8_t  i_data_group_version;
    uint8_t  i_data_group_link_number;
    uint8_t  i_last_data_group_link_number;
    uint16_t i_data_group_size;

    arib_caption_management_data_t *caption_management_data;
    arib_caption_statement_data_t *caption_statement_data;
} arib_data_group_t;

#endif

/*****************************************************************************
 * ARIB STD-B24 JIS 8bit character code decoder
 *****************************************************************************/
struct arib_decoder_t
{
    arib_instance_t *p_instance;
    const unsigned char *buf;
    size_t count;
    char *ubuf;
    size_t ucount;
    int (**handle_gl)(arib_decoder_t *, int);
    int (**handle_gl_single)(arib_decoder_t *, int);
    int (**handle_gr)(arib_decoder_t *, int);
    int (*handle_g0)(arib_decoder_t *, int);
    int (*handle_g1)(arib_decoder_t *, int);
    int (*handle_g2)(arib_decoder_t *, int);
    int (*handle_g3)(arib_decoder_t *, int);
    int kanji_ku;

    int i_control_time;

    int i_color_map;
    int i_foreground_color;
    int i_foreground_color_prev;
    int i_background_color;
    int i_foreground_alpha;
    int i_background_alpha;

    int i_planewidth;
    int i_planeheight;

    int i_width;
    int i_height;
    int i_left;
    int i_top;

    int i_fontwidth;
    int i_fontwidth_cur;
    int i_fontheight;
    int i_fontheight_cur;

    int i_horint;
    int i_horint_cur;
    int i_verint;
    int i_verint_cur;

    int i_charwidth;
    int i_charheight;

    int i_right;
    int i_bottom;

    int i_charleft;
    int i_charbottom;

    arib_buf_region_t *p_region;
    bool b_need_next_region;
};

static void decoder_adjust_position( arib_decoder_t *decoder )
{
#if 0
    if( decoder->i_charleft < decoder->i_left )
    {
        decoder->i_charleft = decoder->i_left - decoder->i_charleft;
        decoder->i_charbottom -= ( 1 + decoder->i_charleft / decoder->i_width ) * decoder->i_charheight;
        decoder->i_charleft = decoder->i_right - ( decoder->i_charleft % decoder->i_width );
        decoder->i_charleft = decoder->i_left + ( decoder->i_charleft - decoder->i_left ) / decoder->i_charwidth * decoder->i_charwidth;
    }
    if( decoder->i_charleft >= decoder->i_right )
    {
        decoder->i_charleft = decoder->i_charleft - decoder->i_right;
        decoder->i_charbottom += ( 1 + decoder->i_charleft / decoder->i_width ) * decoder->i_charheight;
        decoder->i_charleft = decoder->i_left + ( decoder->i_charleft % decoder->i_width );
        decoder->i_charleft = decoder->i_left + ( decoder->i_charleft - decoder->i_left ) / decoder->i_charwidth * decoder->i_charwidth;
    }
#endif

    decoder->b_need_next_region = true;
}

static int u8_uctomb_aux( unsigned char *s, unsigned int uc, int n )
{
    int count;

    if( uc < 0x80 )
    {
        count = 1;
    }
    else if( uc < 0x800 )
    {
        count = 2;
    }
    else if( uc < 0x10000 )
    {
        count = 3;
    }
    else if( uc < 0x110000 )
    {
        count = 4;
    }
    else
    {
        return -1;
    }

    if( n < count )
    {
        return -2;
    }

    switch( count ) /* note: code falls through cases! */
    {
        case 4: s[3] = 0x80 | (uc & 0x3f); uc = uc >> 6; uc |= 0x10000;
        case 3: s[2] = 0x80 | (uc & 0x3f); uc = uc >> 6; uc |= 0x800;
        case 2: s[1] = 0x80 | (uc & 0x3f); uc = uc >> 6; uc |= 0xc0;
        case 1: s[0] = uc;
    }
    return count;
}

static int u8_uctomb( unsigned char *s, unsigned int uc, int n )
{
    if( uc < 0x80 && n > 0 )
    {
        s[0] = uc;
        return 1;
    }
    else
    {
        return u8_uctomb_aux( s, uc, n );
    }
}

static arib_buf_region_t *prepare_new_region( arib_decoder_t *decoder,
                                              char *p_start,
                                              int i_veradj,
                                              int i_horadj )
{
    arib_buf_region_t *p_region =
        (arib_buf_region_t*) calloc( 1, sizeof(arib_buf_region_t) );
    if( p_region == NULL )
    {
        return NULL;
    }
    p_region->p_start = p_start;
    p_region->i_foreground_color = decoder->i_foreground_color;
    p_region->i_background_color = decoder->i_background_color;

    p_region->i_planewidth = decoder->i_planewidth;
    p_region->i_planeheight = decoder->i_planeheight;

    p_region->i_width = decoder->i_width;
    p_region->i_height = decoder->i_height;

    p_region->i_fontwidth = decoder->i_fontwidth_cur;
    p_region->i_fontheight = decoder->i_fontheight_cur;

    p_region->i_verint = decoder->i_verint_cur;
    p_region->i_horint = decoder->i_horint_cur;

    p_region->i_charleft = decoder->i_charleft;
    p_region->i_charbottom = decoder->i_charbottom;

    p_region->i_veradj = i_veradj;
    p_region->i_horadj = i_horadj;

    p_region->p_next = NULL;

    decoder->b_need_next_region = false;

    return p_region;
}

static int decoder_push( arib_decoder_t *decoder, unsigned int uc )
{
    char *p_start = decoder->ubuf;

    if( decoder->p_instance->b_replace_ellipsis && uc == 0x2026 )
    {
        // U+2026: HORIZONTAL ELLIPSIS
        // U+22EF: MIDLINE HORIZONTAL ELLIPSIS
        uc = 0x22ef;
    }

    if( decoder->i_foreground_color_prev != decoder->i_foreground_color )
    {
        decoder->i_foreground_color_prev = decoder->i_foreground_color;
        decoder->b_need_next_region = true;
    }

    /* Check for new paragraph/region */
    if( decoder->i_charleft >= decoder->i_right )
    {
        decoder->i_charleft = decoder->i_left;
        decoder->i_charbottom += decoder->i_charheight;
        decoder->b_need_next_region = true;
    }

    /* Ignore making new region */
    bool b_skip_making_new_region = false;
    if( decoder->b_need_next_region )
    {
        switch( uc )
        {
            //case 0x2026: /* HORIZONTAL ELLIPSIS */
            //case 0x22EF: /* MIDLINE HORIZONTAL ELLIPSIS */
            case 0x2192: /* RIGHTWARDS ARROW */
            case 0x3001: /* IDEOGRAPHIC COMMA */
            case 0x3002: /* IDEOGRAPHIC FULL STOP */
            case 0xFF0C: /* FULLWIDTH COMMA */
            case 0xFF0E: /* FULLWIDTH FULL STOP */
                decoder->b_need_next_region = false;
                b_skip_making_new_region = true;
                break;
            default:
                break;
        }
    }

    /* Adjust for somme characters */
    int i_veradj;
    int i_horadj;
    switch( uc )
    {
        case 0x2026: /* HORIZONTAL ELLIPSIS */
        case 0x22EF: /* MIDLINE HORIZONTAL ELLIPSIS */
        case 0x2192: /* RIGHTWARDS ARROW */
        case 0x2212: /* MINUS SIGN */
        case 0xFF0D: /* FULLWIDTH MINUS SIGN */
            i_veradj = decoder->i_fontheight * 1 / 3;
            i_horadj = 0;
            break;
        case 0x3000: /* IDEOGRAPHIC SPACE */
            i_veradj = decoder->i_fontheight * 2 / 3;
            i_horadj = 0;
            break;
        case 0x3001: /* IDEOGRAPHIC COMMA */
        case 0x3002: /* IDEOGRAPHIC FULL STOP */
            i_veradj = decoder->i_fontheight * 1 / 2;
            i_horadj = 0;
            break;
        case 0xFF1C: /* FULLWIDTH LESS-THAN SIGN */
        case 0xFF1E: /* FULLWIDTH GREATER-THAN SIGN */
        case 0x226A: /* MUCH LESS-THAN */
        case 0x226B: /* MUCH GREATER-THAN */
        //case 0x300A: /* LEFT DOUBLE ANGLE BRACKET */
        //case 0x300B: /* RIGHT DOUBLE ANGLE BRACKET */
            i_veradj = decoder->i_fontheight * 1 / 4;
            i_horadj = 0;
            break;
        case 0x300C: /* LEFT CORNER BRACKET */
        case 0x300E: /* LEFT WHITE CORNER BRACKET */
            i_veradj = 0;
            i_horadj = decoder->i_fontwidth * 1 / 6;
            break;
        case 0x300D: /* RIGHT CORNER BRACKET */
        case 0x300F: /* RIGHT WHITE CORNER BRACKET */
            i_veradj = decoder->i_fontheight * 1 / 6;
            i_horadj = 0;
            break;
        case 0x3063: /* HIRAGANA LETTER SMALL TU */
        case 0x30C3: /* KATAKANA LETTER SMALL TU */
            i_veradj = decoder->i_fontheight * 1 / 3;
            i_horadj = 0;
            break;
        case 0x3041: /* HIRAGANA LETTER SMALL A */
        case 0x30A1: /* KATAKANA LETTER SMALL A */
        case 0x3043: /* HIRAGANA LETTER SMALL I */
        case 0x30A3: /* KATAKANA LETTER SMALL I */
        case 0x3045: /* HIRAGANA LETTER SMALL U */
        case 0x30A5: /* KATAKANA LETTER SMALL U */
        case 0x3047: /* HIRAGANA LETTER SMALL E */
        case 0x30A7: /* KATAKANA LETTER SMALL E */
        case 0x3049: /* HIRAGANA LETTER SMALL O */
        case 0x30A9: /* KATAKANA LETTER SMALL O */
        case 0x3083: /* HIRAGANA LETTER SMALL YA */
        case 0x3085: /* HIRAGANA LETTER SMALL YU */
        case 0x3087: /* HIRAGANA LETTER SMALL YO */
        case 0x30E3: /* KATAKANA LETTER SMALL YA */
        case 0x30E5: /* KATAKANA LETTER SMALL YU */
        case 0x30E7: /* KATAKANA LETTER SMALL YO */
            i_veradj = decoder->i_fontheight * 1 / 6;
            i_horadj = 0;
            break;
        case 0x301C: /* WAVE DASH */
        case 0x30FC: /* KATAKANA-HIRAGANA PROLONGED SOUND MARK */
            i_veradj = decoder->i_fontheight * 1 / 3;
            i_horadj = 0;
            break;
        case 0x30FB: /* KATAKANA MIDDLE DOT */
            i_veradj = decoder->i_fontheight * 1 / 3;
            i_horadj = decoder->i_fontwidth * 1 / 6;
            break;
        case 0xFF08: /* FULLWIDTH LEFT PARENTHESIS */
        case 0xFF09: /* FULLWIDTH RIGHT PARENTHESIS */
            i_veradj = 0;
            i_horadj = decoder->i_fontwidth * 1 / 6;
            break;
        case 0xFF0C: /* FULLWIDTH COMMA */
        case 0xFF0E: /* FULLWIDTH FULL STOP */
            i_veradj = decoder->i_fontheight * 1 / 2;
            i_horadj = 0;
            break;
        default:
            i_veradj = 0;
            i_horadj = 0;
            break;
    }

    int i_cnt = u8_uctomb( (unsigned char*)decoder->ubuf, uc, decoder->ucount );
    if( i_cnt <= 0 )
    {
        return 0;
    }

    decoder->ubuf += i_cnt;
    decoder->ucount -= i_cnt;

    char *p_end = decoder->ubuf;

    decoder->i_charleft += decoder->i_charwidth;

    arib_buf_region_t *p_region = decoder->p_region;
    if( p_region == NULL )
    {
        p_region = decoder->p_region =
            prepare_new_region( decoder, p_start, i_veradj, i_horadj );
        if( p_region == NULL )
        {
            return 0;
        }
    }

    arib_buf_region_t *p_next;
    while( (p_next = p_region->p_next) != NULL )
    {
        p_region = p_next;
    }

    if( decoder->b_need_next_region )
    {
        p_region = p_region->p_next =
            prepare_new_region( decoder, p_start, i_veradj, i_horadj );
        if( p_region == NULL )
        {
            return 0;
        }
    }
    else
    {
        if( p_region->i_veradj > i_veradj )
        {
            p_region->i_veradj = i_veradj;
        }
    }
    p_region->p_end = p_end;

    if( b_skip_making_new_region )
    {
        decoder->b_need_next_region = true;
    }

    return 1;
}

static int decoder_pull( arib_decoder_t *decoder, int *c )
{
    if( decoder->count == 0 )
    {
        return 0;
    }

    *c = decoder->buf[0];

    decoder->count--;
    decoder->buf++;
    return 1;
}

static int decoder_handle_drcs( arib_decoder_t *decoder, int c )
{
    unsigned int uc;

    uc = 0;
    if( c < decoder->p_instance->p->i_drcs_num )
    {
        uc = decoder->p_instance->p->drcs_conv_table[c];
    }
    if( uc == 0 )
    {
        /* uc = 0x3000; */ /* WHITESPACE */
        /* uc = 0x25A1; */ /* WHITE SQUARE */
        uc = 0x3013; /* geta */
    }

    return decoder_push( decoder, uc );
}

static int decoder_handle_alnum( arib_decoder_t *decoder, int c )
{
    unsigned int uc;
    uc = decoder_alnum_table[c];
    uc += 0xfee0; /* FULLWIDTH */;
    return decoder_push( decoder, uc );
}

static int decoder_handle_hiragana( arib_decoder_t *decoder, int c )
{
    unsigned int uc;
    uc = decoder_hiragana_table[c];
    return decoder_push( decoder, uc );
}

static int decoder_handle_katakana( arib_decoder_t *decoder, int c )
{
    unsigned int uc;
    uc = decoder_katakana_table[c];
    return decoder_push( decoder, uc );
}

static int decoder_handle_kanji( arib_decoder_t *decoder, int c )
{
    int ku, ten;
    unsigned int uc;

    ku = decoder->kanji_ku;
    if( ku < 0 )
    {
        decoder->kanji_ku = c;
        return 1;
    }
    decoder->kanji_ku = -1;

    ten = c;

    uc = decoder_kanji_table[ku][ten];
    if (decoder->p_instance->b_use_private_conv && ku >= 89)
    {
        uc = decoder_private_conv_table[ku -89][ten];
    }
    if( uc == 0 )
    {
        return 0;
    }

    return decoder_push( decoder, uc );
}

static int decoder_handle_gl( arib_decoder_t *decoder, int c )
{
    int (*handle)(arib_decoder_t *, int);

    if( c == 0x20 || c == 0x7f )
    {
        c = 0x3000;
        return decoder_push( decoder, c );
    }

    if( decoder->handle_gl_single == NULL )
    {
        handle = *decoder->handle_gl;
    }
    else
    {
        handle = *decoder->handle_gl_single;
    }
    decoder->handle_gl_single = NULL;


    return handle( decoder, c - 0x21 );
}

static int decoder_handle_gr( arib_decoder_t *decoder, int c )
{
    int (*handle)(arib_decoder_t *, int);

    if( c == 0xa0 || c == 0xff )
    {
        return 0;
    }

    handle = *decoder->handle_gr;

    return handle( decoder, c - 0xa1 );
}

static int decoder_handle_esc( arib_decoder_t *decoder )
{
    int c;
    int (**handle)(arib_decoder_t *, int);

    handle = &decoder->handle_g0;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x20: // DRCS
                break;
            case 0x24:
            case 0x28:
                break;
            case 0x29:
                handle = &decoder->handle_g1;
                break;
            case 0x2a:
                handle = &decoder->handle_g2;
                break;
            case 0x2b:
                handle = &decoder->handle_g3;
                break;
            case 0x30:
            case 0x37:
                *handle = decoder_handle_hiragana;
                return 1;
            case 0x31:
            case 0x38:
                *handle = decoder_handle_katakana;
                return 1;
            case 0x39:
            case 0x3b:
            case 0x42:
                *handle = decoder_handle_kanji;
                return 1;
            case 0x36:
            case 0x4a:
                *handle = decoder_handle_alnum;
                return 1;
            case 0x40:
            case 0x41:
            //case 0x42:
            case 0x43:
            case 0x44:
            case 0x45:
            case 0x46:
            case 0x47:
            case 0x48:
            case 0x49:
            //case 0x4a:
            case 0x4b:
            case 0x4c:
            case 0x4d:
            case 0x4e:
            case 0x4f:
                *handle = decoder_handle_drcs;
                return 1;
            case 0x6e: //LS2
                decoder->handle_gl = &decoder->handle_g2;
                return 1;
            case 0x6f: //LS3
                decoder->handle_gl = &decoder->handle_g3;
                return 1;
            case 0x70: //macro
                return 1;
            case 0x7c: //LS3R
                decoder->handle_gr = &decoder->handle_g3;
                return 1;
            case 0x7d: //LS2R
                decoder->handle_gr = &decoder->handle_g2;
                return 1;
            case 0x7e: //LS1R
                decoder->handle_gr = &decoder->handle_g1;
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_papf( arib_decoder_t *decoder )
{
    int c;
    int i = 0;
    int buf[1];
    while( decoder_pull( decoder, &c ) != 0 )
    {
        buf[i] = c;
        i++;
        if( i >= 1 )
        {
            break;
        }
    }
    if( i >= 1 )
    {
        decoder->i_charleft += decoder->i_charwidth * ( buf[0] & 0x3f );
        decoder_adjust_position( decoder );
    }
    return 1;
}

static int decoder_handle_aps( arib_decoder_t *decoder )
{
    int c;
    int i = 0;
    int buf[2];
    while( decoder_pull( decoder, &c ) != 0 )
    {
        buf[i] = c;
        i++;
        if( i >= 2 )
        {
            break;
        }
    }
    if( i >= 2 )
    {
        decoder->i_charbottom = decoder->i_top + decoder->i_charheight * ( 1 + ( buf[0] & 0x3f ) ) - 1;
        decoder->i_charleft = decoder->i_left + decoder->i_charwidth * ( buf[1] & 0x3f );
        decoder_adjust_position( decoder );
    }
    return 1;
}

static int decoder_handle_c0( arib_decoder_t *decoder, int c )
{
    /* ARIB STD-B24 VOLUME 1 Part 2 Chapter 7
     * Table 7-14 Control function character set code table */
    switch( c )
    {
        case 0x00: //NUL
        case 0x07: //BEL
            return 1;
        case 0x08: //APB
            decoder->i_charleft -= decoder->i_charwidth;
            decoder_adjust_position( decoder );
            return 1;
        case 0x09: //APF
            decoder->i_charleft += decoder->i_charwidth;
            decoder_adjust_position( decoder );
            return 1;
        case 0x0a: //APD
            decoder->i_charbottom += decoder->i_charheight;
            decoder_adjust_position( decoder );
            return 1;
        case 0x0b: //APU
            decoder->i_charbottom -= decoder->i_charheight;
            decoder_adjust_position( decoder );
            return 1;
        case 0x0c: //CS
            decoder->i_charleft = decoder->i_left;
            decoder->i_charbottom = decoder->i_top + decoder->i_charheight - 1;
            decoder_adjust_position( decoder );
            return 1;
        case 0x0d: //APR
            decoder->i_charleft = decoder->i_left;
            decoder->i_charbottom += decoder->i_charheight;
            decoder_adjust_position( decoder );
            return 1;
        case 0x0e: //LS1
            decoder->handle_gl = &decoder->handle_g1;
            return 1;
        case 0x0f: //LS0
            decoder->handle_gl = &decoder->handle_g0;
            return 1;
        case 0x16: //PAPF
            return decoder_handle_papf( decoder );
            return 1;
        case 0x18: //CAN
            return 1;
        case 0x19: //SS2
            decoder->handle_gl_single = &decoder->handle_g2;
            return 1;
        case 0x1b: //ESC
            return decoder_handle_esc( decoder );
        case 0x1c: //APS
            return decoder_handle_aps( decoder );
        case 0x1d: //SS3
            decoder->handle_gl_single = &decoder->handle_g3;
            return 1;
        case 0x1e: //RS
        case 0x1f: //US
            return 1;
        default:
            return 0;
    }
}

static int decoder_handle_szx( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x60:
                decoder->i_fontwidth_cur = decoder->i_fontwidth / 4;
                decoder->i_fontheight_cur = decoder->i_fontheight / 6;
                decoder->i_horint_cur = decoder->i_horint / 4;
                decoder->i_verint_cur = decoder->i_verint / 6;
                decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
                decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
                decoder->b_need_next_region = true;
                return 1;
            case 0x41:
                decoder->i_fontwidth_cur = decoder->i_fontwidth;
                decoder->i_fontheight_cur = decoder->i_fontheight * 2;
                decoder->i_horint_cur = decoder->i_horint;
                decoder->i_verint_cur = decoder->i_verint * 2;
                decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
                decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
                decoder->b_need_next_region = true;
                return 1;
            case 0x44:
                decoder->i_fontwidth_cur = decoder->i_fontwidth * 2;
                decoder->i_fontheight_cur = decoder->i_fontheight;
                decoder->i_horint_cur = decoder->i_horint * 2;
                decoder->i_verint_cur = decoder->i_verint;
                decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
                decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
                decoder->b_need_next_region = true;
                return 1;
            case 0x45:
                decoder->i_fontwidth_cur = decoder->i_fontwidth * 2;
                decoder->i_fontheight_cur = decoder->i_fontheight * 2;
                decoder->i_horint_cur = decoder->i_horint * 2;
                decoder->i_verint_cur = decoder->i_verint * 2;
                decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
                decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
                decoder->b_need_next_region = true;
                return 1;
            case 0x6b:
            case 0x64:
                decoder->i_fontwidth_cur = decoder->i_fontwidth;
                decoder->i_fontheight_cur = decoder->i_fontheight;
                decoder->i_horint_cur = decoder->i_horint;
                decoder->i_verint_cur = decoder->i_verint;
                decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
                decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
                decoder->b_need_next_region = true;
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_col( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x20:
                break;
            case 0x48:
                decoder->i_foreground_alpha = 0xff; // fully transparent
                return 1;
            default:
                return 1;
        }
    }
    return 0;
}

static int decoder_handle_flc( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x40:
            case 0x47:
            case 0x4f:
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_cdc( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x20:
                break;
            default:
                return 1;
        }
    }
    return 0;
}

static int decoder_handle_pol( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x40:
            case 0x41:
            case 0x42:
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_wmm( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x40:
            case 0x44:
            case 0x45:
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_macro( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x40:
            case 0x41:
            case 0x4f:
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_hlc( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x43:
            case 0x44:
            case 0x45:
            case 0x46:
            case 0x47:
            case 0x48:
            case 0x49:
            case 0x4a:
            case 0x4b:
            case 0x4c:
            case 0x4d:
            case 0x4e:
            case 0x4f:
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_rpc( arib_decoder_t *decoder )
{
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            default: /* 0x40 - 0x7F */
                return 1;
        }
    }
    return 0;
}

static int decoder_handle_csi( arib_decoder_t *decoder )
{
    int idx = 0;
    int buf[256];
    int c;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x20:
                break;
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3b:
            case 0x3c:
            case 0x3d:
            case 0x3e:
            case 0x3f:
                buf[idx] = c - 0x30;
                idx++;
                break;
            case 0x42: //GSM
            case 0x53: //SWF
            case 0x54: //CCC
            case 0x56: //SDF
                {
                    int i_sep = 0;
                    for( int i = 0; i < idx; i++ )
                    {
                        if( buf[i] == 0x0b )
                        {
                            i_sep = i;
                            break;
                        }
                    }
                    int i_param1 = 0;
                    for( int i = 0; i < i_sep; i++ )
                    {
                        i_param1 = i_param1 * 10;
                        i_param1 = i_param1 + buf[i];
                    }
                    int i_param2 = 0;
                    for( int i = i_sep + 1; i < idx; i++ )
                    {
                        i_param2 = i_param2 * 10;
                        i_param2 = i_param2 + buf[i];
                    }
                    decoder->i_width = i_param1;
                    decoder->i_height = i_param2;
                    decoder->i_right = decoder->i_left + decoder->i_width;
                    decoder->i_bottom = decoder->i_top + decoder->i_height;
                }
                return 1;
            case 0x57: //SSM
            case 0x58: //SHS
            case 0x59: //SVS
            case 0x5d: //GAA
            case 0x5e: //SRC
                return 1;
            case 0x5f: //SDP
                {
                    int i_sep = 0;
                    for( int i = 0; i < idx; i++ )
                    {
                        if( buf[i] == 0x0b )
                        {
                            i_sep = i;
                            break;
                        }
                    }
                    int i_param1 = 0;
                    for( int i = 0; i < i_sep; i++ )
                    {
                        i_param1 = i_param1 * 10;
                        i_param1 = i_param1 + buf[i];
                    }
                    int i_param2 = 0;
                    for( int i = i_sep + 1; i < idx; i++ )
                    {
                        i_param2 = i_param2 * 10;
                        i_param2 = i_param2 + buf[i];
                    }
                    decoder->i_left = i_param1;
                    decoder->i_top = i_param2;
                    decoder->i_right = decoder->i_left + decoder->i_width;
                    decoder->i_bottom = decoder->i_top + decoder->i_height;
                }
                return 1;
            case 0x6e: //RCS
                return 1;
            case 0x61: //ACPS
                {
                    int i_sep = 0;
                    for( int i = 0; i < idx; i++ )
                    {
                        if( buf[i] == 0x0b )
                        {
                            i_sep = i;
                            break;
                        }
                    }
                    int i_param1 = 0;
                    for( int i = 0; i < i_sep; i++ )
                    {
                        i_param1 = i_param1 * 10;
                        i_param1 = i_param1 + buf[i];
                    }
                    int i_param2 = 0;
                    for( int i = i_sep + 1; i < idx; i++ )
                    {
                        i_param2 = i_param2 * 10;
                        i_param2 = i_param2 + buf[i];
                    }
                    decoder->i_charleft = i_param1;
                    decoder->i_charbottom = i_param2;
                }
                decoder->b_need_next_region = true;
                return 1;
            case 0x62: //TCC
            case 0x63: //ORN
            case 0x64: //MDF
            case 0x65: //CFS
            case 0x66: //XCS
            case 0x68: //PRA
            case 0x69: //ACS
            case 0x6a: //UED
            case 0x6f: //SCS
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

static int decoder_handle_time( arib_decoder_t *decoder )
{
    int c;
    int i_mode = 0;
    while( decoder_pull( decoder, &c ) != 0 )
    {
        switch( c )
        {
            case 0x20:
                if( i_mode  == 0 )
                {
                    i_mode = 1;
                }
                break;
            case 0x28:
                i_mode = 5;
                break;
            case 0x29:
                i_mode = 6;
                break;
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3B:
                break;
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x43:
                if( i_mode != 0 )
                    return 1;
                break;
            default:
                if( i_mode == 1 && c >= 0x40 && c <= 0x7F )
                    decoder->i_control_time += c & 0x3f;
                    return 1;
                return 0;
        }
        if( i_mode == 0 )
            return 0;
    }
    return 0;
}

static int decoder_handle_c1( arib_decoder_t *decoder, int c )
{
    /* ARIB STD-B24 VOLUME 1 Part 2 Chapter 7
     * Table 7-14 Control function character set code table */
    switch( c )
    {
        case 0x80: //BKF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0x000000;
            decoder->i_color_map |= 0x0000;
            return 1;
        case 0x81: //RDF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0xFF0000;
            decoder->i_color_map |= 0x0001;
            return 1;
        case 0x82: //GRF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0x00FF00;
            decoder->i_color_map |= 0x0002;
            return 1;
        case 0x83: //YLF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0xFFFF00;
            decoder->i_color_map |= 0x0003;
            return 1;
        case 0x84: //BLF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0x0000FF;
            decoder->i_color_map |= 0x0004;
            return 1;
        case 0x85: //MGF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0xFF00FF;
            decoder->i_color_map |= 0x0005;
            return 1;
        case 0x86: //CNF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0x00FFFF;
            decoder->i_color_map |= 0x0006;
            return 1;
        case 0x87: //WHF
            decoder->i_foreground_color_prev = decoder->i_foreground_color;
            decoder->i_foreground_color = 0xFFFFFF;
            decoder->i_color_map |= 0x0007;
            return 1;
        case 0x88: //SSZ
            decoder->i_fontwidth_cur = decoder->i_fontwidth / 2;
            decoder->i_fontheight_cur = decoder->i_fontheight / 2;
            decoder->i_horint_cur = decoder->i_horint / 2;
            decoder->i_verint_cur = decoder->i_verint / 2;
            decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
            decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
            decoder->b_need_next_region = true;
            return 1;
        case 0x89: //MSZ
            decoder->i_fontwidth_cur = decoder->i_fontwidth / 2;
            decoder->i_fontheight_cur = decoder->i_fontheight;
            decoder->i_horint_cur = decoder->i_horint / 2;
            decoder->i_verint_cur = decoder->i_verint;
            decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
            decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
            decoder->b_need_next_region = true;
            return 1;
        case 0x8a: //NSZ
            decoder->i_fontwidth_cur = decoder->i_fontwidth;
            decoder->i_fontheight_cur = decoder->i_fontheight;
            decoder->i_horint_cur = decoder->i_horint;
            decoder->i_verint_cur = decoder->i_verint;
            decoder->i_charwidth = decoder->i_fontwidth_cur + decoder->i_horint_cur;
            decoder->i_charheight = decoder->i_fontheight_cur + decoder->i_verint_cur;
            decoder->b_need_next_region = true;
            return 1;
        case 0x8b: //SZX
            return decoder_handle_szx( decoder );
        case 0x90: //COL
            return decoder_handle_col( decoder );
        case 0x91: //FLC
            return decoder_handle_flc( decoder );
        case 0x92: //CDC
            return decoder_handle_cdc( decoder );
        case 0x93: //POL
            return decoder_handle_pol( decoder );
        case 0x94: //WMM
            return decoder_handle_wmm( decoder );
        case 0x95: //MACRO
            return decoder_handle_macro( decoder );
        case 0x97: //HLC
            return decoder_handle_hlc( decoder );
        case 0x98: //RPC
            return decoder_handle_rpc( decoder );
        case 0x99: //SPL
        case 0x9a: //STL
            return 1;
        case 0x9b: //CSI
            return decoder_handle_csi( decoder );
        case 0x9d: //TIME
            return decoder_handle_time( decoder );
        default:
            return 0;
    }
}

static int arib_decode( arib_decoder_t *decoder );
static int decoder_handle_default_macro( arib_decoder_t *decoder, int c )
{
    c=c+0x21;
    if( c >= 0x60 && c <= 0x6f )
    {
        const unsigned char* macro;
        int size;
        switch( c )
        {
            case 0x60:
                macro = decoder_default_macro_0;
                size = sizeof(decoder_default_macro_0);
                break;
            case 0x61:
                macro = decoder_default_macro_1;
                size = sizeof(decoder_default_macro_1);
                break;
            case 0x62:
                macro = decoder_default_macro_2;
                size = sizeof(decoder_default_macro_2);
                break;
            case 0x63:
                macro = decoder_default_macro_3;
                size = sizeof(decoder_default_macro_3);
                break;
            case 0x64:
                macro = decoder_default_macro_4;
                size = sizeof(decoder_default_macro_4);
                break;
            case 0x65:
                macro = decoder_default_macro_5;
                size = sizeof(decoder_default_macro_5);
                break;
            case 0x66:
                macro = decoder_default_macro_6;
                size = sizeof(decoder_default_macro_6);
                break;
            case 0x67:
                macro = decoder_default_macro_7;
                size = sizeof(decoder_default_macro_7);
                break;
            case 0x68:
                macro = decoder_default_macro_8;
                size = sizeof(decoder_default_macro_8);
                break;
            case 0x69:
                macro = decoder_default_macro_9;
                size = sizeof(decoder_default_macro_9);
                break;
            case 0x6a:
                macro = decoder_default_macro_a;
                size = sizeof(decoder_default_macro_a);
                break;
            case 0x6b:
                macro = decoder_default_macro_b;
                size = sizeof(decoder_default_macro_b);
                break;
            case 0x6c:
                macro = decoder_default_macro_c;
                size = sizeof(decoder_default_macro_c);
                break;
            case 0x6d:
                macro = decoder_default_macro_d;
                size = sizeof(decoder_default_macro_d);
                break;
            case 0x6e:
                macro = decoder_default_macro_e;
                size = sizeof(decoder_default_macro_e);
                break;
            case 0x6f:
                macro = decoder_default_macro_f;
                size = sizeof(decoder_default_macro_f);
                break;
        }
        {
            const unsigned char* buf = decoder->buf;
            size_t count = decoder->count;
            decoder->buf = macro;
            decoder->count = size;
            arib_decode( decoder );
            decoder->buf = buf;
            decoder->count = count;
        }
        return 1;
    }
    return 0;
}

static void dump( arib_instance_t *p_instance,
                  const unsigned char *start, const unsigned char *end )
{
    arib_log( p_instance, "could not decode ARIB string:" );
    while( start < end )
    {
        arib_log( p_instance, "%02x ", *start++ );
    }
    arib_log( p_instance, "<- here" );
}

static int arib_decode( arib_decoder_t *decoder )
{
    int (*handle)(arib_decoder_t *, int);
    int c;
    /* ARIB STD-B24 VOLUME 1 Part 2 Chapter 7 Figure 7-1 Code Table */
    while( decoder_pull( decoder, &c ) != 0 )
    {
        if( c < 0x20 )
        {
            handle = decoder_handle_c0;
        }
        else if( c <= 0x7f )
        {
            handle = decoder_handle_gl;
        }
        else if( c <= 0xa0 )
        {
            handle = decoder_handle_c1;
        }
        else
        {
            handle = decoder_handle_gr;
        }
        if( handle( decoder, c )  == 0 )
        {
            return 0;
        }
    }
    return 1;
}

void arib_initialize_decoder( arib_decoder_t* decoder )
{
    arib_finalize_decoder( decoder );
    decoder->buf = NULL;
    decoder->count = 0;
    decoder->ubuf = NULL;
    decoder->ucount = 0;
    decoder->handle_gl = &decoder->handle_g0;
    decoder->handle_gl_single = NULL;
    decoder->handle_gr = &decoder->handle_g2;
    decoder->handle_g0 = decoder_handle_kanji;
    decoder->handle_g1 = decoder_handle_alnum;
    decoder->handle_g2 = decoder_handle_hiragana;
    decoder->handle_g3 = decoder_handle_katakana;
    decoder->kanji_ku = -1;

    decoder->i_control_time = 0;

    decoder->i_color_map = 0x0000;
    decoder->i_foreground_color = 0xFFFFFF;
    decoder->i_foreground_color_prev = 0xFFFFFF;
    decoder->i_background_color = 0x000000;
    decoder->i_foreground_alpha = 0x00;
    decoder->i_background_alpha = 0x00;

    decoder->i_planewidth = 0;
    decoder->i_planeheight = 0;

    decoder->i_width = 0;
    decoder->i_height = 0;
    decoder->i_left = 0;
    decoder->i_top = 0;

    decoder->i_fontwidth = 0;
    decoder->i_fontwidth_cur = 0;
    decoder->i_fontheight = 0;
    decoder->i_fontheight_cur = 0;

    decoder->i_horint = 0;
    decoder->i_horint_cur = 0;
    decoder->i_verint = 0;
    decoder->i_verint_cur = 0;

    decoder->i_charwidth = 0;
    decoder->i_charheight = 0;

    decoder->i_right = 0;
    decoder->i_bottom = 0;

    decoder->i_charleft = 0;
    decoder->i_charbottom = 0;

    memset( decoder->p_instance->p->drcs_conv_table, 0,
            sizeof(decoder->p_instance->p->drcs_conv_table) );

    apply_drcs_conversion_table( decoder->p_instance );

    decoder->p_region = NULL;
    decoder->b_need_next_region = true;
}

static void arib_initialize_decoder_size_related( arib_decoder_t* decoder,
        int i_planewidth, int i_planeheight, int i_width, int i_height, int i_left, int i_top,
        int i_fontwidth, int i_fontheight, int i_horint, int i_verint
        )
{
    decoder->i_planewidth = i_planewidth;
    decoder->i_planeheight = i_planeheight;

    decoder->i_width = i_width;
    decoder->i_height = i_height;
    decoder->i_left = i_left;
    decoder->i_top = i_top;

    decoder->i_fontwidth = decoder->i_fontwidth_cur = i_fontwidth;
    decoder->i_fontheight = decoder->i_fontheight_cur = i_fontheight;

    decoder->i_horint = decoder->i_horint_cur = i_horint;
    decoder->i_verint = decoder->i_verint_cur = i_verint;

    decoder->i_charwidth = decoder->i_fontwidth + decoder->i_horint;
    decoder->i_charheight = decoder->i_fontheight + decoder->i_verint;

    decoder->i_right = decoder->i_left + decoder->i_width;
    decoder->i_bottom = decoder->i_top + decoder->i_height;

    decoder->i_charleft = decoder->i_left;
    decoder->i_charbottom = decoder->i_top + decoder->i_charheight - 1;
}

void arib_initialize_decoder_a_profile( arib_decoder_t* decoder )
{
    arib_initialize_decoder( decoder );

    decoder->handle_g3 = decoder_handle_default_macro;

    arib_initialize_decoder_size_related( decoder,
                960, 540, 620, 480, 170, 30,
                36, 36, 4, 24);
}

void arib_initialize_decoder_c_profile( arib_decoder_t* decoder )
{
    arib_initialize_decoder( decoder );

    decoder->handle_g0 = decoder_handle_drcs;
    decoder->handle_g2 = decoder_handle_kanji;

    arib_initialize_decoder_size_related( decoder,
            320, 180, 300, 160, 0, 0,
            18, 18, 2, 12);
}

void arib_finalize_decoder( arib_decoder_t* decoder )
{
    arib_buf_region_t *p_region, *p_region_next;
    for( p_region = decoder->p_region; p_region; p_region = p_region_next )
    {
        p_region_next = p_region->p_next;
        free( p_region );
    }
    decoder->p_region = NULL;
}

size_t arib_decode_buffer( arib_decoder_t* decoder,
                           const unsigned char *buf, size_t count,
                           char *ubuf, size_t ucount )
{
    decoder->buf = buf;
    decoder->count = count;
    decoder->ubuf = ubuf;
    decoder->ucount = ucount;

    if( arib_decode( decoder ) == 0 )
    {
        dump( decoder->p_instance, buf, decoder->buf );
    }
    size_t i_size = ucount - decoder->ucount;
    if ( ucount )
        ubuf[ i_size ] = 0;
    return i_size;
}

arib_decoder_t * arib_decoder_new( arib_instance_t *p_instance )
{
    arib_decoder_t *p_decoder = calloc( 1, sizeof( *p_decoder ) );
    if ( !p_decoder )
        return NULL;
    p_decoder->p_instance = p_instance;
    arib_log( p_decoder->p_instance, "arib decoder was created" );
    return p_decoder;
}

void arib_decoder_free( arib_decoder_t *p_decoder )
{
    arib_finalize_decoder( p_decoder );
    arib_log( p_decoder->p_instance, "arib decoder destroyed" );
    free( p_decoder );
}

time_t arib_decoder_get_time( arib_decoder_t *p_decoder )
{
    return (time_t) p_decoder->i_control_time * 1000000 / 10;
}

const arib_buf_region_t * arib_decoder_get_regions( arib_decoder_t *p_decoder )
{
    return p_decoder->p_region;
}
