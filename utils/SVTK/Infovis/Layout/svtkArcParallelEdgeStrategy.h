/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArcParallelEdgeStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkArcParallelEdgeStrategy
 * @brief   routes parallel edges as arcs
 *
 *
 * Parallel edges are drawn as arcs, and self-loops are drawn as ovals.
 * When only one edge connects two vertices it is drawn as a straight line.
 */

#ifndef svtkArcParallelEdgeStrategy_h
#define svtkArcParallelEdgeStrategy_h

#include "svtkEdgeLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class svtkGraph;

class SVTKINFOVISLAYOUT_EXPORT svtkArcParallelEdgeStrategy : public svtkEdgeLayoutStrategy
{
public:
  static svtkArcParallelEdgeStrategy* New();
  svtkTypeMacro(svtkArcParallelEdgeStrategy, svtkEdgeLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out.
   */
  void Layout() override;

  //@{
  /**
   * Get/Set the number of subdivisions on each edge.
   */
  svtkGetMacro(NumberOfSubdivisions, int);
  svtkSetMacro(NumberOfSubdivisions, int);
  //@}

protected:
  svtkArcParallelEdgeStrategy();
  ~svtkArcParallelEdgeStrategy() override;

  int NumberOfSubdivisions;

private:
  svtkArcParallelEdgeStrategy(const svtkArcParallelEdgeStrategy&) = delete;
  void operator=(const svtkArcParallelEdgeStrategy&) = delete;
};

#endif
