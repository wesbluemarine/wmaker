



#include "WINGsP.h"
#include "WUtil.h"
#include "wconfig.h"

#include <ctype.h>
#include <string.h>

#ifdef XFT
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>
#endif


/* XXX TODO */
char *WMFontPanelFontChangedNotification = "WMFontPanelFontChangedNotification";




typedef struct W_FontPanel {
    WMWindow *win;

    WMFrame *upperF;
    WMTextField *sampleT;

    WMSplitView *split;

    WMFrame *lowerF;
    WMLabel *famL;
    WMList *famLs;
    WMLabel *typL;
    WMList *typLs;
    WMLabel *sizL;
    WMTextField *sizT;
    WMList *sizLs;

    WMAction2 *action;
    void *data;

    WMButton *revertB;
    WMButton *setB;

    WMPropList *fdb;
} FontPanel;


#define MIN_UPPER_HEIGHT	20
#define MIN_LOWER_HEIGHT	140


#define MAX_FONTS_TO_RETRIEVE	2000

#define BUTTON_SPACE_HEIGHT 40

#define MIN_WIDTH	250
#define MIN_HEIGHT 	(MIN_UPPER_HEIGHT+MIN_LOWER_HEIGHT+BUTTON_SPACE_HEIGHT)

#define DEF_UPPER_HEIGHT 60
#define DEF_LOWER_HEIGHT 310

#define DEF_WIDTH	320
#define DEF_HEIGHT	(DEF_UPPER_HEIGHT+DEF_LOWER_HEIGHT)




static int scalableFontSizes[] = {
    8,
    10,
    11,
    12,
    14,
    16,
    18,
    20,
    24,
    36,
    48,
    64
};



#ifdef XFT
static void setFontPanelFontName(FontPanel *panel, FcChar8 *family, FcChar8 *style, double size);
#endif

static int isXLFD(char *font, int *length_ret);

static void arrangeLowerFrame(FontPanel *panel);

static void familyClick(WMWidget *, void *);
static void typefaceClick(WMWidget *, void *);
static void sizeClick(WMWidget *, void *);


static void listFamilies(WMScreen *scr, WMFontPanel *panel);


static void
splitViewConstrainCallback(WMSplitView *sPtr, int indView, int *min, int *max)
{
    if (indView == 0)
        *min = MIN_UPPER_HEIGHT;
    else
        *min = MIN_LOWER_HEIGHT;
}

static void
notificationObserver(void *self, WMNotification *notif)
{
    WMFontPanel *panel = (WMFontPanel*)self;
    void *object = WMGetNotificationObject(notif);

    if (WMGetNotificationName(notif) == WMViewSizeDidChangeNotification) {
        if (object == WMWidgetView(panel->win)) {
            int h = WMWidgetHeight(panel->win);
            int w = WMWidgetWidth(panel->win);

            WMResizeWidget(panel->split, w, h-BUTTON_SPACE_HEIGHT);

            WMMoveWidget(panel->setB, w-80, h-(BUTTON_SPACE_HEIGHT-5));

            WMMoveWidget(panel->revertB, w-240, h-(BUTTON_SPACE_HEIGHT-5));

        } else if (object == WMWidgetView(panel->upperF)) {

            if (WMWidgetHeight(panel->upperF) < MIN_UPPER_HEIGHT) {
                WMResizeWidget(panel->upperF, WMWidgetWidth(panel->upperF),
                               MIN_UPPER_HEIGHT);
            } else {
                WMResizeWidget(panel->sampleT, WMWidgetWidth(panel->upperF)-20,
                               WMWidgetHeight(panel->upperF)-10);
            }

        } else if (object == WMWidgetView(panel->lowerF)) {

            if (WMWidgetHeight(panel->lowerF) < MIN_LOWER_HEIGHT) {
                WMResizeWidget(panel->upperF, WMWidgetWidth(panel->upperF),
                               MIN_UPPER_HEIGHT);

                WMMoveWidget(panel->lowerF, 0, WMWidgetHeight(panel->upperF)
                             + WMGetSplitViewDividerThickness(panel->split));

                WMResizeWidget(panel->lowerF, WMWidgetWidth(panel->lowerF),
                               WMWidgetWidth(panel->split) - MIN_UPPER_HEIGHT
                               - WMGetSplitViewDividerThickness(panel->split));
            } else {
                arrangeLowerFrame(panel);
            }
        }
    }
}


static void
closeWindow(WMWidget *w, void *data)
{
    FontPanel *panel = (FontPanel*)data;

    WMHideFontPanel(panel);
}




