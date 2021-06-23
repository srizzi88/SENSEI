/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCachingInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCachingInterpolatedVelocityField
 * @brief   Interface for obtaining
 * interpolated velocity values
 *
 * svtkCachingInterpolatedVelocityField acts as a continuous velocity field
 * by performing cell interpolation on the underlying svtkDataSet.
 * This is a concrete sub-class of svtkFunctionSet with
 * NumberOfIndependentVariables = 4 (x,y,z,t) and
 * NumberOfFunctions = 3 (u,v,w). Normally, every time an evaluation
 * is performed, the cell which contains the point (x,y,z) has to
 * be found by calling FindCell. This is a computationally expensive
 * operation. In certain cases, the cell search can be avoided or shortened
 * by providing a guess for the cell id. For example, in streamline
 * integration, the next evaluation is usually in the same or a neighbour
 * cell. For this reason, svtkCachingInterpolatedVelocityField stores the last
 * cell id. If caching is turned on, it uses this id as the starting point.
 *
 * @warning
 * svtkCachingInterpolatedVelocityField is not thread safe. A new instance should
 * be created by each thread.
 *
 * @sa
 * svtkFunctionSet svtkStreamTracer
 *
 * @todo
 * Need to clean up style to match svtk/Kitware standards. Please help.
 */

#ifndef svtkCachingInterpolatedVelocityField_h
#define svtkCachingInterpolatedVelocityField_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkFunctionSet.h"
#include "svtkSmartPointer.h" // this is allowed

#include <vector> // we need them

class svtkDataSet;
class svtkDataArray;
class svtkPointData;
class svtkGenericCell;
class svtkAbstractCellLocator;

//---------------------------------------------------------------------------
class IVFDataSetInfo;
//---------------------------------------------------------------------------
class IVFCacheList : public std::vector<IVFDataSetInfo>
{
};
//---------------------------------------------------------------------------

class SVTKFILTERSFLOWPATHS_EXPORT svtkCachingInterpolatedVelocityField : public svtkFunctionSet
{
public:
  svtkTypeMacro(svtkCachingInterpolatedVelocityField, svtkFunctionSet);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a svtkCachingInterpolatedVelocityField with no initial data set.
   * LastCellId is set to -1.
   */
  static svtkCachingInterpolatedVelocityField* New();

  using Superclass::FunctionValues;
  //@{
  /**
   * Evaluate the velocity field, f={u,v,w}, at {x, y, z}.
   * returns 1 if valid, 0 if test failed
   */
  int FunctionValues(double* x, double* f) override;
  virtual int InsideTest(double* x);
  //@}

  /**
   * Add a dataset used by the interpolation function evaluation.
   */
  virtual void SetDataSet(
    int I, svtkDataSet* dataset, bool staticdataset, svtkAbstractCellLocator* locator);

  //@{
  /**
   * If you want to work with an arbitrary vector array, then set its name
   * here. By default this in nullptr and the filter will use the active vector
   * array.
   */
  svtkGetStringMacro(VectorsSelection);
  void SelectVectors(const char* fieldName) { this->SetVectorsSelection(fieldName); }
  //@}

  /**
   * Set LastCellId to c and LastCacheIndex datasetindex, cached from last evaluation.
   * If c isn't -1 then the corresponding cell is stored in Cache->Cell.
   * These values should be valid or an assertion will be triggered.
   */
  void SetLastCellInfo(svtkIdType c, int datasetindex);

  /**
   * Set LastCellId to -1 and Cache to nullptr so that the next
   * search does not start from the previous cell.
   */
  void ClearLastCellInfo();

  //@{
  /**
   * Returns the interpolation weights/pcoords cached from last evaluation
   * if the cached cell is valid (returns 1). Otherwise, it does not
   * change w and returns 0.
   */
  int GetLastWeights(double* w);
  int GetLastLocalCoordinates(double pcoords[3]);
  //@}

  //@{
  /**
   * Caching statistics.
   */
  svtkGetMacro(CellCacheHit, int);
  svtkGetMacro(DataSetCacheHit, int);
  svtkGetMacro(CacheMiss, int);
  //@}

protected:
  svtkCachingInterpolatedVelocityField();
  ~svtkCachingInterpolatedVelocityField() override;

  svtkGenericCell* TempCell;
  int CellCacheHit;
  int DataSetCacheHit;
  int CacheMiss;
  int LastCacheIndex;
  int LastCellId;
  IVFDataSetInfo* Cache;
  IVFCacheList CacheList;
  char* VectorsSelection;

  std::vector<double> Weights;

  svtkSetStringMacro(VectorsSelection);

  // private versions which work on the passed dataset/cache
  // these do the real computation
  int FunctionValues(IVFDataSetInfo* cache, double* x, double* f);
  int InsideTest(IVFDataSetInfo* cache, double* x);

  friend class svtkTemporalInterpolatedVelocityField;
  //@{
  /**
   * If all weights have been computed (parametric coords etc all valid)
   * then we can quickly interpolate a scalar/vector using the known weights
   * and the generic cell which has been stored.
   * This function is primarily reserved for use by
   * svtkTemporalInterpolatedVelocityField
   */
  void FastCompute(IVFDataSetInfo* cache, double f[3]);
  bool InterpolatePoint(svtkPointData* outPD, svtkIdType outIndex);
  bool InterpolatePoint(
    svtkCachingInterpolatedVelocityField* inCIVF, svtkPointData* outPD, svtkIdType outIndex);
  svtkGenericCell* GetLastCell();
  //@}

private:
  svtkCachingInterpolatedVelocityField(const svtkCachingInterpolatedVelocityField&) = delete;
  void operator=(const svtkCachingInterpolatedVelocityField&) = delete;
};

//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// IVFDataSetInfo
///////////////////////////////////////////////////////////////////////////////
#ifndef DOXYGEN_SHOULD_SKIP_THIS
//

//
class IVFDataSetInfo
{
public:
  svtkSmartPointer<svtkDataSet> DataSet;
  svtkSmartPointer<svtkAbstractCellLocator> BSPTree;
  svtkSmartPointer<svtkGenericCell> Cell;
  double PCoords[3];
  float* VelocityFloat;
  double* VelocityDouble;
  double Tolerance;
  bool StaticDataSet;
  IVFDataSetInfo();
  IVFDataSetInfo(const IVFDataSetInfo& ivfci);
  IVFDataSetInfo& operator=(const IVFDataSetInfo& ivfci);
  void SetDataSet(
    svtkDataSet* data, char* velocity, bool staticdataset, svtkAbstractCellLocator* locator);
  //
  static const double TOLERANCE_SCALE;
};

//

//

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif
