/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphLayoutFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphLayoutFilter
 * @brief   nice layout of undirected graphs in 3D
 *
 * svtkGraphLayoutFilter will reposition a network of nodes, connected by
 * lines or polylines, into a more pleasing arrangement. The class
 * implements a simple force-directed placement algorithm
 * (Fruchterman & Reingold "Graph Drawing by Force-directed Placement"
 * Software-Practice and Experience 21(11) 1991).
 *
 * The input to the filter is a svtkPolyData representing the undirected
 * graphs. A graph is represented by a set of polylines and/or lines.
 * The output is also a svtkPolyData, where the point positions have been
 * modified. To use the filter, specify whether you wish the layout to
 * occur in 2D or 3D; the bounds in which the graph should lie (note that you
 * can just use automatic bounds computation); and modify the cool down
 * rate (controls the final process of simulated annealing).
 */

#ifndef svtkGraphLayoutFilter_h
#define svtkGraphLayoutFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkGraphLayoutFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkGraphLayoutFilter* New();

  svtkTypeMacro(svtkGraphLayoutFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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
   * Set/Get the maximum number of iterations to be used.
   * The higher this number, the more iterations through the algorithm
   * is possible, and thus, the more the graph gets modified.
   */
  svtkSetClampMacro(MaxNumberOfIterations, int, 0, SVTK_INT_MAX);
  svtkGetMacro(MaxNumberOfIterations, int);
  //@}

  //@{
  /**
   * Set/Get the Cool-down rate.
   * The higher this number is, the longer it will take to "cool-down",
   * and thus, the more the graph will be modified.
   */
  svtkSetClampMacro(CoolDownRate, double, 0.01, SVTK_DOUBLE_MAX);
  svtkGetMacro(CoolDownRate, double);
  //@}

  // Turn on/off layout of graph in three dimensions. If off, graph
  // layout occurs in two dimensions. By default, three dimensional
  // layout is on.
  svtkSetMacro(ThreeDimensionalLayout, svtkTypeBool);
  svtkGetMacro(ThreeDimensionalLayout, svtkTypeBool);
  svtkBooleanMacro(ThreeDimensionalLayout, svtkTypeBool);

protected:
  svtkGraphLayoutFilter();
  ~svtkGraphLayoutFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double GraphBounds[6];
  svtkTypeBool AutomaticBoundsComputation; // Boolean controls automatic bounds calc.
  int MaxNumberOfIterations;              // Maximum number of iterations.
  double CoolDownRate;                    // Cool-down rate.  Note:  Higher # = Slower rate.
  svtkTypeBool ThreeDimensionalLayout;     // Boolean for a third dimension.
private:
  svtkGraphLayoutFilter(const svtkGraphLayoutFilter&) = delete;
  void operator=(const svtkGraphLayoutFilter&) = delete;
};

#endif
