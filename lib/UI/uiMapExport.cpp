#include "../wpch.hpp"
#include <Es/Common/win.h>

#include "../landscape.hpp"
#include "../Shape/mapTypes.hpp"

#include "uiMap.hpp"
#include "../AI/operMap.hpp"

#include <El/ParamFile/paramFile.hpp>

#include <El/Debugging/debugTrap.hpp>

#define DRAW_BITMAPS		0
#define FLIP_VERTICAL		1

#define DRAW_TEXTS			0

static float Coef = 1.0f;
#define TerrainSize (TerrainRange * TerrainGrid)

#if _VBS3 //maps were upside down?!
  #define CONVY(y) y
#else
  #define CONVY(y) (toInt(TerrainSize - (y)))
#endif

#if defined(_WIN32) 

static COLORREF colorSea = RGB(199, 230, 252);
static HBRUSH brushSea;
static COLORREF colorLand = RGB(255, 255, 255);
static HBRUSH brushLand;

static COLORREF colorForest = RGB(204, 230, 153);
static HBRUSH brushForest;
static COLORREF colorRocks = RGB(214, 199, 181);
static HBRUSH brushRocks;
static COLORREF colorForestBorder = RGB(102, 204, 0);
static HPEN penRocks;
static COLORREF colorRocksBorder = RGB(214, 199, 181);
static HPEN penForest;

static COLORREF colorTracks = RGB(122, 92, 71);
static HPEN penTracks;
static COLORREF colorRoads = RGB(89, 51, 26);
static HPEN penRoads;
static COLORREF colorMainRoads = RGB(0, 0, 0);
static HPEN penMainRoads;
static COLORREF colorTracksFill = RGB(255, 255, 255);
static HBRUSH brushTracks;
static COLORREF colorRoadsFill = RGB(255, 235, 189);
static HBRUSH brushRoads;
static COLORREF colorMainRoadsFill = RGB(241, 73, 80);
static HBRUSH brushMainRoads;

static COLORREF colorCountlines = RGB(209, 186, 148);
static HPEN penCountlines;
static COLORREF colorCountlinesMain = RGB(166, 115, 69);
static HPEN penCountlinesMain;
static COLORREF colorCountlinesWater = RGB(128, 195, 255);
static HPEN penCountlinesWater;
static COLORREF colorCountlinesWaterMain = RGB(0, 135, 255);
static HPEN penCountlinesWaterMain;

static COLORREF colorGrid = RGB(112, 112, 83);
static HPEN penGrid;

static COLORREF colorSpot = RGB(0, 0, 0);
static HPEN penSpot;

MAP_TYPES_ICONS_1(MAP_TYPE_INFO_DECL_STATIC)
MAP_TYPES_ICONS_2(MAP_TYPE_INFO_DECL_STATIC)
#if DRAW_BITMAPS
MAP_TYPES_ICONS_1(MAP_TYPE_MASK_DECL_STATIC)
MAP_TYPES_ICONS_2(MAP_TYPE_MASK_DECL_STATIC)
MAP_TYPES_ICONS_1(MAP_TYPE_BITMAP_DECL_STATIC)
MAP_TYPES_ICONS_2(MAP_TYPE_BITMAP_DECL_STATIC)

#define Compose(r,g,b) \
	((r)>>(8-sRBits)<<sRShift)| \
	((g)>>(8-sGBits)<<sGShift)| \
	((b)>>(8-sBBits)<<sBShift)

static void CreateBitmap(HDC hDC, MapTypeInfo &info, HBITMAP &bmp, HBITMAP &mask)
{
	int sRShift=10;
	int sGShift=5;
	int sBShift=0;
	int sRBits=5;
	int sGBits=5;
	int sBBits=5;

	WORD white = Compose(255, 255, 255);
	WORD black = Compose(0, 0, 0);
	WORD color = Compose
	(
		info.color.R8(), info.color.G8(), info.color.B8()
	);

	MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(info.icon, 0, 0);
	int size = toInt(info.size);
	float invSize = 1.0 / size;
	
	WORD *dataBmp = new WORD[Square(size)];
	WORD *dataMask = new WORD[Square(size)];

	for (int y=0; y<size; y++)
	{
		float v = (y + 0.5) * invSize;
		for (int x=0; x<size; x++)
		{
			float u = (x + 0.5) * invSize;
			Color src = info.icon->GetPixel(0, u, v);
			if (src.A() >= 0.5)
			{
				dataMask[y * size + x] = white;
				dataBmp[y * size + x] = color;
			}
			else
			{
				dataMask[y * size + x] = black;
				dataBmp[y * size + x] = white;
			}
		}
	}

	struct Info: public BITMAPINFOHEADER
	{
		DWORD masks[3]; 
	} bmInfo;
	bmInfo.biSize=sizeof(BITMAPINFOHEADER);
	bmInfo.biWidth=size;
	bmInfo.biHeight=size;
	bmInfo.biPlanes=1;
	bmInfo.biBitCount=16;
	bmInfo.biCompression=BI_RGB;
	bmInfo.biSizeImage=size * size * sizeof(WORD);
	bmInfo.biXPelsPerMeter=5000;
	bmInfo.biYPelsPerMeter=5000;
	bmInfo.biClrUsed=0;
	bmInfo.biClrImportant=0;
	bmInfo.masks[0]=0x1f<<10;
	bmInfo.masks[1]=0x1f<<5;
	bmInfo.masks[2]=0x1f<<0;
	
	// create bitmap object from prepared data
	bmp = CreateDIBitmap
	(
		hDC, &bmInfo, CBM_INIT, dataBmp, (BITMAPINFO *)&bmInfo, DIB_RGB_COLORS
	);
	if (bmp == 0)
	{
		LogF("Cannot create bitmap");
	}

	mask = CreateDIBitmap
	(
		hDC, &bmInfo, CBM_INIT, dataMask, (BITMAPINFO *)&bmInfo, DIB_RGB_COLORS
	);
	if (mask == 0)
	{
		LogF("Cannot create mask");
	}

	delete [] dataBmp;
	delete [] dataMask;
}

static void DeleteBitmap(HBITMAP &bmp, HBITMAP &mask)
{
	if (bmp) DeleteObject(bmp);
	if (mask) DeleteObject(mask);
}

static void DrawSign(HDC hDC, HBITMAP bitmap, HBITMAP mask, Vector3 pos, int size)
{
	if (bitmap == NULL) return;
	float x = Coef * pos.X();
	float y = Coef * pos.Z();
	int newSize = size;

	HDC hDCBmp = CreateCompatibleDC(hDC);
	if (hDCBmp)
	{
		HGDIOBJ bitmapOld = SelectObject(hDCBmp, mask);
		StretchBlt
		(
			hDC, toInt(x - newSize / 2), CONVY(y - newSize / 2), newSize, -newSize,
			hDCBmp, 0, 0, size, size,
			SRCPAINT
		);
		SelectObject(hDCBmp, bitmap);
		StretchBlt
		(
			hDC, toInt(x - newSize / 2), CONVY(y - newSize / 2), newSize, -newSize,
			hDCBmp, 0, 0, size, size,
			SRCAND
		);
		SelectObject(hDCBmp, bitmapOld);
		DeleteDC(hDCBmp);
	}
}

#endif

static float GetHeight(int x, int z)
{
	saturate(x, 1, TerrainRange - 2);
	saturate(z, 1, TerrainRange - 2);
	return GLOB_LAND->GetHeight(z, x);
}

