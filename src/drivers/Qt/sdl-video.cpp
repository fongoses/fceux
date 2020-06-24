/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/// \file
/// \brief Handles the graphical game display for the SDL implementation.

#include "sdl.h"
#include "gl_win.h"
#include "../common/vidblit.h"
#include "../../fceu.h"
#include "../../version.h"
#include "../../video.h"

#include "../../utils/memory.h"

//#include "sdl-icon.h"
#include "dface.h"

#include "../common/configSys.h"
#include "sdl-video.h"
#include "fceuWrapper.h"

#ifdef CREATE_AVI
#include "../videolog/nesvideos-piece.h"
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>

// GLOBALS
extern Config *g_config;

// STATIC GLOBALS
static int s_curbpp = 0;
static int s_srendline, s_erendline;
static int s_tlines;
static int s_inited = 0;

//#ifdef OPENGL
//static int s_useOpenGL = 0;
//#endif
static double s_exs = 1.0, s_eys = 1.0;
static int s_eefx = 0;
static int s_clipSides = 0;
static int s_fullscreen = 0;
static int noframe = 0;
static int initBlitToHighDone = 0;

#define NWIDTH	(256 - (s_clipSides ? 16 : 0))
#define NOFFSET	(s_clipSides ? 8 : 0)

static int s_paletterefresh = 1;

extern bool MaxSpeed;

/**
 * Attempts to destroy the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */

//draw input aids if we are fullscreen
bool FCEUD_ShouldDrawInputAids()
{
	return s_fullscreen!=0;
}
 
int
KillVideo()
{
	//printf("Killing Video\n");

	if ( gl_shm != NULL )
	{
		gl_shm->clear_pixbuf();
	}

	//destroy_gui_video();

	// return failure if the video system was not initialized
	if (s_inited == 0)
		return -1;

		// if the rest of the system has been initialized, shut it down
//		// shut down the system that converts from 8 to 16/32 bpp
//		if (s_curbpp > 8)
//		{
//			KillBlitToHigh();
//		}

	// SDL Video system is not used.
	// shut down the SDL video sub-system
	//SDL_QuitSubSystem(SDL_INIT_VIDEO);

	s_inited = 0;
	return 0;
}


// this variable contains information about the special scaling filters
static int s_sponge = 0;

/**
 * These functions determine an appropriate scale factor for fullscreen/
 */
inline double GetXScale(int xres)
{
	return ((double)xres) / NWIDTH;
}
inline double GetYScale(int yres)
{
	return ((double)yres) / s_tlines;
}
void FCEUD_VideoChanged()
{
	int buf;
	g_config->getOption("SDL.PAL", &buf);
	if(buf == 1)
		PAL = 1;
	else
		PAL = 0; // NTSC and Dendy
}

