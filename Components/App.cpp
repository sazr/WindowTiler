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

#include "stdafx.h"
#include "App.h"

// Class Property Implementation //
Status App::WM_MENU_REPORT_BUG = Status::registerState(_T("WM_MENU_REPORT_BUG"));
Status App::WM_MENU_DONATE = Status::registerState(_T("WM_MENU_DONATE"));
Status App::WM_MENU_EXIT = Status::registerState(_T("WM_MENU_EXIT"));

// Static Function Implementation //
BOOL CALLBACK App::enumWindows(HWND hwnd, LPARAM lParam)
{
	App* app = (App*)lParam;

	if (App::isAltTabWindow(hwnd))
		app->openWnds.push_back(hwnd);

	return TRUE;
}

BOOL CALLBACK App::findWindowCallback(HWND hwnd, LPARAM lParam)
{
	FindWindowInfo& data = *(FindWindowInfo*)lParam;
	
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	
	output(_T("pid: %d, %d\n"), data.pId, lpdwProcessId);
	if (lpdwProcessId == data.pId && App::isAltTabWindow(hwnd)) {
		outputStr("Found hwnd\n");
		data.targetHwnd = hwnd;
		return FALSE;
	}

	return TRUE;
}

bool App::isAltTabWindow(HWND hwnd)
{
	#pragma message("Bug: currently misses some top level windows incl Piriform CCleaner.")
	TITLEBARINFO ti;
	HWND hwndTry, hwndWalk = NULL;

	if (!IsWindowVisible(hwnd))
		return false;

	hwndTry = GetAncestor(hwnd, GA_ROOTOWNER);
	while (hwndTry != hwndWalk)
	{
		hwndWalk = hwndTry;
		hwndTry = GetLastActivePopup(hwndWalk);
		if (IsWindowVisible(hwndTry))
			break;
	}

	if (hwndWalk != hwnd)
		return false;

	// the following removes some task tray programs and "Program Manager"
	ti.cbSize = sizeof(ti);
	GetTitleBarInfo(hwnd, &ti);
	if (ti.rgstate[0] & STATE_SYSTEM_INVISIBLE)
		return false;

	// Tool windows should not be displayed either, these do not appear in the
	// task bar.
	if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
		return false;

	return true;
}


// Function Implementation //
App::App() 
	: Win32App()/*, 
	INIFilePath(INIFilePath)*/
{
	wndDimensions		= RECT{ -400, -400, 420, 126 };
	bkColour			= CreateSolidBrush(RGB(50, 50, 50));
	wndTitle			= _T("Window Tiler");
	wndFlags			= WS_EX_TOOLWINDOW | WS_POPUP;
	iconID				= IDI_APP;
	smallIconID			= IDI_SMALL;
	skipMinimisedHwnds	= true;
	gdiObjects.push_back(bkColour);

	TCHAR szPath[MAX_PATH];
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath) == S_OK) {
		CreateDirectory((tstring(szPath) + _T("\\WindowTiler")).c_str(), NULL);
		INIFilePath = tstring(szPath) + _T("\\WindowTiler\\WindowTiler.ini");
	}
}

App::~App()
{
	std::for_each(gdiObjects.begin(), gdiObjects.end(), [](HGDIOBJ& obj) {
		
		if (obj)
			DeleteObject(obj);
	});
}

