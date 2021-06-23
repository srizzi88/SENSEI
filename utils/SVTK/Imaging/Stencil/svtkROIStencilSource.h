/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkROIStencilSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkROIStencilSource
 * @brief   create simple mask shapes
 *
 * svtkROIStencilSource will create an image stencil with a
 * simple shape like a box, a sphere, or a cylinder.  Its output can
 * be used with svtkImageStecil or other svtk classes that apply
 * a stencil to an image.
 * @sa
 * svtkImplicitFunctionToImageStencil svtkLassoStencilSource
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkROIStencilSource_h
#define svtkROIStencilSource_h

#include "svtkImageStencilSource.h"
#include "svtkImagingStencilModule.h" // For export macro

class SVTKIMAGINGSTENCIL_EXPORT svtkROIStencilSource : public svtkImageStencilSource
{
public:
  static svtkROIStencilSource* New();
  svtkTypeMacro(svtkROIStencilSource, svtkImageStencilSource);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    BOX = 0,
    ELLIPSOID = 1,
    CYLINDERX = 2,
    CYLINDERY = 3,
    CYLINDERZ = 4
  };

  //@{
  /**
   * The shape of the region of interest.  Cylinders can be oriented
   * along the X, Y, or Z axes.  The default shape is "Box".
   */
  svtkGetMacro(Shape, int);
  svtkSetClampMacro(Shape, int, BOX, CYLINDERZ);
  void SetShapeToBox() { this->SetShape(BOX); }
  void SetShapeToEllipsoid() { this->SetShape(ELLIPSOID); }
  void SetShapeToCylinderX() { this->SetShape(CYLINDERX); }
  void SetShapeToCylinderY() { this->SetShape(CYLINDERY); }
  void SetShapeToCylinderZ() { this->SetShape(CYLINDERZ); }
  virtual const char* GetShapeAsString();
  //@}

  //@{
  /**
   * Set the bounds of the region of interest.  The bounds take
   * the spacing and origin into account.
   */
  svtkGetVector6Macro(Bounds, double);
  svtkSetVector6Macro(Bounds, double);
  //@}

protected:
  svtkROIStencilSource();
  ~svtkROIStencilSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int Shape;
  double Bounds[6];

private:
  svtkROIStencilSource(const svtkROIStencilSource&) = delete;
  void operator=(const svtkROIStencilSource&) = delete;
};

#endif
