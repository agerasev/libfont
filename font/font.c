#include "font.h"

#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <media/log.h>

struct fRasterizer
{
	FT_Face face;
};

static FT_Library library;
static fRasterizer *_ras = NULL;

void fInit()
{
	FT_Error error = FT_Init_FreeType(&library);
	if(error)
	{
	  printWarn("[error] FT_Init_FreeType \n");
	}
}

void fDispose()
{
	FT_Done_FreeType(library);
}

fRasterizer *fCreateRasterizer(void *font_file_base, long file_size)
{
	fRasterizer *ras = (fRasterizer*)malloc(sizeof(fRasterizer));
	FT_Error error = FT_New_Memory_Face(library,(FT_Byte*)font_file_base,file_size,0,&ras->face);
	// FT_Error error = FT_New_Face(library,filename,0,&ras->face);
	if(error == FT_Err_Unknown_File_Format)
	{
	  printWarn("[error] FT_New_Memory_Face: FT_Err_Unknown_File_Format \n");
	}
	else if(error) { printWarn("[error] FT_New_Memory_Face \n"); }
	_ras = ras;
	return ras;
}

void fDestroyRasterizer(fRasterizer *rasterizer)
{
	if(!rasterizer)
	{
		_ras = rasterizer;
	}
	FT_Done_Face(rasterizer->face);
	free(rasterizer);
}

