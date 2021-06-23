/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLassoStencilSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLassoStencilSource
 * @brief   Create a stencil from a contour
 *
 * svtkLassoStencilSource will create an image stencil from a
 * set of points that define a contour.  Its output can be
 * used with svtkImageStecil or other svtk classes that apply
 * a stencil to an image.
 * @sa
 * svtkROIStencilSource svtkPolyDataToImageStencil
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkLassoStencilSource_h
#define svtkLassoStencilSource_h

#include "svtkImageStencilSource.h"
#include "svtkImagingStencilModule.h" // For export macro

class svtkPoints;
class svtkSpline;
class svtkLSSPointMap;

class SVTKIMAGINGSTENCIL_EXPORT svtkLassoStencilSource : public svtkImageStencilSource
{
public:
  static svtkLassoStencilSource* New();
  svtkTypeMacro(svtkLassoStencilSource, svtkImageStencilSource);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    POLYGON = 0,
    SPLINE = 1
  };

  //@{
  /**
   * The shape to use, default is "Polygon".  The spline is a
   * cardinal spline.  Bezier splines are not yet supported.
   */
  svtkGetMacro(Shape, int);
  svtkSetClampMacro(Shape, int, POLYGON, SPLINE);
  void SetShapeToPolygon() { this->SetShape(POLYGON); }
  void SetShapeToSpline() { this->SetShape(SPLINE); }
  virtual const char* GetShapeAsString();
  //@}

  //@{
  /**
   * The points that make up the lassoo.  The loop does not
   * have to be closed, the last point will automatically be
   * connected to the first point by a straight line segment.
   */
  virtual void SetPoints(svtkPoints* points);
  svtkGetObjectMacro(Points, svtkPoints);
  //@}

  //@{
  /**
   * The slice orientation.  The default is 2, which is XY.
   * Other values are 0, which is YZ, and 1, which is XZ.
   */
  svtkGetMacro(SliceOrientation, int);
  svtkSetClampMacro(SliceOrientation, int, 0, 2);
  //@}

  //@{
  /**
   * The points for a particular slice.  This will override the
   * points that were set by calling SetPoints() for the slice.
   * To clear the setting, call SetSlicePoints(slice, nullptr).
   */
  virtual void SetSlicePoints(int i, svtkPoints* points);
  virtual svtkPoints* GetSlicePoints(int i);
  //@}

  /**
   * Remove points from all slices.
   */
  virtual void RemoveAllSlicePoints();

  /**
   * Overload GetMTime() to include the timestamp on the points.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkLassoStencilSource();
  ~svtkLassoStencilSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int Shape;
  int SliceOrientation;
  svtkPoints* Points;
  svtkSpline* SplineX;
  svtkSpline* SplineY;
  svtkLSSPointMap* PointMap;

private:
  svtkLassoStencilSource(const svtkLassoStencilSource&) = delete;
  void operator=(const svtkLassoStencilSource&) = delete;
};

#endif
