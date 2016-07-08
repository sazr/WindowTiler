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
#include "BorderWindowComponent.h"

// Class Property Implementation //


// Static Function Implementation //


// Function Implementation //
BorderWindowComponent::BorderWindowComponent(const std::weak_ptr<IApp>& app, RECT wndDim) 
	: Component(app), wndDim(wndDim), borderWndId(Status::registerState(_T("Border Window Id")).state)
{
	//addComponent<CustomWindowComponent>(app, _T("WT_BorderWindow"), &DispatchWindowComponent::customWndCallback, (HBRUSH)GetStockObject(GRAY_BRUSH));
	addComponent<DispatchWindowComponent>(app);
	registerEvents();
}

BorderWindowComponent::~BorderWindowComponent()
{

}

Status BorderWindowComponent::init(const IEventArgs& evtArgs)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);

	auto dispatchCmp = addComponent<DispatchWindowComponent>(app);
	
	DPIAwareComponent::scaleRect(wndDim);

	HWND borderWnd = CreateWindowEx(0, _T("Static"), _T(""), WS_VISIBLE | WS_CHILD, // | SS_OWNERDRAW,
		wndDim.left, wndDim.top, wndDim.right, wndDim.bottom, 
		args.hwnd, (HMENU)borderWndId, args.hinstance, 0);

	dispatchCmp->addDispatcher(borderWnd);

	return S_SUCCESS;
}

Status BorderWindowComponent::terminate(const IEventArgs& evtArgs)
{
	return S_SUCCESS;
}

Status BorderWindowComponent::registerEvents()
{
	registerEvent(WM_CREATE, &BorderWindowComponent::init);
	registerEvent(DispatchWindowComponent::translateMessage(borderWndId, WM_PAINT), &BorderWindowComponent::paint);
	return S_SUCCESS;
}

Status BorderWindowComponent::paint(const IEventArgs& evtArgs)
{
	const WinEventArgs& args = static_cast<const WinEventArgs&>(evtArgs);
	HDC hdc;
	PAINTSTRUCT ps;
	hdc = BeginPaint(args.hwnd, &ps);

	RECT mRect;
	GetClientRect(args.hwnd, &mRect);

	HBRUSH brush = CreateSolidBrush(RGB(50, 50, 50));
	HPEN pen = CreatePen(PS_SOLID, 5, RGB(191, 191, 191));
	SelectObject(hdc, pen);
	SelectObject(hdc, brush);
	Rectangle(hdc, 0, 0, mRect.right, mRect.bottom);
	DeleteObject(pen);
	DeleteObject(brush);

	EndPaint(args.hwnd, &ps);

	return S_SUCCESS;
}