static void DrawSea(HDC hDC, int x, int z, int xStep, int zStep)
{
	RECT rect;
	rect.left = toInt(Coef * x * TerrainGrid);
	rect.right = toInt(Coef * (x + xStep) * TerrainGrid);
	rect.top = toInt(Coef * z * TerrainGrid);
	rect.bottom = toInt(Coef * (z + zStep) * TerrainGrid);

	float hTL = GetHeight(x, z);
	float hTR = GetHeight(x + xStep, z);
	float hBL = GetHeight(x, z + zStep);
	float hBR = GetHeight(x + xStep, z + zStep);

	if (hTL <= 0 && hTR <= 0 && hBL <= 0 && hBR <= 0)
  {
    rect.top = CONVY(rect.top);
    rect.bottom = CONVY(rect.bottom);
    FillRect(hDC, &rect, brushSea);
  }
	else if (hTL >= 0 && hTR >= 0 && hBL >= 0 && hBR >= 0)
  {
    rect.top = CONVY(rect.top);
    rect.bottom = CONVY(rect.bottom);
    FillRect(hDC, &rect, brushLand);
  }
	else
	{
		POINT ptSea[5];
		POINT ptLand[5];
		int nSea = 0;
		int nLand = 0;
		if (hTL <= 0)
		{
			ptSea[nSea].x = rect.left;
			ptSea[nSea].y = CONVY(rect.top);
			nSea++;
			if (hTR > 0) goto TLTR;
		}
		else
		{
			ptLand[nLand].x = rect.left;
			ptLand[nLand].y = CONVY(rect.top);
			nLand++;
			if (hTR <= 0)
			{
TLTR:
				int dx = -toInt(Coef * xStep * TerrainGrid * hTL / (hTR - hTL));
				ptLand[nLand].x = rect.left + dx;
				ptLand[nLand].y = CONVY(rect.top);
				nLand++;
				ptSea[nSea].x = rect.left + dx;
				ptSea[nSea].y = CONVY(rect.top);
				nSea++;
			}
		}
		if (hTR <= 0)
		{
			ptSea[nSea].x = rect.right;
			ptSea[nSea].y = CONVY(rect.top);
			nSea++;
			if (hBR > 0) goto TRBR;
		}
		else
		{
			ptLand[nLand].x = rect.right;
			ptLand[nLand].y = CONVY(rect.top);
			nLand++;
			if (hBR <= 0)
			{
TRBR:
				int dz = -toInt(Coef * zStep * TerrainGrid * hTR / (hBR - hTR));
				ptLand[nLand].x = rect.right;
				ptLand[nLand].y = CONVY(rect.top + dz);
				nLand++;
				ptSea[nSea].x = rect.right;
				ptSea[nSea].y = CONVY(rect.top + dz);
				nSea++;
			}
		}
		if (hBR <= 0)
		{
			ptSea[nSea].x = rect.right;
			ptSea[nSea].y = CONVY(rect.bottom);
			nSea++;
			if (hTL > 0) goto BRTL;
		}
		else
		{
			ptLand[nLand].x = rect.right;
			ptLand[nLand].y = CONVY(rect.bottom);
			nLand++;
			if (hTL <= 0)
			{
BRTL:
				int dx = -toInt(Coef * xStep * TerrainGrid * hBR / (hTL - hBR));
        int dz = -toInt(Coef * zStep * TerrainGrid * hBR / (hTL - hBR));
				ptLand[nLand].x = rect.right - dx;
				ptLand[nLand].y = CONVY(rect.bottom - dz);
				nLand++;
				ptSea[nSea].x = rect.right - dx;
				ptSea[nSea].y = CONVY(rect.bottom - dz);
				nSea++;
			}
		}
		if (nSea >= 2)
		{
			ptSea[nSea].x = ptSea[0].x;
			ptSea[nSea].y = ptSea[0].y;
			nSea++;
			SelectObject(hDC, brushSea);
			Polygon(hDC, ptSea, nSea);
		}
		if (nLand >= 2)
		{
			ptLand[nLand].x = ptLand[0].x;
			ptLand[nLand].y = ptLand[0].y;
			nLand++;
			SelectObject(hDC, brushLand);
			Polygon(hDC, ptLand, nLand);
		}

		nSea = 0;
		nLand = 0;
		if (hTL <= 0)
		{
			ptSea[nSea].x = rect.left;
			ptSea[nSea].y = CONVY(rect.top);
			nSea++;
			if (hBR > 0) goto TLBR;
		}
		else
		{
			ptLand[nLand].x = rect.left;
			ptLand[nLand].y = CONVY(rect.top);
			nLand++;
			if (hBR <= 0)
			{
TLBR:
				int dx = -toInt(Coef * xStep * TerrainGrid * hTL / (hBR - hTL));
        int dz = -toInt(Coef * zStep * TerrainGrid * hTL / (hBR - hTL));
				ptLand[nLand].x = rect.left + dx;
				ptLand[nLand].y = CONVY(rect.top + dz);
				nLand++;
				ptSea[nSea].x = rect.left + dx;
				ptSea[nSea].y = CONVY(rect.top + dz);
				nSea++;
			}
		}
		if (hBR <= 0)
		{
			ptSea[nSea].x = rect.right;
			ptSea[nSea].y = CONVY(rect.bottom);
			nSea++;
			if (hBL > 0) goto BRBL;
		}
		else
		{
			ptLand[nLand].x = rect.right;
			ptLand[nLand].y = CONVY(rect.bottom);
			nLand++;
			if (hBL <= 0)
			{
BRBL:
				int dx = -toInt(Coef * xStep * TerrainGrid * hBR / (hBL - hBR));
				ptLand[nLand].x = rect.right - dx;
				ptLand[nLand].y = CONVY(rect.bottom);
				nLand++;
				ptSea[nSea].x = rect.right - dx;
				ptSea[nSea].y = CONVY(rect.bottom);
				nSea++;
			}
		}
		if (hBL <= 0)
		{
			ptSea[nSea].x = rect.left;
			ptSea[nSea].y = CONVY(rect.bottom);
			nSea++;
			if (hTL > 0) goto BLTL;
		}
		else
		{
			ptLand[nLand].x = rect.left;
			ptLand[nLand].y = CONVY(rect.bottom);
			nLand++;
			if (hTL <= 0)
			{
BLTL:
				int dz = -toInt(Coef * zStep * TerrainGrid * hBL / (hTL - hBL));
				ptLand[nLand].x = rect.left;
				ptLand[nLand].y = CONVY(rect.bottom - dz);
				nLand++;
				ptSea[nSea].x = rect.left;
				ptSea[nSea].y = CONVY(rect.bottom - dz);
				nSea++;
			}
		}
		if (nSea >= 2)
		{
			ptSea[nSea].x = ptSea[0].x;
			ptSea[nSea].y = ptSea[0].y;
			nSea++;
			SelectObject(hDC, brushSea);
			Polygon(hDC, ptSea, nSea);
		}
		if (nLand >= 2)
		{
			ptLand[nLand].x = ptLand[0].x;
			ptLand[nLand].y = ptLand[0].y;
			nLand++;
			SelectObject(hDC, brushLand);
			Polygon(hDC, ptLand, nLand);
		}
	}
}