static void
setClickedAction(WMWidget *w, void *data)
{
    FontPanel *panel = (FontPanel*)data;

    if (panel->action)
        (*panel->action)(panel, panel->data);
}


static void
revertClickedAction(WMWidget *w, void *data)
{
    /*FontPanel *panel = (FontPanel*)data;*/
    /* XXX TODO */
}



WMFontPanel*
WMGetFontPanel(WMScreen *scr)
{
    FontPanel *panel;
    WMColor *dark, *white;
    WMFont *font;
    int divThickness;

    if (scr->sharedFontPanel)
        return scr->sharedFontPanel;


    panel = wmalloc(sizeof(FontPanel));
    memset(panel, 0, sizeof(FontPanel));

    panel->win = WMCreateWindow(scr, "fontPanel");
    /*    WMSetWidgetBackgroundColor(panel->win, WMWhiteColor(scr));*/
    WMSetWindowTitle(panel->win, _("Font Panel"));
    WMResizeWidget(panel->win, DEF_WIDTH, DEF_HEIGHT);
    WMSetWindowMinSize(panel->win, MIN_WIDTH, MIN_HEIGHT);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->win), True);

    WMSetWindowCloseAction(panel->win, closeWindow, panel);

    panel->split = WMCreateSplitView(panel->win);
    WMResizeWidget(panel->split, DEF_WIDTH, DEF_HEIGHT - BUTTON_SPACE_HEIGHT);
    WMSetSplitViewConstrainProc(panel->split, splitViewConstrainCallback);

    divThickness = WMGetSplitViewDividerThickness(panel->split);

    panel->upperF = WMCreateFrame(panel->win);
    WMSetFrameRelief(panel->upperF, WRFlat);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->upperF), True);
    panel->lowerF = WMCreateFrame(panel->win);
    /*    WMSetWidgetBackgroundColor(panel->lowerF, WMBlackColor(scr));*/
    WMSetFrameRelief(panel->lowerF, WRFlat);
    WMSetViewNotifySizeChanges(WMWidgetView(panel->lowerF), True);

    WMAddSplitViewSubview(panel->split, W_VIEW(panel->upperF));
    WMAddSplitViewSubview(panel->split, W_VIEW(panel->lowerF));

    WMResizeWidget(panel->upperF, DEF_WIDTH, DEF_UPPER_HEIGHT);

    WMResizeWidget(panel->lowerF, DEF_WIDTH, DEF_LOWER_HEIGHT);

    WMMoveWidget(panel->lowerF, 0, 60+divThickness);

    white = WMWhiteColor(scr);
    dark = WMDarkGrayColor(scr);

    panel->sampleT = WMCreateTextField(panel->upperF);
    WMResizeWidget(panel->sampleT, DEF_WIDTH - 20, 50);
    WMMoveWidget(panel->sampleT, 10, 10);
    WMSetTextFieldText(panel->sampleT, _("The quick brown fox jumps over the lazy dog"));

    font = WMBoldSystemFontOfSize(scr, 12);

    panel->famL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->famL, dark);
    WMSetLabelText(panel->famL, _("Family"));
    WMSetLabelFont(panel->famL, font);
    WMSetLabelTextColor(panel->famL, white);
    WMSetLabelRelief(panel->famL, WRSunken);
    WMSetLabelTextAlignment(panel->famL, WACenter);

    panel->famLs = WMCreateList(panel->lowerF);
    WMSetListAction(panel->famLs, familyClick, panel);

    panel->typL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->typL, dark);
    WMSetLabelText(panel->typL, _("Typeface"));
    WMSetLabelFont(panel->typL, font);
    WMSetLabelTextColor(panel->typL, white);
    WMSetLabelRelief(panel->typL, WRSunken);
    WMSetLabelTextAlignment(panel->typL, WACenter);

    panel->typLs = WMCreateList(panel->lowerF);
    WMSetListAction(panel->typLs, typefaceClick, panel);

    panel->sizL = WMCreateLabel(panel->lowerF);
    WMSetWidgetBackgroundColor(panel->sizL, dark);
    WMSetLabelText(panel->sizL, _("Size"));
    WMSetLabelFont(panel->sizL, font);
    WMSetLabelTextColor(panel->sizL, white);
    WMSetLabelRelief(panel->sizL, WRSunken);
    WMSetLabelTextAlignment(panel->sizL, WACenter);

    panel->sizT = WMCreateTextField(panel->lowerF);
    /*    WMSetTextFieldAlignment(panel->sizT, WARight);*/

    panel->sizLs = WMCreateList(panel->lowerF);
    WMSetListAction(panel->sizLs, sizeClick, panel);

    WMReleaseFont(font);
    WMReleaseColor(white);
    WMReleaseColor(dark);

    panel->setB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->setB, 70, 24);
    WMMoveWidget(panel->setB, 240, DEF_HEIGHT - (BUTTON_SPACE_HEIGHT-5));
    WMSetButtonText(panel->setB, _("Set"));
    WMSetButtonAction(panel->setB, setClickedAction, panel);

    panel->revertB = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->revertB, 70, 24);
    WMMoveWidget(panel->revertB, 80, DEF_HEIGHT - (BUTTON_SPACE_HEIGHT-5));
    WMSetButtonText(panel->revertB, _("Revert"));
    WMSetButtonAction(panel->revertB, revertClickedAction, panel);


    WMRealizeWidget(panel->win);

    WMMapSubwidgets(panel->upperF);
    WMMapSubwidgets(panel->lowerF);
    WMMapSubwidgets(panel->split);
    WMMapSubwidgets(panel->win);

    WMUnmapWidget(panel->revertB);

    arrangeLowerFrame(panel);

    scr->sharedFontPanel = panel;


    /* register notification observers */
    WMAddNotificationObserver(notificationObserver, panel,
                              WMViewSizeDidChangeNotification,
                              WMWidgetView(panel->win));
    WMAddNotificationObserver(notificationObserver, panel,
                              WMViewSizeDidChangeNotification,
                              WMWidgetView(panel->upperF));
    WMAddNotificationObserver(notificationObserver, panel,
                              WMViewSizeDidChangeNotification,
                              WMWidgetView(panel->lowerF));


    listFamilies(scr, panel);


    return panel;
}


