/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStackedTreeLayoutStrategy.h

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
 * @class   svtkStackedTreeLayoutStrategy
 * @brief   lays out tree in stacked boxes or rings
 *
 *
 * Performs a tree ring layout or "icicle" layout on a tree.
 * This involves assigning a sector region to each vertex in the tree,
 * and placing that information in a data array with four components per
 * tuple representing (innerRadius, outerRadius, startAngle, endAngle).
 *
 * This class may be assigned as the layout strategy to svtkAreaLayout.
 *
 * @par Thanks:
 * Thanks to Jason Shepherd from Sandia National Laboratories
 * for help developing this class.
 */

#ifndef svtkStackedTreeLayoutStrategy_h
#define svtkStackedTreeLayoutStrategy_h

#include "svtkAreaLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkTree;
class svtkDataArray;

class SVTKINFOVISLAYOUT_EXPORT svtkStackedTreeLayoutStrategy : public svtkAreaLayoutStrategy
{
public:
  static svtkStackedTreeLayoutStrategy* New();
  svtkTypeMacro(svtkStackedTreeLayoutStrategy, svtkAreaLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout of the input tree, and store the sector
   * bounds of each vertex as a tuple
   * (innerRadius, outerRadius, startAngle, endAngle)
   * in a data array.
   */
  void Layout(svtkTree* inputTree, svtkDataArray* sectorArray, svtkDataArray* sizeArray) override;

  /**
   * Fill edgeRoutingTree with points suitable for routing edges of
   * an overlaid graph.
   */
  void LayoutEdgePoints(svtkTree* inputTree, svtkDataArray* sectorArray, svtkDataArray* sizeArray,
    svtkTree* edgeRoutingTree) override;

  //@{
  /**
   * Define the tree ring's interior radius.
   */
  svtkSetMacro(InteriorRadius, double);
  svtkGetMacro(InteriorRadius, double);
  //@}

  //@{
  /**
   * Define the thickness of each of the tree rings.
   */
  svtkSetMacro(RingThickness, double);
  svtkGetMacro(RingThickness, double);
  //@}

  //@{
  /**
   * Define the start angle for the root node.
   * NOTE: It is assumed that the root end angle is greater than the
   * root start angle and subtends no more than 360 degrees.
   */
  svtkSetMacro(RootStartAngle, double);
  svtkGetMacro(RootStartAngle, double);
  //@}

  //@{
  /**
   * Define the end angle for the root node.
   * NOTE: It is assumed that the root end angle is greater than the
   * root start angle and subtends no more than 360 degrees.
   */
  svtkSetMacro(RootEndAngle, double);
  svtkGetMacro(RootEndAngle, double);
  //@}

  //@{
  /**
   * Define whether or not rectangular coordinates are being used
   * (as opposed to polar coordinates).
   */
  svtkSetMacro(UseRectangularCoordinates, bool);
  svtkGetMacro(UseRectangularCoordinates, bool);
  svtkBooleanMacro(UseRectangularCoordinates, bool);
  //@}

  //@{
  /**
   * Define whether to reverse the order of the tree stacks from
   * low to high.
   */
  svtkSetMacro(Reverse, bool);
  svtkGetMacro(Reverse, bool);
  svtkBooleanMacro(Reverse, bool);
  //@}

  //@{
  /**
   * The spacing of tree levels in the edge routing tree.
   * Levels near zero give more space
   * to levels near the root, while levels near one (the default)
   * create evenly-spaced levels. Levels above one give more space
   * to levels near the leaves.
   */
  svtkSetMacro(InteriorLogSpacingValue, double);
  svtkGetMacro(InteriorLogSpacingValue, double);
  //@}

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it).
   */
  svtkIdType FindVertex(svtkTree* tree, svtkDataArray* array, float pnt[2]) override;

protected:
  svtkStackedTreeLayoutStrategy();
  ~svtkStackedTreeLayoutStrategy() override;

  float InteriorRadius;
  float RingThickness;
  float RootStartAngle;
  float RootEndAngle;
  bool UseRectangularCoordinates;
  bool Reverse;
  double InteriorLogSpacingValue;

  void ComputeEdgeRoutingPoints(svtkTree* inputTree, svtkDataArray* coordsArray, svtkTree* outputTree);

  void LayoutChildren(svtkTree* tree, svtkDataArray* coordsArray, svtkDataArray* sizeArray,
    svtkIdType nchildren, svtkIdType parent, svtkIdType begin, float parentInnerRad,
    float parentOuterRad, float parentStartAng, float parentEndAng);

private:
  svtkStackedTreeLayoutStrategy(const svtkStackedTreeLayoutStrategy&) = delete;
  void operator=(const svtkStackedTreeLayoutStrategy&) = delete;
};

#endif
