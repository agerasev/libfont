#pragma once

#include <wchar.h>

typedef struct FRasterizer FRasterizer;

typedef struct FRaster
{
	void *data;
	int width, height;
	int origin_x, origin_y;
}
FRaster;

#ifdef __cplusplus
extern "C" {
#endif

void fInit();
void fDispose();

FRasterizer *fCreateRasterizer(void *font_file_base, long file_size);
void fDestroyRasterizer(FRasterizer *rasterizer);

FRaster *fRasterize(FRasterizer *rasterizer, const wchar_t *text, int size);
void fFreeRaster(FRaster *raster);

#ifdef __cplusplus
}
#endif
