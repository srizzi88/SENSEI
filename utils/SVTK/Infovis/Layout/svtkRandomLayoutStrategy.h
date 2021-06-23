/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomLayoutStrategy.h

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
 * @class   svtkRandomLayoutStrategy
 * @brief   randomly places vertices in 2 or 3 dimensions
 *
 *
 * Assigns points to the vertices of a graph randomly within a bounded range.
 *
 * .SECTION Thanks
 * Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
 * layout capabilities.
 */

#ifndef svtkRandomLayoutStrategy_h
#define svtkRandomLayoutStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class SVTKINFOVISLAYOUT_EXPORT svtkRandomLayoutStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkRandomLayoutStrategy* New();

  svtkTypeMacro(svtkRandomLayoutStrategy, svtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Seed the random number generator used to compute point positions.
   * This has a significant effect on their final positions when
   * the layout is complete.
   */
  svtkSetClampMacro(RandomSeed, int, 0, SVTK_INT_MAX);
  svtkGetMacro(RandomSeed, int);
  //@}

  //@{
  /**
   * Set / get the region in space in which to place the final graph.
   * The GraphBounds only affects the results if AutomaticBoundsComputation
   * is off.
   */
  svtkSetVector6Macro(GraphBounds, double);
  svtkGetVectorMacro(GraphBounds, double, 6);
  //@}

  //@{
  /**
   * Turn on/off automatic graph bounds calculation. If this
   * boolean is off, then the manually specified GraphBounds is used.
   * If on, then the input's bounds us used as the graph bounds.
   */
  svtkSetMacro(AutomaticBoundsComputation, svtkTypeBool);
  svtkGetMacro(AutomaticBoundsComputation, svtkTypeBool);
  svtkBooleanMacro(AutomaticBoundsComputation, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off layout of graph in three dimensions. If off, graph
   * layout occurs in two dimensions. By default, three dimensional
   * layout is on.
   */
  svtkSetMacro(ThreeDimensionalLayout, svtkTypeBool);
  svtkGetMacro(ThreeDimensionalLayout, svtkTypeBool);
  svtkBooleanMacro(ThreeDimensionalLayout, svtkTypeBool);
  //@}

  /**
   * Set the graph to layout.
   */
  void SetGraph(svtkGraph* graph) override;

  /**
   * Perform the random layout.
   */
  void Layout() override;

protected:
  svtkRandomLayoutStrategy();
  ~svtkRandomLayoutStrategy() override;

  int RandomSeed;
  double GraphBounds[6];
  svtkTypeBool AutomaticBoundsComputation;
  svtkTypeBool ThreeDimensionalLayout; // Boolean for a third dimension.
private:
  svtkRandomLayoutStrategy(const svtkRandomLayoutStrategy&) = delete;
  void operator=(const svtkRandomLayoutStrategy&) = delete;
};

#endif
