#include "QrDrawer.h"
#include "../defs.h"


QrDrawer::QrDrawer()
{
	_dirty = true;
	_fgColor = DEFAULT_FOREGROUND_COLOR;
	_bgColor = DEFAULT_BACKGROUND_COLOR;
	_shape = MODULE_SHAPE::DEFAULT_MODULE_SHAPE;
	_isInit = Init();
}
QrDrawer::~QrDrawer()
{
	DeInit();
}
void QrDrawer::CreateQrCode(std::tstring str, qrcodegen::QrCode::Ecc correctionLevel)
{
	LPSTR strUtf8 = TcharToCharBuff(str.c_str(), CP_UTF8);
	// encodeText accepts only UTF8
	_qrCode = std::make_shared<qrcodegen::QrCode>(qrcodegen::QrCode::encodeText(strUtf8, correctionLevel));
	free(strUtf8);
	_dcSize = (_qrCode->getSize() + 2) * _moduleSize;
	_dirty = true;
}
bool QrDrawer::DrawQrCode(HWND wnd)
{
	if (_dirty)
	{
		ResizeWindow(wnd);
		if (_memDC != NULL)
		{
			DeleteDC(_memDC);
		}
	}
	HDC clientDC = GetDC(wnd);
	_memDC = CreateCompatibleDC(clientDC);
	HBITMAP bmp = CreateCompatibleBitmap(clientDC, _dcSize, _dcSize);
	SelectObject(_memDC, bmp);
	int qrSize = _qrCode->getSize();
	RECT dcRect = { 0, 0, _dcSize, _dcSize };
	HBRUSH bgBrush = CreateSolidBrush(_bgColor);
	HBRUSH fgBrush = CreateSolidBrush(_fgColor);
	HPEN fgPen = CreatePen(PS_SOLID, 0, _fgColor);
	HGDIOBJ prevBrush, prevPen;
	if (DIRECT_REDRAW)
	{
		prevBrush = SelectObject(clientDC, fgBrush);
		prevPen = SelectObject(clientDC, fgPen);
	}
	else
	{
		SelectObject(_memDC, fgBrush);
		SelectObject(_memDC, fgPen);
	}
	/*Actual Draw*/
	FillRect(_memDC, &dcRect, bgBrush);
	for (int i = 0; i < qrSize; i++)
		for (int j = 0; j < qrSize; j++)
		{
			if (_qrCode->getModule(i, j))
			{
				if(DIRECT_REDRAW)
					DrawModule(clientDC, { i + 1, j + 1 }, fgBrush);
				else
					DrawModule(_memDC, { i + 1, j + 1 }, fgBrush);
			}
		}
	// Delete GDI objects
	DeleteObject(bgBrush);
	DeleteObject(fgBrush);
	DeleteObject(fgPen);
	//DeleteObject(bmp);
	ReleaseDC(wnd, clientDC);
	if(DIRECT_REDRAW)
	{
		SelectObject(clientDC, prevBrush);
		SelectObject(clientDC, prevPen);
	}

	_dirty = false;
	return true;
}
bool QrDrawer::RedrawQrCode(HWND wnd)
{
	if (_dirty)
	{
		DrawQrCode(wnd);
	}
	bool res = true;
	if(!DIRECT_REDRAW)
	{
		HDC clientDC = GetDC(wnd);
		res = BitBlt(clientDC, 0, 0, _dcSize, _dcSize, _memDC, 0, 0, SRCCOPY);
		ReleaseDC(wnd, clientDC);
	}
	return res;
}

void QrDrawer::SetModuleSize(int moduleSize)
{
	_moduleSize = moduleSize;
	_dcSize = (_qrCode->getSize() + 2) * _moduleSize;
	_dirty = true;
}
void QrDrawer::SetFgColor(COLORREF fgColor)
{
	_fgColor = fgColor;
	_dirty = true;
}
void QrDrawer::SetBgColor(COLORREF bgColor)
{
	_bgColor = bgColor;
	_dirty = true;
}
void QrDrawer::SetShape(MODULE_SHAPE shape)
{
	_shape = shape;
	_dirty = true;
}
int QrDrawer::GetWindowSize()
{
	return _dcSize;
}
bool QrDrawer::Init()
{
	return true;
}
void QrDrawer::DeInit()
{
	if (_isInit)
	{
		DeleteDC(_memDC);
	}
}
void QrDrawer::ResizeWindow(HWND wnd)
{
	SIZE screenSize = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	POINT newWndPos = { (screenSize.cx - _dcSize) / 2, (screenSize.cy - _dcSize) / 2 };
	RECT clientRect = { newWndPos.x, newWndPos.y, newWndPos.x + _dcSize, newWndPos.y + _dcSize };
	AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, false);
	MoveWindow(wnd, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, true);
}
void QrDrawer::DrawModule(HDC dc, POINT pos, HBRUSH fgBrush)
{
	RECT moduleRect = { pos.x * _moduleSize, pos.y * _moduleSize, (pos.x + 1) * _moduleSize, (pos.y + 1) * _moduleSize };
	switch (_shape)
	{
	case MODULE_SHAPE::ROUND:
		{
			int expander = 0;
			if(_moduleSize > 10)
			{
				expander = _moduleSize * 0.1;
			}
			// expand ellipses for better connection
			Ellipse(dc, moduleRect.left - expander, moduleRect.top - expander, moduleRect.right + expander, moduleRect.bottom + expander);
		}
		break;
	case MODULE_SHAPE::SQUARE:
		FillRect(dc, &moduleRect, fgBrush);
		break;
	}
}