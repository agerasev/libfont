#pragma once

#include <wchar.h>

typedef struct fRasterizer fRasterizer;

typedef struct fRaster
{
	void *data;
	int width, height;
	int origin_x, origin_y;
}
fRaster;

#ifdef __cplusplus
extern "C" {
#endif

void fInit();
void fDispose();

fRasterizer *fCreateRasterizer(void *font_file_base, long file_size);
void fDestroyRasterizer(fRasterizer *rasterizer);

fRaster *fRasterize(fRasterizer *rasterizer, const wchar_t *text, int size);
void fFreeRaster(fRaster *raster);

#ifdef __cplusplus
}
#endif
