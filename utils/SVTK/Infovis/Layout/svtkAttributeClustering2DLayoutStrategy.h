/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAttributeClustering2DLayoutStrategy.h

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
 * @class   svtkAttributeClustering2DLayoutStrategy
 * @brief   a simple fast 2D graph layout
 *
 *
 * This class is a density grid based force directed layout strategy.
 * Also please note that 'fast' is relative to quite slow. :)
 * The layout running time is O(V+E) with an extremely high constant.
 * @par Thanks:
 * Thanks to Godzilla for not eating my computer so that this class
 * could be written.
 */

#ifndef svtkAttributeClustering2DLayoutStrategy_h
#define svtkAttributeClustering2DLayoutStrategy_h

#include "svtkGraphLayoutStrategy.h"
#include "svtkInfovisLayoutModule.h" // For export macro

#include "svtkSmartPointer.h" // Required for smart pointer internal ivars.

class svtkFastSplatter;
class svtkImageData;
class svtkIntArray;
class svtkFloatArray;

class SVTKINFOVISLAYOUT_EXPORT svtkAttributeClustering2DLayoutStrategy : public svtkGraphLayoutStrategy
{
public:
  static svtkAttributeClustering2DLayoutStrategy* New();

  svtkTypeMacro(svtkAttributeClustering2DLayoutStrategy, svtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name of the array on the vertices, whose values will be used for
   * determining clusters.
   */
  svtkGetStringMacro(VertexAttribute);
  void SetVertexAttribute(const char*);
  //@}

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
   * Set/Get the maximum number of iterations to be used.
   * The higher this number, the more iterations through the algorithm
   * is possible, and thus, the more the graph gets modified.
   * The default is '100' for no particular reason
   * Note: The strong recommendation is that you do not change
   * this parameter. :)
   */
  svtkSetClampMacro(MaxNumberOfIterations, int, 0, SVTK_INT_MAX);
  svtkGetMacro(MaxNumberOfIterations, int);
  //@}

  //@{
  /**
   * Set/Get the number of iterations per layout.
   * The only use for this ivar is for the application
   * to do visualizations of the layout before it's complete.
   * The default is '100' to match the default 'MaxNumberOfIterations'
   * Note: Changing this parameter is just fine :)
   */
  svtkSetClampMacro(IterationsPerLayout, int, 0, SVTK_INT_MAX);
  svtkGetMacro(IterationsPerLayout, int);
  //@}

  //@{
  /**
   * Set the initial temperature.  The temperature default is '5'
   * for no particular reason
   * Note: The strong recommendation is that you do not change
   * this parameter. :)
   */
  svtkSetClampMacro(InitialTemperature, float, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(InitialTemperature, float);
  //@}

  //@{
  /**
   * Set/Get the Cool-down rate.
   * The higher this number is, the longer it will take to "cool-down",
   * and thus, the more the graph will be modified. The default is '10'
   * for no particular reason.
   * Note: The strong recommendation is that you do not change
   * this parameter. :)
   */
  svtkSetClampMacro(CoolDownRate, double, 0.01, SVTK_DOUBLE_MAX);
  svtkGetMacro(CoolDownRate, double);
  //@}

  //@{
  /**
   * Manually set the resting distance. Otherwise the
   * distance is computed automatically.
   */
  svtkSetMacro(RestDistance, float);
  svtkGetMacro(RestDistance, float);
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
  svtkAttributeClustering2DLayoutStrategy();
  ~svtkAttributeClustering2DLayoutStrategy() override;

  int MaxNumberOfIterations; // Maximum number of iterations.
  float InitialTemperature;
  float CoolDownRate; // Cool-down rate.  Note:  Higher # = Slower rate.

private:
  // This class 'has a' svtkFastSplatter for the density grid
  svtkSmartPointer<svtkFastSplatter> DensityGrid;
  svtkSmartPointer<svtkImageData> SplatImage;
  svtkSmartPointer<svtkFloatArray> RepulsionArray;
  svtkSmartPointer<svtkFloatArray> AttractionArray;
  svtkSmartPointer<svtkIntArray> EdgeCountArray;

  int RandomSeed;
  int IterationsPerLayout;
  int TotalIterations;
  int LayoutComplete;
  float Temp;
  float RestDistance;
  float CuttingThreshold;
  char* VertexAttribute;

  // Private helper methods
  void GenerateCircularSplat(svtkImageData* splat, int x, int y);
  void GenerateGaussianSplat(svtkImageData* splat, int x, int y);
  void ResolveCoincidentVertices();

  class Internals;
  Internals* Implementation;

  svtkAttributeClustering2DLayoutStrategy(const svtkAttributeClustering2DLayoutStrategy&) = delete;
  void operator=(const svtkAttributeClustering2DLayoutStrategy&) = delete;
};

#endif
