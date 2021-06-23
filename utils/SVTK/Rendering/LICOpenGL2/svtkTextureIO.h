/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextureIO.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextureIO
 *
 * A small collection of I/O routines that write svtkTextureObject
 * to disk for debugging.
 */

#ifndef svtkTextureIO_h
#define svtkTextureIO_h

#include "svtkPixelExtent.h"               // for pixel extent
#include "svtkRenderingLICOpenGL2Module.h" // for export

// included svtkSystemIncludes in svtkPixelExtent
#include <cstddef> // for NULL
#include <deque>   // for deque
#include <string>  // for string

class svtkTextureObject;

class SVTKRENDERINGLICOPENGL2_EXPORT svtkTextureIO
{
public:
  /**
   * Write to disk as image data with subset(optional) at dataset origin(optional)
   */
  static void Write(const char* filename, svtkTextureObject* texture,
    const unsigned int* subset = nullptr, const double* origin = nullptr);

  /**
   * Write to disk as image data with subset(optional) at dataset origin(optional)
   */
  static void Write(std::string filename, svtkTextureObject* texture,
    const unsigned int* subset = nullptr, const double* origin = nullptr)
  {
    Write(filename.c_str(), texture, subset, origin);
  }

  /**
   * Write to disk as image data with subset(optional) at dataset origin(optional)
   */
  static void Write(std::string filename, svtkTextureObject* texture, const svtkPixelExtent& subset,
    const double* origin = nullptr)
  {
    Write(filename.c_str(), texture, subset.GetDataU(), origin);
  }

  /**
   * Write list of subsets to disk as multiblock image data at dataset origin(optional).
   */
  static void Write(const char* filename, svtkTextureObject* texture,
    const std::deque<svtkPixelExtent>& exts, const double* origin = nullptr);

  //@{
  /**
   * Write list of subsets to disk as multiblock image data at dataset origin(optional).
   */
  static void Write(std::string filename, svtkTextureObject* texture,
    const std::deque<svtkPixelExtent>& exts, const double* origin = nullptr)
  {
    Write(filename.c_str(), texture, exts, origin);
  }
  //@}
};

#endif
// SVTK-HeaderTest-Exclude: svtkTextureIO.h