void
WMFreeFontPanel(WMFontPanel *panel)
{
    if (panel == WMWidgetScreen(panel->win)->sharedFontPanel) {
        WMWidgetScreen(panel->win)->sharedFontPanel = NULL;
    }
    WMRemoveNotificationObserver(panel);
    WMUnmapWidget(panel->win);
    WMDestroyWidget(panel->win);
    wfree(panel);
}


void
WMShowFontPanel(WMFontPanel *panel)
{
    WMMapWidget(panel->win);
}


void
WMHideFontPanel(WMFontPanel *panel)
{
    WMUnmapWidget(panel->win);
}


WMFont*
WMGetFontPanelFont(WMFontPanel *panel)
{
    return WMGetTextFieldFont(panel->sampleT);
}


void
WMSetFontPanelFont(WMFontPanel *panel, char *fontName)
{
#ifdef XFT
    int fname_len;
    FcPattern *pattern;
    FcChar8 *family, *style;
    double size;

    if (!isXLFD(fontName, &fname_len)) {
        /* maybe its proper fontconfig and we can parse it */
        pattern = FcNameParse(fontName);
    } else {
        /* maybe its proper xlfd and we can convert it to an FcPattern */
        pattern = XftXlfdParse(fontName, False, False);
        //FcPatternPrint(pattern);
    }

    if (!pattern)
        return;

    if (FcPatternGetString(pattern, FC_FAMILY, 0, &family)==FcResultMatch)
        if (FcPatternGetString(pattern, FC_STYLE, 0, &style)==FcResultMatch)
            if (FcPatternGetDouble(pattern, "pixelsize", 0, &size)==FcResultMatch)
                setFontPanelFontName(panel, family, style, size);

    FcPatternDestroy(pattern);
#endif
}


void
WMSetFontPanelAction(WMFontPanel *panel, WMAction2 *action, void *data)
{
    panel->action = action;
    panel->data = data;
}





static void
arrangeLowerFrame(FontPanel *panel)
{
    int width = WMWidgetWidth(panel->lowerF) - 55 - 30;
    int height = WMWidgetHeight(panel->split) - WMWidgetHeight(panel->upperF);
    int fw, tw, sw;

#define LABEL_HEIGHT 20

    height -= WMGetSplitViewDividerThickness(panel->split);


    height -= LABEL_HEIGHT + 8;

    fw = (125*width) / 235;
    tw = (110*width) / 235;
    sw = 55;

    WMMoveWidget(panel->famL, 10, 0);
    WMResizeWidget(panel->famL, fw, LABEL_HEIGHT);

    WMMoveWidget(panel->famLs, 10, 23);
    WMResizeWidget(panel->famLs, fw, height);

    WMMoveWidget(panel->typL, 10+fw+3, 0);
    WMResizeWidget(panel->typL, tw, LABEL_HEIGHT);

    WMMoveWidget(panel->typLs, 10+fw+3, 23);
    WMResizeWidget(panel->typLs, tw, height);

    WMMoveWidget(panel->sizL, 10+fw+3+tw+3, 0);
    WMResizeWidget(panel->sizL, sw+4, LABEL_HEIGHT);

    WMMoveWidget(panel->sizT, 10+fw+3+tw+3, 23);
    WMResizeWidget(panel->sizT, sw+4, 20);

    WMMoveWidget(panel->sizLs, 10+fw+3+tw+3, 46);
    WMResizeWidget(panel->sizLs, sw+4, height-23);
}




