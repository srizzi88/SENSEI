/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoxLayoutStrategy.h

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
 * @class   svtkBoxLayoutStrategy
 * @brief   a tree map layout that puts vertices in square-ish boxes
 *
 *
 * svtkBoxLayoutStrategy recursively partitions the space for children vertices
 * in a tree-map into square regions (or regions very close to a square).
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for creating this class.
 */

#ifndef svtkBoxLayoutStrategy_h
#define svtkBoxLayoutStrategy_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkTreeMapLayoutStrategy.h"

class SVTKINFOVISLAYOUT_EXPORT svtkBoxLayoutStrategy : public svtkTreeMapLayoutStrategy
{
public:
  static svtkBoxLayoutStrategy* New();

  svtkTypeMacro(svtkBoxLayoutStrategy, svtkTreeMapLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout of a tree and place the results as 4-tuples in
   * coordsArray (Xmin, Xmax, Ymin, Ymax).
   */
  void Layout(svtkTree* inputTree, svtkDataArray* coordsArray, svtkDataArray* sizeArray) override;

protected:
  svtkBoxLayoutStrategy();
  ~svtkBoxLayoutStrategy() override;

private:
  void LayoutChildren(svtkTree* inputTree, svtkDataArray* coordsArray, svtkIdType parentId,
    float parentMinX, float parentMaxX, float parentMinY, float parentMaxY);

  svtkBoxLayoutStrategy(const svtkBoxLayoutStrategy&) = delete;
  void operator=(const svtkBoxLayoutStrategy&) = delete;
};

#endif