inline static void DrawForestPoly3(HDC hDC, float x1, float y1, float x2, float y2, float x3, float y3)
{
  const int n = 3;
  POINT vs[n];
  vs[0].x = toInt(x3);
  vs[0].y = CONVY(y3);
  vs[1].x = toInt(x2);
  vs[1].y = CONVY(y2);
  vs[2].x = toInt(x1);
  vs[2].y = CONVY(y1);
  Polygon(hDC, vs, n);
}

inline static void DrawForestPoly4(HDC hDC, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
  const int n = 4;
  POINT vs[n];
  vs[0].x = toInt(x4);
  vs[0].y = CONVY(y4);
  vs[1].x = toInt(x3);
  vs[1].y = CONVY(y3);
  vs[2].x = toInt(x2);
  vs[2].y = CONVY(y2);
  vs[3].x = toInt(x1);
  vs[3].y = CONVY(y1);
  Polygon(hDC, vs, n);
}

inline static void DrawForestPoly5(HDC hDC, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float x5, float y5)
{
  const int n = 5;
  POINT vs[n];
  vs[0].x = toInt(x5);
  vs[0].y = CONVY(y5);
  vs[1].x = toInt(x4);
  vs[1].y = CONVY(y4);
  vs[2].x = toInt(x3);
  vs[2].y = CONVY(y3);
  vs[3].x = toInt(x2);
  vs[3].y = CONVY(y2);
  vs[4].x = toInt(x1);
  vs[4].y = CONVY(y1);
  Polygon(hDC, vs, n);
}

inline static void DrawForestLine(HDC hDC, float x1, float y1, float x2, float y2)
{
  MoveToEx(hDC, toInt(x1), CONVY(y1), NULL);
  LineTo(hDC, toInt(x2), CONVY(y2));
}