#define ALL_FONTS_MASK  "-*-*-*-*-*-*-*-*-*-*-*-*-*-*"

#define FOUNDRY	0
#define FAMILY	1
#define WEIGHT	2
#define SLANT	3
#define SETWIDTH 4
#define ADD_STYLE 5
#define PIXEL_SIZE 6
#define POINT_SIZE 7
#define RES_X	8
#define RES_Y	9
#define SPACING	10
#define AV_WIDTH 11
#define REGISTRY 12
#define ENCODING 13

#define NUM_FIELDS 14



static int
isXLFD(char *font, int *length_ret)
{
    int c = 0;

    *length_ret = 0;
    while (*font) {
        (*length_ret)++;
        if (*font++ == '-')
            c++;
    }

    return c==NUM_FIELDS;
}

#ifndef XFT
static Bool
parseFont(char *font, char values[NUM_FIELDS][256])
{
    char *ptr;
    int part;
    char buffer[256], *bptr;

    part = FOUNDRY;
    ptr = font;
    ptr++; /* skip first - */
    bptr = buffer;
    while (*ptr) {
        if (*ptr == '-') {
            *bptr = 0;
            strcpy(values[part], buffer);
            bptr = buffer;
            part++;
        } else {
            *bptr++ = *ptr;
        }
        ptr++;
    }
    *bptr = 0;
    strcpy(values[part], buffer);

    return True;
}





typedef struct {
    char *weight;
    char *slant;

    char *setWidth;
    char *addStyle;

    char showSetWidth; /* when duplicated */
    char showAddStyle; /* when duplicated */

    WMArray *sizes;
} Typeface;


typedef struct {
    char *name;

    char *foundry;
    char *registry, *encoding;

    char showFoundry; /* when duplicated */
    char showRegistry; /* when duplicated */

    WMArray *typefaces;
} Family;
#endif
#ifdef XFT
typedef struct {
    char *typeface;
    WMArray *sizes;
} Xft_Typeface;

typedef struct {
    char *name; /* gotta love simplicity */
    WMArray *typefaces;
} Xft_Family;
#endif




static int
compare_int(const void *a, const void *b)
{
    int i1 = *(int*)a;
    int i2 = *(int*)b;

    if (i1 < i2)
        return -1;
    else if (i1 > i2)
        return 1;
    else
        return 0;
}


static void
#ifdef XFT
addSizeToTypeface(Xft_Typeface *face, int size)
#else
addSizeToTypeface(Typeface *face, int size)
#endif
{
    if (size == 0) {
        int j;

        for (j = 0; j < sizeof(scalableFontSizes)/sizeof(int); j++) {
            size = scalableFontSizes[j];

            if (!WMCountInArray(face->sizes, (void*)size)) {
                WMAddToArray(face->sizes, (void*)size);
            }
        }
        WMSortArray(face->sizes, compare_int);
    } else {
        if (!WMCountInArray(face->sizes, (void*)size)) {
            WMAddToArray(face->sizes, (void*)size);
            WMSortArray(face->sizes, compare_int);
        }
    }
}

#ifdef XFT
static void
addTypefaceToXftFamily(Xft_Family *fam, char *style)
{
    Xft_Typeface *face;
    WMArrayIterator i;

    if(fam->typefaces) {
        WM_ITERATE_ARRAY(fam->typefaces, face, i) {
            if(strcmp(face->typeface, style) != 0)
                continue; /* go to next interation */
            addSizeToTypeface(face, 0);
            return;
        }
    } else {
        fam->typefaces = WMCreateArray(4);
    }

    face = wmalloc(sizeof(Xft_Typeface));
    memset(face, 0 , sizeof(Xft_Typeface));

    face->typeface = wstrdup(style);
    face->sizes = WMCreateArray(4);
    addSizeToTypeface(face, 0);

    WMAddToArray(fam->typefaces, face);
}

#else /* XFT */

