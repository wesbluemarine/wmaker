/* gnome.h-- stuff for support for gnome hints
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 1998-2003 Alfredo K. Kojima
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */


#ifndef _GNOME_H_
#define _GNOME_H_


void wGNOMEInitStuff(WScreen *scr);

void wGNOMEUpdateClientListHint(WScreen *scr);

void wGNOMEUpdateWorkspaceHints(WScreen *scr);

void wGNOMEUpdateCurrentWorkspaceHint(WScreen *scr);

void wGNOMEUpdateWorkspaceNamesHint(WScreen *scr);

Bool wGNOMECheckClientHints(WWindow *wwin, int *layer, int *workspace);

void wGNOMEUpdateClientStateHint(WWindow *wwin, Bool changedWorkspace);

Bool wGNOMEProcessClientMessage(XClientMessageEvent *event);

void wGNOMERemoveClient(WWindow *wwin);

Bool wGNOMECheckInitialClientState(WWindow *wwin);

Bool wGNOMEProxyizeButtonEvent(WScreen *scr, XEvent *event);

Bool wGNOMEGetUsableArea(WScreen *scr, int head, WArea *area);

#endif