static void DrawForests(HDC hDC, int i, int j, bool drawLines, bool rocks)
{
	RECT rect;
	rect.left = toInt(Coef * i * LandGrid);
	rect.right = toInt(Coef * (i + 1) * LandGrid);
	rect.top = toInt(Coef * j * LandGrid);
	rect.bottom = toInt(Coef * (j + 1) * LandGrid);

  float x = rect.left;
  float y = rect.top;
  float w = rect.right - rect.left;
  float h = rect.bottom - rect.top;

  const MapObjectListUsed &listUsed = GLandscape->UseMapObjects(i, j);
  const MapObjectListFull *list = listUsed.GetList();

	if (list) for (int o=0; o<list->Size(); o++)
	{
    MapObject *obj = list->Get(o);
    if (obj == NULL) continue;

    if (rocks)
    {
      if (obj->GetType() != MapRocks) continue;
    }
    else
    {
      if (obj->GetType() != MapForest) continue;
    }

    MapObjectForest *forest = static_cast<MapObjectForest *>(obj);

    int row = forest->_row;
    int col = forest->_col;

    float wGrid = w / forest->_cols;
    float hGrid = h / forest->_rows;

    float tl = forest->_tl;
    float tr = forest->_tr;
    float bl = forest->_bl;
    float br = forest->_br;
    float limit = 1.0f;

    int bits = 0;
    if (tl > limit) bits |= 8;
    if (tr > limit) bits |= 4;
    if (bl > limit) bits |= 2;
    if (br > limit) bits |= 1;
    switch (bits)
    {
    case 0:
      // nothing to draw - no forest
      // Fail("No forest");
      break;
    case 1:
      {
        // br
        float coef1 = (limit - bl) / (br - bl); 
        float coef2 = (limit - tr) / (br - tr);
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + (row + 1) * hGrid;
        float x2 = x + (col + 1) * wGrid;
        float y2 = y + (row + coef2) * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x2, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 2:
      {
        // bl
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (bl - limit) / (bl - br); 
        float x1 = x + col * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + (row + 1) * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x1, y2);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 3:
      {
        // bl, br
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (limit - tr) / (br - tr); 
        float x1 = x + col * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + (col + 1) * wGrid;
        float y2 = y + (row + coef2) * hGrid;
        float y3 = y + (row + 1) * hGrid;

        if (!drawLines) DrawForestPoly4(hDC, x1, y1, x2, y2, x2, y3, x1, y3);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 4:
      {
        // tr
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (limit - tl) / (tr - tl); 
        float x1 = x + (col + 1) * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + row * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x1, y2);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 5:
      {
        // tr, br
        float coef1 = (limit - bl) / (br - bl);
        float coef2 = (limit - tl) / (tr - tl); 
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + (row + 1) * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + row * hGrid;
        float x3 = x + (col + 1) * wGrid;

        if (!drawLines) DrawForestPoly4(hDC, x1, y1, x2, y2, x3, y2, x3, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 6:
      {
        // tr
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (limit - tl) / (tr - tl); 
        float x1 = x + (col + 1) * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + row * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x1, y2);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      {
        // bl
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (bl - limit) / (bl - br); 
        float x1 = x + col * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + (row + 1) * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x1, y2);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 7:
      {
        // tr, bl, br
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (limit - tl) / (tr - tl); 
        float x1 = x + col * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + row * hGrid;
        float x3 = x + (col + 1) * wGrid;
        float y3 = y + (row + 1) * hGrid;

        if (!drawLines) DrawForestPoly5(hDC, x1, y1, x2, y2, x3, y2, x3, y3, x1, y3);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 8:
      {
        // tl
        float coef1 = (tl - limit) / (tl - tr); 
        float coef2 = (tl - limit) / (tl - bl);
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + row * hGrid;
        float x2 = x + col * wGrid;
        float y2 = y + (row + coef2) * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x2, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 9:
      {
        // tl
        float coef1 = (tl - limit) / (tl - tr); 
        float coef2 = (tl - limit) / (tl - bl);
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + row * hGrid;
        float x2 = x + col * wGrid;
        float y2 = y + (row + coef2) * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x2, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      {
        // br
        float coef1 = (limit - bl) / (br - bl); 
        float coef2 = (limit - tr) / (br - tr);
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + (row + 1) * hGrid;
        float x2 = x + (col + 1) * wGrid;
        float y2 = y + (row + coef2) * hGrid;

        if (!drawLines) DrawForestPoly3(hDC, x1, y1, x2, y2, x2, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 10:
      {
        // tl, bl
        float coef1 = (tl - limit) / (tl - tr);
        float coef2 = (bl - limit) / (bl - br); 
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + row * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + (row + 1) * hGrid;
        float x3 = x + col * wGrid;

        if (!drawLines) DrawForestPoly4(hDC, x1, y1, x2, y2, x3, y2, x3, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 11:
      {
        // tl, bl, br
        float coef1 = (tl - limit) / (tl - tr);
        float coef2 = (limit - tr) / (br - tr);
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + row * hGrid;
        float x2 = x + (col + 1) * wGrid;
        float y2 = y + (row + coef2) * hGrid;
        float x3 = x + col * wGrid;
        float y3 = y + (row + 1) * hGrid;

        if (!drawLines) DrawForestPoly5(hDC, x1, y1, x2, y2, x2, y3, x3, y3, x3, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 12:
      {
        // tl, tr
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (tl - limit) / (tl - bl); 
        float x1 = x + (col + 1) * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + col * wGrid;
        float y2 = y + (row + coef2) * hGrid;
        float y3 = y + row * hGrid;

        if (!drawLines) DrawForestPoly4(hDC, x1, y1, x2, y2, x2, y3, x1, y3);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 13:
      {
        // tl, tr, br
        float coef1 = (limit - bl) / (br - bl);
        float coef2 = (tl - limit) / (tl - bl); 
        float x1 = x + (col + coef1) * wGrid;
        float y1 = y + (row + 1) * hGrid;
        float x2 = x + col * wGrid;
        float y2 = y + (row + coef2) * hGrid;
        float x3 = x + (col + 1) * wGrid;
        float y3 = y + row * hGrid;

        if (!drawLines) DrawForestPoly5(hDC, x1, y1, x2, y2, x2, y3, x3, y3, x3, y1);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 14:
      {
        // tl, tr, bl
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (bl - limit) / (bl - br); 
        float x1 = x + (col + 1) * wGrid;
        float y1 = y + (row + coef1) * hGrid;
        float x2 = x + (col + coef2) * wGrid;
        float y2 = y + (row + 1) * hGrid;
        float x3 = x + col * wGrid;
        float y3 = y + row * hGrid;

        if (!drawLines) DrawForestPoly5(hDC, x1, y1, x2, y2, x3, y2, x3, y3, x1, y3);
        else DrawForestLine(hDC, x1, y1, x2, y2);
      }
      break;
    case 15:
      {
        // full forest
        float x1 = x + col * wGrid;
        float y1 = y + row * hGrid;
        float x2 = x + (col + 1) * wGrid;
        float y2 = y + (row + 1) * hGrid;

        if (!drawLines) DrawForestPoly4(hDC, x1, y1, x2, y1, x2, y2, x1, y2);
      }
      break;
    }
	}
}

static void DrawRoads(HDC hDC, int x, int z)
{
  const MapObjectListUsed &listUsed = GLandscape->UseMapObjects(x, z);
  const MapObjectListFull *list = listUsed.GetList();
  if (list) for (int o=0; o<list->Size(); o++)
  {
    MapObject *obj = list->Get(o);
    if (!obj) continue;
    HPEN pen = NULL;
    HBRUSH brush = NULL;
    switch (obj->GetType())
    {
    case MapTrack:
      pen = penTracks;
      brush = brushTracks;
      break;
    case MapRoad:
      pen = penRoads;
      brush = brushRoads;
      break;
    case MapMainRoad:
      pen = penMainRoads;
      brush = brushMainRoads;
      break;
    default:
      // not a road
      continue;
    }

    Vector3 ptTL = obj->GetTLPosition();
    Vector3 ptTR = obj->GetTRPosition();
    Vector3 ptBL = obj->GetBLPosition();
    Vector3 ptBR = obj->GetBRPosition();

    const int n = 4;
    POINT vs[n];
    vs[0].x = toInt(Coef * ptTL.X());
    vs[0].y = CONVY(Coef * ptTL.Z());
    vs[1].x = toInt(Coef * ptBL.X());
    vs[1].y = CONVY(Coef * ptBL.Z());
    vs[2].x = toInt(Coef * ptBR.X());
    vs[2].y = CONVY(Coef * ptBR.Z());
    vs[3].x = toInt(Coef * ptTR.X());
    vs[3].y = CONVY(Coef * ptTR.Z());
    SelectObject(hDC, brush);
    SelectObject(hDC, GetStockObject(NULL_PEN));
    Polygon(hDC, vs, n);

    SelectObject(hDC, pen);
    MoveToEx(hDC, toInt(Coef * ptTL.X()), CONVY(Coef * ptTL.Z()), NULL);
    LineTo(hDC, toInt(Coef * ptBL.X()), CONVY(Coef * ptBL.Z()));
    MoveToEx(hDC, toInt(Coef * ptTR.X()), CONVY(Coef * ptTR.Z()), NULL);
    LineTo(hDC, toInt(Coef * ptBR.X()), CONVY(Coef * ptBR.Z()));
  }
}

static void DrawBuilding(HDC hDC, MapObject *obj)
{
  Vector3 mapTL = obj->GetTLPosition();
  Vector3 mapTR = obj->GetTRPosition();
  Vector3 mapBL = obj->GetBLPosition();
  Vector3 mapBR = obj->GetBRPosition();

  const int n = 4;
  POINT vs[n];
  // 0
  vs[0].x = toInt(Coef * mapTL.X());
  vs[0].y = CONVY(Coef * mapTL.Z());
  // 1
  vs[1].x = toInt(Coef * mapBL.X());
  vs[1].y = CONVY(Coef * mapBL.Z());
  // 2
  vs[2].x = toInt(Coef * mapBR.X());
  vs[2].y = CONVY(Coef * mapBR.Z());
  // 3
  vs[3].x = toInt(Coef * mapTR.X());
  vs[3].y = CONVY(Coef * mapTR.Z());

  SelectObject(hDC, GetStockObject(NULL_PEN));
  PackedColor color = obj->GetColor();
  HBRUSH brush = CreateSolidBrush(RGB(color.R8(), color.G8(), color.B8()));
  HGDIOBJ brushOld = SelectObject(hDC, brush);
  Polygon(hDC, vs, n);
  SelectObject(hDC, brushOld);
  DeleteObject(brush);
}

static void DrawObjects(HDC hDC, int x, int z)
{
  const MapObjectListUsed &listUsed = GLandscape->UseMapObjects(x, z);
  const MapObjectListFull *list = listUsed.GetList();
	if (list) for (int o=0; o<list->Size(); o++)
	{
		MapObject *obj = list->Get(o);
    if (!obj) continue;
    Vector3 pos = obj->GetPosition(   VZero);
    if (pos.X() < 0 || pos.Z() < 0 || pos.X() >= TerrainSize  || pos.Z() >= TerrainSize)
    {
      LogF("Object %d out of map", (int)(obj->GetId()));
      continue;
    }
		switch (obj->GetType())
		{
#if DRAW_BITMAPS
    MAP_TYPES_ICONS_1(MAP_TYPE_EXPORT_DRAW_SIGN)
    MAP_TYPES_ICONS_2(MAP_TYPE_EXPORT_DRAW_SIGN_AND_BUILDING)
#else
    case MapTree:
      {
        int radius = toIntFloor(0.5f * infoTree.size);
        Vector3 pos = obj->GetPosition(VZero);
        int xs = toInt(Coef * pos.X() - radius);
        int ys = CONVY(Coef * pos.Z() - radius);
        int xe = toInt(Coef * pos.X() + radius);
        int ye = CONVY(Coef * pos.Z() + radius);
        SelectObject(hDC, penForest);
        SelectObject(hDC, GetStockObject(NULL_BRUSH));
        Ellipse(hDC, xs, ys, xe, ye);
      }
      break;
    // MAP_TYPES_ICONS_1(MAP_TYPE_CASE)
    MAP_TYPES_ICONS_2(MAP_TYPE_CASE)
      // continue with DrawBuilding
#endif
		case MapBuilding:
		case MapHouse:
		case MapFence:
		case MapWall:
      DrawBuilding(hDC, obj);
			break;
    case MapPowerLines:
      {
        Vector3 mapTL = obj->GetTLPosition();
        Vector3 mapTR = obj->GetTRPosition();

        // draw line from
        int xs = toInt(Coef * mapTL.X());
        int ys = CONVY(Coef * mapTL.Z());
        int xe = toInt(Coef * mapTR.X());
        int ye = CONVY(Coef * mapTR.Z());
        SelectObject(hDC, GetStockObject(BLACK_PEN));
        MoveToEx(hDC, xs, ys, NULL);
        LineTo(hDC, xe, ye);
      }
      break;
    case MapRailWay:
      {/*
        Vector3 mapTL = obj->GetTLPosition();
        Vector3 mapTR = obj->GetTRPosition();

        // draw line from
        int xs = toInt(Coef * mapTL.X());
        int ys = CONVY(Coef * mapTL.Z());
        int xe = toInt(Coef * mapTR.X());
        int ye = CONVY(Coef * mapTR.Z());
        SelectObject(hDC, GetStockObject(BLACK_PEN));
        MoveToEx(hDC, xs, ys, NULL);
        LineTo(hDC, xe, ye);
        */
      }
      break;
		}
	}
}

static void DrawLines(HDC hDC, POINT *pt, float *height, float step, float stepMain, float minLevel, float maxLevel, HPEN pen, HPEN penMain)
{
	float invStep = 1.0 / step;

	int n0, n1, n2, nt;
	int xs, ys, xe, ye;
	for (int t=0; t<2; t++)
	{
		// t = 0, 1
		// draw triangel <t, t+1, t+2>
		n0 = t; n1 = t + 1; n2 = t + 2;
		// sort vertices by height
		if (height[n0] > height[n1])
		{ // swap n0, n1
			nt = n0; n0 = n1; n1 = nt;
		}
		if (height[n0] > height[n2])
		{ // swap n0, n2
			nt = n0; n0 = n2; n2 = nt;
		}
		if (height[n1] > height[n2])
		{ // swap n1, n2
			nt = n1; n1 = n2; n2 = nt;
		}

		float level = step * toIntCeil(height[n0] * invStep);
		saturateMax(level, minLevel);
		float toLevel = floatMin(height[n2], maxLevel);
		for (; level<toLevel; level+=step)
		{
			// draw one line (at level <level>)
			float coef = (level - height[n0]) * (1.0 / (height[n2] - height[n0]));
			xe = pt[n0].x + toInt(coef * (pt[n2].x - pt[n0].x));
			ye = pt[n0].y + toInt(coef * (pt[n2].y - pt[n0].y));
			if (level == height[n1])
			{
				xs = pt[n1].x;
				ys = pt[n1].y;
			}
			else if (level < height[n1])
			{
				float coef = (level - height[n0]) * (1.0 / (height[n1] - height[n0]));
				xs = pt[n0].x + coef * (pt[n1].x - pt[n0].x);
				ys = pt[n0].y + coef * (pt[n1].y - pt[n0].y);
			}
			else
			{
				float coef = (level - height[n1]) * (1.0 / (height[n2] - height[n1]));
				xs = pt[n1].x + coef * (pt[n2].x - pt[n1].x);
				ys = pt[n1].y + coef * (pt[n2].y - pt[n1].y);
			}
      if (fastFmod(level + 0.5 * step, stepMain) < step) SelectObject(hDC, penMain);
      else SelectObject(hDC, pen);
			// draw line from <xs, ys> to <xe, ye>
			MoveToEx(hDC, toInt(xs), CONVY(ys), NULL);
			LineTo(hDC, toInt(xe), CONVY(ye));
		}
	}
}

static void DrawCountlines(HDC hDC, int x, int z, int xStep, int zStep)
{
	RECT rect;
	rect.left = toInt(Coef * x * TerrainGrid);
	rect.right = toInt(Coef * (x + xStep) * TerrainGrid);
	rect.top = toInt(Coef * z * TerrainGrid);
	rect.bottom = toInt(Coef * (z + zStep) * TerrainGrid);
	POINT pt[4];
	pt[0].x = rect.left; pt[0].y = rect.bottom;
	pt[1].x = rect.right; pt[1].y = rect.bottom;
	pt[2].x = rect.left; pt[2].y = rect.top;
	pt[3].x = rect.right; pt[3].y = rect.top;

	float height[4];
	height[0] = GLOB_LAND->GetHeight(z + zStep, x);
	height[1] = GLOB_LAND->GetHeight(z + zStep, x + xStep);
	height[2] = GLOB_LAND->GetHeight(z, x);
	height[3] = GLOB_LAND->GetHeight(z, x + xStep);

	// step, minLevel, maxLevel
	DrawLines(hDC, pt, height, 10, 50, -10000, 5, penCountlinesWater, penCountlinesWaterMain);
	DrawLines(hDC, pt, height, 10, 50, 10, 10000, penCountlines, penCountlinesMain);
}

static void DrawName(HDC hDC, ParamEntryPar cls)
{
	float xx = (cls >> "position")[0];
	float yy = (cls >> "position")[1];
	int x = toInt(Coef * xx);
	int y = toInt(Coef * yy);

	RString text = cls >> "name";
	SIZE size;
	GetTextExtentPoint32(hDC, text, text.GetLength(), &size);
	y += size.cy / 2;

/*
	RECT rect;
	DrawTextA(hDC, text, -1, &rect, DT_CALCRECT);
	DrawTextA(hDC, text, -1, &rect, 0);
*/
	TextOut(hDC, x, CONVY(y), text, text.GetLength());
}

#if DRAW_TEXTS
static void TextOutCenter(HDC hDC, int x, int y, const char *buffer, float div)
{
	int width = 0;
	for (const char *p = buffer; *p != 0; p++)
	{
		int wChar;
		GetCharWidth32(hDC, *p, *p, &wChar);
		width += toIntFloor(wChar / div);
	}

	x -= width / 2;

	for (const char *p = buffer; *p != 0; p++)
	{
		TextOut(hDC, x, CONVY(y), p, 1);
		int wChar;
		GetCharWidth32(hDC, *p, *p, &wChar);
		x += toIntFloor(wChar / div);
	}
}
#endif

static void DrawMount(HDC hDC, Vector3Par pos)
{
	int x = toInt(Coef * pos.X());
	int y = toInt(Coef * pos.Z());

	SelectObject(hDC, penSpot);

	MoveToEx(hDC, toInt(x), CONVY(y - 1), NULL);
	LineTo(hDC, toInt(x + 1), CONVY(y - 1));
	LineTo(hDC, toInt(x + 1), CONVY(y + 1));
	LineTo(hDC, toInt(x), CONVY(y + 1));
	LineTo(hDC, toInt(x), CONVY(y - 1));

#if DRAW_TEXTS
	char buffer[256];
	sprintf(buffer, "%.0f", pos.Y());

	TextOutCenter(hDC, x, y + 15, buffer, 5);
#endif
}

static void DrawGrid(HDC hDC)
{
	for (int i=0; i<100; i++)
	{
		int x = toInt(Coef * 0.01 * i * TerrainSize);
		MoveToEx(hDC, x, CONVY(0), NULL);
		LineTo(hDC, x, CONVY(Coef * TerrainSize));
#if DRAW_TEXTS
		char buffer[3];
		buffer[0] = 'A' + i / 10;
		buffer[1] = 'a' + i % 10;
		buffer[2] = 0;
		x += toInt(Coef * 0.005 * TerrainSize);
		TextOutCenter(hDC, x, CONVY(0), buffer, 2.5);
#endif
	}

	for (int i=0; i<100; i++)
	{
		int y = toInt(Coef * 0.01 * i * TerrainSize);
		MoveToEx(hDC, 0, CONVY(y), NULL);
		LineTo(hDC, toInt(Coef * TerrainSize), CONVY(y));
#if DRAW_TEXTS
		char buffer[3];
		buffer[0] = '0' + (99 - i) / 10;
		buffer[1] = '0' + (99 - i) % 10;
		buffer[2] = 0;
		y += toInt(Coef * 0.005 * TerrainSize);
		TextOutCenter(hDC, 15, CONVY(y + 10), buffer, 3);
#endif
	}
}

#if USE_LANDSCAPE_LOCATIONS

// select all locations
static bool LocationAll(int x, int y, int size)
{
  return true;
}

struct LocationCollectMountainPositions
{
  Ref<LocationType> _type;
  AutoArray<Vector3> _mountains;

  LocationCollectMountainPositions()
  {
    _type = LocationTypes.New("Mount");
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (location->_type == _type)
        _mountains.Add(location->_position);
    }
    return false; // continue with enumeration
  }
};

#include <Es/Algorithms/qsort.hpp>

static int CmpMountains(const Vector3 *v0, const Vector3 *v1)
{
  return sign(v1->Y() - v0->Y());
}

#endif

static COLORREF ConvertColor(const Color &color)
{
  int r = toInt(255.0f * (color.R() + (1.0f - color.A()) * (1.0f - color.R())));
  saturate(r, 0, 255);
  int g = toInt(255.0f * (color.G() + (1.0f - color.A()) * (1.0f - color.G())));
  saturate(g, 0, 255);
  int b = toInt(255.0f * (color.B() + (1.0f - color.A()) * (1.0f - color.B())));
  saturate(b, 0, 255);
  return RGB(r, g, b);
}

void ExportWMF(const char *name, bool grid
#if _VBS3
               ,bool contour = true
#endif
               )
{
	GDebugger.PauseCheckingAlive();

  RString description = RString(APP_NAME) + RString("\0Island Map\0\0");
/*
	RECT rect;
	rect.left = 0;
	rect.right = toInt(Coef * TerrainSize);
	rect.top = 0;
	rect.bottom = toInt(Coef * TerrainSize);
	HDC hDC = CreateEnhMetaFile(NULL, name, &rect, description);
	SetMapMode(hDC, MM_HIMETRIC);
*/
  HDC hDC = CreateEnhMetaFile(NULL, name, NULL, description);
	SetBkMode(hDC, TRANSPARENT);

	brushSea = CreateSolidBrush(colorSea);
	brushLand = CreateSolidBrush(colorLand);

  brushForest = CreateSolidBrush(colorForest);
  brushRocks = CreateSolidBrush(colorRocks);
	penForest = CreatePen(PS_SOLID, 1, colorForestBorder);
  penRocks = CreatePen(PS_SOLID, 1, colorRocksBorder);

  penTracks = CreatePen(PS_SOLID, 1, colorTracks);
  penRoads = CreatePen(PS_SOLID, 1, colorRoads);
  penMainRoads = CreatePen(PS_SOLID, 1, colorMainRoads);
  brushTracks = CreateSolidBrush(colorTracksFill);
  brushRoads = CreateSolidBrush(colorRoadsFill);
  brushMainRoads = CreateSolidBrush(colorMainRoadsFill);

  penCountlines = CreatePen(PS_SOLID, 1, colorCountlines);
  penCountlinesMain = CreatePen(PS_SOLID, 1, colorCountlinesMain);
	penCountlinesWater = CreatePen(PS_SOLID, 1, colorCountlinesWater);
  penCountlinesWaterMain = CreatePen(PS_SOLID, 1, colorCountlinesWaterMain);

  penGrid = CreatePen(PS_SOLID, 1, colorGrid);

  penSpot = CreatePen(PS_SOLID, 1, colorSpot);

	ParamEntryVal cls = Pars >> "RscMapControl";
#if DRAW_BITMAPS
  MAP_TYPES_ICONS_1(MAP_TYPE_EXPORT_LOAD_AND_CREATE)
  MAP_TYPES_ICONS_2(MAP_TYPE_EXPORT_LOAD_AND_CREATE)
#else
  MAP_TYPES_ICONS_1(MAP_TYPE_EXPORT_LOAD)
  MAP_TYPES_ICONS_2(MAP_TYPE_EXPORT_LOAD)
#endif

	HGDIOBJ brushOld = SelectObject(hDC, GetStockObject(NULL_BRUSH));
	HGDIOBJ penOld = SelectObject(hDC, GetStockObject(NULL_PEN));

  // land / sea
  int stepLog = GLandscape->GetTerrainRangeLog() - 9;
  saturateMax(stepLog, 0);
  int step = 1 << stepLog;
#if 0 && _VBS3
  step = 1;
#endif
	for (int z=0; z<TerrainRange; z+=step)
		for (int x=0; x<TerrainRange; x+=step)
		{
			DrawSea(hDC, x, z, step, step);
		}

  // forests
  SelectObject(hDC, brushForest);
  SelectObject(hDC, GetStockObject(NULL_PEN));
	for (int z=0; z<LandRange; z++)
		for (int x=0; x<LandRange; x++)
		{
      DrawForests(hDC, x, z, false, false);
		}

	SelectObject(hDC, penForest);
	for (int z=0; z<LandRange; z++)
		for (int x=0; x<LandRange; x++)
		{
      DrawForests(hDC, x, z, true, false);
		}

  // rocks
  SelectObject(hDC, brushRocks);
  SelectObject(hDC, GetStockObject(NULL_PEN));
  for (int z=0; z<LandRange; z++)
    for (int x=0; x<LandRange; x++)
    {
      DrawForests(hDC, x, z, false, true);
    }

  SelectObject(hDC, penRocks);
  for (int z=0; z<LandRange; z++)
    for (int x=0; x<LandRange; x++)
    {
      DrawForests(hDC, x, z, true, true);
    }

  // countlines
#if _VBS3
  if(contour)
#endif  
	for (int z=0; z<TerrainRange; z+=step)
		for (int x=0; x<TerrainRange; x+=step)
		{
			DrawCountlines(hDC, x, z, step, step);
		}

	for (int z=0; z<LandRange; z++)
		for (int x=0; x<LandRange; x++)
		{
			DrawRoads(hDC, x, z);
		}

	for (int z=0; z<LandRange; z++)
		for (int x=0; x<LandRange; x++)
		{
			DrawObjects(hDC, x, z);
		}

/*
	HFONT font = CreateFont
	(
		toInt(Coef * 100), 0, 0, 0,
		FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		"Tahoma"
	);
	HGDIOBJ fontOld = SelectObject(hDC, font);

	// town names
	ParamEntryVal names = Pars >> "CfgWorlds" >> Glob.header.worldname >> "Names";
	for (int i=0; i<names.GetEntryCount(); i++)
	{
		DrawName(hDC, names.GetEntry(i));
	}
*/

	HFONT font = CreateFont
	(
		toInt(Coef * 30), 0, 0, 0,
		FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE,
		"Courier New"
	);
	HGDIOBJ fontOld = SelectObject(hDC, font);

  // mountains
  // TODO: all locations
/*
#if USE_LANDSCAPE_LOCATIONS
  LocationCollectMountainPositions action;
  GLandscape->GetLocations().ForEachInRegion(LocationAll, action);
  QSort(action._mountains.Data(), action._mountains.Size(), CmpMountains);
  const AutoArray<Vector3> &mountains = action._mountains;
#else
	const AutoArray<Vector3> &mountains = GLandscape->GetMountains();
#endif
	float minDist2 = Square(25);
	for (int i=0; i<mountains.Size(); i++)
	{
		const Vector3& pos = mountains[i];
		bool skip = false;
		for (int j=0; j<i; j++)
		{
			if (pos.DistanceXZ2(mountains[j]) < minDist2)
			{
				skip = true;
				break;
			}
		}
		if (!skip) DrawMount(hDC, pos);
	}
*/

	HFONT fontGrid = CreateFont
	(
		toInt(Coef * 50), 0, 0, 0,
		FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		"Arial"
	);
	if (grid)
	{
		SetTextColor(hDC, colorGrid);
		SelectObject(hDC, fontGrid);
		SelectObject(hDC, penGrid);
		DrawGrid(hDC);
	}

	SelectObject(hDC, brushOld);
	SelectObject(hDC, penOld);
	SelectObject(hDC, fontOld);

	DeleteObject(brushSea);
	DeleteObject(brushLand);
	DeleteObject(brushForest);
  DeleteObject(brushRocks);
	DeleteObject(penForest);
  DeleteObject(penRocks);
  DeleteObject(penTracks);
  DeleteObject(penRoads);
	DeleteObject(penMainRoads);
  DeleteObject(brushTracks);
  DeleteObject(brushRoads);
  DeleteObject(brushMainRoads);
	DeleteObject(penCountlines);
  DeleteObject(penCountlinesMain);
  DeleteObject(penCountlinesWater);
	DeleteObject(penCountlinesWaterMain);
	DeleteObject(penGrid);
	DeleteObject(penSpot);
	DeleteObject(font);
	DeleteObject(fontGrid);

#if DRAW_BITMAPS
  MAP_TYPES_ICONS_1(MAP_TYPE_BITMAP_DELETE)
  MAP_TYPES_ICONS_2(MAP_TYPE_BITMAP_DELETE)
#endif

  MAP_TYPES_ICONS_1(MAP_TYPE_EXPORT_UNLOAD)
  MAP_TYPES_ICONS_2(MAP_TYPE_EXPORT_UNLOAD)

	if (CloseEnhMetaFile(hDC) == NULL)
  {
    RptF("Export of map failed - 0x%x", GetLastError());
  }

	GDebugger.ResumeCheckingAlive();
}

#define BUF_OPT ( 64L*1024 )
#define BUF_MIN ( 1024 )

static int fputiw( int W, FILE *f )
{
	if( fputc((byte)W,f)<0 ) return EOF;
	return fputc(W>>8,f);
}
static int fputi24( long W, FILE *f )
{
	if( fputc((byte)W,f)<0 ) return EOF;
	if( fputc((byte)(W>>8),f)<0 ) return EOF;
	if( fputc((byte)(W>>16),f)<0 ) return EOF;
	return 0;
}

#if 0
int TGANSave( const char *N, int W, int H, void *_Buf, unsigned long *RGB, int NC ) /* paleta - NC barev */
{ /* color map uncompressed */
	FILE *f;
	byte *Buf=(byte *)_Buf;
	int r;
	f=fopen(N,"wb");
	r=-1;
	if( f )
	{
		int I;
		long L=(long)W*H;
		if( setvbuf(f,NULL,_IOFBF,BUF_OPT)<0 )
		{
			if( setvbuf(f,NULL,_IOFBF,BUF_MIN)<0 ) goto Error;
		}
		fputc(0,f); /*  Number of Characters in Identification Field. */
		fputc(1,f); /*  Color Map Type. */
		fputc(1,f); /*  Image Type Code. */ /* CM uncompress. */
		/*   3   : Color Map Specification. */
		fputiw(0,f); /* beg */
		fputiw(NC,f); /* count */
		fputc(24,f); /* bit RGBA */
		/*   8   : Image Specification. */
		fputiw(0,f); /*  X Origin of Image. */
		fputiw(0,f); /*  Y Origin of Image. */
		fputiw(W,f); /*  Width Image. */
		fputiw(H,f); /*  Height Image. */
		fputc(8,f); /*  Image Pixel Size. */
		fputc(0x20,f); /*  Image Descriptor Byte. */
		/* 18 */
		for( I=0; I<NC; I++ ) fputi24(RGB[I],f);
		fwrite(Buf,sizeof(char),L,f);
		r=0;
		Error:
		fclose(f);
	}
	return r;
}
#endif

enum {MaxRep=128};

static int UlozBBlok( word *Blok, int *LBlok, FILE *f )
{
	if( *LBlok>0 )
	{
		int L=*LBlok;
		if( fputc(L-1,f)<0 ) return EOF;
		while( --L>=0 )
		{
			if( fputc(*Blok++,f)<0 ) return EOF;
		}
	}
	*LBlok=0;
	return 0;
}

static int PridejBBlok( word *Blok, int *LBlok, FILE *f, word W )
{
	Blok[*LBlok]=W;
	(*LBlok)++;
	if( *LBlok>=MaxRep )
	{
		return UlozBBlok(Blok,LBlok,f);
	}
	return 0;
}

static int PridejBRep( word *Blok, int *LBlok, FILE *f, word LW, int rep )
{
	if( rep>0 )
	{
		if( rep<3 )
		{
			while( --rep>=0 ) if( PridejBBlok(Blok,LBlok,f,LW)<0 ) return EOF;
		}
		else
		{
			if( *LBlok>0 ) if( UlozBBlok(Blok,LBlok,f)<0 ) return EOF;
			if( fputc(rep-1+0x80,f)<0 ) return EOF;
			if( fputc(LW,f)<0 ) return EOF;
		}
	}
	return 0;
}

static int SavePACB( FILE *f, byte *Buf, long L )
{
	int LBloku=0;
	word LW=0;
	int rep=0;
	word *Blok=new word[MaxRep];
	if( !Blok ) return -1;
	
	while( L>0 )
	{
		word A=*Buf++;
		L--;
		if( rep<MaxRep && A==LW ) rep++;
		else
		{
			if( PridejBRep(Blok,&LBloku,f,LW,rep)<0 ) goto Error;
			LW=A,rep=1;
		}
	}
	if( PridejBRep(Blok,&LBloku,f,LW,rep)<0 ) goto Error;
	if( UlozBBlok(Blok,&LBloku,f)<0 ) goto Error;
	
	delete[] Blok;
	return 0;
	Error:
	delete[] Blok;
	return -1;
}

int SavePAC256( const char *N, int W, int H, void *_Buf, unsigned long *RGB, int NC ) /* paleta - NC barev */
{ /* run-length compress. */
	byte *Buf=(byte *)_Buf;
	//word *Blok=mallocSpc(MaxRep*sizeof(*Blok),'PACS');
	FILE *f;
	int r;
	//if( !Blok ) return -1;
	f=fopen(N,"wb");
	r=-1;
	if( f )
	{
		long L=(long)W*H;
		int I;
		if( setvbuf(f,NULL,_IOFBF,BUF_OPT)<0 )
		{
			if( setvbuf(f,NULL,_IOFBF,BUF_MIN)<0 ) goto Error;
		}
		fputc(0,f); /*  Number of Characters in Identification Field. */
		fputc(1,f); /*  Color Map Type. */
		fputc(9,f); /*  Image Type Code. 1 (index) + 8 (compressed) */
		/*   3   : Color Map Specification. */
		fputiw(0,f); /* beg */
		fputiw(NC,f); /* count */
		fputc(24,f); /* bit RGBA */
		/*   8   : Image Specification. */
		fputiw(0,f); /*  X Origin of Image. */
		fputiw(0,f); /*  Y Origin of Image. */
		fputiw(W,f); /*  Width Image. */
		fputiw(H,f); /*  Height Image. */
		fputc(8,f); /*  Image Pixel Size. */
		fputc(0x20,f); /*  Image Descriptor Byte. */
		/* color map */
		for( I=0; I<NC; I++ ) fputi24(RGB[I],f);
		/* 18 */
		if( SavePACB(f,Buf,L)<0 ) goto Error;
		
		r=0;
		Error:
		fclose(f);
	}
	//freeSpc(Blok);
	return r;
}

#define XRGB(r, g, b) RGB(b, g, r)

static void FillArea(char *itemsPath, int size, int xs, int zs)
{
  int index = size * (size - 1 - zs) + xs;
  if (itemsPath[index] == 7)
  {
    itemsPath[index] = 8;
  }
  else
  {
    static const int nDirs = 8;
    static int xDirs[nDirs] = {1,1,0,-1,-1,-1,0,1};
    static int zDirs[nDirs] = {0,1,1,1,0,-1,-1,-1};

    AutoArray<int> stack;
    stack.Add(index);
    while (stack.Size() > 0)
    {
      int i = stack.Size() - 1;
      int index = stack[i];
      stack.Delete(i);
      int z = index / size;
      int x = index - z * size;
      for (int j=0; j<nDirs; j++)
      {
        int zz = z + zDirs[j];
        if (zz < 0 || zz >= size) continue;
        int xx = x + xDirs[j];
        if (xx < 0 || xx >= size) continue;
        int index = zz * size + xx;
        int val = itemsPath[index];
        if (val >= 7) // unaccessible or already accessed
          continue;
        itemsPath[index] += 8;
        stack.Add(index);
      }
    }
  }
}

void ExportOperMaps(RString prefix, RString vehicle, Vector3Par pos)
{
	#if _ENABLE_CHEATS
	GDebugger.PauseCheckingAlive();

	COLORREF palette[] =
	{
		XRGB(255, 255, 255), // OITNormal
		XRGB(192, 255, 255),	// OITAvoidBush
		XRGB(192, 255, 192),	// OITAvoidTree
		XRGB(255, 192, 192),	// OITAvoid
		XRGB(0, 0, 255),			// OITWater
		XRGB(0, 0, 0), 		// OITSpaceRoad
		XRGB(127, 127, 127),		// OITRoad
		XRGB(0, 255, 255),			// OITSpaceBush
    XRGB(0, 255, 127),			// OITSpaceHardBush
		XRGB(0, 255, 0),			// OITSpaceTree
		XRGB(255, 0, 0),			// OITSpace
		XRGB(255, 255, 0)	// OITRoadForced
	};

	const int size = LandRange * OperItemRange;

	char *items = new char[size * size];
	char *itemsSoldier = new char[size * size];
	
	int oz = 0; 
  for (LandIndex zz(0); zz<LandRange; zz++)
	{
		int ox = 0;
		for (LandIndex xx(0); xx<LandRange; xx++)
		{
		  OperMap map;
			const OperMapField &field = map.CreateField(xx, zz, MASK_AVOID_OBJECTS, NULL, CMSafe, true);
      for (OperFieldIndex z(0); z<OperItemRange; z++)
				for (OperFieldIndex x(0); x<OperItemRange; x++)
				{
					OperItem item = field._operField->GetItem(x, z);
					int index = size * (size - 1 - oz - z) + ox + x;
					items[index] = item.GetType();
					itemsSoldier[index] = item.GetTypeSoldier();
				}
			ox += OperItemRange;
		}
		oz += OperItemRange;
	}

	RString name;
	name = prefix + RString("Veh.tga");
	SavePAC256(name, size, size, items, palette, sizeof(palette) / sizeof(COLORREF));
	
	name = prefix + RString("Sol.tga");
	SavePAC256(name, size, size, itemsSoldier, palette, sizeof(palette) / sizeof(COLORREF));

	if (vehicle.GetLength() > 0)
	{
	  Ref<EntityType> eType = VehicleTypes.New(vehicle);
		const EntityAIType *vehType = dynamic_cast<EntityAIType *>(eType.GetRef());

		if (vehType)
		{
			char *itemsPath = new char[size * size];
			char *src = vehType->IsKindOf(GWorld->Preloaded(VTypeMan)) ? itemsSoldier : items;

			int oz = 0; 
			for (int zz=0; zz<LandRange; zz++)
			{
				int ox = 0;
				for (int xx=0; xx<LandRange; xx++)
				{
					float baseCost = OperItemGrid * vehType->GetBaseCost(GLandscape->GetGeography(xx, zz), false, true);
					for (int z=0; z<OperItemRange; z++)
						for (int x=0; x<OperItemRange; x++)
						{
							int index = size * (size - 1 - oz - z) + ox + x;
							float cost = baseCost * vehType->GetTypeCost((OperItemType)(int)src[index],CMAware,ClearanceNormal).cost;
							if (cost >= GET_UNACCESSIBLE)
								itemsPath[index] = 7;
							else
							{
								float value = cost * vehType->GetTypSpeedMs();
								int intValue = toIntFloor(value);
								saturateMin(intValue, 6);
								itemsPath[index] = intValue;
							}
						}
					ox += OperItemRange;
				}
				oz += OperItemRange;
			}

			int xs = toIntFloor(pos.X()*InvOperItemGrid);
			int zs = toIntFloor(pos.Z()*InvOperItemGrid);
      FillArea(itemsPath, size, xs, zs);

			COLORREF pathPalette[] =
			{
				XRGB(255,   0,   0),	// 0
				XRGB(224,   0,   0),	// 1
				XRGB(192,   0,   0),	// 2
				XRGB(128,   0,   0),	// 3
				XRGB(128,   0,   0),	// 4
				XRGB(128,   0,   0),	// 5
				XRGB(128,   0,   0),	// 6
				XRGB(  0,   0,   0), 	// 7
				XRGB(255, 255, 255),	// 0
				XRGB(224, 224, 224),	// 1
				XRGB(192, 192, 192),	// 2
				XRGB(160, 160, 160),	// 3
				XRGB(128, 128, 128),	// 4
				XRGB( 96,  96,  96),	// 5
				XRGB( 64,  64,  64),	// 6
				XRGB(  0,   0,   0),	// 7
			};
			name = prefix + vehicle + RString(".tga");
			SavePAC256(name, size, size, itemsPath, pathPalette, sizeof(pathPalette) / sizeof(COLORREF));

      name = prefix + vehicle + RString(".txt");
      QOFStream out;
      out.open(name);
      for (int z=0; z<size; z++)
      {
        for (int x=0; x<size; x++)
        {
          int index = size * (size - 1 - z) + x;
          if (itemsPath[index] < 7)
          {
            float xf = x * OperItemGrid + 0.5f * OperItemGrid;
            float zf = z * OperItemGrid + 0.5f * OperItemGrid;
            char buf[256];
            sprintf(buf, "%.0f, %.0f\n", xf, zf);
            out.write(buf, strlen(buf));
            FillArea(itemsPath, size, x, z);
          }
        }
      }
      out.close();

      delete [] itemsPath;
		}
	}

	delete [] items;
	delete [] itemsSoldier;

	GDebugger.ResumeCheckingAlive();
	#endif
}

#else
void ExportOperMaps(RString prefix)
{
}
void ExportWMF(const char *name, bool grid)
{
}

#endif