static void
addTypefaceToFamily(Family *family, char fontFields[NUM_FIELDS][256])
{
    Typeface *face;
    WMArrayIterator i;

    if (family->typefaces) {
        WM_ITERATE_ARRAY(family->typefaces, face, i) {
            int size;

            if (strcmp(face->weight, fontFields[WEIGHT]) != 0) {
                continue;
            }
            if (strcmp(face->slant, fontFields[SLANT]) != 0) {
                continue;
            }

            size = atoi(fontFields[PIXEL_SIZE]);

            addSizeToTypeface(face, size);

            return;
        }
    } else {
        family->typefaces = WMCreateArray(4);
    }

    face = wmalloc(sizeof(Typeface));
    memset(face, 0, sizeof(Typeface));

    face->weight = wstrdup(fontFields[WEIGHT]);
    face->slant = wstrdup(fontFields[SLANT]);
    face->setWidth = wstrdup(fontFields[SETWIDTH]);
    face->addStyle = wstrdup(fontFields[ADD_STYLE]);

    face->sizes = WMCreateArray(4);
    addSizeToTypeface(face, atoi(fontFields[PIXEL_SIZE]));

    WMAddToArray(family->typefaces, face);
}
#endif

/*
 * families (same family name) (Hashtable of family -> array)
 * 	registries (same family but different registries)
 *
 */

#ifdef XFT
static void
addFontToXftFamily(WMHashTable *families, char *name, char *style)
{
    WMArrayIterator i;
    WMArray *array;
    Xft_Family *fam;

    array = WMHashGet(families, name);
    if(array) {
        WM_ITERATE_ARRAY(array, fam, i) {
            if(strcmp(fam->name, name) == 0 )
                addTypefaceToXftFamily(fam, style);
            return;
        }
    }

    array = WMCreateArray(8);

    fam = wmalloc(sizeof(Xft_Family));
    memset(fam, 0, sizeof(Xft_Family));

    fam->name = wstrdup(name);

    addTypefaceToXftFamily(fam, style);

    WMAddToArray(array, fam);

    WMHashInsert(families, fam->name, array);
}

#else /* XFT */

static void
addFontToFamily(WMHashTable *families, char fontFields[NUM_FIELDS][256])
{
    WMArrayIterator i;
    Family *fam;
    WMArray *family;


    family = WMHashGet(families, fontFields[FAMILY]);

    if (family) {
        /* look for same encoding/registry and foundry */
        WM_ITERATE_ARRAY(family, fam, i) {
            int enc, reg, found;

            enc = (strcmp(fam->encoding, fontFields[ENCODING]) == 0);
            reg = (strcmp(fam->registry, fontFields[REGISTRY]) == 0);
            found = (strcmp(fam->foundry, fontFields[FOUNDRY]) == 0);

            if (enc && reg && found) {
                addTypefaceToFamily(fam, fontFields);
                return;
            }
        }
        /* look for same encoding/registry */
        WM_ITERATE_ARRAY(family, fam, i) {
            int enc, reg;

            enc = (strcmp(fam->encoding, fontFields[ENCODING]) == 0);
            reg = (strcmp(fam->registry, fontFields[REGISTRY]) == 0);

            if (enc && reg) {
                /* has the same encoding, but the foundry is different */
                fam->showFoundry = 1;

                fam = wmalloc(sizeof(Family));
                memset(fam, 0, sizeof(Family));

                fam->name = wstrdup(fontFields[FAMILY]);
                fam->foundry = wstrdup(fontFields[FOUNDRY]);
                fam->registry = wstrdup(fontFields[REGISTRY]);
                fam->encoding = wstrdup(fontFields[ENCODING]);
                fam->showFoundry = 1;

                addTypefaceToFamily(fam, fontFields);

                WMAddToArray(family, fam);
                return;
            }
        }
        /* look for same foundry */
        WM_ITERATE_ARRAY(family, fam, i) {
            int found;

            found = (strcmp(fam->foundry, fontFields[FOUNDRY]) == 0);

            if (found) {
                /* has the same foundry, but encoding is different */
                fam->showRegistry = 1;

                fam = wmalloc(sizeof(Family));
                memset(fam, 0, sizeof(Family));

                fam->name = wstrdup(fontFields[FAMILY]);
                fam->foundry = wstrdup(fontFields[FOUNDRY]);
                fam->registry = wstrdup(fontFields[REGISTRY]);
                fam->encoding = wstrdup(fontFields[ENCODING]);
                fam->showRegistry = 1;

                addTypefaceToFamily(fam, fontFields);

                WMAddToArray(family, fam);
                return;
            }
        }
        /* foundry and encoding do not match anything known */
        fam = wmalloc(sizeof(Family));
        memset(fam, 0, sizeof(Family));

        fam->name = wstrdup(fontFields[FAMILY]);
        fam->foundry = wstrdup(fontFields[FOUNDRY]);
        fam->registry = wstrdup(fontFields[REGISTRY]);
        fam->encoding = wstrdup(fontFields[ENCODING]);
        fam->showFoundry = 1;
        fam->showRegistry = 1;

        addTypefaceToFamily(fam, fontFields);

        WMAddToArray(family, fam);
        return;
    }

    family = WMCreateArray(8);

    fam = wmalloc(sizeof(Family));
    memset(fam, 0, sizeof(Family));

    fam->name = wstrdup(fontFields[FAMILY]);
    fam->foundry = wstrdup(fontFields[FOUNDRY]);
    fam->registry = wstrdup(fontFields[REGISTRY]);
    fam->encoding = wstrdup(fontFields[ENCODING]);

    addTypefaceToFamily(fam, fontFields);

    WMAddToArray(family, fam);

    WMHashInsert(families, fam->name, family);
}
#endif /* XFT */



