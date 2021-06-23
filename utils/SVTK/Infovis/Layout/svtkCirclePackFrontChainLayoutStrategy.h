/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCirclePackFrontChainLayoutStrategy.h

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
 * @class   svtkCirclePackFrontChainLayoutStrategy
 * @brief   layout a svtkTree into packed circles
 * using the front chain algorithm.
 *
 *
 * svtkCirclePackFrontChainLayoutStrategy assigns circles to each node of the input svtkTree
 * using the front chain algorithm.  The algorithm packs circles by searching a "front
 * chain" of circles around the perimeter of the circles that have already been packed for
 * the current level in the tree hierarchy.  Searching the front chain is in general faster
 * than searching all of the circles that have been packed at the current level.
 *
 * WARNING:  The algorithm tends to break down and produce packings with overlapping
 * circles when there is a large difference in the radii of the circles at a given
 * level of the tree hierarchy.  Roughly on the order a 1000:1 ratio of circle radii.
 *
 * Please see the following reference for more details on the algorithm.
 *
 * Title: "Visualization of large hierarchical data by circle packing"
 * Authors:  Weixin Wang, Hui Wang, Guozhong Dai, Hongan Wang
 * Conference: Proceedings of the SIGCHI conference on Human Factors in computing systems
 * Year: 2006
 *
 */

#ifndef svtkCirclePackFrontChainLayoutStrategy_h
#define svtkCirclePackFrontChainLayoutStrategy_h

#include "svtkCirclePackLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkCirclePackFrontChainLayoutStrategyImplementation;

class SVTKINFOVISLAYOUT_EXPORT svtkCirclePackFrontChainLayoutStrategy
  : public svtkCirclePackLayoutStrategy
{
public:
  static svtkCirclePackFrontChainLayoutStrategy* New();

  svtkTypeMacro(svtkCirclePackFrontChainLayoutStrategy, svtkCirclePackLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout of the input tree, and store the circle
   * bounds of each vertex as a tuple in a data array.
   * (Xcenter, Ycenter, Radius).
   */
  void Layout(svtkTree* inputTree, svtkDataArray* areaArray, svtkDataArray* sizeArray) override;

  //@{
  /**
   * Width and Height define the size of the output window that the
   * circle packing is placed inside.  Defaults to Width 1, Height 1
   */
  svtkGetMacro(Width, int);
  svtkSetMacro(Width, int);
  svtkGetMacro(Height, int);
  svtkSetMacro(Height, int);
  //@}

protected:
  svtkCirclePackFrontChainLayoutStrategy();
  ~svtkCirclePackFrontChainLayoutStrategy() override;

  char* CirclesFieldName;
  int Width;
  int Height;

private:
  svtkCirclePackFrontChainLayoutStrategyImplementation* pimpl; // Private implementation

  svtkCirclePackFrontChainLayoutStrategy(const svtkCirclePackFrontChainLayoutStrategy&) = delete;
  void operator=(const svtkCirclePackFrontChainLayoutStrategy&) = delete;
};

#endif