int InitVideo(FCEUGI *gi)
{
	int doublebuf, xstretch, ystretch, xres, yres, show_fps;

	FCEUI_printf("Initializing video...");

	// load the relevant configuration variables
	g_config->getOption("SDL.Fullscreen", &s_fullscreen);
	g_config->getOption("SDL.DoubleBuffering", &doublebuf);
//#ifdef OPENGL
//	g_config->getOption("SDL.OpenGL", &s_useOpenGL);
//#endif
	//g_config->getOption("SDL.SpecialFilter", &s_sponge);
	g_config->getOption("SDL.XStretch", &xstretch);
	g_config->getOption("SDL.YStretch", &ystretch);
	//g_config->getOption("SDL.LastXRes", &xres);
	//g_config->getOption("SDL.LastYRes", &yres);
	g_config->getOption("SDL.ClipSides", &s_clipSides);
	g_config->getOption("SDL.NoFrame", &noframe);
	g_config->getOption("SDL.ShowFPS", &show_fps);
	//g_config->getOption("SDL.XScale", &s_exs);
	//g_config->getOption("SDL.YScale", &s_eys);
	uint32_t  rmask, gmask, bmask;

	s_sponge = 0;
	s_exs = 1.0;
	s_eys = 1.0;
	xres = gui_draw_area_width;
	yres = gui_draw_area_height;
	// check the starting, ending, and total scan lines

	FCEUI_GetCurrentVidSystem(&s_srendline, &s_erendline);
	s_tlines = s_erendline - s_srendline + 1;

	//init_gui_video( s_useOpenGL );

	s_inited = 1;

	// check to see if we are showing FPS
	FCEUI_SetShowFPS(show_fps);

#ifdef LSB_FIRST
	rmask = 0x000000FF;
	gmask = 0x0000FF00;
	bmask = 0x00FF0000;
#else
	rmask = 0x00FF0000;
	gmask = 0x0000FF00;
	bmask = 0x000000FF;
#endif

	s_curbpp = 32; // Bits per pixel is always 32

	FCEU_printf(" Video Mode: %d x %d x %d bpp %s\n",
				xres, yres, s_curbpp,
				s_fullscreen ? "full screen" : "");

	if (s_curbpp != 8 && s_curbpp != 16 && s_curbpp != 24 && s_curbpp != 32) 
	{
		FCEU_printf("  Sorry, %dbpp modes are not supported by FCE Ultra.  Supported bit depths are 8bpp, 16bpp, and 32bpp.\n", s_curbpp);
		KillVideo();
		return -1;
	}

#ifdef OPENGL
	if(s_exs <= 0.01) {
		FCEUD_PrintError("xscale out of bounds.");
		KillVideo();
		return -1;
	}
	if(s_eys <= 0.01) {
		FCEUD_PrintError("yscale out of bounds.");
		KillVideo();
		return -1;
	}
	//if(s_sponge && s_useOpenGL) {
	//	FCEUD_PrintError("scalers not compatible with openGL mode.");
	//	KillVideo();
	//	return -1;
	//}
#endif

	if ( !initBlitToHighDone )
	{
		InitBlitToHigh(s_curbpp >> 3,
							rmask,
							gmask,
							bmask,
							s_eefx, s_sponge, 0);

		initBlitToHighDone = 1;
	}

	return 0;
}

/**
 * Toggles the full-screen display.
 */
void ToggleFS(void)
{
    // pause while we we are making the switch
	bool paused = FCEUI_EmulationPaused();
	if(!paused)
		FCEUI_ToggleEmulationPause();

	int error, fullscreen = s_fullscreen;

	// shut down the current video system
	KillVideo();

	// flip the fullscreen flag
	g_config->setOption("SDL.Fullscreen", !fullscreen);
#ifdef _GTK
	if(noGui == 0)
	{
		if(!fullscreen)
			showGui(0);
		else
			showGui(1);
	}
#endif
	// try to initialize the video
	error = InitVideo(GameInfo);
	if(error) {
		// if we fail, just continue with what worked before
		g_config->setOption("SDL.Fullscreen", fullscreen);
		InitVideo(GameInfo);
	}
	// if we paused to make the switch; unpause
	if(!paused)
		FCEUI_ToggleEmulationPause();
}

static SDL_Color s_psdl[256];

/**
 * Sets the color for a particular index in the palette.
 */
void
FCEUD_SetPalette(uint8 index,
                 uint8 r,
                 uint8 g,
                 uint8 b)
{
	s_psdl[index].r = r;
	s_psdl[index].g = g;
	s_psdl[index].b = b;

	s_paletterefresh = 1;
}

/**
 * Gets the color for a particular index in the palette.
 */
void
FCEUD_GetPalette(uint8 index,
				uint8 *r,
				uint8 *g,
				uint8 *b)
{
	*r = s_psdl[index].r;
	*g = s_psdl[index].g;
	*b = s_psdl[index].b;
}

/** 
 * Pushes the palette structure into the underlying video subsystem.
 */
static void RedoPalette()
{
	if (s_curbpp > 8) 
	{
		SetPaletteBlitToHigh((uint8*)s_psdl);
	} 
}
// XXX soules - console lock/unlock unimplemented?

///Currently unimplemented.
void LockConsole(){}

///Currently unimplemented.
void UnlockConsole(){}

/**
 * Pushes the given buffer of bits to the screen.
 */
