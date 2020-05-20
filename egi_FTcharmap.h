/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


                        --- Definition and glossary ---

1. char:    A printable ASCII code OR a local character with UFT-8 encoding.
2. charmap: A EGI_FTCHAR_MAP struct that holds data of currently displayed chars,
            of both their corresponding coordinates in displaying window and offset position in memory.
3. dline:  displayed/charmapped line, A line starts/ends at displaying window left/right end side.
   retline: A line starts/ends by a new line token '\n'.

4. scroll_up/down:
                scroll up/down charmap for one dline
                keep cursor position unchanged(relative to txtbuff)
                Functions: FTcharmap_scroll_oneline_up(),  FTcharmap_scroll_oneline_down()

5. shift_up/down/left/right:
                shift typing cursor up/down/left/right.
                charmap follow the cursor to scroll if it gets out of current charmap.
                Functions: FTcharmap_shift_cursor_up(),  FTcharmap_shift_cursor_down()
                            FTcharmap_shift_cursor_left(),  FTcharmap_shift_cursor_right()

6. page_up/down:
                scroll whole charmap up/down totally out of current displayed charmap,
                keep cursor position unchanged(relative to txtbuff)
                Functions: FTcharmap_page_up(),  FTcharmap_page_down()


                        --- PRE_Charmap Actions ---

PRE_1:  Set chmap->txtdlncount
PRE_2:  Set chmap->pref
PRE_3:  Set chmap->pchoff/pchoff2   ( chmap->pch/pch2: to be derived from pchoff/pchoff2 in charmapping! )
PRE_4:  Set chmap->fix_cursor (option)
PRE_5:  Set chmap->request

                        ---  Charmap ---

charmap_1:      Update chmap->chcount
Charmap_2:      Update chmap->charX[], charY[],charPos[]

charmap_3:      Update chmap->maplncount
charmap_4:      Update chmap->maplinePos[]

charmap_5:      Update chmap->txtdlcount   ( NOTE: chmap->txtdlinePos[txtdlncount]==chmap->maplinePos[0] )
charmap_6:      Update chmap->txtdlinePos[]

                        --- POST_Charmap Actions ---

POST_1: Check chmap->errbits
POST_2: Redraw cursor


Midas Zhou
midaszhou@yahoo.com
-------------------------------------------------------------------*/
#ifndef __EGI_FTCHARMAP_H__
#define __EGI_FTCHARMPA_H__

#include "egi_color.h"
#include "egi_FTsymbol.h"
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>
#include <arpa/inet.h>
#include FT_FREETYPE_H

typedef enum FTcharmap_errcode FTCHARMAP_ERRCODE;
enum FTcharmap_errcode {
	CHMAPERR_NO_ERR=0,

	/* Hit limit with chmap.txtsize > chmap.txtlen+1 */
	CHMAPERR_TXTSIZE_LIMIT =1,

	/* Hit limit with chmap.chcount > chmap.mapsize, some displayed chars have NO charX/charY/charPos data!
	 * It'll fail to locate those chars later! AND cause unexpected behaviors/results.
	 */
	CHMAPERR_MAPSIZE_LIMIT =1<<1,

	/* Hit limit with chmap.txtdlncount > chmap.txtdlines, some displayed lines have NO txtdlinesPos data!
	 * It'll fail to locate those lines later! AND cause unexpected behaviors/results.
	 */
	CHMAPERR_TXTDLINES_LIMIT =1<<2,
};


typedef struct FTsymbol_char_map	EGI_FTCHAR_MAP;		/* Char map for visiable/displayed characters. ---NOW!
								 * A map to relate displayed chars with their positions on LCD.
								 * Call FTsymbol_uft8strings_writeFB() to fill in the struct.
								 */
