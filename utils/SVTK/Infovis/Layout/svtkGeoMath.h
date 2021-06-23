/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoMath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkGeoMath
 * @brief   Useful geographic calculations
 *
 *
 * svtkGeoMath provides some useful geographic calculations.
 */

#ifndef svtkGeoMath_h
#define svtkGeoMath_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkObject.h"

class SVTKINFOVISLAYOUT_EXPORT svtkGeoMath : public svtkObject
{
public:
  static svtkGeoMath* New();
  svtkTypeMacro(svtkGeoMath, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns the average radius of the earth in meters.
   */
  static double EarthRadiusMeters() { return 6356750.0; }

  /**
   * Returns the squared distance between two points.
   */
  static double DistanceSquared(double pt0[3], double pt1[3]);

  /**
   * Converts a (longitude, latitude, altitude) triple to
   * world coordinates where the center of the earth is at the origin.
   * Units are in meters.
   * Note that having altitude realtive to sea level causes issues.
   */
  static void LongLatAltToRect(double lla[3], double rect[3]);

protected:
  svtkGeoMath();
  ~svtkGeoMath() override;

private:
  svtkGeoMath(const svtkGeoMath&) = delete;
  void operator=(const svtkGeoMath&) = delete;
};

#endif
