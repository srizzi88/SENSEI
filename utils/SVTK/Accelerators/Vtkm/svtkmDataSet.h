//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef svtkmDataSet_h
#define svtkmDataSet_h

#include "svtkAcceleratorsSVTKmModule.h" // For export macro
#include "svtkDataSet.h"

#include <memory> // for std::shared_ptr

namespace svtkm
{
namespace cont
{

class DataSet;

}
} // svtkm::cont

class svtkPoints;
class svtkCell;
class svtkGenericCell;

class SVTKACCELERATORSSVTKM_EXPORT svtkmDataSet : public svtkDataSet
{
public:
  svtkTypeMacro(svtkmDataSet, svtkDataSet);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkmDataSet* New();

  void SetVtkmDataSet(const svtkm::cont::DataSet& ds);
  svtkm::cont::DataSet GetVtkmDataSet() const;

  /**
   * Copy the geometric and topological structure of an object. Note that
   * the invoking object and the object pointed to by the parameter ds must
   * be of the same type.
   */
  void CopyStructure(svtkDataSet* ds) override;

  /**
   * Determine the number of points composing the dataset.
   */
  svtkIdType GetNumberOfPoints() override;

  /**
   * Determine the number of cells composing the dataset.
   */
  svtkIdType GetNumberOfCells() override;

  /**
   * Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
   */
  double* GetPoint(svtkIdType ptId) SVTK_SIZEHINT(3) override;

  /**
   * Copy point coordinates into user provided array x[3] for specified
   * point id.
   */
  void GetPoint(svtkIdType id, double x[3]) override;

  using svtkDataSet::GetCell;
  /**
   * Get cell with cellId such that: 0 <= cellId < NumberOfCells.
   */
  svtkCell* GetCell(svtkIdType cellId) override;
  void GetCell(svtkIdType cellId, svtkGenericCell* cell) override;

  /**
   * Get the bounds of the cell with cellId such that:
   * 0 <= cellId < NumberOfCells.
   */
  void GetCellBounds(svtkIdType cellId, double bounds[6]) override;

  /**
   * Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
   */
  int GetCellType(svtkIdType cellId) override;

  /**
   * Topological inquiry to get points defining cell.
   */
  void GetCellPoints(svtkIdType cellId, svtkIdList* ptIds) override;

  /**
   * Topological inquiry to get cells using point.
   */
  void GetPointCells(svtkIdType ptId, svtkIdList* cellIds) override;

  //@{
  /**
   * Locate the closest point to the global coordinate x. Return the
   * point id. If point id < 0; then no point found. (This may arise
   * when point is outside of dataset.)
   */
  svtkIdType FindPoint(double x[3]) override;
  //@}

  /**
   * Locate cell based on global coordinate x and tolerance
   * squared. If cell and cellId is non-nullptr, then search starts from
   * this cell and looks at immediate neighbors.  Returns cellId >= 0
   * if inside, < 0 otherwise.  The parametric coordinates are
   * provided in pcoords[3]. The interpolation weights are returned in
   * weights[]. (The number of weights is equal to the number of
   * points in the found cell). Tolerance is used to control how close
   * the point is to be considered "in" the cell.
   */
  svtkIdType FindCell(double x[3], svtkCell* cell, svtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) override;

  /**
   * This is a version of the above method that can be used with
   * multithreaded applications. A svtkGenericCell must be passed in
   * to be used in internal calls that might be made to GetCell()
   */
  svtkIdType FindCell(double x[3], svtkCell* cell, svtkGenericCell* gencell, svtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;

  /**
   * Reclaim any extra memory used to store data.
   */
  void Squeeze() override;

  /**
   * Compute the data bounding box from data points.
   */
  void ComputeBounds() override;

  /**
   * Restore data object to initial state.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void Initialize() override;

  /**
   * Convenience method returns largest cell size in dataset. This is generally
   * used to allocate memory for supporting data structures.
   */
  int GetMaxCellSize() override;

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value).
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Return the type of data object.
   */
  int GetDataObjectType() override { return SVTK_DATA_SET; }

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(svtkDataObject* src) override;
  void DeepCopy(svtkDataObject* src) override;
  //@}

protected:
  svtkmDataSet();
  ~svtkmDataSet() override;

private:
  svtkmDataSet(const svtkmDataSet&) = delete;
  void operator=(const svtkmDataSet&) = delete;

  struct DataMembers;
  std::shared_ptr<DataMembers> Internals;
};

#endif // svtkmDataSet_h
// SVTK-HeaderTest-Exclude: svtkmDataSet.h
