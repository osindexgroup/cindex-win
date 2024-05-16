#include "stdafx.h"
#include <stdarg.h>
#include "errors.h"
#include "util.h"
#include "apiservice.h"

short err_eflag;	/* global error flag -- TRUE after any error */

struct elevel{
	char beep;
	char box;
};

static struct elevel exx[4][4] ={
	{{1,1},{1,1},{2,1},{3,1}},
	{{1,0},{1,1},{1,1},{1,1}},
	{{0,1},{1,1},{1,1},{1,1}},
	{{0,1},{0,1},{0,1},{0,1}}
};
static TCHAR cinname[] = TEXT("CINDEX");

__declspec( thread ) short xt_lasterr, xt_samecount;
__declspec( thread ) long xt_lasttime;
__declspec( thread ) TCHAR xt_laststring[256];

int _centerMode;
HWND _parent, _hooked;
static TCHAR* _buttonOKText, * _buttonCancelText, * _button3Text;

/************************************************************************/
static BOOL CALLBACK child(HWND hwnd, LPARAM okptr)		// for setting button titles

{
	TCHAR buffer[256];
	GetClassName(hwnd, buffer, 256);
	if (!nstrcmp(buffer, L"Button")) {
		GetWindowText(hwnd, buffer, 256);
		if (_buttonOKText && !nstrcmp(buffer, L"OK"))
			SetWindowText(hwnd, _buttonOKText);
		else if (_buttonCancelText && !nstrcmp(buffer, L"Cancel"))
			SetWindowText(hwnd, _buttonCancelText);
	}
	return (TRUE);
}
/*******************************************************************************/
static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)

