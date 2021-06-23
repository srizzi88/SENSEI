/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProperty2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProperty2D
 * @brief   represent surface properties of a 2D image
 *
 * svtkProperty2D contains properties used to render two dimensional images
 * and annotations.
 *
 * @sa
 * svtkActor2D
 */

#ifndef svtkProperty2D_h
#define svtkProperty2D_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkViewport;

#define SVTK_BACKGROUND_LOCATION 0
#define SVTK_FOREGROUND_LOCATION 1

class SVTKRENDERINGCORE_EXPORT svtkProperty2D : public svtkObject
{
public:
  svtkTypeMacro(svtkProperty2D, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a svtkProperty2D with the following default values:
   * Opacity 1, Color (1,1,1)
   */
  static svtkProperty2D* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(svtkProperty2D* p);

  //@{
  /**
   * Set/Get the RGB color of this property.
   */
  svtkSetVector3Macro(Color, double);
  svtkGetVector3Macro(Color, double);
  //@}

  //@{
  /**
   * Set/Get the Opacity of this property.
   */
  svtkGetMacro(Opacity, double);
  svtkSetMacro(Opacity, double);
  //@}

  //@{
  /**
   * Set/Get the diameter of a Point. The size is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  svtkSetClampMacro(PointSize, float, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(PointSize, float);
  //@}

  //@{
  /**
   * Set/Get the width of a Line. The width is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  svtkSetClampMacro(LineWidth, float, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(LineWidth, float);
  //@}

  //@{
  /**
   * Set/Get the stippling pattern of a Line, as a 16-bit binary pattern
   * (1 = pixel on, 0 = pixel off).
   * This is only implemented for OpenGL, not OpenGL2. The default is 0xFFFF.
   */
  svtkSetMacro(LineStipplePattern, int);
  svtkGetMacro(LineStipplePattern, int);
  //@}

  //@{
  /**
   * Set/Get the stippling repeat factor of a Line, which specifies how
   * many times each bit in the pattern is to be repeated.
   * This is only implemented for OpenGL, not OpenGL2. The default is 1.
   */
  svtkSetClampMacro(LineStippleRepeatFactor, int, 1, SVTK_INT_MAX);
  svtkGetMacro(LineStippleRepeatFactor, int);
  //@}

  //@{
  /**
   * The DisplayLocation is either background or foreground.
   * If it is background, then this 2D actor will be drawn
   * behind all 3D props or foreground 2D actors. If it is
   * background, then this 2D actor will be drawn in front of
   * all 3D props and background 2D actors. Within 2D actors
   * of the same DisplayLocation type, order is determined by
   * the order in which the 2D actors were added to the viewport.
   */
  svtkSetClampMacro(DisplayLocation, int, SVTK_BACKGROUND_LOCATION, SVTK_FOREGROUND_LOCATION);
  svtkGetMacro(DisplayLocation, int);
  void SetDisplayLocationToBackground() { this->DisplayLocation = SVTK_BACKGROUND_LOCATION; }
  void SetDisplayLocationToForeground() { this->DisplayLocation = SVTK_FOREGROUND_LOCATION; }
  //@}

  /**
   * Have the device specific subclass render this property.
   */
  virtual void Render(svtkViewport* svtkNotUsed(viewport)) {}

protected:
  svtkProperty2D();
  ~svtkProperty2D() override;

  double Color[3];
  double Opacity;
  float PointSize;
  float LineWidth;
  int LineStipplePattern;
  int LineStippleRepeatFactor;
  int DisplayLocation;

private:
  svtkProperty2D(const svtkProperty2D&) = delete;
  void operator=(const svtkProperty2D&) = delete;
};

#endif
