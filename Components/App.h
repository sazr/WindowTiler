/*
Copyright (c) 2016 Sam Zielke-Ryner All rights reserved.

For job opportunities or to work together on projects please contact
myself via Github:   https://github.com/sazr

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. The source code, API or snippets cannot be used for commercial
purposes without written consent from the author.

THIS SOFTWARE IS PROVIDED ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef WT_APP_H
#define WT_APP_H

#include "stdafx.h"

class App : public Win32App
{
public:
	friend class IApp;

	static Status WM_MENU_REPORT_BUG;
	static Status WM_MENU_DONATE;
	static Status WM_MENU_EXIT;

	static BOOL CALLBACK enumWindows(HWND hwnd, LPARAM lParam);

	virtual ~App();

	Status init(const IEventArgs& evtArgs);
	Status terminate(const IEventArgs& evtArgs);

protected:

	App();

	Status onTrayIconInteraction(const IEventArgs& evtArgs);
	Status showWindow(const IEventArgs& evtArgs=NULL_ARGS);
	Status hideWindow(const IEventArgs& evtArgs = NULL_ARGS);
	Status onKillFocus(const IEventArgs& evtArgs);
	bool isAltTabWindow(HWND hwnd);

	Status initMenu();
	Status gridBtnCallback(const IEventArgs& evtArgs);
	Status columnsBtnCallback(const IEventArgs& evtArgs);
	Status customBtnCallback(const IEventArgs& evtArgs, const tstring iconPath);
	Status onCustomBtnDown(const IEventArgs& evtArgs);
	Status onCustomBtnUp(const IEventArgs& evtArgs);
	Status onImageSelectorMouseLeave(const IEventArgs& evtArgs);
	Status onLayoutBtnClick(const IEventArgs& evtArgs, int custLayoutIndex);

	Status readINIFile();
	Status writeINIFile(const CustomLayout& layout);
	Status loadCustomLayoutIcons();

private:
	Status IDB_GRID = Status::registerState(_T("Grid Button"));
	Status IDB_COLS = Status::registerState(_T("Columns Button"));
	Status IDB_CTM = Status::registerState(_T("Custom Button"));

	/*const*/ tstring INIFilePath;
	std::vector <HWND> openWnds;
	std::vector <CustomLayout> customLayouts;
	std::vector<HGDIOBJ> gdiObjects;
	SIZE btnSize;
	//HWND layoutBtn;
	HWND gridBtn;
	HWND columnBtn;
	HWND customBtn;
	HWND voidBtn;
	HMENU hMenu;
	bool skipMinimisedHwnds;
	std::shared_ptr<HorzListBoxComponent> horizListBoxCmp;
	std::shared_ptr<VertListBoxComponent> vertListBoxCmp;
	std::shared_ptr<SystemTrayComponent> sysTrayCmp;
};

#endif // WT_APP_H