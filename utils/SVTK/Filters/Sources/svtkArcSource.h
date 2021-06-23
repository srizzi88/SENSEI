/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArcSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkArcSource
 * @brief   create a circular arc
 *
 *
 * svtkArcSource is a source object that creates an arc defined by two
 * endpoints and a center. The number of segments composing the polyline
 * is controlled by setting the object resolution.
 * Alternatively, one can use a better API (that does not allow for
 * inconsistent nor ambiguous inputs), using a starting point (polar vector,
 * measured from the arc's center), a normal to the plane of the arc,
 * and an angle defining the arc length.
 * Since the default API remains the original one, in order to use
 * the improved API, one must switch the UseNormalAndAngle flag to TRUE.
 *
 * The development of an improved, consistent API (based on point, normal,
 * and angle) was supported by CEA/DIF - Commissariat a l'Energie Atomique,
 * Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France, and implemented
 * by Philippe Pebay, Kitware SAS 2012.
 *
 * @sa
 * svtkEllipseArcSource
 */

#ifndef svtkArcSource_h
#define svtkArcSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkArcSource : public svtkPolyDataAlgorithm
{
public:
  static svtkArcSource* New();
  svtkTypeMacro(svtkArcSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set position of the first end point.
   */
  svtkSetVector3Macro(Point1, double);
  svtkGetVectorMacro(Point1, double, 3);
  //@}

  //@{
  /**
   * Set position of the other end point.
   */
  svtkSetVector3Macro(Point2, double);
  svtkGetVectorMacro(Point2, double, 3);
  //@}

  //@{
  /**
   * Set position of the center of the circle that defines the arc.
   * Note: you can use the function svtkMath::Solve3PointCircle to
   * find the center from 3 points located on a circle.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the normal vector to the plane of the arc.
   * By default it points in the positive Z direction.
   * Note: This is only used when UseNormalAndAngle is ON.
   */
  svtkSetVector3Macro(Normal, double);
  svtkGetVectorMacro(Normal, double, 3);
  //@}

  //@{
  /**
   * Set polar vector (starting point of the arc).
   * By default it is the unit vector in the positive X direction.
   * Note: This is only used when UseNormalAndAngle is ON.
   */
  svtkSetVector3Macro(PolarVector, double);
  svtkGetVectorMacro(PolarVector, double, 3);
  //@}

  //@{
  /**
   * Arc length (in degrees), beginning at the polar vector.
   * The direction is counterclockwise by default;
   * a negative value draws the arc in the clockwise direction.
   * Note: This is only used when UseNormalAndAngle is ON.
   */
  svtkSetClampMacro(Angle, double, -360.0, 360.0);
  svtkGetMacro(Angle, double);
  //@}

  //@{
  /**
   * Define the number of segments of the polyline that draws the arc.
   * Note: if the resolution is set to 1 (the default value),
   * the arc is drawn as a straight line.
   */
  svtkSetClampMacro(Resolution, int, 1, SVTK_INT_MAX);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * By default the arc spans the shortest angular sector point1 and point2.
   * By setting this to true, the longest angular sector is used instead
   * (i.e. the negative coterminal angle to the shortest one).
   * Note: This is only used when UseNormalAndAngle is OFF. False by default.
   */
  svtkSetMacro(Negative, bool);
  svtkGetMacro(Negative, bool);
  svtkBooleanMacro(Negative, bool);
  //@}

  //@{
  /**
   * Activate the API based on a normal vector, a starting point
   * (polar vector) and an angle defining the arc length.
   * The previous API (which remains the default) allows for inputs that are
   * inconsistent (when Point1 and Point2 are not equidistant from Center)
   * or ambiguous (when Point1, Point2, and Center are aligned).
   * Note: false by default.
   */
  svtkSetMacro(UseNormalAndAngle, bool);
  svtkGetMacro(UseNormalAndAngle, bool);
  svtkBooleanMacro(UseNormalAndAngle, bool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkArcSource(int res = 1);
  ~svtkArcSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double Point1[3];
  double Point2[3];
  double Center[3];
  double Normal[3];
  double PolarVector[3];
  double Angle;
  int Resolution;
  bool Negative;
  bool UseNormalAndAngle;
  int OutputPointsPrecision;

private:
  svtkArcSource(const svtkArcSource&) = delete;
  void operator=(const svtkArcSource&) = delete;
};

#endif
