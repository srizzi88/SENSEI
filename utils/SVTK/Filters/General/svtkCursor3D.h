/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCursor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCursor3D
 * @brief   generate a 3D cursor representation
 *
 * svtkCursor3D is an object that generates a 3D representation of a cursor.
 * The cursor consists of a wireframe bounding box, three intersecting
 * axes lines that meet at the cursor focus, and "shadows" or projections
 * of the axes against the sides of the bounding box. Each of these
 * components can be turned on/off.
 *
 * This filter generates two output datasets. The first (Output) is just the
 * geometric representation of the cursor. The second (Focus) is a single
 * point at the focal point.
 */

#ifndef svtkCursor3D_h
#define svtkCursor3D_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkCursor3D : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkCursor3D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with model bounds = (-1,1,-1,1,-1,1), focal point = (0,0,0),
   * all parts of cursor visible, and wrapping off.
   */
  static svtkCursor3D* New();

  //@{
  /**
   * Set / get the boundary of the 3D cursor.
   */
  void SetModelBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  void SetModelBounds(const double bounds[6]);
  svtkGetVectorMacro(ModelBounds, double, 6);
  //@}

  //@{
  /**
   * Set/Get the position of cursor focus. If translation mode is on,
   * then the entire cursor (including bounding box, cursor, and shadows)
   * is translated. Otherwise, the focal point will either be clamped to the
   * bounding box, or wrapped, if Wrap is on. (Note: this behavior requires
   * that the bounding box is set prior to the focal point.)
   */
  void SetFocalPoint(double x[3]);
  void SetFocalPoint(double x, double y, double z)
  {
    double xyz[3];
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    this->SetFocalPoint(xyz);
  }
  svtkGetVectorMacro(FocalPoint, double, 3);
  //@}

  //@{
  /**
   * Turn on/off the wireframe bounding box.
   */
  svtkSetMacro(Outline, svtkTypeBool);
  svtkGetMacro(Outline, svtkTypeBool);
  svtkBooleanMacro(Outline, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the wireframe axes.
   */
  svtkSetMacro(Axes, svtkTypeBool);
  svtkGetMacro(Axes, svtkTypeBool);
  svtkBooleanMacro(Axes, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the wireframe x-shadows.
   */
  svtkSetMacro(XShadows, svtkTypeBool);
  svtkGetMacro(XShadows, svtkTypeBool);
  svtkBooleanMacro(XShadows, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the wireframe y-shadows.
   */
  svtkSetMacro(YShadows, svtkTypeBool);
  svtkGetMacro(YShadows, svtkTypeBool);
  svtkBooleanMacro(YShadows, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the wireframe z-shadows.
   */
  svtkSetMacro(ZShadows, svtkTypeBool);
  svtkGetMacro(ZShadows, svtkTypeBool);
  svtkBooleanMacro(ZShadows, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable/disable the translation mode. If on, changes in cursor position
   * cause the entire widget to translate along with the cursor.
   * By default, translation mode is off.
   */
  svtkSetMacro(TranslationMode, svtkTypeBool);
  svtkGetMacro(TranslationMode, svtkTypeBool);
  svtkBooleanMacro(TranslationMode, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off cursor wrapping. If the cursor focus moves outside the
   * specified bounds, the cursor will either be restrained against the
   * nearest "wall" (Wrap=off), or it will wrap around (Wrap=on).
   */
  svtkSetMacro(Wrap, svtkTypeBool);
  svtkGetMacro(Wrap, svtkTypeBool);
  svtkBooleanMacro(Wrap, svtkTypeBool);
  //@}

  /**
   * Get the focus for this filter.
   */
  svtkPolyData* GetFocus() { return this->Focus; }

  //@{
  /**
   * Turn every part of the 3D cursor on or off.
   */
  void AllOn();
  void AllOff();
  //@}

protected:
  svtkCursor3D();
  ~svtkCursor3D() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkPolyData* Focus;
  double ModelBounds[6];
  double FocalPoint[3];
  svtkTypeBool Outline;
  svtkTypeBool Axes;
  svtkTypeBool XShadows;
  svtkTypeBool YShadows;
  svtkTypeBool ZShadows;
  svtkTypeBool TranslationMode;
  svtkTypeBool Wrap;

private:
  svtkCursor3D(const svtkCursor3D&) = delete;
  void operator=(const svtkCursor3D&) = delete;
};

#endif
