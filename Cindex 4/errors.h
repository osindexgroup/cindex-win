#pragma once

enum {			/* error level ids */
	WARN = 0,	/* 1 sound, box; 1 sound, box; 2 sounds, box; 3 sounds, box */
	WARNNB,		/* 1 sound, no box; 1 sound, box; 1 sound, box; 1 sound, box */
	WARNNS,		/* no sound, box; 1 sound, box; 1 sound, box; 1 sound, box */
	WNEVER		/* box but never sound */
};

extern short err_eflag;	/* global error flag -- TRUE after any error */

short showInfoOption(HWND parent,TCHAR * title, const int warnno, ...);		/*  Yes,no */
void showInfo(HWND parent, const int warnno, ...);		/*  O.K. */
short showWarning(HWND parent, const int warnno, ...);		/* cancel, O.K. */
short savewarning(const int warnno, ...);		/* discard, cancel, o.k. */
short showError(HWND parent, const int errnum, const int level, ...);
short senderronstatusline(const int errnum, const int level, ...);
void senditemerr(HWND hwnd,int item);	/* flags item error and sets focus */