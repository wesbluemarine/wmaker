
#include "WINGsP.h"


typedef struct W_Frame {
    W_Class widgetClass;
    W_View *view;

    char *caption;

    
    struct {
	WMReliefType relief:4;
	WMTitlePosition titlePosition:4;
    } flags;
} Frame;


#define DEFAULT_RELIEF 	WRGroove
#define DEFAULT_TITLE_POSITION	WTPAtTop
#define DEFAULT_WIDTH		40
#define DEFAULT_HEIGHT		40


static void destroyFrame(Frame *fPtr);
static void paintFrame(Frame *fPtr);
static void repaintFrame(Frame *fPtr);



void
WMSetFrameTitlePosition(WMFrame *fPtr, WMTitlePosition position)
{
    fPtr->flags.titlePosition = position;

    if (fPtr->view->flags.realized) {
	repaintFrame(fPtr);
    }
}


void
WMSetFrameRelief(WMFrame *fPtr, WMReliefType relief)
{
    fPtr->flags.relief = relief;
    
   if (fPtr->view->flags.realized) {
	repaintFrame(fPtr);
    }
}


void
WMSetFrameTitle(WMFrame *fPtr, char *title)
{
    if (fPtr->caption)
	wfree(fPtr->caption);
    if (title)
	fPtr->caption = wstrdup(title);
    else
	fPtr->caption = NULL;

   if (fPtr->view->flags.realized) {
	repaintFrame(fPtr);
    }
}


static void
repaintFrame(Frame *fPtr)
{
    W_View *view = fPtr->view;
    W_Screen *scrPtr = view->screen;

    XClearWindow(scrPtr->display, view->window);
    paintFrame(fPtr);
}


static void
paintFrame(Frame *fPtr)
{
    W_View *view = fPtr->view;
    W_Screen *scrPtr = view->screen;
    int tx, ty, tw, th;
    int fy, fh;
    Bool drawTitle;
    
    if (fPtr->caption!=NULL)
	th = WMFontHeight(scrPtr->normalFont);
    else {
	th = 0;
    }
    
    fh = view->size.height;
    fy = 0;
    
    switch (fPtr->flags.titlePosition) {
     case WTPAboveTop:
	ty = 0;
	fy = th + 4;
	fh = view->size.height - fy;
	break;
	
     case WTPAtTop:
	ty = 0;
	fy = th/2;
	fh = view->size.height - fy;
	break;
	
     case WTPBelowTop:
	ty = 4;
	fy = 0;
	fh = view->size.height;
	break;
	
     case WTPAboveBottom:
	ty = view->size.height - th - 4;
	fy = 0;
	fh = view->size.height;
	break;
	
     case WTPAtBottom:
	ty = view->size.height - th;
	fy = 0;
	fh = view->size.height - th/2;
	break;
	
     case WTPBelowBottom:
	ty = view->size.height - th;
	fy = 0;
	fh = view->size.height - th - 4;
	break;
	
     default:
	ty = 0;
	fy = 0;
	fh = view->size.height;
    }

    if (fPtr->caption!=NULL && fPtr->flags.titlePosition!=WTPNoTitle) {
	tw = WMWidthOfString(scrPtr->normalFont, fPtr->caption, 
			     strlen(fPtr->caption));

	tx = (view->size.width - tw) / 2;
	
	drawTitle = True;
    } else {
	drawTitle = False;
    }

    {
	XRectangle rect;
	Region region, tmp;
	GC gc[4];
	int i;

	region = XCreateRegion();

	if (drawTitle) {
	    tmp = XCreateRegion();
	    rect.x = tx;
	    rect.y = ty;
	    rect.width = tw;
	    rect.height = th;
	    XUnionRectWithRegion(&rect, tmp, tmp);
	}
	rect.x = 0;
	rect.y = 0;
	rect.width = view->size.width;
	rect.height = view->size.height;
	XUnionRectWithRegion(&rect, region, region);
	if (drawTitle) {
	    XSubtractRegion(region, tmp, region);
	    XDestroyRegion(tmp);
	}
	gc[0] = WMColorGC(scrPtr->black);
	gc[1] = WMColorGC(scrPtr->darkGray);
	gc[2] = WMColorGC(scrPtr->gray);
	gc[3] = WMColorGC(scrPtr->white);

	for (i = 0; i < 4; i++) {
	    XSetRegion(scrPtr->display, gc[i], region);
	}
	XDestroyRegion(region);

	W_DrawReliefWithGC(scrPtr, view->window, 0, fy, view->size.width, fh, 
			   fPtr->flags.relief, gc[0], gc[1], gc[2], gc[3]);

	for (i = 0; i < 4; i++) {
	    XSetClipMask(scrPtr->display, gc[i], None);
	}
    }

    if (drawTitle) {
	WMDrawString(scrPtr, view->window, WMColorGC(scrPtr->black),
		     scrPtr->normalFont, tx, ty, fPtr->caption,
		     strlen(fPtr->caption));
    }
}





static void
handleEvents(XEvent *event, void *data)
{
    Frame *fPtr = (Frame*)data;

    CHECK_CLASS(data, WC_Frame);

    switch (event->type) {
     case Expose:
	if (event->xexpose.count == 0)
	    paintFrame(fPtr);
	break;
	
     case DestroyNotify:
	destroyFrame(fPtr);
	break;
    }
}


WMFrame*
WMCreateFrame(WMWidget *parent)
{
    Frame *fPtr;
    
    fPtr = wmalloc(sizeof(Frame));
    memset(fPtr, 0, sizeof(Frame));

    fPtr->widgetClass = WC_Frame;

    fPtr->view = W_CreateView(W_VIEW(parent));
    if (!fPtr->view) {
	wfree(fPtr);
	return NULL;
    }
    fPtr->view->self = fPtr;
    
    WMCreateEventHandler(fPtr->view, ExposureMask|StructureNotifyMask,
			 handleEvents, fPtr);


    fPtr->flags.relief = DEFAULT_RELIEF;
    fPtr->flags.titlePosition = DEFAULT_TITLE_POSITION;

    WMResizeWidget(fPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    
    return fPtr;
}


static void
destroyFrame(Frame *fPtr)
{    
    if (fPtr->caption)
	wfree(fPtr->caption);

    wfree(fPtr);
}
