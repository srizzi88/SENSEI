/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeMapLayoutStrategy.h

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
 * @class   svtkTreeMapLayoutStrategy
 * @brief   abstract superclass for all tree map layout strategies
 *
 *
 * All subclasses of this class perform a tree map layout on a tree.
 * This involves assigning a rectangular region to each vertex in the tree,
 * and placing that information in a data array with four components per
 * tuple representing (Xmin, Xmax, Ymin, Ymax).
 *
 * Instances of subclasses of this class may be assigned as the layout
 * strategy to svtkTreeMapLayout
 *
 * @par Thanks:
 * Thanks to Brian Wylie and Ken Moreland from Sandia National Laboratories
 * for help developing this class.
 */

#ifndef svtkTreeMapLayoutStrategy_h
#define svtkTreeMapLayoutStrategy_h

#include "svtkAreaLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkTree;
class svtkDataArray;

class SVTKINFOVISLAYOUT_EXPORT svtkTreeMapLayoutStrategy : public svtkAreaLayoutStrategy
{
public:
  svtkTypeMacro(svtkTreeMapLayoutStrategy, svtkAreaLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Find the vertex at a certain location, or -1 if none found.
   */
  svtkIdType FindVertex(svtkTree* tree, svtkDataArray* areaArray, float pnt[2]) override;

protected:
  svtkTreeMapLayoutStrategy();
  ~svtkTreeMapLayoutStrategy() override;
  void AddBorder(float* boxInfo);

private:
  svtkTreeMapLayoutStrategy(const svtkTreeMapLayoutStrategy&) = delete;
  void operator=(const svtkTreeMapLayoutStrategy&) = delete;
};

#endif