Status App::init(const IEventArgs& evtArgs)
{
	const Win32AppInit& initArgs = static_cast<const Win32AppInit&>(evtArgs);
	
	readINIFile();

	#ifndef _DEBUG
		auto sCmp = addComponent<ScheduleAppComponent>(app, _T("WindowTilerCBA"));

		char exePath[MAX_PATH];
		GetModuleFileNameA(NULL, exePath, MAX_PATH);
		std::string exePathStr = std::string(exePath);
		std::string exeDir = exePathStr.substr(0, exePathStr.rfind("\\"));
		addComponent<AutoUpdateComponent>(app, "http://windowtiler.soribo.com.au/version.php?application=window-tiler",
			"http://windowtiler.soribo.com.au/WindowTiler.exe", 
			exeDir+"\\updated.exe",
			WStringToString(INIFilePath));
	#endif // _DEBUG

	HBRUSH grayBrush = CreateSolidBrush(RGB(50, 50, 50));
	gdiObjects.push_back(grayBrush);

	// Add pre-init components. TODO: edit CBA API to provide pre-init create components function
	addComponent<DPIAwareComponent>(app);
	auto tooltipCmp			= addComponent<TooltipComponent>(app);
	auto dispatchCmp		= addComponent<DispatchWindowComponent>(app);
	sysTrayCmp				= addComponent<SystemTrayComponent>(app, _T("images/small.ico"), _T("Window Tiler"));
	addComponent<BorderWindowComponent>(app, RECT{6, 6, 408, 114});
	horizListBoxCmp			= addComponent<HorzListBoxComponent>(app, RECT{ 18, 16,
								386, 94 }, grayBrush, 8, 2, true);
	vertListBoxCmp			= addComponent<VertListBoxComponent>(app, RECT{ 0, 0,
								96, 300 }, grayBrush, 3, 8, true, WS_POPUP);
	vertListBoxCmp->setScroller<HoverScrollerComponent>(IScrollerComponent::ScrollDirection::SCROLL_VERT);

	Win32App::init(evtArgs);

	DPIAwareComponent::scaleRect(wndDimensions);
	SetWindowPos(hwnd, 0, wndDimensions.left, wndDimensions.top, wndDimensions.right, wndDimensions.bottom, 0);
	btnSize					= SIZE { DPIAwareComponent::scaleUnits(90 ), DPIAwareComponent::scaleUnits(90) };
	HWND horizLb			= horizListBoxCmp->getHwnd();
	HWND vertLb				= vertListBoxCmp->getHwnd();
	ShowWindow(vertLb, SW_HIDE);

	HBITMAP gridBm = (HBITMAP)LoadImage(NULL,
		_T("Images/btn1.bmp"), IMAGE_BITMAP, btnSize.cx, btnSize.cy,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_VGACOLOR);

	HBITMAP colsBm = (HBITMAP)LoadImage(NULL,
		_T("Images/btn2.bmp"), IMAGE_BITMAP, btnSize.cx, btnSize.cy,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_VGACOLOR);

	HBITMAP customBm = (HBITMAP)LoadImage(NULL,
		_T("Images/btn3.bmp"), IMAGE_BITMAP, btnSize.cx, btnSize.cy,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_VGACOLOR);

	gdiObjects.push_back(gridBm);
	gdiObjects.push_back(colsBm);
	gdiObjects.push_back(customBm);

	gridBtn = CreateWindowEx(WS_EX_TRANSPARENT, _T("BUTTON"), _T("Grid"), WS_CHILD | WS_VISIBLE | BS_VCENTER | BS_DEFPUSHBUTTON | BS_BITMAP,
		DPIAwareComponent::scaleUnits(10), DPIAwareComponent::scaleUnits(195), 
		btnSize.cx, btnSize.cy, 
		horizLb, (HMENU)IDB_GRID.state, hinstance, 0);

	columnBtn = CreateWindowEx(WS_EX_TRANSPARENT, _T("BUTTON"), _T("Columns"), WS_CHILD | WS_VISIBLE | BS_VCENTER | BS_DEFPUSHBUTTON | BS_BITMAP,
		DPIAwareComponent::scaleUnits(106), DPIAwareComponent::scaleUnits(195), 
		btnSize.cx, btnSize.cy,
		horizLb, (HMENU)IDB_COLS.state, hinstance, 0);

	customBtn = CreateWindowEx(WS_EX_TRANSPARENT, _T("BUTTON"), _T("+"), WS_CHILD | WS_VISIBLE | BS_VCENTER | BS_DEFPUSHBUTTON | BS_BITMAP,
		DPIAwareComponent::scaleUnits(202), DPIAwareComponent::scaleUnits(195),
		btnSize.cx, btnSize.cy,
		horizLb, (HMENU)IDB_CTM.state, hinstance, 0); 

	tooltipCmp->addTooltip(gridBtn, _T("Arrange windows in grid pattern"));
	tooltipCmp->addTooltip(columnBtn, _T("Arrange windows in columns"));
	tooltipCmp->addTooltip(customBtn, _T("Create new custom layout from current window arrangement"));

	SendMessage(gridBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)gridBm);
	SendMessage(columnBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)colsBm);
	SendMessage(customBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)customBm);
	horizListBoxCmp->addChild(gridBtn);
	horizListBoxCmp->addChild(columnBtn);

	loadCustomLayouts();
	loadCustomLayoutIcons();

	horizListBoxCmp->addChild(customBtn);

	if (customLayouts.empty()) {
		HBITMAP voidBm = (HBITMAP)LoadImage(NULL,
			_T("Images/btn4.bmp"), IMAGE_BITMAP, btnSize.cx, btnSize.cy,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_VGACOLOR);

		gdiObjects.push_back(voidBm);

		voidBtn = CreateWindowEx(WS_EX_TRANSPARENT, _T("BUTTON"), _T("+"), WS_CHILD | WS_VISIBLE | BS_VCENTER | BS_DEFPUSHBUTTON | BS_BITMAP,
			DPIAwareComponent::scaleUnits(202), DPIAwareComponent::scaleUnits(195),
			btnSize.cx, btnSize.cy,
			horizLb, 0, hinstance, 0);

		SendMessage(voidBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)voidBm);
		horizListBoxCmp->addChild(voidBtn);
		tooltipCmp->addTooltip(voidBtn, _T("New layouts will appear here"));
	}
	
	registerEvent(SystemTrayComponent::WM_TRAY_ICON.state, &App::onTrayIconInteraction);
	registerEvent(IDB_GRID.state, &App::gridBtnCallback);
	registerEvent(IDB_COLS.state, &App::columnsBtnCallback);
	registerEvent(DispatchWindowComponent::translateMessage(customBtn, WM_LBUTTONDOWN), &App::onCustomBtnDown);
	registerEvent(DispatchWindowComponent::translateMessage(customBtn, WM_LBUTTONUP), &App::onCustomBtnUp);
	registerEvent(WM_ACTIVATEAPP, &App::onKillFocus);
	//registerEvent(WM_KILLFOCUS, &App::onKillFocus);

	initMenu();

	// Dont show window on scheduled startup load
	tstring cmdArgs = initArgs.cmdLine;
	if (cmdArgs.find(_T("/scheduledstart")) != tstring::npos)
		hideWindow();
	else showWindow();

	return S_SUCCESS;
}

