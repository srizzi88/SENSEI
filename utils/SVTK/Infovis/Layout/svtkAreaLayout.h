/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAreaLayout.h

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
 * @class   svtkAreaLayout
 * @brief   layout a svtkTree into a tree map
 *
 *
 * svtkAreaLayout assigns sector regions to each vertex in the tree,
 * creating a tree ring.  The data is added as a data array with four
 * components per tuple representing the location and size of the
 * sector using the format (StartAngle, EndAngle, innerRadius, outerRadius).
 *
 * This algorithm relies on a helper class to perform the actual layout.
 * This helper class is a subclass of svtkAreaLayoutStrategy.
 *
 * @par Thanks:
 * Thanks to Jason Shepherd from Sandia National Laboratories
 * for help developing this class.
 */

#ifndef svtkAreaLayout_h
#define svtkAreaLayout_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class svtkAreaLayoutStrategy;

class SVTKINFOVISLAYOUT_EXPORT svtkAreaLayout : public svtkTreeAlgorithm
{
public:
  static svtkAreaLayout* New();
  svtkTypeMacro(svtkAreaLayout, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The array name to use for retrieving the relative size of each vertex.
   * If this array is not found, use constant size for each vertex.
   */
  virtual void SetSizeArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  //@{
  /**
   * The name for the array created for the area for each vertex.
   * The rectangles are stored in a quadruple float array
   * (startAngle, endAngle, innerRadius, outerRadius).
   * For rectangular layouts, this is (minx, maxx, miny, maxy).
   */
  svtkGetStringMacro(AreaArrayName);
  svtkSetStringMacro(AreaArrayName);
  //@}

  //@{
  /**
   * Whether to output a second output tree with vertex locations
   * appropriate for routing bundled edges. Default is on.
   */
  svtkGetMacro(EdgeRoutingPoints, bool);
  svtkSetMacro(EdgeRoutingPoints, bool);
  svtkBooleanMacro(EdgeRoutingPoints, bool);
  //@}

  //@{
  /**
   * The strategy to use when laying out the tree map.
   */
  svtkGetObjectMacro(LayoutStrategy, svtkAreaLayoutStrategy);
  void SetLayoutStrategy(svtkAreaLayoutStrategy* strategy);
  //@}

  /**
   * Get the modification time of the layout algorithm.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Get the vertex whose area contains the point, or return -1
   * if no vertex area covers the point.
   */
  svtkIdType FindVertex(float pnt[2]);

  /**
   * The bounding area information for a certain vertex id.
   */
  void GetBoundingArea(svtkIdType id, float* sinfo);

protected:
  svtkAreaLayout();
  ~svtkAreaLayout() override;

  char* AreaArrayName;
  bool EdgeRoutingPoints;
  char* EdgeRoutingPointsArrayName;
  svtkAreaLayoutStrategy* LayoutStrategy;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkAreaLayout(const svtkAreaLayout&) = delete;
  void operator=(const svtkAreaLayout&) = delete;
};

#endif
