/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "quakedef.h"
#include <SDL2/SDL.h>

// TODO: implement (SDL_PauseAudio func)
// I currently just clear the ringbuffer when inactive - dimman
cvar_t sys_inactivesound = {"sys_inactivesound", "0", CVAR_ARCHIVE};
cvar_t sys_inactivesleep = {"sys_inactivesleep", "1"};

void Sys_CvarInit(void)
{
	Cvar_SetCurrentGroup(CVAR_GROUP_SYSTEM_SETTINGS);
	Cvar_Register(&sys_inactivesound);
	Cvar_Register(&sys_inactivesleep);
	Cvar_ResetCurrentGroup();
}

wchar *Sys_GetClipboardTextW(void)
{
	char *tmp;
	wchar *wtmp = NULL;

	if (SDL_HasClipboardText()) {
		tmp = SDL_GetClipboardText();
		wtmp = str2wcs(tmp);
		free(tmp);
	}

	return wtmp;
}

void Sys_CopyToClipboard(char *text)
{
	SDL_SetClipboardText(text);
}