void
BlitScreen(uint8 *XBuf)
{
	uint8 *dest;
	int w, h, pitch;

	// refresh the palette if required
	if (s_paletterefresh) 
	{
		RedoPalette();
		s_paletterefresh = 0;
	}

	// XXX soules - not entirely sure why this is being done yet
	XBuf += s_srendline * 256;

	dest  = (uint8*)gl_shm->pixbuf;
	w     = GL_NES_WIDTH;
	h     = GL_NES_HEIGHT;
	pitch = w*4;

	gl_shm->ncol    = NWIDTH;
	gl_shm->nrow    = s_tlines;
	gl_shm->pitch   = pitch;

	if ( dest == NULL ) return;

	Blit8ToHigh(XBuf + NOFFSET, dest, NWIDTH, s_tlines, pitch, 1, 1);

	//guiPixelBufferReDraw();

#ifdef CREATE_AVI
 { int fps = FCEUI_GetDesiredFPS();
   static unsigned char* result = NULL;
   static unsigned resultsize = 0;
   int width = NWIDTH, height = s_tlines;
   if(!result || resultsize != width*height*3*2)
   {
       if(result) free(result);
       result = (unsigned char*) FCEU_dmalloc(resultsize = width*height*3*2);
   }
   switch(s_curbpp)
   {
   #if 0
     case 24: case 32: case 15: case 16:
       /* Convert to I420 if possible, because our I420 conversion is optimized
        * and it'll produce less network traffic, hence faster throughput than
        * anything else. And H.264 eats only I420, so it'd be converted sooner
        * or later anyway if we didn't do it. Win-win situation.
        */
       switch(s_curbpp)
       {
         case 32: Convert32To_I420Frame(s_screen->pixels, &result[0], width*height, width); break;
         case 24: Convert24To_I420Frame(s_screen->pixels, &result[0], width*height, width); break;
         case 15: Convert15To_I420Frame(s_screen->pixels, &result[0], width*height, width); break;
         case 16: Convert16To_I420Frame(s_screen->pixels, &result[0], width*height, width); break;
       }
       NESVideoLoggingVideo(&result[0], width,height, fps, 12);
       break;
   #endif
     default:
       NESVideoLoggingVideo( dest, width,height, fps, s_curbpp);
   }
 }
#endif // CREATE_AVI

#if REALTIME_LOGGING
 {
   static struct timeval last_time;
   static int first_time=1;
   extern long soundrate;
   
   struct timeval cur_time;
   gettimeofday(&cur_time, NULL);
   
   double timediff =
       (cur_time.tv_sec *1e6 + cur_time.tv_usec
     - (last_time.tv_sec *1e6 + last_time.tv_usec)) / 1e6;
   
   int nframes = timediff * 60 - 1;
   if(first_time)
     first_time = 0;
   else while(nframes > 0)
   {
     static const unsigned char Buf[800*4] = {0};
     NESVideoLoggingVideo(screen->pixels, 256,tlines, FCEUI_GetDesiredFPS(), s_curbpp);
     NESVideoLoggingAudio(Buf, soundrate,16,1, soundrate/60.0);
     --nframes;
   }
   memcpy(&last_time, &cur_time, sizeof(last_time));
 }
#endif // REALTIME_LOGGING

}

/**
 *  Converts an x-y coordinate in the window manager into an x-y
 *  coordinate on FCEU's screen.
 */
uint32
PtoV(uint16 x,
	uint16 y)
{
	y = (uint16)((double)y / s_eys);
	x = (uint16)((double)x / s_exs);
	if(s_clipSides) {
		x += 8;
	}
	y += s_srendline;
	return (x | (y << 16));
}

bool enableHUDrecording = false;
bool FCEUI_AviEnableHUDrecording()
{
	if (enableHUDrecording)
		return true;

	return false;
}
void FCEUI_SetAviEnableHUDrecording(bool enable)
{
	enableHUDrecording = enable;
}

bool disableMovieMessages = false;
bool FCEUI_AviDisableMovieMessages()
{
	if (disableMovieMessages)
		return true;

	return false;
}
void FCEUI_SetAviDisableMovieMessages(bool disable)
{
	disableMovieMessages = disable;
}
