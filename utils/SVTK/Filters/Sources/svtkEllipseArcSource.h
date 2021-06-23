/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEllipseArcSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEllipseArcSource
 * @brief   create an elliptical arc
 *
 *
 * svtkEllipseArcSource is a source object that creates an elliptical arc
 * defined by a normal, a center and the major radius vector.
 * You can define an angle to draw only a section of the ellipse. The number of
 * segments composing the polyline is controlled by setting the object
 * resolution.
 *
 * @sa
 * svtkArcSource
 */

#ifndef svtkEllipseArcSource_h
#define svtkEllipseArcSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkEllipseArcSource : public svtkPolyDataAlgorithm
{
public:
  static svtkEllipseArcSource* New();
  svtkTypeMacro(svtkEllipseArcSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set position of the center of the ellipse that define the arc.
   * Default is 0, 0, 0.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set normal vector. Represents the plane in which the ellipse will be drawn.
   * Default 0, 0, 1.
   */
  svtkSetVector3Macro(Normal, double);
  svtkGetVectorMacro(Normal, double, 3);
  //@}

  //@{
  /**
   * Set Major Radius Vector. It defines the origin of polar angle and the major
   * radius size.
   * Default is 1, 0, 0.
   */
  svtkSetVector3Macro(MajorRadiusVector, double);
  svtkGetVectorMacro(MajorRadiusVector, double, 3);
  //@}

  //@{
  /**
   * Set the start angle. The angle where the plot begins.
   * Default is 0.
   */
  svtkSetClampMacro(StartAngle, double, -360.0, 360.0);
  svtkGetMacro(StartAngle, double);
  //@}

  //@{
  /**
   * Angular sector occupied by the arc, beginning at Start Angle
   * Default is 90.
   */
  svtkSetClampMacro(SegmentAngle, double, 0.0, 360.0);
  svtkGetMacro(SegmentAngle, double);
  //@}

  //@{
  /**
   * Divide line into resolution number of pieces.
   * Note: if Resolution is set to 1 the arc is a
   * straight line. Default is 100.
   */
  svtkSetClampMacro(Resolution, int, 1, SVTK_INT_MAX);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Set/get whether to close the arc with a final line segment connecting the first
   * and last points in the arc. Off by default
   */
  svtkSetMacro(Close, bool);
  svtkGetMacro(Close, bool);
  svtkBooleanMacro(Close, bool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point,
   * This is the default.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  //@{
  /**
   * Set the ratio of the ellipse, i.e. the ratio b/a _ b: minor radius;
   * a: major radius
   * default is 1.
   */
  svtkSetClampMacro(Ratio, double, 0.001, 100.0);
  svtkGetMacro(Ratio, double);
  //@}

protected:
  svtkEllipseArcSource();
  ~svtkEllipseArcSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Center[3];
  double Normal[3];
  double MajorRadiusVector[3];
  double StartAngle;
  double SegmentAngle;
  int Resolution;
  double Ratio;
  bool Close;
  int OutputPointsPrecision;

private:
  svtkEllipseArcSource(const svtkEllipseArcSource&) = delete;
  void operator=(const svtkEllipseArcSource&) = delete;
};

#endif
