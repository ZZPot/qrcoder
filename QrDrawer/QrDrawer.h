#pragma once
#include "QrCode.hpp"
#include <memory>
#include <windows.h>
#include "common.h"
enum class MODULE_SHAPE
{
	SQUARE,
	ROUND
};

class QrDrawer
{
public:
	QrDrawer();
	virtual ~QrDrawer();
	void CreateQrCode(std::tstring str, qrcodegen::QrCode::Ecc correctionLevel = qrcodegen::QrCode::Ecc::MEDIUM);
	bool DrawQrCode(HWND wnd);
	bool RedrawQrCode(HWND wnd);

	void SetModuleSize(int moduleSize);
	void SetFgColor(COLORREF fgColor);
	void SetBgColor(COLORREF fgColor);
	void SetShape(MODULE_SHAPE shape);
	bool SaveToFile(bool autoName = true);
	int GetWindowSize();
protected:
	bool Init();
	void DeInit();
	void DrawModule(HDC dc, POINT pos, HBRUSH fgBrush);
	void ResizeWindow(HWND wnd);
protected:
	bool _isInit;
	bool _dirty;
	std::shared_ptr<qrcodegen::QrCode> _qrCode;
	COLORREF _fgColor;
	COLORREF _bgColor;
	int _moduleSize;
	MODULE_SHAPE _shape;
	int _dcSize;
	HDC _memDC;
	std::tstring _str;
};