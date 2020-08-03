/*------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

A program to draw basic geometries of EGI.

Midas Zhou
midaszhou@yahoo.com
https://github.com/widora/wegi
------------------------------------------------------------------*/
#include <egi_common.h>
#include <egi_utils.h>
#include <egi_matrix.h>


int main(int argc, char **argv)
{
  int i;

  /* <<<<<<  1. EGI general init  EGI初始化流程 >>>>>> */
  /* 对不必要的一些步骤可以忽略 */

  /* 1.1 Start sys tick 	开启系统计数 (忽略) */
  /* 1.2 Start egi log 		开启日志记录 (忽略) */
  /* 1.3 Load symbol pages 	加载图形/符号映像 (忽略) */
  /* 1.4 Load freetype fonts 	加载FreeType字体 (忽略) */

  /* 1.5 Initilize sys FBDEV 	初始化FB显示设备 */
  printf("init_fbdev()...\n");
  if(init_fbdev(&gv_fb_dev))
        return -1;

  /* 1.6 Start touch read thread 启动触摸屏线程 (忽略) */

  /* 1.7 Set sys FB mode 	设置显示模式: 是否直接操作FB映像数据， 设置横竖屏 */
  fb_set_directFB(&gv_fb_dev,true);   /* 直接操作FB映像数据,不通过FBbuffer. 播放动画时可能出现撕裂线。 */
  fb_position_rotate(&gv_fb_dev,0);   /* 横屏模式 */

  /* <<<<<  End of EGI general init EGI初始化流程结束  >>>>>> */


   /**************************************
    *  	      2. Draw Geometry 程序部分
    ***************************************/
   /* 三角形顶点坐标 */
   EGI_POINT	tripoints[3]={ {60,20}, {140,80}, {20,100} };
   /* 折线顶点坐标 */
   EGI_POINT	mpoints[6]={ {10,230}, {60,100}, {100, 190}, {180,50}, {230,120}, {300,20} };

   fbset_speed(10); /* 设置画点速度 */

   /* 在屏幕上绘制几何图形  */
   /* 2.1 清屏 */
   clear_screen( &gv_fb_dev, WEGI_COLOR_GRAY5);

   /* 2.2 绘制线条 */
   fbset_color(WEGI_COLOR_BLACK); /* 设置画笔颜色 */
   for(i=0; i<21; i++)
	draw_wline_nc(&gv_fb_dev, 15+i*20, 10, 15+i*20, 240-10, i/3+1);	/* 绘制垂直线条 */

   /* 2.8 绘制折线 */
   fbset_color(WEGI_COLOR_FIREBRICK);
   draw_pline(&gv_fb_dev, mpoints, 6, 5);


   /* Spline */
//   getchar();

   fbset_speed(0);
   fbset_color(WEGI_COLOR_GREEN);
   draw_spline(&gv_fb_dev, 6, mpoints, 0, 5);
   fbset_color(WEGI_COLOR_BLUE);
   draw_spline(&gv_fb_dev, 6, mpoints, 1, 5);
   fbset_color(WEGI_COLOR_PINK);
   draw_spline(&gv_fb_dev, 6, mpoints, 2, 5);
   usleep(300000);
   getchar();

//   EGI_POINT	pts[9]={ {0,80}, {80,0}, {160, 80}, {240,160}, {320,80}, {40,0}, {160,80},{80,160},{0,80} };
   int np=5;
   int mx,my;
//   EGI_POINT	pts[5]={ {0,80}, {80,160}, {160,80}, {240,160}, {320,80} }; //, {240,0}, {320,80},{240,160},{160,240} };

//   EGI_POINT	pts[5]={ {160,240}, {320-20,80}, {240-1,0}, {160,80}, {80,0} };
   EGI_POINT	pts[5]={ {80,0},{160,80},{240-1,0},{320-20,80},{160,240} };

   for(i=0; i<320; i+=5)
   {
	   pts[1].y = 80+i;
	   clear_screen( &gv_fb_dev, WEGI_COLOR_GRAY5);
	   fbset_color(WEGI_COLOR_ORANGE);
	   draw_pline(&gv_fb_dev, pts, np, 5);
	   fbset_color(WEGI_COLOR_GREEN);
	   draw_spline(&gv_fb_dev, np, pts, 0, 5);
	   getchar();
   }


  /* <<<<<  3. EGI general release EGI释放流程	 >>>>>> */
  /* 根据初始化流程做对应的释放　*/
  /* 3.1 Release sysfonts and appfonts 释放所有FreeTpype字体 (忽略) */
  /* 3.2 Release all symbol pages 释放所有图形/符号映像 (忽略) */
  /* 3.3 Release FBDEV and its data 释放FB显示设备及数据 */
  printf("fb_filo_flush() and release_fbdev()...\n");
  fb_filo_flush(&gv_fb_dev);
  release_fbdev(&gv_fb_dev);
  /* 3.4 Release virtual FBDEV 释放虚拟FB显示设备 (忽略) */
  /* 3.5 End touch read thread  结束触摸屏线程 (忽略) */
  /* 3.6 结束日志记录  (忽略) */

return 0;
}