Status App::terminate(const IEventArgs& evtArgs)
{
	Win32App::terminate(evtArgs);

	return S_SUCCESS;
}

Status App::initMenu()
{
	hMenu = CreatePopupMenu();

	AppendMenu(hMenu, MF_STRING, WM_MENU_REPORT_BUG.state, TEXT("Report a bug"));
	AppendMenu(hMenu, MF_STRING, WM_MENU_DONATE.state, TEXT("Donate"));
	AppendMenu(hMenu, MF_STRING, WM_MENU_EXIT.state, TEXT("Exit"));
	return S_SUCCESS;
}

Status App::onTrayIconInteraction(const IEventArgs& evtArgs)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);
	
	// if the tray icon message is not for this SystemTrayComponent tray icon: return
	if (args.wParam != sysTrayCmp->trayIconID.state)
		return S_SUCCESS;
	
	if (args.lParam == WM_LBUTTONDOWN) {
		if (IsWindowVisible(hwnd))
			hideWindow();
		else showWindow();
	}
	else if (args.lParam == WM_RBUTTONDOWN) {
		POINT curPoint;
		GetCursorPos(&curPoint);
		SetForegroundWindow(args.hwnd);

		UINT clicked = TrackPopupMenu(hMenu,
			TPM_RETURNCMD | TPM_NONOTIFY | TPM_VERPOSANIMATION,
			curPoint.x, curPoint.y, 0, args.hwnd, NULL);

		if (clicked == WM_MENU_REPORT_BUG.state)
		{
			output(_T("Report\n"));
			ShellExecute(NULL, _T("open"), _T("http://windowtiler.soribo.com.au/report-a-bug/"), NULL, NULL, SW_SHOWNORMAL);
		}
		else if (clicked == WM_MENU_DONATE.state)
		{
			output(_T("Donate\n"));
			ShellExecute(NULL, _T("open"), _T("http://windowtiler.soribo.com.au/donate"), NULL, NULL, SW_SHOWNORMAL);
		}
		else if (clicked == WM_MENU_EXIT.state)
		{
			output(_T("Exit\n"));
			PostMessage(args.hwnd, WM_CLOSE, 0, 0);
		}
	}

	return S_SUCCESS;
}

