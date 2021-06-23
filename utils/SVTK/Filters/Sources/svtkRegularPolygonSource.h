/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRegularPolygonSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRegularPolygonSource
 * @brief   create a regular, n-sided polygon and/or polyline
 *
 * svtkRegularPolygonSource is a source object that creates a single n-sided polygon and/or
 * polyline. The polygon is centered at a specified point, orthogonal to
 * a specified normal, and with a circumscribing radius set by the user. The user can
 * also specify the number of sides of the polygon ranging from [3,N].
 *
 * This object can be used for seeding streamlines or defining regions for clipping/cutting.
 */

#ifndef svtkRegularPolygonSource_h
#define svtkRegularPolygonSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkRegularPolygonSource : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type and printing instance values.
   */
  static svtkRegularPolygonSource* New();
  svtkTypeMacro(svtkRegularPolygonSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the number of sides of the polygon. By default, the number of sides
   * is set to six.
   */
  svtkSetClampMacro(NumberOfSides, int, 3, SVTK_INT_MAX);
  svtkGetMacro(NumberOfSides, int);
  //@}

  //@{
  /**
   * Set/Get the center of the polygon. By default, the center is set at the
   * origin (0,0,0).
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set/Get the normal to the polygon. The ordering of the polygon will be
   * counter-clockwise around the normal (i.e., using the right-hand rule).
   * By default, the normal is set to (0,0,1).
   */
  svtkSetVector3Macro(Normal, double);
  svtkGetVectorMacro(Normal, double, 3);
  //@}

  //@{
  /**
   * Set/Get the radius of the polygon. By default, the radius is set to 0.5.
   */
  svtkSetMacro(Radius, double);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Control whether a polygon is produced. By default, GeneratePolygon is enabled.
   */
  svtkSetMacro(GeneratePolygon, svtkTypeBool);
  svtkGetMacro(GeneratePolygon, svtkTypeBool);
  svtkBooleanMacro(GeneratePolygon, svtkTypeBool);
  //@}

  //@{
  /**
   * Control whether a polyline is produced. By default, GeneratePolyline is enabled.
   */
  svtkSetMacro(GeneratePolyline, svtkTypeBool);
  svtkGetMacro(GeneratePolyline, svtkTypeBool);
  svtkBooleanMacro(GeneratePolyline, svtkTypeBool);
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
  svtkRegularPolygonSource();
  ~svtkRegularPolygonSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int NumberOfSides;
  double Center[3];
  double Normal[3];
  double Radius;
  svtkTypeBool GeneratePolygon;
  svtkTypeBool GeneratePolyline;
  int OutputPointsPrecision;

private:
  svtkRegularPolygonSource(const svtkRegularPolygonSource&) = delete;
  void operator=(const svtkRegularPolygonSource&) = delete;
};

#endif
