"""Exercise the production nonblocking Wayland frame gate without a compositor."""
import argparse, subprocess, tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--engine',type=Path,required=True);p.add_argument('--compiler',default='cc');a=p.parse_args()
s=(a.engine/'gl/gl_vidwayland.c').read_text();code=s[s.index('qboolean WL_MayRefresh(void)'):s.index('static void frame_handle_done')]
shim=r"""
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#define true 1
#define false 0
#define CON_ERROR ""
#define RESTRICT_LOCAL 0
typedef int qboolean;
struct pollfd {int fd;short events,revents;};
#define POLLIN 1
#define POLLERR 8
#define POLLHUP 16
#define POLLNVAL 32
static struct {void *display;int wait_for_configure,waylandisblocking;} w;
static struct {float value;} vid_vsync;
static int prepareAgain,pendingError,flushError,eventMask,readError,reads,cancels,dispatches,restarts;
static int pwl_display_prepare_read(void *d){(void)d;if(prepareAgain){prepareAgain=0;return -1;}return 0;}
static int pwl_display_dispatch_pending(void *d){(void)d;dispatches++;return pendingError?-1:0;}
static int pwl_display_flush(void *d){(void)d;errno=flushError;return flushError?-1:0;}
static void pwl_display_cancel_read(void *d){(void)d;cancels++;}
static int pwl_display_get_fd(void *d){(void)d;return 7;}
static int pwl_display_read_events(void *d){(void)d;reads++;if(readError)return -1;w.wait_for_configure=w.waylandisblocking=0;return 0;}
static int poll(struct pollfd *fd,int count,int timeout){assert(count==1&&timeout<=2&&fd->fd==7);fd->revents=eventMask;return eventMask?1:0;}
static void Con_Printf(const char *text){(void)text;}
static void Cbuf_InsertText(const char *text,int level,int immediate){(void)text;(void)level;(void)immediate;restarts++;}
"""
checks=r"""
int main(void){
 vid_vsync.value=1;assert(WL_MayRefresh());
 w.waylandisblocking=1;assert(!WL_MayRefresh()&&cancels==1&&reads==0);
 vid_vsync.value=0;assert(WL_MayRefresh()&&cancels==2&&reads==0);
 vid_vsync.value=1;eventMask=POLLIN;assert(WL_MayRefresh()&&reads==1&&!w.waylandisblocking);
 w.wait_for_configure=1;eventMask=0;vid_vsync.value=0;assert(!WL_MayRefresh());
 eventMask=POLLIN;assert(WL_MayRefresh()&&!w.wait_for_configure);
 w.waylandisblocking=1;prepareAgain=1;assert(WL_MayRefresh());
 w.waylandisblocking=1;eventMask=POLLERR;assert(!WL_MayRefresh()&&restarts==1);
 eventMask=0;flushError=EIO;assert(!WL_MayRefresh()&&restarts==2);
 flushError=0;prepareAgain=1;pendingError=1;assert(!WL_MayRefresh()&&restarts==3);
 puts("PASS: bounded event reads, hidden-window progress, VSync, configure, pending events and disconnect recovery");
}
"""
with tempfile.TemporaryDirectory() as folder:
 f=Path(folder)/'test.c';exe=Path(folder)/'test.exe';f.write_text(shim+code+checks)
 subprocess.run([a.compiler,'-std=c99',str(f),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
