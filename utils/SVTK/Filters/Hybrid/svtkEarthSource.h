/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEarthSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEarthSource
 * @brief   create the continents of the Earth as a sphere
 *
 * svtkEarthSource creates a spherical rendering of the geographical shapes
 * of the major continents of the earth. The OnRatio determines
 * how much of the data is actually used. The radius defines the radius
 * of the sphere at which the continents are placed. Obtains data from
 * an embedded array of coordinates.
 */

#ifndef svtkEarthSource_h
#define svtkEarthSource_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSHYBRID_EXPORT svtkEarthSource : public svtkPolyDataAlgorithm
{
public:
  static svtkEarthSource* New();
  svtkTypeMacro(svtkEarthSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set radius of earth.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Turn on every nth entity. This controls how much detail the model
   * will have. The maximum ratio is sixteen. (The smaller OnRatio, the more
   * detail there is.)
   */
  svtkSetClampMacro(OnRatio, int, 1, 16);
  svtkGetMacro(OnRatio, int);
  //@}

  //@{
  /**
   * Turn on/off drawing continents as filled polygons or as wireframe outlines.
   * Warning: some graphics systems will have trouble with the very large, concave
   * filled polygons. Recommend you use OutlienOn (i.e., disable filled polygons)
   * for now.
   */
  svtkSetMacro(Outline, svtkTypeBool);
  svtkGetMacro(Outline, svtkTypeBool);
  svtkBooleanMacro(Outline, svtkTypeBool);
  //@}

protected:
  svtkEarthSource();
  ~svtkEarthSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Radius;
  int OnRatio;
  svtkTypeBool Outline;

private:
  svtkEarthSource(const svtkEarthSource&) = delete;
  void operator=(const svtkEarthSource&) = delete;
};

#endif