struct  FTsymbol_char_map {

/* 1. Global vars for txtbuff.  ( offset position relative to txtbuff ) */
	int		request;		/* ==0: charmap is clear, charmap data are consistent with each other.
						 * !=0: Some params are reset, and is requested to do charmap immediately
					 	 *	to make pch/pchoff, txtdlncount/pref and other data consistent!
						 *
						 * 			--- IMPORATNT ---
						 * After an operation which needs an immediate re_charmap afterwards, the 'request' must be set.
						 * It will then acts as an rejection semaphore to other thread functions which are also trying to
						 * modify charmap parameters before the urgent re_charmap is completed.
						 * TODO:  not applied for all functions yet!
						 */
	unsigned int	errbits;		/* to record types of errs that have gone through.  */
	pthread_mutex_t mutex;      		/* mutex lock for char map */



	unsigned char	*txtbuff;		/* txt buffer */
	int		txtsize;		/* Size of txtbuff mem space allocated, in bytes */
	int		txtlen;			/* strlen of txtbuff, exclude '\0', need to be updated if content of txtbuff changed.  */

	/* disline/dline: displayed/charmapped line, A line starts/ends at displaying window left/right end side.
	 * retline: A line starts/ends by a new line token '\n'.
	 */
	int		txtdlines;		/* LIMIT: Size of txtdlinePos[]  */
	unsigned int	*txtdlinePos;		/* An array to store offset position(relative to txtbuff) of each txt dlines, which
						 * have already been charmapped, and MAYBE not displayed in the current charmap.
					         */
	int		txtdlncount;		/* displayed/charmapped line count, for txtbuff
						 * 			--- NOTE ---
						 * 1. ALWAYS set txtdlncount to the start/right position before calling FTcharmap_uft8strings_writeFB(),
						 *     			--- IMPORTANT ---
						 *    so that txtdlinePos[] will be updated properly! Its index will start from txtdlncount
						 * 2. After each FTcharmap_uft8strings_writeFB() calling, reset it to let:
					         *    chmap->txtdlinePos[txtdlncount]==chmap->maplinePos[0]
						 *    So it can sustain a stable loop calling.
						 *    !!! However, txtdlinePos[txtdlncount+1],txtdlinePos[txtdlncount+2]... +maplncount-1] still
						 *    hold valid data !!!
						 * 3. Only a line_shift OR page_shift action can change/reset the value of txtdlncount.
						 */

	EGI_16BIT_COLOR	markcolor;		 /* Selection mark color */
	EGI_8BIT_ALPHA	markalpha;		 /* Selection mark alpha */

/* 2. Temporary vars for being displayed/charmapped txt.  ( offset position relative to pref )
	 * 1. A map for displayed chars only! A map to tell LCD coordinates of displayed chars and their offset/position in pref[].
	 *    Each char(and typing cursor) displayed will then have its charX/Y and charPos data in current charmap struct, by which
	 *    each pixel on the screen may be mapped to its corresponding char in mem.
	 * 2. Change .pref to display different blocks in txtbuff. ( pref is starting char/line for displaying).
	 * 3. Change .pchoff AND .pch to locate/keep typing(deleting,inserting) cursor position.
	 *    .pchoff --- in bytes, position in txtbuff[]
	 *    .pch    --- in chars, position inc charX[] charY[] charPos[]
	 * 4. Maplines limits the max. number of lines displayed/charmapped each time. It's MAY NOT be the exacte number of actual available
	 *    diplaying lines on LCD, it may be just for charmapping.
	 */
	unsigned char	*pref;			/* A pointer to strings to be displayed, a ref pointer to somewhere of txtbuff[].
					         * content pointed by pref will be charmapped and displayed.
						 * Initial pref=txtbuff.
						 */
	int		mapsize;		/* Size of map member arrays(charX,charY,charPos) mem space allocated , Max. number for chcount+1 */
	int		chcount;	 	/* Total number of displayed/charmapped chars.
					         * (EOF also include if charmapped, '\n' also counted in. NOT as index.
					   	 * Note: chcount-1 chars, the last data pref[charPos[chcount-1]] is EOF, and is always
						 * an inserting point. ( If EOF is charmapped.  )
					   	 */

