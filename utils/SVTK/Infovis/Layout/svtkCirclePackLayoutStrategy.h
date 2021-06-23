/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkCirclePackLayoutStrategy.h

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
 * @class   svtkCirclePackLayoutStrategy
 * @brief   abstract superclass for all circle packing
 * layout strategies.
 *
 *
 * All subclasses of this class perform a circle packing layout on a svtkTree.
 * This involves assigning a circle to each vertex in the tree,
 * and placing that information in a data array with three components per
 * tuple representing (Xcenter, Ycenter, Radius).
 *
 * Instances of subclasses of this class may be assigned as the layout
 * strategy to svtkCirclePackLayout
 *
 * @par Thanks:
 * Thanks to Thomas Otahal from Sandia National Laboratories
 * for help developing this class.
 */

#ifndef svtkCirclePackLayoutStrategy_h
#define svtkCirclePackLayoutStrategy_h

#include "svtkInfovisLayoutModule.h" // For export macro
#include "svtkObject.h"

class svtkTree;
class svtkDataArray;

class SVTKINFOVISLAYOUT_EXPORT svtkCirclePackLayoutStrategy : public svtkObject
{
public:
  svtkTypeMacro(svtkCirclePackLayoutStrategy, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the layout of the input tree, and store the circle
   * bounds of each vertex as a tuple in a data array.
   * (Xcenter, Ycenter, Radius).

   * The sizeArray may be nullptr, or may contain the desired
   * size of each vertex in the tree.
   */
  virtual void Layout(svtkTree* inputTree, svtkDataArray* areaArray, svtkDataArray* sizeArray) = 0;

protected:
  svtkCirclePackLayoutStrategy();
  ~svtkCirclePackLayoutStrategy() override;

private:
  svtkCirclePackLayoutStrategy(const svtkCirclePackLayoutStrategy&) = delete;
  void operator=(const svtkCirclePackLayoutStrategy&) = delete;
};

#endif
