/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlyph2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGlyph2D
 * @brief   copy oriented and scaled glyph geometry to every input point (2D specialization)
 *
 * This subclass of svtkGlyph3D is a specialization to 2D. Transformations
 * (i.e., translation, scaling, and rotation) are constrained to the plane.
 * For example, rotations due to a vector are computed from the x-y
 * coordinates of the vector only, and are assumed to occur around the
 * z-axis. (See svtkGlyph3D for documentation on the interface to this
 * class.)
 *
 * @sa
 * svtkTensorGlyph svtkGlyph3D svtkProgrammableGlyphFilter
 */

#ifndef svtkGlyph2D_h
#define svtkGlyph2D_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkGlyph3D.h"

class SVTKFILTERSCORE_EXPORT svtkGlyph2D : public svtkGlyph3D
{
public:
  svtkTypeMacro(svtkGlyph2D, svtkGlyph3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with scaling on, scaling mode is by scalar value,
   * scale factor = 1.0, the range is (0,1), orient geometry is on, and
   * orientation is by vector. Clamping and indexing are turned off. No
   * initial sources are defined.
   */
  static svtkGlyph2D* New();

protected:
  svtkGlyph2D() {}
  ~svtkGlyph2D() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkGlyph2D(const svtkGlyph2D&) = delete;
  void operator=(const svtkGlyph2D&) = delete;
};

#endif