static void
listFamilies(WMScreen *scr, WMFontPanel *panel)
{
#ifdef XFT
    FcObjectSet *os = 0;
    FcFontSet	*fs;
    FcPattern	*pat;
#else /* XFT */
    char **fontList;
    char fields[NUM_FIELDS][256];
    int count;
#endif /* XFT */
    WMHashTable *families;
    WMHashEnumerator enumer;
    WMArray *array;
    int i;

#ifdef XFT
    pat = FcPatternCreate();
    os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, 0);
    fs = FcFontList(0, pat, os);
    if (!fs) {
        WMRunAlertPanel(scr, panel->win, _("Error"),
                        _("Could not init font config library\n"), _("OK"), NULL, NULL);
        return;
    }
    if (pat)
        FcPatternDestroy (pat);
#else /* XFT */
    fontList = XListFonts(scr->display, ALL_FONTS_MASK, MAX_FONTS_TO_RETRIEVE,
                          &count);
    if (!fontList) {
        WMRunAlertPanel(scr, panel->win, _("Error"),
                        _("Could not retrieve font list"), _("OK"), NULL, NULL);
        return;
    }
#endif /* XFT */

    families = WMCreateHashTable(WMStringPointerHashCallbacks);

#ifdef XFT
    if(fs) {
        for (i = 0; i < fs->nfont; i++) {
            FcChar8 *family;
            FcChar8 *style;

            if (FcPatternGetString(fs->fonts[i],FC_FAMILY,0,&family)==FcResultMatch)
                if (FcPatternGetString(fs->fonts[i],FC_STYLE,0,&style)==FcResultMatch)
                    addFontToXftFamily(families, family, style);
        }
        FcFontSetDestroy(fs);
    }
#else /* XFT */
    for (i = 0; i < count; i++) {
        int fname_len;

        if (!isXLFD(fontList[i], &fname_len)) {
            *fontList[i] = '\0';
            continue;
        }
        if (fname_len > 255) {
            wwarning(_("font name %s is longer than 256, which is invalid."),
                     fontList[i]);
            *fontList[i] = '\0';
            continue;
        }
        if (!parseFont(fontList[i], fields)) {
            *fontList[i] = '\0';
            continue;
        }
        addFontToFamily(families, fields);
    }
#endif  /* XFT */

    enumer = WMEnumerateHashTable(families);

#ifdef XFT
    while ((array = WMNextHashEnumeratorItem(&enumer))) {
        WMArrayIterator i;
        Xft_Family *fam;
        char buffer[256];
        WMListItem *item;

        WM_ITERATE_ARRAY(array, fam, i) {
            strcpy(buffer, fam->name);
            item = WMAddListItem(panel->famLs, buffer);

            item->clientData = fam;
        }

        WMFreeArray(array);
    }
#else /* XFT */
    while ((array = WMNextHashEnumeratorItem(&enumer))) {
        WMArrayIterator i;
        Family *fam;
        char buffer[256];
        WMListItem *item;

        WM_ITERATE_ARRAY(array, fam, i) {
            strcpy(buffer, fam->name);

            if (fam->showFoundry) {
                strcat(buffer, " ");
                strcat(buffer, fam->foundry);
                strcat(buffer, " ");
            }
            if (fam->showRegistry) {
                strcat(buffer, " (");
                strcat(buffer, fam->registry);
                strcat(buffer, "-");
                strcat(buffer, fam->encoding);
                strcat(buffer, ")");
            }
            item = WMAddListItem(panel->famLs, buffer);

            item->clientData = fam;
        }
        WMFreeArray(array);
    }
