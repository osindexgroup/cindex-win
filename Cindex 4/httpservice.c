#include "stdafx.h"
#include <winhttp.h>
#include "commands.h"
#include "errors.h"
#include "httpservice.h"
#include "util.h"
#include "json.h"
#include "registry.h"

static char* fileVersion();
static BOOL parseJson(char* string, DWORD size);
static BOOL runupdate(TCHAR* URL, BOOL close);	// opens browser to load, then optionally close Cindex

/****************************************************************/
BOOL http_connect(BOOL silent)		// checks for available update

{
#ifdef PUBLISH
	TCHAR* path = L"/.well-known/cinwinpub.json";
#else
	TCHAR* path = L"/.well-known/cinwin.json";
#endif

	BOOL bResults = TRUE;
	BOOL result = FALSE;
	time_t lastcheck = 0;
	time_t now = time(NULL);
	DWORD size = sizeof(time_t);

	reg_getkeyvalue(K_GENERAL, MESSAGECHECK, &lastcheck, &size);	// retrieve last check date

	if (!silent || g_prefs.gen.autoupdate && now > lastcheck + 86400 * 1) {	// if checks enabled, do daily
		DWORD dwSize = 0;
		DWORD dwDownloaded = 0;
		LPSTR pszOutBuffer;
		HINTERNET  hSession = NULL, hConnect = NULL, hRequest = NULL;

		// Use WinHttpOpen to obtain a session handle.
		hSession = WinHttpOpen(L"Cindex/4.0",
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		// Specify an HTTP server.
		if (hSession)
			hConnect = WinHttpConnect(hSession, L"opencindex.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
		// Create an HTTP request handle.
		if (hConnect)
			hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
				NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
		// Send a request.
		if (hRequest)
			bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
		// End the request.
		if (bResults)
			bResults = WinHttpReceiveResponse(hRequest, NULL);
		if (bResults) {
			do {		// until data done
				dwSize = 0;
				if (WinHttpQueryDataAvailable(hRequest, &dwSize)) {	// Check for available data.
					pszOutBuffer = (LPSTR)calloc(dwSize + 1, 1);
					if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {	// when fully read
						if (dwSize) {	// if have data
							BOOL hasupDate = parseJson(pszOutBuffer, dwSize);
							if (!hasupDate && !silent)	// if no update, and user asked
								showInfo(g_hwframe,INFO_UPTODATE);
						}
					}
					free(pszOutBuffer);
				}
			} while (dwSize > 0);
		}
		if (!bResults && !silent)	// Report any errors.
			showError(NULL,ERR_NOCONNECTION, WARN, "www.opencindex.com");

		// Close any open handles.
		if (hRequest)
			WinHttpCloseHandle(hRequest);
		if (hConnect)
			WinHttpCloseHandle(hConnect);
		if (hSession)
			WinHttpCloseHandle(hSession);
		reg_setkeyvalue(K_GENERAL, MESSAGECHECK, REG_BINARY, &now, sizeof(now));	// save
	}
	return (bResults);
}
/****************************************************************/
static BOOL parseJson(char * string, DWORD size)

// https://github.com/udp/json-parser

{
	BOOL result = NO;	// no update available

	json_value* value = json_parse((json_char*)string, size);
	if (value) {	// if full version info
		char* title = NULL, * message = NULL, * detail = NULL, * url = NULL, * version = NULL;
		char mBuffer[2048];

		for (int x = 0; x < value->u.object.length; x++) {
			char* name = value->u.object.values[x].name;
			json_value* vv = value->u.object.values[x].value;
			if (!strcmp(name, "title"))
				title = vv->u.string.ptr;
			else if (!strcmp(name, "message"))
				message = vv->u.string.ptr;
			else if (!strcmp(name, "detail"))
				detail = vv->u.string.ptr;
			else if (!strcmp(name, "url"))
				url = vv->u.string.ptr;
			else if (!strcmp(name, "version"))
				version = vv->u.string.ptr;
		}
		if (version && strcmp(version, fileVersion()) > 0) {	// if versions differ
			if (detail != NULL)
				sprintf(mBuffer,"%s\n\n%s",message,detail);
			else
				sprintf(mBuffer, "%s", message);
			if (showInfoOption(g_hwframe,toNative(title), INFO_UPDATEAVAILABLE, mBuffer))
				runupdate(toNative(url),NO);
			result = YES;
		}
		json_value_free(value);
	}
	return result;
}
/****************************************************************/
static BOOL runupdate(TCHAR * URL, BOOL close)	// opens browser to load, then optionally close Cindex

{
	HINSTANCE result;

	result = ShellExecute(NULL, TEXT("open"), URL, NULL, NULL, SW_SHOWNORMAL);
	if ((int)result > 32 && close)	{
		SendMessage(g_hwframe,WM_CLOSE,0,0);
		return TRUE;
	}
	return FALSE;
}
/****************************************************************/
static char * fileVersion()

{
	char* version = NULL;
	TCHAR path[MAX_PATH];
	DWORD vsize;
	void* vdata;
	LPVOID  valptr;
	UINT vlen;

	GetModuleFileName(NULL, path, MAX_PATH);
	if (vsize = GetFileVersionInfoSize(path, NULL)) {
		if (vdata = getmem(vsize)) {
			if (GetFileVersionInfo(path, 0, vsize, vdata)) {
				VerQueryValue(vdata, TEXT("\\StringFileInfo\\040904E4\\FileVersion"), &valptr, &vlen);
				version = fromNative(valptr);
			}
			freemem(vdata);
		}
	}
	return version;
}
