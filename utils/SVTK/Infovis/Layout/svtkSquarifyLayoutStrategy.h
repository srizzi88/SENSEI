/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSquarifyLayoutStrategy.h

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
 * @class   svtkSquarifyLayoutStrategy
 * @brief   uses the squarify tree map layout algorithm
 *
 *
 * svtkSquarifyLayoutStrategy partitions the space for child vertices into regions
 * that use all available space and are as close to squares as possible.
 * The algorithm also takes into account the relative vertex size.
 *
 * @par Thanks:
 * The squarified tree map algorithm comes from:
 * Bruls, D.M., C. Huizing, J.J. van Wijk. Squarified Treemaps.
 * In: W. de Leeuw, R. van Liere (eds.), Data Visualization 2000,
 * Proceedings of the joint Eurographics and IEEE TCVG Symposium on Visualization,
 * 2000, Springer, Vienna, p. 33-42.
 */

#ifndef svtkSquarifyLayoutStrategy_h
#define svtkSquarifyLayoutStrategy_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkTreeMapLayoutStrategy.h"

class svtkIdList;

class SVTKINFOVISLAYOUT_EXPORT svtkSquarifyLayoutStrategy : public svtkTreeMapLayoutStrategy
{
public:
  static svtkSquarifyLayoutStrategy* New();
  svtkTypeMacro(svtkSquarifyLayoutStrategy, svtkTreeMapLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout of a tree and place the results as 4-tuples in
   * coordsArray (Xmin, Xmax, Ymin, Ymax).
   */
  void Layout(svtkTree* inputTree, svtkDataArray* coordsArray, svtkDataArray* sizeArray) override;

protected:
  svtkSquarifyLayoutStrategy();
  ~svtkSquarifyLayoutStrategy() override;

private:
  void LayoutChildren(svtkTree* tree, svtkDataArray* coordsArray, svtkDataArray* sizeArray,
    svtkIdType nchildren, svtkIdType parent, svtkIdType begin, float minX, float maxX, float minY,
    float maxY);

  svtkSquarifyLayoutStrategy(const svtkSquarifyLayoutStrategy&) = delete;
  void operator=(const svtkSquarifyLayoutStrategy&) = delete;
};

#endif
