/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSliceAndDiceLayoutStrategy.h

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
 * @class   svtkSliceAndDiceLayoutStrategy
 * @brief   a horizontal and vertical slicing tree map layout
 *
 *
 * Lays out a tree-map alternating between horizontal and vertical slices,
 * taking into account the relative size of each vertex.
 *
 * @par Thanks:
 * Slice and dice algorithm comes from:
 * Shneiderman, B. 1992. Tree visualization with tree-maps: 2-d space-filling approach.
 * ACM Trans. Graph. 11, 1 (Jan. 1992), 92-99.
 */

#ifndef svtkSliceAndDiceLayoutStrategy_h
#define svtkSliceAndDiceLayoutStrategy_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkTreeMapLayoutStrategy.h"

class SVTKINFOVISLAYOUT_EXPORT svtkSliceAndDiceLayoutStrategy : public svtkTreeMapLayoutStrategy
{
public:
  static svtkSliceAndDiceLayoutStrategy* New();

  svtkTypeMacro(svtkSliceAndDiceLayoutStrategy, svtkTreeMapLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout of a tree and place the results as 4-tuples in
   * coordsArray (Xmin, Xmax, Ymin, Ymax).
   */
  void Layout(svtkTree* inputTree, svtkDataArray* coordsArray, svtkDataArray* sizeArray) override;

protected:
  svtkSliceAndDiceLayoutStrategy();
  ~svtkSliceAndDiceLayoutStrategy() override;

private:
  svtkSliceAndDiceLayoutStrategy(const svtkSliceAndDiceLayoutStrategy&) = delete;
  void operator=(const svtkSliceAndDiceLayoutStrategy&) = delete;
};

#endif