fRaster *fRasterize(fRasterizer *rasterizer, const wchar_t *text, int size)
{
	int i, j;
	FT_Error error;
	
	if(!rasterizer)
	{
		rasterizer = _ras;
	}
	
	FT_Face				face = rasterizer->face;
	int num_chars = wcslen(text);
	
	FT_GlyphSlot  slot = face->glyph;   /* a small shortcut */
	FT_UInt       glyph_index;
	FT_Bool       use_kerning;
	FT_UInt       previous;
	int           pen_x, pen_y, n;
	
	FT_Glyph     *glyphs = (FT_Glyph*)malloc(sizeof(FT_Glyph)*num_chars); /* glyph image    */
	FT_Vector    *pos = (FT_Vector*)malloc(sizeof(FT_Vector)*num_chars); /* glyph position */
	FT_UInt       num_glyphs;
	
	error = FT_Set_Pixel_Sizes(face,0,size);
	if(error)
	{
		printWarn("[error] FT_Set_Pixel_Sizes \n");
	}
	
	pen_x = 0;   /* start at (0,0) */
	pen_y = 0;
	
	num_glyphs  = 0;
	use_kerning = FT_HAS_KERNING( face );
	previous    = 0;
	
	for ( n = 0; n < num_chars; n++ )
	{
		/* convert character code to glyph index */
		glyph_index = FT_Get_Char_Index( face, text[n] );
	
		/* retrieve kerning distance and move pen position */
		if ( use_kerning && previous && glyph_index )
		{
			FT_Vector  delta;
	
	
			FT_Get_Kerning( face, previous, glyph_index,
											FT_KERNING_DEFAULT, &delta );
	
			pen_x += delta.x >> 6;
		}
	
		/* store current pen position */
		pos[num_glyphs].x = pen_x;
		pos[num_glyphs].y = pen_y;
	
		/* load glyph image into the slot without rendering */
		error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
		if ( error )
		{
			printWarn("[error] FT_Load_Glyph \n");
			continue;  /* jump to next glyph */
		}
	
		/* extract glyph image and store it in our table */
		error = FT_Get_Glyph( face->glyph, &glyphs[num_glyphs] );
		if ( error )
		{
			printWarn("[error] FT_Get_Glyph \n");
			continue;  /* jump to next glyph */
		}
	
		/* increment pen position */
		pen_x += slot->advance.x >> 6;
	
		/* record current glyph index */
		previous = glyph_index;
	
		/* increment number of glyphs */
		num_glyphs++;
	}
	
	/* Compute width */
	FT_BBox  bbox;
  FT_BBox  glyph_bbox;


  /* initialize string bbox to "empty" values */
  bbox.xMin = bbox.yMin =  32000;
  bbox.xMax = bbox.yMax = -32000;

  /* for each glyph image, compute its bounding box, */
  /* translate it, and grow the string bbox          */
  for ( n = 0; n < num_glyphs; n++ )
  {
    FT_Glyph_Get_CBox( glyphs[n], ft_glyph_bbox_pixels,
                       &glyph_bbox );

    glyph_bbox.xMin += pos[n].x;
    glyph_bbox.xMax += pos[n].x;
    glyph_bbox.yMin += pos[n].y;
    glyph_bbox.yMax += pos[n].y;

    if ( glyph_bbox.xMin < bbox.xMin )
      bbox.xMin = glyph_bbox.xMin;

    if ( glyph_bbox.yMin < bbox.yMin )
      bbox.yMin = glyph_bbox.yMin;

    if ( glyph_bbox.xMax > bbox.xMax )
      bbox.xMax = glyph_bbox.xMax;

    if ( glyph_bbox.yMax > bbox.yMax )
      bbox.yMax = glyph_bbox.yMax;
  }

  /* check that we really grew the string bbox */
  if ( bbox.xMin > bbox.xMax )
  {
    bbox.xMin = 0;
    bbox.yMin = 0;
    bbox.xMax = 0;
    bbox.yMax = 0;
  }
	
	/* compute string dimensions in integer pixels */
	int string_width  = bbox.xMax - bbox.xMin;
	int string_height = bbox.yMax - bbox.yMin;
	
	fRaster *raster = (fRaster*)malloc(sizeof(fRaster));
	raster->width = string_width + 2;
	raster->height = string_height + 2;
	raster->origin_x = -(bbox.xMin - 1);
	raster->origin_y = -(bbox.yMin - 1);
	raster->data = malloc(sizeof(unsigned char)*4*raster->width*raster->height);
	for(i = 0; i < raster->width*raster->height; ++i)
	{
		unsigned char *pixel = (unsigned char*)raster->data + 4*i;
		for(j = 0; j < 3; ++j)
		{
			pixel[j] = 0xff;
		}
		pixel[3] = 0;
	}
	
	// printInfo("string bounds: %d, %d \n",string_width,string_height);
	
	/* compute start pen position in 26.6 Cartesian pixels */
	int start_x = -(bbox.xMin - 1) * 64;//( ( raster->width  - string_width  ) / 2 ) * 64;
	int start_y = -(bbox.yMin - 1) * 64;//( ( raster->height - string_height ) / 2 ) * 64;
	
	for ( n = 0; n < num_glyphs; n++ )
	{
	  FT_Glyph   image;
	  FT_Vector  pen;
	
	
	  image = glyphs[n];
	
	  pen.x = start_x + (pos[n].x << 6);
	  pen.y = start_y + (pos[n].y << 6);
		
		// printInfo("pen%d: %d, %d \n",n,(int)pen.x,(int)pen.y);
	
	  error = FT_Glyph_To_Bitmap( &image, FT_RENDER_MODE_NORMAL,
	                              &pen, 0 );
	  if ( !error )
	  {
	    FT_BitmapGlyph  bit = (FT_BitmapGlyph)image;
	
			FT_Bitmap *bitmap = &bit->bitmap;
			
			FT_Int p, q;
			for(q = 0; q < bitmap->rows; ++q)
			{
				for(p = 0; p < bitmap->width; ++p)
				{
					// int x = p + bit->left, y = q + raster->height - bit->top;
					int x = p + bit->left, y = q + raster->height - bit->top;
					if(x >= 0 && x < raster->width && y >= 0 && y < raster->height)
					{
						unsigned char *pixel = (unsigned char*)raster->data + 4*(raster->width*y + x);
						pixel[3] |= bitmap->buffer[q*bitmap->width + p];
					}
				}
			}
	
	    FT_Done_Glyph( image );
	  }
	}
	
	free(glyphs);
	free(pos);
	
	return raster;
}

void fFreeRaster(fRaster *raster)
{
	free(raster->data);
	free(raster);
}
