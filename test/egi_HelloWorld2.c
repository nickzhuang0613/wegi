/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

	    A demon for a simple rectangular button.

A more beautiful button is usually an (PNG/JPG) image icon.

Midas Zhou
midaszhou@yahoo.com
------------------------------------------------------------------*/
#include <egi_common.h>
#include <egi_utils.h>
#include <egi_FTsymbol.h>


void writeFB_button(EGI_RECTBTN *btn0, char *tag, EGI_16BIT_COLOR tagcolor);


int main(int argc, char **argv)
{
 /* <<<<<<  EGI general init  >>>>>> */

  /* Start sys tick */
  printf("tm_start_egitick()...\n");
  tm_start_egitick();
  #if 0
  /* Start egi log */
  printf("egi_init_log()...\n");
  if(egi_init_log("/mmc/log_scrollinput")!=0) {
        printf("Fail to init egi logger, quit.\n");
        return -1;
  }
  /* Load symbol pages */
  printf("FTsymbol_load_allpages()...\n");
  if(FTsymbol_load_allpages() !=0) /* FT derived sympg_ascii */
        printf("Fail to load FTsym pages! go on anyway...\n");
  printf("symbol_load_allpages()...\n");
  if(symbol_load_allpages() !=0) {
        printf("Fail to load sym pages, quit.\n");
        return -1;
  }
  #endif
  /* Load freetype fonts */
  printf("FTsymbol_load_sysfonts()...\n");
  if(FTsymbol_load_sysfonts() !=0) {
        printf("Fail to load FT sysfonts, quit.\n");
        return -1;
  }
  #if 0
  printf("FTsymbol_load_appfonts()...\n");
  if(FTsymbol_load_appfonts() !=0) {
        printf("Fail to load FT appfonts, quit.\n");
        return -1;
  }
  #endif
  /* Initilize sys FBDEV */
  printf("init_fbdev()...\n");
  if(init_fbdev(&gv_fb_dev))
        return -1;

  /* Start touch read thread */
  printf("Start touchread thread...\n");
  if(egi_start_touchread() !=0)
        return -1;

  /* Set sys FB mode */
  fb_set_directFB(&gv_fb_dev,false);
  fb_position_rotate(&gv_fb_dev,3);

 /* <<<<<  End of EGI general init  >>>>>> */



	    		  /*-----------------------------------
    			   *            Main Program
    			   -----------------------------------*/

  EGI_TOUCH_DATA  touch_data;
  EGI_16BIT_COLOR bkcolor=WEGI_COLOR_OCEAN; //WEGI_COLOR_GRAY1

  /* A rectangular button */
  EGI_RECTBTN 	  btn0={ .color=WEGI_COLOR_OCEAN, .x0=110, .y0=60, .width=100, .height=50, .sw=4, .pressed=false };
  btn0.color=bkcolor;

  /* Clear screen */
  fb_clear_backBuff(&gv_fb_dev, bkcolor);
  writeFB_button(&btn0, "OK", WEGI_COLOR_BLACK);
  fb_page_refresh(&gv_fb_dev, 0);

while(1) {
  	if(egi_touch_timeWait_press(0, 500, &touch_data)!=0)
		continue;
        /* touch_data converted to the same coord as of FB */
        egi_touch_fbpos_data(&gv_fb_dev, &touch_data);

	/* Check if touched the button */
	if( !pxy_inbox( touch_data.coord.x, touch_data.coord.y, btn0.x0, btn0.y0, btn0.x0+btn0.width, btn0.y0+btn0.height) )
		continue;

	/* Toggle button */
        btn0.pressed=!btn0.pressed;

	/* Draw button */
  	fb_clear_backBuff(&gv_fb_dev, bkcolor);
  	writeFB_button(&btn0, "OK", WEGI_COLOR_BLACK);

	/* Write Hello World! */
  	if(btn0.pressed)
     		FTsymbol_uft8strings_writeFB(   &gv_fb_dev, egi_sysfonts.bold,          /* FBdev, fontface */
                		                30, 30,(const unsigned char *)"EGI: Hello World!",    /* fw,fh, pstr */
                                	 	320, 1, 0,                                  /* pixpl, lines, gap */
                                 		20, 150,                                    /* x0,y0, */
                                 		WEGI_COLOR_RED, -1, -1,       /* fontcolor, transcolor,opaque */
                                 		NULL, NULL, NULL, NULL);      /* int *cnt, int *lnleft, int* penx, int* peny */
  	fb_page_refresh(&gv_fb_dev, 0);

}



 /* --- my releae --- */

 /* <<<<<  EGI general release 	 >>>>>> */
 printf("FTsymbol_release_allfonts()...\n");
 FTsymbol_release_allfonts(); /* release sysfonts and appfonts */
 printf("symbol_release_allpages()...\n");
 symbol_release_allpages(); /* release all symbol pages*/
 printf("FTsymbol_release_allpage()...\n");
 FTsymbol_release_allpages(); /* release FT derived symbol pages: sympg_ascii */

 printf("fb_filo_flush() and release_fbdev()...\n");
 fb_filo_flush(&gv_fb_dev);
 release_fbdev(&gv_fb_dev);
 #if 0
 printf("release virtual fbdev...\n");
 release_virt_fbdev(&vfb);
 #endif
 printf("egi_end_touchread()...\n");
 egi_end_touchread();
 #if 0
 printf("egi_quit_log()...\n");
 egi_quit_log();
 #endif
 printf("<-------  END  ------>\n");


return 0;
}



/*-----------------------------------
    Draw button and put tag
-----------------------------------*/
void writeFB_button(EGI_RECTBTN *btn, char *tag, EGI_16BIT_COLOR tagcolor)
{
	int fw=25;
	int fh=25;
	int pixlen;

	if(btn==NULL)return;

	/* Draw frame ( fbdev, type, x0, y0, width, height, w ) */
  	draw_button_frame( &gv_fb_dev, btn->pressed, btn->color, btn->x0, btn->y0, btn->width, btn->height, btn->sw);

	int bith=FTsymbol_get_symheight(egi_sysfonts.bold, fw, fh);
	printf("bith=%d\n",bith);

	pixlen=FTsymbol_uft8strings_pixlen( egi_sysfonts.bold, fw, fh,(const unsigned char *)tag);

 	/* Write tag */
  	FTsymbol_uft8strings_writeFB(  &gv_fb_dev, egi_sysfonts.bold,          	/* FBdev, fontface */
        	                       fw, fh,(const unsigned char *)tag,      	/* fw,fh, pstr */
                	               320, 1, 0,                              	/* pixpl, lines, gap */
				       btn->x0+(btn->width-pixlen)/2,		/* x0 */
				btn->y0+(btn->height-fh)/2-(fh-bith)/2 +(btn->pressed?2:0),		/* y0 */
                                       tagcolor, -1, -1,       			/* fontcolor, transcolor,opaque */
                                       NULL, NULL, NULL, NULL);      		/* int *cnt, int *lnleft, int* penx, int* peny */
}
