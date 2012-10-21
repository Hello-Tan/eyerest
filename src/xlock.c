/*
 * =====================================================================================
 *
 *       Filename:  xlock.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年10月21日 22时45分03秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zhanglei (zhanglei), zhangleibruce@gmail.com
 *        Company:  zlb.me
 *
 * =====================================================================================
 */
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "xlock.h"
#include "eye_log.h"

typedef struct {
	int screen;
	Window root, win;
	Pixmap pmap;
} Lock;

static Lock **s_locks = NULL;
static Display* s_dpy = NULL;

static void xlock_unlockeachscreen(Display *dpy, Lock *lock)
{
	if(dpy == NULL || lock == NULL)
		return;

	XUngrabPointer(dpy, CurrentTime);
	XFreePixmap(dpy, lock->pmap);
	XDestroyWindow(dpy, lock->win);

	free(lock);
    lock = NULL;
}

static Lock* xlock_lockeachscreen(Display* dpy, int screen)
{
	char curs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int len;
	Lock *lock;
	XColor black, dummy;
	XSetWindowAttributes wa;
	Cursor invisible;

	if(dpy == NULL || screen < 0)
		return NULL;

	lock = malloc(sizeof(Lock));
	if(lock == NULL)
		return NULL;

	lock->screen = screen;

	lock->root = RootWindow(dpy, lock->screen);

	/* init */
	wa.override_redirect = 1;
	wa.background_pixel = BlackPixel(dpy, lock->screen);
	lock->win = XCreateWindow(dpy, lock->root, 0, 0, DisplayWidth(dpy, lock->screen), DisplayHeight(dpy, lock->screen),
			0, DefaultDepth(dpy, lock->screen), CopyFromParent,
			DefaultVisual(dpy, lock->screen), CWOverrideRedirect | CWBackPixel, &wa);
	XAllocNamedColor(dpy, DefaultColormap(dpy, lock->screen), "black", &black, &dummy);
	lock->pmap = XCreateBitmapFromData(dpy, lock->win, curs, 8, 8);
	invisible = XCreatePixmapCursor(dpy, lock->pmap, lock->pmap, &black, &black, 0, 0);
	XDefineCursor(dpy, lock->win, invisible);
	XMapRaised(dpy, lock->win);
	for(len = 1000; len; len--) {
		if(XGrabPointer(dpy, lock->root, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
			GrabModeAsync, GrabModeAsync, None, invisible, CurrentTime) == GrabSuccess)
			break;
		usleep(1000);
	}
    int running = 1;
	if(running && (len > 0)) {
		for(len = 1000; len; len--) {
			if(XGrabKeyboard(dpy, lock->root, True, GrabModeAsync, GrabModeAsync, CurrentTime)
				== GrabSuccess)
				break;
			usleep(1000);
		}
		running = (len > 0);
	}

	if(!running) {
		xlock_unlockeachscreen(dpy, lock);
		lock = NULL;
	}
	else 
		XSelectInput(dpy, lock->root, SubstructureNotifyMask);

	return lock;
}

void xlock_lockscreen()
{
    if(!(s_dpy = XOpenDisplay(getenv("DISPLAY"))))
    {
        eye_error("cannot open display");
        return;
    }

    int nscreens = ScreenCount(s_dpy);
    
    eye_debug("nscreens = %d\n", nscreens);

	s_locks = malloc(sizeof(Lock *) * nscreens);
	if(s_locks == NULL)
    {
		eye_error("malloc Lock error: %s", strerror(errno));
        return;
    }

    int i;
    for(i = 0; i < nscreens; i++ )
    {
		s_locks[i] = xlock_lockeachscreen(s_dpy, i);
    }
	XSync(s_dpy, False);
}

void xlock_unlockscreen()
{
    int nscreens = ScreenCount(s_dpy);
    int i;
	for(i = 1; i < nscreens; i++)
		xlock_unlockeachscreen(s_dpy, s_locks[i]);

	free(s_locks);
    s_locks = NULL;
	XCloseDisplay(s_dpy);

    s_dpy = NULL;
}