{
	if (nCode == HCBT_CREATEWND) {
		CBT_CREATEWND* s = (CBT_CREATEWND*)lParam;
		if (s->lpcs->hwndParent == NULL) {	
			HWND twind, bgnd = NULL;
			RECT prect, frect;

			if (_parent)	// for some reason nothing works if really set parent (vs. using parent as background)
				bgnd = _parent;
			else if (g_mdlg)
				bgnd = g_mdlg;
			else if (_centerMode) {	// center on top document window
				bgnd = g_hwclient;	// default is center on client
				twind = FORWARD_WM_MDIGETACTIVE(g_hwclient, SendMessage);
				if (IsWindow(twind) && WX(twind, owner)) {
					if (getmmstate(twind, NULL) != SW_SHOWMINIMIZED && getdata(twind) && WX(twind, owner)->vwind)	/* if not minimized, etc */
						bgnd = WX(twind, owner)->vwind;
				}
			}
			if (!bgnd)
				bgnd = g_hwframe;
			GetWindowRect(g_hwframe, &frect);	// frame rect
			if (IsWindowVisible(bgnd))		// if bgnd is visible
				GetWindowRect(bgnd, &prect);	// get rect
			else
				prect = frect;	// bgnd rect becomes frame
			s->lpcs->x = prect.left + ((prect.right - prect.left) - s->lpcs->cx) / 2;
			s->lpcs->y = prect.top + ((prect.bottom - prect.top) - s->lpcs->cy) / 2;
			// ensure box is contained within frame
			if (s->lpcs->x < frect.left)	// if too far left
				s->lpcs->x = frect.left;
			else if (s->lpcs->x + s->lpcs->cx > frect.right)	// if too far right
				s->lpcs->x = frect.right - s->lpcs->cx;
			if (s->lpcs->y < frect.top)		// if too high
				s->lpcs->y = frect.top;
			else if (s->lpcs->y + s->lpcs->cy > frect.bottom)	// if too low
				s->lpcs->y = frect.bottom - s->lpcs->cy;
			_hooked = (HWND)wParam;
		}
	}
	else if (nCode == HCBT_ACTIVATE) {
		BOOL okflag = TRUE;
		EnumChildWindows((HWND)wParam, child, (LPARAM)&okflag);
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
/*******************************************************************************/
static int CenteredMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)

// provides mechanism for positioning box and changing button titles
{
	HHOOK hHook = SetWindowsHookEx(WH_CBT, &CBTProc, NULL, GetCurrentThreadId());
	_centerMode = 1;
	_parent = hWnd;
	int result = MessageBox(NULL, lpText, lpCaption, uType| MB_TASKMODAL| MB_TOPMOST);
	if (hHook)
		UnhookWindowsHookEx(hHook);
	return result;
}
/*******************************************************************************/
short showInfoOption(HWND parent, TCHAR* title, const int warnno, ...)		/*  O.K. */

{
	int action;

	if (!api_active)	{
		TCHAR text[256], tbuff[256];
		va_list aptr;

		LoadString(g_hinst, warnno,text,255);
		va_start(aptr, warnno);		 /* initialize arg pointer */
		u_vsprintf_u(tbuff, text, aptr); /* get string */
		va_end(aptr);
		_buttonOKText = L"Details...";
		_buttonCancelText = L"Not Now";
		action = CenteredMessageBox(parent,tbuff,title,MB_ICONINFORMATION| MB_OKCANCEL);	/* display alert */
	}
	return action == IDOK;
}
/*******************************************************************************/
void showInfo(HWND parent,const int warnno, ...)		/*  O.K. */

{
	if (!api_active) {
		TCHAR text[256], tbuff[256];
		va_list aptr;

		LoadString(g_hinst, warnno, text, 255);
		va_start(aptr, warnno);		 /* initialize arg pointer */
		u_vsprintf_u(tbuff, text, aptr); /* get string */
		va_end(aptr);
		_buttonOKText = NULL;
		_buttonCancelText = NULL;
		CenteredMessageBox(parent, tbuff, cinname,  MB_ICONINFORMATION | MB_OK );	/* display alert */
	}
}
/*******************************************************************************/
short showWarning(HWND parent,const int warnno, ...)		/* cancel, O.K. */

{
	TCHAR text[STSTRING], tbuff[STSTRING];
	va_list aptr;
	int action;

	if (api_active)
		return FALSE;
	LoadString(g_hinst, warnno, text, STSTRING);
	va_start(aptr, warnno);		 /* initialize arg pointer */
	u_vsprintf_u(tbuff, text, aptr); /* get string */
	va_end(aptr);
	_buttonOKText = NULL;
	_buttonCancelText = NULL;
	action = CenteredMessageBox(parent, tbuff, cinname, MB_ICONWARNING | MB_YESNO);	/* display alert */
	return action == IDYES;
}
/*******************************************************************************/
short savewarning(const int warnno, ...)		/* discard, cancel, o.k. */

{
	TCHAR text[STSTRING], tbuff[STSTRING];
	va_list aptr;
	int action;

	if (!api_active)	{
		LoadString(g_hinst, warnno,text,STSTRING);
		va_start(aptr, warnno);		 /* initialize arg pointer */
		u_vsprintf_u(tbuff, text, aptr); /* get string */
		va_end(aptr);
		_buttonOKText = NULL;
		_buttonCancelText = NULL;
		action = CenteredMessageBox(NULL,tbuff,cinname,MB_ICONWARNING|MB_YESNOCANCEL);	/* display alert */
		if (action == IDYES)
			return (1);
		if (action == IDNO)
			return (-1);
	}
	return (FALSE);
}
/*******************************************************************************/
short showError(HWND parent, const int errnum, const int level, ...)

{
	TCHAR text[STSTRING], tbuff[MAXREC];
	short bcount;
	va_list aptr;

	if (api_active)
		return -1;
	LoadString(g_hinst, errnum, text, STSTRING);
	va_start(aptr, level);		 /* initialize arg pointer */
	u_vsprintf_u(tbuff, text, aptr); /* get string */
	va_end(aptr);
	if (xt_lasterr != errnum || time(NULL) > xt_lasttime + 10 || nstrcmp(tbuff, xt_laststring)) {	/* if changed error or more than 10 sec or diff string */
		xt_lasterr = errnum;
		xt_samecount = 0;		/* reset alert stage */
		xt_lasttime = time(NULL);
	}
	nstrcpy(xt_laststring, tbuff);
	xt_samecount &= 3;			/* limit alert level */
	for (bcount = 0; bcount < exx[level][xt_samecount].beep; bcount++)
		MessageBeep(MB_ICONHAND);
	if (exx[level][xt_samecount].box) {
		_buttonOKText = NULL;
		_buttonCancelText = NULL;
		CenteredMessageBox(parent, tbuff, cinname, MB_ICONERROR | MB_OK);	/* display alert */
	}
	xt_samecount++;	/* counts number of identical errors */
	err_eflag = -1;
	return (err_eflag);
}
/*******************************************************************************/
short senderronstatusline(const int errnum, const int level, ...)

{
	TCHAR text[STSTRING], tbuff[MAXREC];
	short bcount;
	va_list aptr;

	LoadString(g_hinst, errnum,text,STSTRING);
	va_start(aptr, level);		 /* initialize arg pointer */
	u_vsprintf_u(tbuff, text, aptr); /* get string */
	va_end(aptr);
	if (xt_lasterr != errnum || time(NULL) > xt_lasttime+10 || nstrcmp(tbuff,xt_laststring)) 	{	/* if changed error or more than 10 sec or diff string */
		xt_lasterr = errnum;
		xt_samecount = 0;		/* reset alert stage */
		xt_lasttime = time(NULL);
	}
	nstrcpy(xt_laststring,tbuff);
	xt_samecount &= 3;			/* limit alert level */
	for (bcount = 0; bcount < exx[level][xt_samecount].beep; bcount++)
		MessageBeep(MB_ICONHAND);
	if (exx[level][xt_samecount].box)	{
		SendMessage(g_hwstatus,SB_SIMPLE,TRUE,0);	/* set simple status window */
		SendMessage(g_hwstatus,SB_SETTEXT,255|SBT_NOBORDERS,(LPARAM)tbuff);	/* display on status line */
	}
	xt_samecount++;	/* counts number of identical errors */
	err_eflag = -1;
	return (err_eflag);
}
/*******************************************************************************/
void senditemerr(HWND hwnd,int item)	/* flags item error and sets focus */

{
	TCHAR tstring[STSTRING];
	HWND hwed;

	hwed = GetDlgItem(hwnd,item);
	GetWindowText(hwed,tstring,STSTRING);
	showError(hwnd,ERR_BADITEMTEXT,WARNNB,tstring);
	SetFocus(hwed);
	SendMessage(hwed,EM_SETSEL,0,-1);
}