Status App::showWindow(const IEventArgs& evtArgs)
{
	int x, y, buffer = 20;
	RECT wndDim, screenDim;
	APPBARDATA barData{ 0 };
	barData.cbSize = sizeof(APPBARDATA);

	GetWindowRect(hwnd, &wndDim);
	util->getClientRect(screenDim);
	SHAppBarMessage(ABM_GETTASKBARPOS, &barData);

	switch (barData.uEdge)
	{
	case ABE_LEFT:
	{
		x = barData.rc.right + buffer;
		y = barData.rc.bottom - buffer - (wndDim.bottom - wndDim.top);
	}
	break;
	case ABE_RIGHT:
	{
		x = barData.rc.left - buffer - (wndDim.right - wndDim.left);
		y = barData.rc.bottom - buffer - (wndDim.bottom - wndDim.top);
	}
	break;
	case ABE_TOP:
	{
		x = barData.rc.right - buffer - (wndDim.right - wndDim.left);
		y = barData.rc.bottom + buffer;
	}
	break;
	//case ABE_BOTTOM:
	default:
	{
		x = barData.rc.right - buffer - (wndDim.right - wndDim.left);
		y = barData.rc.top - buffer - (wndDim.bottom - wndDim.top);
	}
	break;
	}

	SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);
	ShowWindow(hwnd, SW_SHOW);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	return S_SUCCESS;
}

Status App::hideWindow(const IEventArgs& evtArgs)
{
	ShowWindow(vertListBoxCmp->getHwnd(), SW_HIDE);
	ShowWindow(hwnd, SW_HIDE);

	return S_SUCCESS;
}

Status App::onKillFocus(const IEventArgs& evtArgs)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);

	/* Handle WM_KILLFOCUS
	if (!IsChild(args.hwnd, (HWND)args.wParam))
		hideWindow(evtArgs);*/

	// HANDLE WM_ACTIVATEAPP
	// if being deactived
	if (args.wParam == FALSE)
		hideWindow(evtArgs);

	return S_SUCCESS;
}

Status App::gridBtnCallback(const IEventArgs& evtArgs)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);
	int message = HIWORD(args.wParam);

	if (message != BN_CLICKED)
		return S_SUCCESS;
	
	// Identify top-level HWNDs
	openWnds.clear();
	EnumWindows(enumWindows, (LPARAM)this);

	if (openWnds.empty())
		return S_SUCCESS;

	RECT screenDim;
	util->getClientRect(screenDim);
	int index			= 0;
	int x				= 0;
	int y				= 0;
	int evenWnds		= (openWnds.size() % 2 == 0) ? openWnds.size() : openWnds.size() + 1;
	int nRows			= (int)floor(sqrt(evenWnds));
	int nCols			= (int)ceil(evenWnds / nRows);
	SIZE gridDim		= { (screenDim.right - screenDim.left) / nCols,
							(screenDim.bottom - screenDim.top) / nRows };

	for (int row = 0; row < nRows; row++) {
		for (int col = 0; col < nCols; col++) {

			WINDOWPLACEMENT wndPlacement;
			wndPlacement.length = sizeof(WINDOWPLACEMENT);
			wndPlacement.showCmd = SW_SHOWNORMAL;

			if (index == openWnds.size() - 1 && evenWnds != openWnds.size()) {
				RECT r { x, y, x + gridDim.cx * 2, y + gridDim.cy };
				wndPlacement.rcNormalPosition = r;
				SetWindowPlacement(openWnds.at(index), &wndPlacement);
				break;
			}

			//int index = ((row + 1) * (col + 1)) - 1;

			RECT r{ x, y, x + gridDim.cx, y + gridDim.cy };
			wndPlacement.rcNormalPosition = r;
			SetWindowPlacement(openWnds.at(index), &wndPlacement);
			// Bring to front
			SetWindowPos(openWnds.at(index), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			x += gridDim.cx;
			index++;
		}

		x = 0;
		y += gridDim.cy;
	}

	return S_SUCCESS;
}

