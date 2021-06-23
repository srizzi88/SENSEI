/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTIFFReaderInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkTIFFReaderInternal
 * @brief
 *
 */

#ifndef svtkTIFFReaderInternal_h
#define svtkTIFFReaderInternal_h

extern "C"
{
#include "svtk_tiff.h"
}

class svtkTIFFReader::svtkTIFFReaderInternal
{
public:
  svtkTIFFReaderInternal();
  ~svtkTIFFReaderInternal() = default;

  bool Initialize();
  void Clean();
  bool CanRead();
  bool Open(const char* filename);
  TIFF* Image;
  bool IsOpen;
  unsigned int Width;
  unsigned int Height;
  unsigned short NumberOfPages;
  unsigned short CurrentPage;
  unsigned short SamplesPerPixel;
  unsigned short Compression;
  unsigned short BitsPerSample;
  unsigned short Photometrics;
  bool HasValidPhotometricInterpretation;
  unsigned short PlanarConfig;
  unsigned short Orientation;
  unsigned long int TileDepth;
  unsigned int TileRows;
  unsigned int TileColumns;
  unsigned int TileWidth;
  unsigned int TileHeight;
  unsigned short NumberOfTiles;
  unsigned int SubFiles;
  unsigned int ResolutionUnit;
  float XResolution;
  float YResolution;
  short SampleFormat;
  static void ErrorHandler(const char* module, const char* fmt, va_list ap);

private:
  svtkTIFFReaderInternal(const svtkTIFFReaderInternal&) = delete;
  void operator=(const svtkTIFFReaderInternal&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkTIFFReaderInternal.h