	int		mapx0;			/* charmap area left top point  */
	int		mapy0;

	unsigned int	mappixpl;		/* pixels per disline */
	int		maplndis;		/* line distance betwee two dlines */

	unsigned int	maplines;		/*  LIMIT: Max. number of displayed lines for current displaying window,
					         *  and also size of maplinePos[] mem space allocatd.
						 */
	int		maplncount;		/* Total number of displayed char lines in current charmap,  NOT index. */
	unsigned int	*maplinePos;		/* Offset position(relative to pref) of the first char of each displayed lines, in bytes
						 * redundant with: txtdlinePos[txtdlncount],...[txtdlncount+1],...[txtdlncount+2]... +maplncount-1].
						 * ....relative to txtbuff thought.
						 */

	unsigned int	pchoff;			/* Offset postion to txtbuff !!!, OnlyIf pchoff>0, it will be used to relocate pch after charmapping!
						 *			--- MOST IMPORTANT ---
						 * pchoff/pch points to the CURRENT/IMMEDIATE typing/inserting position.
						 * In case that after inserting a new char, chmapsize reaches LIMIT and need to shift one line down,
                           			 * then we'll use pchoff to locate the typing position in charmapping. so after inserting a new char
						 * into txtbuff, ALWAY update pchoff to keep track of typing/inserting cursor position.
						 *			--- pchoff V.S. pchoff2 ---
					 	 * NOTE: pchoff is the master of pchoff2, if pchoff changes, pchoff2 changes with it,
						 * 	 to keep as pchoff2==pchoff.
						 * Only when selecting action is triggered, then will pchoff2 diffs from pchoff.
						 */

	unsigned int    pchoff2;		/* OnlyIf pchoff2>0, it will be used to relocate pch2. see pch2. */

	bool		fix_cursor;		/* If true: chmap->pch/pch2 and pchoff/pchoff2 will all be updated by charPos_nolock() in charmapping.
						 * After charmapping, set pch as to re_locate cursor nearest to its previous (x,y) postion.
						 * set as true when shift cursor up/down to cross top/bottom dline.
						 */
	int		pch;			/* Index of displayed char as of charX[],charY and charPos[], pch=0 is the first displayed char.
						 * chmap->pch/pch2 is derived from chmap->pchoff/pchoff2 in charmapping!
						 *   			--- MOST IMPORTANT ---
						 * pchoff/pch points to the CURRENT/IMMEDIATE typing/inserting position.
						 * If pchoff NOT in current charmap, then pch<0.
					 	 * xxx pch is used to locate inserting positioin and typing_cursor position.
					   	 * xxx In some case, we just change chmap->pch in advance(before charmapping), and chmap->pch may be
						 * xxx greater than chmap->chcount-1 at that point, after charmapping it will be adjusted to point
						 * xxx to the same char(same offset value to chmap->txtbuff)
						 *			--- NOTE pch v.s. pch2 ---
					 	 * NOTE: pch is the master of pch2, if pch changes, pch2 changes with it, to keep as pch2==pch.
						 * Only when selecting action is triggered, then will pch2 diffs from pch.
						 */
	int		pch2;			/* Applied as second pch to mark beginning OR ending of a selection.
						 * chmap->pch/pch2 is derived from chmap->pchoff/pchoff2 in charmapping!
						 * If pchoff2 NOT in current charmap, then pch2<0.
						 */
	int 		*charX;			/* Array, Char start point(left top) FB/LCD coordinates X,Y */
	int 		*charY;
	unsigned int	*charPos;		/* Array, Char offset position relative to pref, in bytes. */
	//unsigned int 	*charW;			/* Array,Widths of char */

	/* Extension: color,size,...*/
};


EGI_FTCHAR_MAP* FTcharmap_create(size_t txtsize, int x0, int y0, size_t mapsize, size_t maplines, size_t mappixpl, int maplngap);