Status App::columnsBtnCallback(const IEventArgs& evtArgs)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);
	int message = HIWORD(args.wParam);

	if (message != BN_CLICKED)
		return S_SUCCESS;

	// Identify top-level HWNDs
	openWnds.clear();
	BOOL res = EnumWindows(enumWindows, (LPARAM)this);

	if (openWnds.empty())
		return S_SUCCESS;

	RECT screenDim;
	util->getClientRect(screenDim);
	int x					= 0;
	int y					= 0;
	int nCols				= openWnds.size();
	SIZE gridDim			= { (screenDim.right - screenDim.left) / nCols,
								screenDim.bottom - screenDim.top };
	
	for (int i = 0; i < openWnds.size(); i++) {

		WINDOWPLACEMENT wndPlacement;
		wndPlacement.length = sizeof(WINDOWPLACEMENT);
		wndPlacement.showCmd = SW_SHOWNORMAL;
		wndPlacement.rcNormalPosition = RECT{ x, y, x + gridDim.cx, y + gridDim.cy };
		SetWindowPlacement(openWnds.at(i), &wndPlacement);
		// Bring to front
		SetWindowPos(openWnds.at(i), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		x += gridDim.cx;
	}

	return S_SUCCESS;
}

Status App::onCustomBtnDown(const IEventArgs& evtArgs)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);

	RECT btnDim, vlbDim;
	APPBARDATA barData{ 0 };
	barData.cbSize			= sizeof(APPBARDATA);
	HWND vertLb				= vertListBoxCmp->getHwnd();
	GetWindowRect((HWND)args.hwnd, &btnDim);
	GetWindowRect(vertLb, &vlbDim);
	SHAppBarMessage(ABM_GETTASKBARPOS, &barData);

	int btnW			= btnDim.right - btnDim.left;
	int lbW				= vlbDim.right - vlbDim.left;
	int lbH				= vlbDim.bottom - vlbDim.top;
	int xPos			= btnDim.left + ((btnW - lbW) / 2);
	int yPos			= 0;

	switch (barData.uEdge)
	{
	case ABE_LEFT:
	case ABE_RIGHT:
	{
		yPos = btnDim.top - lbH - 10;
	}
	break;
	case ABE_TOP:
	{
		yPos = btnDim.bottom + 10;
	}
	break;
	//case ABE_BOTTOM:
	default:
	{
		yPos = btnDim.top - lbH - 10;
	}
	break;
	}

	SetWindowPos(vertLb, HWND_TOPMOST, xPos, yPos, 0, 0, SWP_NOSIZE);
	ShowWindow(vertLb, SW_SHOW);
	//SetFocus(vertLb);

	return DispatchWindowComponent::WM_STOP_PROPAGATION_MSG;
}

Status App::onCustomBtnUp(const IEventArgs& evtArgs)
{
	ShowWindow(vertListBoxCmp->getHwnd(), SW_HIDE);

	return S_SUCCESS;
}

Status App::onImageSelectorMouseLeave(const IEventArgs& evtArgs)
{
	#pragma message("TODO: detect mouse leave of vertical listbox")

	if (GetKeyState(VK_LBUTTON) >= 0)
		ShowWindow(vertListBoxCmp->getHwnd(), SW_HIDE);

	return S_SUCCESS;
}

