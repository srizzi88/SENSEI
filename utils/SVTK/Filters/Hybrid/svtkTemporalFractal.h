/*=========================================================================

  Program:   ParaView
  Module:    svtkTemporalFractal.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalFractal
 * @brief   A source to test AMR data object.
 *
 * svtkTemporalFractal is a collection of uniform grids.  All have the same
 * dimensions. Each block has a different origin and spacing.  It uses
 * mandelbrot to create cell data. The fractal array is scaled to look like a
 * volume fraction.
 *
 * I may also add block id and level as extra cell arrays.
 * This source produces a svtkHierarchicalBoxDataSet when
 * GenerateRectilinearGrids is off, otherwise produces a svtkMultiBlockDataSet.
 */

#ifndef svtkTemporalFractal_h
#define svtkTemporalFractal_h

#include "svtkAlgorithm.h"
#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkSmartPointer.h"        //for ivars

class svtkCompositeDataSet;
class svtkDataSet;
class svtkHierarchicalBoxDataSet;
class svtkIntArray;
class svtkRectilinearGrid;
class svtkUniformGrid;
class TemporalFractalOutputUtil;

class SVTKFILTERSHYBRID_EXPORT svtkTemporalFractal : public svtkAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static svtkTemporalFractal* New();
  svtkTypeMacro(svtkTemporalFractal, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Essentially the iso surface value.  The fractal array is scaled to map
   * this value to 0.5 for use as a volume fraction.
   */
  svtkSetMacro(FractalValue, float);
  svtkGetMacro(FractalValue, float);
  //@}

  //@{
  /**
   * Any blocks touching a predefined line will be subdivided to this level.
   * Other blocks are subdivided so that neighboring blocks only differ
   * by one level.
   */
  svtkSetMacro(MaximumLevel, int);
  svtkGetMacro(MaximumLevel, int);
  //@}

  //@{
  /**
   * XYZ dimensions of cells.
   */
  svtkSetMacro(Dimensions, int);
  svtkGetMacro(Dimensions, int);
  //@}

  //@{
  /**
   * For testing ghost levels.
   */
  svtkSetMacro(GhostLevels, svtkTypeBool);
  svtkGetMacro(GhostLevels, svtkTypeBool);
  svtkBooleanMacro(GhostLevels, svtkTypeBool);
  //@}

  //@{
  /**
   * Generate either rectilinear grids either uniform grids.
   * Default is false.
   */
  svtkSetMacro(GenerateRectilinearGrids, svtkTypeBool);
  svtkGetMacro(GenerateRectilinearGrids, svtkTypeBool);
  svtkBooleanMacro(GenerateRectilinearGrids, svtkTypeBool);
  //@}

  //@{
  /**
   * Limit this source to discrete integer time steps
   * Default is off (continuous)
   */
  svtkSetMacro(DiscreteTimeSteps, svtkTypeBool);
  svtkGetMacro(DiscreteTimeSteps, svtkTypeBool);
  svtkBooleanMacro(DiscreteTimeSteps, svtkTypeBool);
  //@}

  //@{
  /**
   * Make a 2D data set to test.
   */
  svtkSetMacro(TwoDimensional, svtkTypeBool);
  svtkGetMacro(TwoDimensional, svtkTypeBool);
  svtkBooleanMacro(TwoDimensional, svtkTypeBool);
  //@}

  //@{
  /**
   * Test the case when the blocks do not have the same sizes.
   * Adds 2 to the x extent of the far x blocks (level 1).
   */
  svtkSetMacro(Asymmetric, int);
  svtkGetMacro(Asymmetric, int);
  //@}

  //@{
  /**
   * Make the division adaptive or not, defaults to Adaptive
   */
  svtkSetMacro(AdaptiveSubdivision, svtkTypeBool);
  svtkGetMacro(AdaptiveSubdivision, svtkTypeBool);
  svtkBooleanMacro(AdaptiveSubdivision, svtkTypeBool);
  //@}

protected:
  svtkTemporalFractal();
  ~svtkTemporalFractal() override;

  int FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;

  int StartBlock;
  int EndBlock;
  int BlockCount;

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);
  virtual int RequestOneTimeStep(svtkCompositeDataSet* output, svtkInformation* request,
    svtkInformationVector** inputVector, svtkInformationVector* outputVector);
  //@}

  void Traverse(int& blockId, int level, svtkDataObject* output, int x0, int x1, int y0, int y1,
    int z0, int z1, int onFace[6]);

  int LineTest2(float x0, float y0, float z0, float x1, float y1, float z1, double bds[6]);
  int LineTest(float x0, float y0, float z0, float x1, float y1, float z1, double bds[6], int level,
    int target);

  void SetBlockInfo(svtkUniformGrid* grid, int level, int* ext, int onFace[6]);
  void SetRBlockInfo(svtkRectilinearGrid* grid, int level, int* ext, int onFace[6]);

  void AddVectorArray(svtkHierarchicalBoxDataSet* output);
  void AddTestArray(svtkHierarchicalBoxDataSet* output);
  void AddFractalArray(svtkCompositeDataSet* output);
  void AddBlockIdArray(svtkHierarchicalBoxDataSet* output);
  void AddDepthArray(svtkHierarchicalBoxDataSet* output);

  void AddGhostLevelArray(svtkDataSet* grid, int dim[3], int onFace[6]);

  int MandelbrotTest(double x, double y);
  int TwoDTest(double bds[6], int level, int target);

  void CellExtentToBounds(int level, int ext[6], double bds[6]);

  void ExecuteRectilinearMandelbrot(svtkRectilinearGrid* grid, double* ptr);
  double EvaluateSet(double p[4]);
  void GetContinuousIncrements(int extent[6], svtkIdType& incX, svtkIdType& incY, svtkIdType& incZ);

  // Dimensions:
  // Specify blocks relative to this top level block.
  // For now this has to be set before the blocks are defined.
  svtkSetVector3Macro(TopLevelSpacing, double);
  svtkGetVector3Macro(TopLevelSpacing, double);
  svtkSetVector3Macro(TopLevelOrigin, double);
  svtkGetVector3Macro(TopLevelOrigin, double);

  void InternalImageDataCopy(svtkTemporalFractal* src);

  int Asymmetric;
  int MaximumLevel;
  int Dimensions;
  float FractalValue;
  svtkTypeBool GhostLevels;
  svtkIntArray* Levels;
  svtkTypeBool TwoDimensional;
  svtkTypeBool DiscreteTimeSteps;

  // New method of specifying blocks.
  double TopLevelSpacing[3];
  double TopLevelOrigin[3];

  svtkTypeBool GenerateRectilinearGrids;

  double CurrentTime;

  svtkTypeBool AdaptiveSubdivision;
  svtkSmartPointer<TemporalFractalOutputUtil> OutputUtil;

private:
  svtkTemporalFractal(const svtkTemporalFractal&) = delete;
  void operator=(const svtkTemporalFractal&) = delete;
};

#endif
