/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoEdgeStrategy.h

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
 * @class   svtkGeoEdgeStrategy
 * @brief   Layout graph edges on a globe as arcs.
 *
 *
 * svtkGeoEdgeStrategy produces arcs for each edge in the input graph.
 * This is useful for viewing lines on a sphere (e.g. the earth).
 * The arcs may "jump" above the sphere's surface using ExplodeFactor.
 */

#ifndef svtkGeoEdgeStrategy_h
#define svtkGeoEdgeStrategy_h

#include "svtkEdgeLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class SVTKINFOVISLAYOUT_EXPORT svtkGeoEdgeStrategy : public svtkEdgeLayoutStrategy
{
public:
  static svtkGeoEdgeStrategy* New();
  svtkTypeMacro(svtkGeoEdgeStrategy, svtkEdgeLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The base radius used to determine the earth's surface.
   * Default is the earth's radius in meters.
   * TODO: Change this to take in a svtkGeoTerrain to get altitude.
   */
  svtkSetMacro(GlobeRadius, double);
  svtkGetMacro(GlobeRadius, double);
  //@}

  //@{
  /**
   * Factor on which to "explode" the arcs away from the surface.
   * A value of 0.0 keeps the values on the surface.
   * Values larger than 0.0 push the arcs away from the surface by a distance
   * proportional to the distance between the points.
   * The default is 0.2.
   */
  svtkSetMacro(ExplodeFactor, double);
  svtkGetMacro(ExplodeFactor, double);
  //@}

  //@{
  /**
   * The number of subdivisions in the arc.
   * The default is 20.
   */
  svtkSetMacro(NumberOfSubdivisions, int);
  svtkGetMacro(NumberOfSubdivisions, int);
  //@}

  /**
   * Perform the layout.
   */
  void Layout() override;

protected:
  svtkGeoEdgeStrategy();
  ~svtkGeoEdgeStrategy() override {}

  double GlobeRadius;
  double ExplodeFactor;
  int NumberOfSubdivisions;

private:
  svtkGeoEdgeStrategy(const svtkGeoEdgeStrategy&) = delete;
  void operator=(const svtkGeoEdgeStrategy&) = delete;
};

#endif