Status App::customBtnCallback(const IEventArgs& evtArgs, const tstring iconPath)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);

	if (customLayouts.size() >= 5) {
		MessageBox(NULL, _T("Maximum number custom layouts created"),
			_T(""), MB_OK | MB_ICONINFORMATION);
		return S_SUCCESS;
	}

	// Detect top-level HWND's
	openWnds.clear();
	BOOL res = EnumWindows(enumWindows, (LPARAM)this);
	
	CustomLayout newCustomLayout;
	_tcscpy(newCustomLayout.layoutIconPath, iconPath.c_str());

	// Create WndInfo structs
	for (int i = 0; i < openWnds.size(); i++) {

		RECTF wndDimScaled;
		RECT wndDim, clientRect;
		bool res1 = util->getClientRect(clientRect) == S_SUCCESS;

		// if is minimised
		if (IsIconic(openWnds.at(i))) {
			if (skipMinimisedHwnds)
				continue;

			WINDOWPLACEMENT wndPlacement;
			wndPlacement.length = sizeof(WINDOWPLACEMENT);
			BOOL res2 = GetWindowPlacement(openWnds.at(i), &wndPlacement);
			wndDim = wndPlacement.rcNormalPosition;
		}
		else {
			GetWindowRect(openWnds.at(i), &wndDim);
			wndDim.left -= clientRect.left;
			wndDim.top -= clientRect.top;
			wndDim.right -= clientRect.left;
			wndDim.bottom -= clientRect.top;
		}

		float w					= clientRect.right	- clientRect.left;
		float h					= clientRect.bottom - clientRect.top;
		wndDimScaled.left		= (wndDim.left != 0)	? wndDim.left / w	: 0;
		wndDimScaled.right		= (wndDim.right != 0)	? wndDim.right / w	: 0;
		wndDimScaled.top		= (wndDim.top != 0)		? wndDim.top / h	: 0;
		wndDimScaled.bottom		= (wndDim.bottom != 0)	? wndDim.bottom / h : 0;

		DWORD pid;
		tstring exePath;
		GetWindowThreadProcessId(openWnds.at(i), &pid);
		util->getProcessFilePath(pid, exePath);

		HwndInfo hInfo;
		_tcscpy(hInfo.exePath, exePath.c_str());
		hInfo.openApp			= true;
		hInfo.wndDimScaled		= wndDimScaled;
		newCustomLayout.hwndInfos.push_back(hInfo);
	}

	// Remove 'void-layout' HWND
	if (customLayouts.empty())
		horizListBoxCmp->removeLastChild();

	// Write new custom layout
	if (!writeINIFile(newCustomLayout))
		return S_UNDEFINED_ERROR;

	// Add new Layout button to horiz listbox
	int custLayoutIndex			= customLayouts.size() - 1;
	Status custMsg				= Status::registerState(_T("Custom Layout"));
	HWND custLytBtn				= CreateWindowEx(WS_EX_TRANSPARENT, _T("BUTTON"), _T("Custom2"), 
												WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_VCENTER | BS_BITMAP,
												DPIAwareComponent::scaleUnits(0), DPIAwareComponent::scaleUnits(0),
												btnSize.cx, btnSize.cy,
												horizListBoxCmp->getHwnd(), (HMENU)custMsg.state, hinstance, 0);

	// Register button click callback
	registerEventLambda<App>(custMsg, [custLayoutIndex, this](const IEventArgs& evtArgs)->Status {

		return this->onLayoutBtnClick(evtArgs, custLayoutIndex);
	});


	registerEventLambda<App>(DispatchWindowComponent::translateMessage(custLytBtn, WM_RBUTTONDOWN),
		[custLayoutIndex, this](const IEventArgs& evtArgs)->Status {

		output(_T("Right btn\n"));
		return S_SUCCESS;
	});

	horizListBoxCmp->addChild(custLytBtn, horizListBoxCmp->size() - 1);
	HBITMAP bm = (HBITMAP)SendMessage(args.hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
	SendMessage(custLytBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bm);
	InvalidateRect(horizListBoxCmp->getHwnd(), NULL, TRUE);
	UpdateWindow(horizListBoxCmp->getHwnd());

	return S_SUCCESS;
}

