/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkForceDirectedLayoutStrategy.h

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
 * @class   svtkForceDirectedLayoutStrategy
 * @brief   a force directed graph layout algorithm
 *
 *
 * Lays out a graph in 2D or 3D using a force-directed algorithm.
 * The user may specify whether to layout the graph randomly initially,
 * the bounds, the number of dimensions (2 or 3), and the cool-down rate.
 *
 * @par Thanks:
 * Thanks to Brian Wylie for adding functionality for allowing this layout
 * to be incremental.
 */

#ifndef svtkForceDirectedLayoutStrategy_h
#define svtkForceDirectedLayoutStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

class SVTKINFOVISLAYOUT_EXPORT svtkForceDirectedLayoutStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkForceDirectedLayoutStrategy* New();

  svtkTypeMacro(svtkForceDirectedLayoutStrategy, svtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Seed the random number generator used to jitter point positions.
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
   * Set/Get the maximum number of iterations to be used.
   * The higher this number, the more iterations through the algorithm
   * is possible, and thus, the more the graph gets modified.
   * The default is '50' for no particular reason
   */
  svtkSetClampMacro(MaxNumberOfIterations, int, 0, SVTK_INT_MAX);
  svtkGetMacro(MaxNumberOfIterations, int);
  //@}

  //@{
  /**
   * Set/Get the number of iterations per layout.
   * The only use for this ivar is for the application
   * to do visualizations of the layout before it's complete.
   * The default is '50' to match the default 'MaxNumberOfIterations'
   */
  svtkSetClampMacro(IterationsPerLayout, int, 0, SVTK_INT_MAX);
  svtkGetMacro(IterationsPerLayout, int);
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

  //@{
  /**
   * Turn on/off layout of graph in three dimensions. If off, graph
   * layout occurs in two dimensions. By default, three dimensional
   * layout is off.
   */
  svtkSetMacro(ThreeDimensionalLayout, svtkTypeBool);
  svtkGetMacro(ThreeDimensionalLayout, svtkTypeBool);
  svtkBooleanMacro(ThreeDimensionalLayout, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off use of random positions within the graph bounds as initial points.
   */
  svtkSetMacro(RandomInitialPoints, svtkTypeBool);
  svtkGetMacro(RandomInitialPoints, svtkTypeBool);
  svtkBooleanMacro(RandomInitialPoints, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the initial temperature.  If zero (the default) , the initial temperature
   * will be computed automatically.
   */
  svtkSetClampMacro(InitialTemperature, float, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(InitialTemperature, float);
  //@}

  /**
   * This strategy sets up some data structures
   * for faster processing of each Layout() call
   */
  void Initialize() override;

  /**
   * This is the layout method where the graph that was
   * set in SetGraph() is laid out. The method can either
   * entirely layout the graph or iteratively lay out the
   * graph. If you have an iterative layout please implement
   * the IsLayoutComplete() method.
   */
  void Layout() override;

  /**
   * I'm an iterative layout so this method lets the caller
   * know if I'm done laying out the graph
   */
  int IsLayoutComplete() override { return this->LayoutComplete; }

protected:
  svtkForceDirectedLayoutStrategy();
  ~svtkForceDirectedLayoutStrategy() override;

  double GraphBounds[6];
  svtkTypeBool AutomaticBoundsComputation; // Boolean controls automatic bounds calc.
  int MaxNumberOfIterations;              // Maximum number of iterations.
  double CoolDownRate;                    // Cool-down rate.  Note:  Higher # = Slower rate.
  double InitialTemperature;
  svtkTypeBool ThreeDimensionalLayout; // Boolean for a third dimension.
  svtkTypeBool RandomInitialPoints;    // Boolean for having random points
private:
  // A vertex contains a position and a displacement.
  typedef struct
  {
    double x[3];
    double d[3];
  } svtkLayoutVertex;

  // An edge consists of two vertices joined together.
  // This struct acts as a "pointer" to those two vertices.
  typedef struct
  {
    int t;
    int u;
  } svtkLayoutEdge;

  int RandomSeed;
  int IterationsPerLayout;
  int TotalIterations;
  int LayoutComplete;
  double Temp;
  double optDist;
  svtkLayoutVertex* v;
  svtkLayoutEdge* e;

  svtkForceDirectedLayoutStrategy(const svtkForceDirectedLayoutStrategy&) = delete;
  void operator=(const svtkForceDirectedLayoutStrategy&) = delete;
};

#endif