#endif /* XFT */
    WMSortListItems(panel->famLs);

    WMFreeHashTable(families);
}


static void
getSelectedFont(FontPanel *panel, char buffer[], int bufsize)
{
    WMListItem *item;
#ifdef XFT
    Xft_Family *family;
    Xft_Typeface *face;
#else
    Family *family;
    Typeface *face;
#endif
    char *size;


    item = WMGetListSelectedItem(panel->famLs);
    if (!item)
        return;
#ifdef XFT
    family = (Xft_Family*)item->clientData;
#else
    family = (Family*)item->clientData;
#endif

    item = WMGetListSelectedItem(panel->typLs);
    if (!item)
        return;
#ifdef XFT
    face = (Xft_Typeface*)item->clientData;
#else
    face = (Typeface*)item->clientData;
#endif

    size = WMGetTextFieldText(panel->sizT);

#ifdef XFT
    snprintf(buffer, bufsize, "%s:style=%s:pixelsize=%s",
             family->name,
             face->typeface,
             size);
#else
    snprintf(buffer, bufsize, "-%s-%s-%s-%s-%s-%s-%s-*-*-*-*-*-%s-%s",
             family->foundry,
             family->name,
             face->weight,
             face->slant,
             face->setWidth,
             face->addStyle,
             size,
             family->registry,
             family->encoding);
#endif /* XFT */
    wfree(size);
}



static void
preview(FontPanel *panel)
{
    char buffer[512];
    WMFont *font;

    getSelectedFont(panel, buffer, sizeof(buffer));
    font = WMCreateFont(WMWidgetScreen(panel->win), buffer);
    if (font) {
        WMSetTextFieldFont(panel->sampleT, font);
        WMReleaseFont(font);
    }
}



static void
familyClick(WMWidget *w, void *data)
{
    WMList *lPtr = (WMList*)w;
    WMListItem *item;
#ifdef XFT
    Xft_Family *family;
    Xft_Typeface *face;
#else
    Family *family;
    Typeface *face;
#endif
    FontPanel *panel = (FontPanel*)data;
    WMArrayIterator i;
    /* current typeface and size */
    char *oface = NULL;
    char *osize = NULL;
    int facei = -1;
    int sizei = -1;

    /* must try to keep the same typeface and size for the new family */
    item = WMGetListSelectedItem(panel->typLs);
    if (item)
        oface = wstrdup(item->text);

    osize = WMGetTextFieldText(panel->sizT);


    item = WMGetListSelectedItem(lPtr);
#ifdef XFT
    family = (Xft_Family*)item->clientData;
#else
    family = (Family*)item->clientData;
#endif

    WMClearList(panel->typLs);


    WM_ITERATE_ARRAY(family->typefaces, face, i) {
        char buffer[256];
        int top=0;
        WMListItem *fitem;

#ifdef XFT
        strcpy(buffer, face->typeface);
        if(strcasecmp(face->typeface, "Roman") == 0)
            top = 1;
        if(strcasecmp(face->typeface, "Regular") == 0)
            top = 1;
#else
        if (strcmp(face->weight, "medium") == 0) {
            buffer[0] = 0;
        } else {
            if (*face->weight) {
                strcpy(buffer, face->weight);
                buffer[0] = toupper(buffer[0]);
                strcat(buffer, " ");
            } else {
                buffer[0] = 0;
            }
        }

        if (strcmp(face->slant, "r") == 0) {
            strcat(buffer, _("Roman"));
            top = 1;
        } else if (strcmp(face->slant, "i") == 0) {
            strcat(buffer, _("Italic"));
        } else if (strcmp(face->slant, "o") == 0) {
            strcat(buffer, _("Oblique"));
        } else if (strcmp(face->slant, "ri") == 0) {
            strcat(buffer, _("Rev Italic"));
        } else if (strcmp(face->slant, "ro") == 0) {
            strcat(buffer, _("Rev Oblique"));
        } else {
            strcat(buffer, face->slant);
        }

        if (buffer[0] == 0) {
            strcpy(buffer, _("Normal"));
        }
#endif
        if (top)
            fitem = WMInsertListItem(panel->typLs, 0, buffer);
        else
            fitem = WMAddListItem(panel->typLs, buffer);
        fitem->clientData = face;
    }

    if (oface) {
        facei = WMFindRowOfListItemWithTitle(panel->typLs, oface);
        wfree(oface);
    }
    if (facei < 0) {
        facei = 0;
    }
    WMSelectListItem(panel->typLs, facei);
    typefaceClick(panel->typLs, panel);

    if (osize) {
        sizei = WMFindRowOfListItemWithTitle(panel->sizLs, osize);
    }
    if (sizei >= 0) {
        WMSelectListItem(panel->sizLs, sizei);
        sizeClick(panel->sizLs, panel);
    }

    if (osize)
        wfree(osize);


    preview(panel);
}