Status App::onLayoutBtnClick(const IEventArgs& evtArgs, int custLayoutIndex)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);
	int message = HIWORD(args.wParam);

	if (message != BN_CLICKED)
		return S_SUCCESS;

	// Detect top-level HWND's
	openWnds.clear();
	BOOL res = EnumWindows(enumWindows, (LPARAM)this);
	
	RECT clientRect;
	util->getClientRect(clientRect);
	float w							= clientRect.right - clientRect.left;
	float h							= clientRect.bottom - clientRect.top;
	CustomLayout c					= customLayouts.at(custLayoutIndex);
	std::vector<HwndInfo> hwndInfos = c.hwndInfos;
	
	for (int i = 0; i < openWnds.size(); i++) {

		DWORD pid;
		tstring exePath;
		GetWindowThreadProcessId(openWnds.at(i), &pid);
		util->getProcessFilePath(pid, exePath);

		for (int j = 0; j < hwndInfos.size(); j++) {
			if (_tcscmp(hwndInfos.at(j).exePath, exePath.c_str()) != 0)
				continue;

			RECT wndDim;
			wndDim.left						= w * hwndInfos.at(j).wndDimScaled.left;
			wndDim.top						= h * hwndInfos.at(j).wndDimScaled.top;
			wndDim.right					= w * hwndInfos.at(j).wndDimScaled.right;
			wndDim.bottom					= h * hwndInfos.at(j).wndDimScaled.bottom;

			WINDOWPLACEMENT wndPlacement;
			wndPlacement.length				= sizeof(WINDOWPLACEMENT);
			wndPlacement.showCmd			= SW_SHOWNORMAL;
			wndPlacement.rcNormalPosition	= wndDim;
			SetWindowPlacement(openWnds.at(i), &wndPlacement);

			// Bring to front
			SetWindowPos(openWnds.at(i), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

			hwndInfos.erase(hwndInfos.begin() + j);
			break;
		}
	}

	/*for (int j = 0; j < hwndInfos.size(); j++) {
		if (!hwndInfos[j].openApp)
			continue;

		RECT wndDim;
		wndDim.left = w * hwndInfos.at(j).wndDimScaled.left;
		wndDim.top = h * hwndInfos.at(j).wndDimScaled.top;
		wndDim.right = w * hwndInfos.at(j).wndDimScaled.right;
		wndDim.bottom = h * hwndInfos.at(j).wndDimScaled.bottom;

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USEPOSITION | STARTF_USESIZE;
		si.dwX = wndDim.left;
		si.dwY = wndDim.top;
		si.dwXSize = wndDim.right;
		si.dwYSize = wndDim.bottom;

		if (!CreateProcess(hwndInfos[j].exePath, _T(""),
			NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL,
			NULL, &si, &pi)) {
			output(_T("Failed to open: %s\n"), hwndInfos[j].exePath);
			continue;
		}
	}*/

	return S_SUCCESS;
}

Status App::readINIFile()
{
	// Get Sections

	std::vector<tstring> sectionNames;
	if (!util->getINISectionNames(INIFilePath, sectionNames)) {
		output(_T("Failed to get section names\n"));
		return S_UNDEFINED_ERROR;
	}

	// Get Section data
	for (int i = 0; i < sectionNames.size(); i++) {

		CustomLayout layout;
		std::vector<tstring> keys, values;

		if (sectionNames[i].find(_T("CustomLayout")) == tstring::npos)
			continue;

		if (!util->getINISectionKeyValues(INIFilePath, sectionNames[i], keys, values)) {
			output(_T("Failed to get section key value pairs\n"));
			return S_UNDEFINED_ERROR;
		}

		for (int j = 0; j < keys.size(); j++) {

			if (keys[j] == _T("layoutIconPath")) {
				if (!GetPrivateProfileStruct(sectionNames[i].c_str(), keys[j].c_str(), &layout.layoutIconPath[0], sizeof(layout.layoutIconPath)/sizeof(layout.layoutIconPath[0]), INIFilePath.c_str()))
					output(_T("Failed to read struct\n"));

				continue;
			}

			HwndInfo hInfo;

			#ifdef USE_MEMCPY
			memcpy(&hInfo, value.c_str(), value.size());
			#else
			if (!GetPrivateProfileStruct(sectionNames[i].c_str(), keys[j].c_str(), &hInfo, sizeof(HwndInfo), INIFilePath.c_str()))
				output(_T("Failed to read struct\n"));
			#endif // USE_MEMCPY

			layout.hwndInfos.push_back(hInfo);
		}

		customLayouts.push_back(layout);
	}

	return S_SUCCESS;
}

