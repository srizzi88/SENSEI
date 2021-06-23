/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMaskPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMaskPoints
 * @brief   selectively filter points
 *
 * svtkMaskPoints is a filter that passes through points and point attributes
 * from input dataset. (Other geometry is not passed through.) It is
 * possible to mask every nth point, and to specify an initial offset
 * to begin masking from.
 * It is possible to also generate different random selections
 * (jittered strides, real random samples, and spatially stratified
 * random samples) from the input data.
 * The filter can also generate vertices (topological
 * primitives) as well as points. This is useful because vertices are
 * rendered while points are not.
 */

#ifndef svtkMaskPoints_h
#define svtkMaskPoints_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkMaskPoints : public svtkPolyDataAlgorithm
{
public:
  static svtkMaskPoints* New();
  svtkTypeMacro(svtkMaskPoints, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Turn on every nth point (strided sampling), ignored by random modes.
   */
  svtkSetClampMacro(OnRatio, int, 1, SVTK_INT_MAX);
  svtkGetMacro(OnRatio, int);
  //@}

  //@{
  /**
   * Limit the number of points that can be passed through (i.e.,
   * sets the output sample size).
   */
  svtkSetClampMacro(MaximumNumberOfPoints, svtkIdType, 0, SVTK_ID_MAX);
  svtkGetMacro(MaximumNumberOfPoints, svtkIdType);
  //@}

  //@{
  /**
   * Start sampling with this point. Ignored by certain random modes.
   */
  svtkSetClampMacro(Offset, svtkIdType, 0, SVTK_ID_MAX);
  svtkGetMacro(Offset, svtkIdType);
  //@}

  //@{
  /**
   * Special flag causes randomization of point selection.
   */
  svtkSetMacro(RandomMode, svtkTypeBool);
  svtkGetMacro(RandomMode, svtkTypeBool);
  svtkBooleanMacro(RandomMode, svtkTypeBool);
  //@}

  //@{
  /**
   * Special mode selector that switches between random mode types.
   * 0 - randomized strides: randomly strides through the data (default);
   * fairly certain that this is not a statistically random sample
   * because the output depends on the order of the input and
   * the input points do not have an equal chance to appear in the output
   * (plus Vitter's incremental random algorithms are more complex
   * than this, while not a proof it is good indication this isn't
   * a statistically random sample - the closest would be algorithm S)
   * 1 - random sample: create a statistically random sample using Vitter's
   * incremental algorithm D without A described in Vitter
   * "Faster Mthods for Random Sampling", Communications of the ACM
   * Volume 27, Issue 7, 1984
   * (OnRatio and Offset are ignored) O(sample size)
   * 2 - spatially stratified random sample: create a spatially
   * stratified random sample using the first method described in
   * Woodring et al. "In-situ Sampling of a Large-Scale Particle
   * Simulation for Interactive Visualization and Analysis",
   * Computer Graphics Forum, 2011 (EuroVis 2011).
   * (OnRatio and Offset are ignored) O(N log N)
   */
  svtkSetClampMacro(RandomModeType, int, 0, 2);
  svtkGetMacro(RandomModeType, int);
  //@}

  //@{
  /**
   * THIS ONLY WORKS WITH THE PARALLEL IMPLEMENTATION svtkPMaskPoints RUNNING
   * IN PARALLEL.
   * NOTHING WILL CHANGE IF THIS IS NOT THE PARALLEL svtkPMaskPoints.
   * Determines whether maximum number of points is taken per processor
   * (default) or if the maximum number of points is proportionally
   * taken across processors (i.e., number of points per
   * processor = points on a processor * maximum number of points /
   * total points across all processors).  In the first case,
   * the total number of points = maximum number of points *
   * number of processors.  In the second case, the total number of
   * points = maximum number of points.
   */
  svtkSetMacro(ProportionalMaximumNumberOfPoints, svtkTypeBool);
  svtkGetMacro(ProportionalMaximumNumberOfPoints, svtkTypeBool);
  svtkBooleanMacro(ProportionalMaximumNumberOfPoints, svtkTypeBool);
  //@}

  //@{
  /**
   * Generate output polydata vertices as well as points. A useful
   * convenience method because vertices are drawn (they are topology) while
   * points are not (they are geometry). By default this method is off.
   */
  svtkSetMacro(GenerateVertices, svtkTypeBool);
  svtkGetMacro(GenerateVertices, svtkTypeBool);
  svtkBooleanMacro(GenerateVertices, svtkTypeBool);
  //@}

  //@{
  /**
   * When vertex generation is enabled, by default vertices are produced
   * as multi-vertex cells (more than one per cell), if you wish to have
   * a single vertex per cell, enable this flag.
   */
  svtkSetMacro(SingleVertexPerCell, svtkTypeBool);
  svtkGetMacro(SingleVertexPerCell, svtkTypeBool);
  svtkBooleanMacro(SingleVertexPerCell, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkMaskPoints();
  ~svtkMaskPoints() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int OnRatio;      // every OnRatio point is on; all others are off.
  svtkIdType Offset; // offset (or starting point id)
  int RandomMode;   // turn on/off randomization
  svtkIdType MaximumNumberOfPoints;
  svtkTypeBool GenerateVertices; // generate polydata verts
  svtkTypeBool SingleVertexPerCell;
  int RandomModeType; // choose the random sampling mode
  svtkTypeBool ProportionalMaximumNumberOfPoints;
  int OutputPointsPrecision;

  virtual void InternalScatter(unsigned long*, unsigned long*, int, int) {}
  virtual void InternalGather(unsigned long*, unsigned long*, int, int) {}
  virtual int InternalGetNumberOfProcesses() { return 1; }
  virtual int InternalGetLocalProcessId() { return 0; }
  virtual void InternalSplitController(int, int) {}
  virtual void InternalResetController() {}
  virtual void InternalBarrier() {}
  unsigned long GetLocalSampleSize(svtkIdType, int);

private:
  svtkMaskPoints(const svtkMaskPoints&) = delete;
  void operator=(const svtkMaskPoints&) = delete;
};

#endif