static void
typefaceClick(WMWidget *w, void *data)
{
    FontPanel *panel = (FontPanel*)data;
    WMListItem *item;
#ifdef XFT
    Xft_Typeface *face;
#else
    Typeface *face;
#endif
    WMArrayIterator i;
    char buffer[32];

    char *osize = NULL;
    int sizei = -1;
    void *size;

    osize = WMGetTextFieldText(panel->sizT);


    item = WMGetListSelectedItem(panel->typLs);
#ifdef XFT
    face = (Xft_Typeface*)item->clientData;
#else
    face = (Typeface*)item->clientData;
#endif

    WMClearList(panel->sizLs);

    WM_ITERATE_ARRAY(face->sizes, size, i) {
        if ((int)size != 0) {
            sprintf(buffer, "%i", (int)size);

            WMAddListItem(panel->sizLs, buffer);
        }
    }

    if (osize) {
        sizei = WMFindRowOfListItemWithTitle(panel->sizLs, osize);
    }
    if (sizei < 0) {
        sizei = WMFindRowOfListItemWithTitle(panel->sizLs, "12");
    }
    if (sizei < 0) {
        sizei = 0;
    }
    WMSelectListItem(panel->sizLs, sizei);
    WMSetListPosition(panel->sizLs, sizei);
    sizeClick(panel->sizLs, panel);

    if (osize)
        wfree(osize);

    preview(panel);
}

static void
sizeClick(WMWidget *w, void *data)
{
    FontPanel *panel = (FontPanel*)data;
    WMListItem *item;

    item = WMGetListSelectedItem(panel->sizLs);
    WMSetTextFieldText(panel->sizT, item->text);

    WMSelectTextFieldRange(panel->sizT, wmkrange(0, strlen(item->text)));

    preview(panel);
}


#ifdef XFT
static void
setFontPanelFontName(FontPanel *panel, FcChar8 *family, FcChar8 *style, double size)
{
    int famrow;
    int stlrow;
    int sz;
    char asize[64];
    void *vsize;
    WMListItem *item;
    Xft_Family *fam;
    Xft_Typeface *face;
    WMArrayIterator i;

    famrow = WMFindRowOfListItemWithTitle(panel->famLs, family);
    if (famrow < 0 ){
        famrow = 0;
        return;
    }
    WMSelectListItem(panel->famLs, famrow);
    WMSetListPosition(panel->famLs, famrow);

    WMClearList(panel->typLs);

    item = WMGetListSelectedItem(panel->famLs);

    fam = (Xft_Family*)item->clientData;
    WM_ITERATE_ARRAY(fam->typefaces, face, i) {
        char buffer[256];
        int top=0;
        WMListItem *fitem;

        strcpy(buffer, face->typeface);
        if(strcasecmp(face->typeface, "Roman") == 0)
            top = 1;
        if (top)
            fitem = WMInsertListItem(panel->typLs, 0, buffer);
        else
            fitem = WMAddListItem(panel->typLs, buffer);
        fitem->clientData = face;
    }


    stlrow = WMFindRowOfListItemWithTitle(panel->typLs, style);

    if (stlrow < 0) {
        stlrow = 0;
        return;
    }

    WMSelectListItem(panel->typLs, stlrow);

    item = WMGetListSelectedItem(panel->typLs);

    face = (Xft_Typeface*)item->clientData;

    WMClearList(panel->sizLs);


    WM_ITERATE_ARRAY(face->sizes, vsize, i) {
        char buffer[32];
        if ((int)vsize != 0) {
            sprintf(buffer, "%i", (int)vsize);

            WMAddListItem(panel->sizLs, buffer);
        }
    }

    snprintf(asize, sizeof(asize)-1, "%d",(int)(size+0.5));

    sz = WMFindRowOfListItemWithTitle(panel->sizLs, asize);

    if (sz < 0) {
        sz = 4;
        return;
    }

    WMSelectListItem(panel->sizLs, sz);
    sizeClick(panel->sizLs, panel);

    return;
}

#endif