Status App::writeINIFile(const CustomLayout& layout)
{
	TCHAR index[10];
	_itot(customLayouts.size(), index, 10);
	tstring sectionName = tstring(_T("CustomLayout")) + tstring(index);

	if (!WritePrivateProfileStruct(sectionName.c_str(), _T("layoutIconPath"), (LPVOID)(layout.layoutIconPath), sizeof(layout.layoutIconPath) / sizeof(layout.layoutIconPath[0])/*MAX_PATH*/, INIFilePath.c_str()))
		output(_T("Failed to write HwndInfo to INI file\n"));

	for (int i = 0; i < layout.hwndInfos.size(); i++) {
		TCHAR keyIndex[10];
		_itot(i, keyIndex, 10);
		tstring keyName = tstring(_T("Wnd")) + tstring(keyIndex);

		if (!WritePrivateProfileStruct(sectionName.c_str(), keyName.c_str(), (LPVOID)&(layout.hwndInfos[i]), sizeof(HwndInfo), INIFilePath.c_str()))
			output(_T("Failed to write HwndInfo to INI file\n"));
	}

	customLayouts.push_back(layout);

	return S_SUCCESS;
}

Status App::loadCustomLayoutIcons()
{
	WIN32_FIND_DATA dirData;
	HWND vertLb			= vertListBoxCmp->getHwnd();
	HANDLE dir			= FindFirstFileEx(_T("Images\\CustomLayoutIcons\\*"), FindExInfoStandard, 
							&dirData, FindExSearchNameMatch, NULL, 0);

	if (dir == INVALID_HANDLE_VALUE)
		return S_UNDEFINED_ERROR;

	while (FindNextFile(dir, &dirData) != 0)
	{
		tstring fileName = dirData.cFileName;
		std::transform(fileName.begin(), fileName.end(), fileName.begin(), tolower);
		size_t bmpPos = fileName.rfind(_T(".bmp")); 
		
		// if not .bmp file
		if (bmpPos != fileName.length() - 4)
			continue;

		tstring imgPath = _T("Images\\CustomLayoutIcons\\") + fileName;
		HBITMAP bmp = (HBITMAP)::LoadImage(NULL, imgPath.c_str(), IMAGE_BITMAP, btnSize.cx, btnSize.cy,
											LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_VGACOLOR | LR_SHARED);
		gdiObjects.push_back(bmp);

		HWND custLytBtn = CreateWindowEx(WS_EX_TRANSPARENT, _T("BUTTON"), _T("Custom3"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_VCENTER | BS_BITMAP,
										DPIAwareComponent::scaleUnits(202), DPIAwareComponent::scaleUnits(195),
										btnSize.cx, btnSize.cy,
										vertLb, 0, hinstance, 0);

		SendMessage(custLytBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp);
		vertListBoxCmp->addChild(custLytBtn);

		registerEventLambda<App>(DispatchWindowComponent::translateMessage(custLytBtn, WM_LBUTTONUP),
			[&, imgPath](const IEventArgs& evtArgs)->Status {

			ShowWindow(vertListBoxCmp->getHwnd(), SW_HIDE);
			customBtnCallback(evtArgs, imgPath);
			return S_SUCCESS;
		});
	}

	return S_SUCCESS;
}

Status App::loadCustomLayouts()
{
	for (int i = 0; i < customLayouts.size(); i++) {
		Status custMsg = Status::registerState(_T("Custom Layout"));
		HWND custLytBtn = CreateWindowEx(WS_EX_TRANSPARENT, _T("BUTTON"),
			_T("Custom1"),
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_VCENTER | BS_BITMAP,
			0, 0,
			btnSize.cx, btnSize.cy,
			horizListBoxCmp->getHwnd(), (HMENU)custMsg.state, hinstance, 0);

		output(_T("img path: %s\n"), customLayouts[i].layoutIconPath);
		HBITMAP bmp = (HBITMAP)::LoadImage(NULL,
			customLayouts[i].layoutIconPath, IMAGE_BITMAP, btnSize.cx, btnSize.cy,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_VGACOLOR | LR_SHARED);

		gdiObjects.push_back(bmp);
		horizListBoxCmp->addChild(custLytBtn);
		SendMessage(custLytBtn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp);

		registerEventLambda<App>(custMsg,
			[i, this](const IEventArgs& evtArgs)->Status {

			return this->onLayoutBtnClick(evtArgs, i);
		});

		registerEventLambda<App>(DispatchWindowComponent::translateMessage(custLytBtn, WM_RBUTTONDOWN),
			[i, this](const IEventArgs& evtArgs)->Status {

			output(_T("Right btn\n"));

			// remove button
			// delete from customLayouts
			// delete from ini file

			return S_SUCCESS;
		});
	}

	return S_SUCCESS;
}