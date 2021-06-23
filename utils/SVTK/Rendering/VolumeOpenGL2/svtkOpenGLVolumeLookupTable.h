/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeLookupTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=============================================================================
Copyright and License information
=============================================================================*/
/**
 * @class svtkOpenGLVolumeLookupTable
 * @brief Base class for OpenGL texture management of scalar color,
 * opacity and gradient opacity lookup tables.
 */

#ifndef svtkOpenGLVolumeLookupTable_h
#define svtkOpenGLVolumeLookupTable_h
#ifndef __SVTK_WRAP__

#include "svtkObject.h"

// Forward declarations
class svtkOpenGLRenderWindow;
class svtkTextureObject;
class svtkWindow;

class svtkOpenGLVolumeLookupTable : public svtkObject
{
public:
  svtkTypeMacro(svtkOpenGLVolumeLookupTable, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // static svtkOpenGLVolumeLookupTable* New();

  /**
   * Get the texture unit associated with the managed texture object
   */
  int GetTextureUnit();

  /**
   * Activate / deactivate the internal texture object
   */
  //@{
  void Activate();
  void Deactivate();
  //@}

  /**
   * Get the maximum supported texture width for the target OpenGL environment.
   */
  int GetMaximumSupportedTextureWidth(svtkOpenGLRenderWindow* renWin, int idealWidth);

  /**
   * Release graphics resources
   */
  void ReleaseGraphicsResources(svtkWindow* window);

  /**
   * Update the internal texture object using the transfer function provided.
   */
  virtual void Update(svtkObject* func, double scalarRange[2], int blendMode, double sampleDistance,
    double unitDistance, int filterValue, svtkOpenGLRenderWindow* renWin);

  /**
   * Get access to the texture height used by this object
   */
  svtkGetMacro(TextureHeight, int);

  /**
   * Get access to the texture width used by this object
   */
  svtkGetMacro(TextureWidth, int);

protected:
  svtkOpenGLVolumeLookupTable() = default;
  virtual ~svtkOpenGLVolumeLookupTable() override;

  double LastRange[2] = { 0.0, 0.0 };
  float* Table = nullptr;
  int LastInterpolation = -1;
  int NumberOfColorComponents = 1;
  int TextureWidth = 1024;
  int TextureHeight = 1;
  svtkTextureObject* TextureObject = nullptr;
  svtkTimeStamp BuildTime;

  /**
   * Test whether the internal function needs to be updated.
   */
  virtual bool NeedsUpdate(
    svtkObject* func, double scalarRange[2], int blendMode, double sampleDistance);

  /**
   * Internal method to actually update the texture object
   */
  virtual void InternalUpdate(
    svtkObject* func, int blendMode, double sampleDistance, double unitDistance, int filterValue);

  /**
   * Compute ideal width and height for the texture based on function provided
   */
  virtual void ComputeIdealTextureSize(
    svtkObject* func, int& width, int& height, svtkOpenGLRenderWindow* renWin);

  /**
   * Allocate internal data table
   */
  virtual void AllocateTable();

private:
  svtkOpenGLVolumeLookupTable(const svtkOpenGLVolumeLookupTable&) = delete;
  void operator=(const svtkOpenGLVolumeLookupTable&) = delete;
};

#endif //__SVTK_WRAP__
#endif // svtkOpenGLVolumeLookupTable_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeLookupTable.h
