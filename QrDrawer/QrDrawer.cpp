#include "QrDrawer.h"
#include "../defs.h"
#include "png.h"

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
	_str = str;
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
	DeleteObject(bmp);
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
bool QrDrawer::SaveToFile(bool autoName)
{
	// http://pididu.com/wordpress/blog/save-client-area-of-a-window-to-png-file/
	if(_dirty)
		return false;
	long code;  // return code
	FILE* fp = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep row = NULL;
	HBITMAP hBitmap = NULL;
	BITMAPINFO bi = { sizeof(BITMAPINFOHEADER) };

	// get dimensions of client area to save
	// note that this is generally smaller than the whole window

	// Open file for writing (binary mode)
	std::tstring proposedName = commonFunc::MakeSafeFilename(_str) + _T(".png");
	std::tstring pngFileName = autoName ? commonFunc::GetDesktopPath() + _T('\\') + proposedName : commonFunc::SaveFile(NULL, proposedName.c_str(), _T("Save QR-code"), _T("PNG\0*.png\0All files\0*\0\0"));
	code = _tfopen_s(&fp, pngFileName.c_str(), TEXT("wb"));
	if (code != NO_ERROR) 
	{
		goto finalise;
	}

	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) 
	{
		code = ERROR_NOT_ENOUGH_MEMORY;
		goto finalise;
	}

	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) 
	{
		code = ERROR_NOT_ENOUGH_MEMORY;
		goto finalise;
	}

	// Setup Exception handling
	// Any subsequent errors in png_xxxx calls will cause this
	// to execute
	if (setjmp(png_jmpbuf(png_ptr))) 
	{
		code = ERROR_INVALID_DATA;  // best match standard error message
		goto finalise;
	}

	png_init_io(png_ptr, fp);

	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, _dcSize, _dcSize,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	// Set title
	//if (title != NULL) {
	//	png_text title_text;
	//	title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	//	title_text.key = "Title";
	//	title_text.text = title;
	//	png_set_text(png_ptr, info_ptr, &title_text, 1);
	//}

	png_write_info(png_ptr, info_ptr);

	// create a compatible bitmap to store the pixels
	//hBitmap = CreateCompatibleBitmap(_memDC, _dcSize, _dcSize);
	// Use current _memDC
	hBitmap = (HBITMAP) GetCurrentObject(_memDC, OBJ_BITMAP);

	
	// First call with buffer pointer null just retrieves
	// bitmap information into bi structure
	bi.bmiHeader.biBitCount = 0;  // must set to 0 to retrieve infoheader
	GetDIBits(_memDC, hBitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);
	// Modify to the format we want to retrieve:
	// uncompressed, 3-byte RGB format
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;

	// Allocate memory for one row of the picture.
	// Format is 3 bytes (24 bits) per pixel.
	// The buffer is filled by GetDIBits().
	// The buffer is output to a file by libpng calls.
	// For sake of GetDIBits(), must round up to whole number of DWORDs (the "stride")
	row = (png_bytep)malloc((_dcSize * bi.bmiHeader.biBitCount / 8 * sizeof(png_byte) + 3) & ~3);
	if (row == NULL) 
	{
		code = ERROR_NOT_ENOUGH_MEMORY;
		goto finalise;
	}

	// Write image data one row at a time
	// Microsoft image is bottom-up, png is top-down, so must go
	// in reverse order for y.
	// Each Microsoft pixel is BGR, must swap bytes for RGB orderof png
	int x, y;
	UCHAR temp;
	for (y = _dcSize - 1; y >= 0; y--) 
	{
		GetDIBits(_memDC, hBitmap, y, 1, row, &bi, DIB_RGB_COLORS);
		for (x = 0; x < _dcSize; x++) 
		{
			temp = row[x * 3];
			row[x * 3] = row[x * 3 + 2];
			//row[x*3+1] = row[x*3+1];  // middle byte unchanged
			row[x * 3 + 2] = temp;
		}
		png_write_row(png_ptr, row);
	}

	// End write
	png_write_end(png_ptr, NULL);

finalise:
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL) free(row);	
	if (hBitmap != NULL) DeleteObject(hBitmap);

	return true;
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