void 	FTcharmap_set_markcolor(EGI_FTCHAR_MAP *chmap, EGI_16BIT_COLOR color, EGI_8BIT_ALPHA alpha);

int 	FTcharmap_load_file(const char *fpath, EGI_FTCHAR_MAP *chmap, size_t txtsize);
int 	FTcharmap_save_file(const char *fpath, EGI_FTCHAR_MAP *chmap);	/* mutex_lock */

void 	FTcharmap_free(EGI_FTCHAR_MAP **chmap);
int 	FTcharmap_set_pref_nextDispLine(EGI_FTCHAR_MAP *chmap);
int  	FTcharmap_uft8strings_writeFB( FBDEV *fb_dev, EGI_FTCHAR_MAP *chmap,			/* mutex_lock, request_clear */
                                    FT_Face face, int fw, int fh,
                                    int fontcolor, int transpcolor, int opaque,
                                    int *cnt, int *lnleft, int* penx, int* peny );
//static void FTcharmap_mark_selection(FBDEV *fb_dev, EGI_FTCHAR_MAP *chmap); /*without mutex_lock */

int 	FTcharmap_page_up(EGI_FTCHAR_MAP *chmap);			/* mutex_lock + request */
int 	FTcharmap_page_down(EGI_FTCHAR_MAP *chmap);			/* mutex_lock + request */
int 	FTcharmap_scroll_oneline_up(EGI_FTCHAR_MAP *chmap);		/* mutex_lock + request */
int 	FTcharmap_scroll_oneline_down(EGI_FTCHAR_MAP *chmap);		/* mutex_lock + request */

/* To locate chmap->pch */
int  	FTcharmap_locate_charPos( EGI_FTCHAR_MAP *chmap, int x, int y);		/* mutex_lock + request_check */

//static int FTcharmap_locate_charPos_nolock( EGI_FTCHAR_MAP *chmap, int x, int y);  /* without mutex_lock, no request_check */

/* To locate chmap->pch2 */
int  	FTcharmap_locate_charPos2( EGI_FTCHAR_MAP *chmap, int x, int y);		/* mutex_lock */
int 	FTcharmap_reset_charPos2( EGI_FTCHAR_MAP *chmap );  /* reset pch2=pch */        /* mutex_lock */

int 	FTcharmap_shift_cursor_up(EGI_FTCHAR_MAP *chmap);		/* mutex_lock + request */
int 	FTcharmap_shift_cursor_down(EGI_FTCHAR_MAP *chmap);		/* mutex_lock + request */
int 	FTcharmap_shift_cursor_right(EGI_FTCHAR_MAP *chmap);		/* mutex_lock + request */
int 	FTcharmap_shift_cursor_left(EGI_FTCHAR_MAP *chmap);		/* mutex_lock + request */

int 	FTcharmap_goto_lineBegin( EGI_FTCHAR_MAP *chmap );  	/* mutex_lock + request */ 	/* As retline, NOT displine */
int 	FTcharmap_goto_lineEnd( EGI_FTCHAR_MAP *chmap );	/* mutex_lock + request */ 	/* As retline, NOT displine */

int 	FTcharmap_getPos_lastCharOfDline(EGI_FTCHAR_MAP *chmap,  int dln); /* ret pos is relative to txtdlinePos[] */
int 	FTcharmap_get_txtdlIndex(EGI_FTCHAR_MAP *chmap,  int pchoff);

int 	FTcharmap_go_backspace( EGI_FTCHAR_MAP *chmap );		/* mutex_lock + request */
int 	FTcharmap_insert_char( EGI_FTCHAR_MAP *chmap, const char *ch );	/* mutex_lock + request */

/* Delete a char preceded by cursor OR chars selected between pchoff2 and pchoff */
int 	FTcharmap_delete_char( EGI_FTCHAR_MAP *chmap );			/* mutex_lock + request */

#endif
