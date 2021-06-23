/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAreaLayoutStrategy.h

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
 * @class   svtkAreaLayoutStrategy
 * @brief   abstract superclass for all area layout strategies
 *
 *
 * All subclasses of this class perform a area layout on a tree.
 * This involves assigning a region to each vertex in the tree,
 * and placing that information in a data array with four components per
 * tuple representing (innerRadius, outerRadius, startAngle, endAngle).
 *
 * Instances of subclasses of this class may be assigned as the layout
 * strategy to svtkAreaLayout
 *
 * @par Thanks:
 * Thanks to Jason Shepherd from Sandia National Laboratories
 * for help developing this class.
 */

#ifndef svtkAreaLayoutStrategy_h
#define svtkAreaLayoutStrategy_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkObject.h"

class svtkTree;
class svtkDataArray;

class SVTKINFOVISLAYOUT_EXPORT svtkAreaLayoutStrategy : public svtkObject
{
public:
  svtkTypeMacro(svtkAreaLayoutStrategy, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout of the input tree, and store the sector
   * bounds of each vertex as a tuple in a data array.
   * For radial layout, this is
   * (innerRadius, outerRadius, startAngle, endAngle).
   * For rectangular layout, this is
   * (xmin, xmax, ymin, ymax).

   * The sizeArray may be nullptr, or may contain the desired
   * size of each vertex in the tree.
   */
  virtual void Layout(svtkTree* inputTree, svtkDataArray* areaArray, svtkDataArray* sizeArray) = 0;

  // Modify edgeLayoutTree to have point locations appropriate
  // for routing edges on a graph overlaid on the tree.
  // Layout() is called before this method, so inputTree will contain the
  // layout locations.
  // If you do not override this method,
  // the edgeLayoutTree vertex locations are the same as the input tree.
  virtual void LayoutEdgePoints(
    svtkTree* inputTree, svtkDataArray* areaArray, svtkDataArray* sizeArray, svtkTree* edgeLayoutTree);

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it)
   */
  virtual svtkIdType FindVertex(svtkTree* tree, svtkDataArray* array, float pnt[2]) = 0;

  // Descripiton:
  // The amount that the regions are shrunk as a value from
  // 0.0 (full size) to 1.0 (shrink to nothing).
  svtkSetClampMacro(ShrinkPercentage, double, 0.0, 1.0);
  svtkGetMacro(ShrinkPercentage, double);

protected:
  svtkAreaLayoutStrategy();
  ~svtkAreaLayoutStrategy() override;

  double ShrinkPercentage;

private:
  svtkAreaLayoutStrategy(const svtkAreaLayoutStrategy&) = delete;
  void operator=(const svtkAreaLayoutStrategy&) = delete;
};

#endif
