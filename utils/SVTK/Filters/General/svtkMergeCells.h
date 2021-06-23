/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMergeCells.h

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
 * @class   svtkMergeCells
 * @brief   merges any number of svtkDataSets back into a single
 *   svtkUnstructuredGrid
 *
 *
 *    Designed to work with distributed svtkDataSets, this class will take
 *    svtkDataSets and merge them back into a single svtkUnstructuredGrid.
 *
 *    The svtkPoints object of the unstructured grid will have data type
 *    SVTK_FLOAT if input is not of type svtkPointSet, otherwise it will have same
 *    data type than the input point set.
 *
 *    It is assumed the different DataSets have the same field arrays.  If
 *    the name of a global point ID array is provided, this class will
 *    refrain from including duplicate points in the merged Ugrid.  This
 *    class differs from svtkAppendFilter in these ways: (1) it uses less
 *    memory than that class (which uses memory equal to twice the size
 *    of the final Ugrid) but requires that you know the size of the
 *    final Ugrid in advance (2) this class assumes the individual DataSets have
 *    the same field arrays, while svtkAppendFilter intersects the field
 *    arrays (3) this class knows duplicate points may be appearing in
 *    the DataSets and can filter those out, (4) this class is not a filter.
 */

#ifndef svtkMergeCells_h
#define svtkMergeCells_h

#include "svtkDataSetAttributes.h"    // Needed for FieldList
#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkObject.h"
#include "svtkSmartPointer.h" //fot svtkSmartPointer

class svtkCellData;
class svtkDataSet;
class svtkMergeCellsSTLCloak;
class svtkMergePoints;
class svtkPointData;
class svtkUnstructuredGrid;

class SVTKFILTERSGENERAL_EXPORT svtkMergeCells : public svtkObject
{
public:
  svtkTypeMacro(svtkMergeCells, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkMergeCells* New();

  //@{
  /**
   * Set the svtkUnstructuredGrid object that will become the
   * union of the DataSets specified in MergeDataSet calls.
   * svtkMergeCells assumes this grid is empty at first.
   */
  virtual void SetUnstructuredGrid(svtkUnstructuredGrid*);
  svtkGetObjectMacro(UnstructuredGrid, svtkUnstructuredGrid);
  //@}

  //@{
  /**
   * Specify the total number of cells in the final svtkUnstructuredGrid.
   * Make this call before any call to MergeDataSet().
   */
  svtkSetMacro(TotalNumberOfCells, svtkIdType);
  svtkGetMacro(TotalNumberOfCells, svtkIdType);
  //@}

  //@{
  /**
   * Specify the total number of points in the final svtkUnstructuredGrid
   * Make this call before any call to MergeDataSet().  This is an
   * upper bound, since some points may be duplicates.
   */
  svtkSetMacro(TotalNumberOfPoints, svtkIdType);
  svtkGetMacro(TotalNumberOfPoints, svtkIdType);
  //@}

  //@{
  /**
   * svtkMergeCells attempts eliminate duplicate points when merging
   * data sets.  This is done most efficiently if a global point ID
   * field array is available.  Set the name of the point array if you
   * have one.
   */
  svtkSetMacro(UseGlobalIds, int);
  svtkGetMacro(UseGlobalIds, int);
  svtkBooleanMacro(UseGlobalIds, int);
  //@}

  //@{
  /**
   * svtkMergeCells attempts eliminate duplicate points when merging
   * data sets.  If no global point ID field array name is provided,
   * it will use a point locator to find duplicate points.  You can
   * set a tolerance for that locator here.  The default tolerance
   * is 10e-4.
   */
  svtkSetClampMacro(PointMergeTolerance, float, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(PointMergeTolerance, float);
  //@}

  //@{
  /**
   * svtkMergeCells will detect and filter out duplicate cells if you
   * provide it the name of a global cell ID array.
   */
  svtkSetMacro(UseGlobalCellIds, int);
  svtkGetMacro(UseGlobalCellIds, int);
  svtkBooleanMacro(UseGlobalCellIds, int);
  //@}

  //@{
  /**
   * svtkMergeCells attempts eliminate duplicate points when merging
   * data sets.  If for some reason you don't want it to do this,
   * than MergeDuplicatePointsOff().
   */
  svtkSetMacro(MergeDuplicatePoints, bool);
  svtkGetMacro(MergeDuplicatePoints, bool);
  svtkBooleanMacro(MergeDuplicatePoints, bool);
  //@}

  /**
   * Clear the Locator and set it to nullptr.
   */
  void InvalidateCachedLocator();

  //@{
  /**
   * We need to know the number of different data sets that will
   * be merged into one so we can pre-allocate some arrays.
   * This can be an upper bound, not necessarily exact.
   */
  svtkSetMacro(TotalNumberOfDataSets, int);
  svtkGetMacro(TotalNumberOfDataSets, int);
  //@}

  /**
   * Provide a DataSet to be merged in to the final UnstructuredGrid.
   * This call returns after the merge has completed.  Be sure to call
   * SetTotalNumberOfCells, SetTotalNumberOfPoints, and SetTotalNumberOfDataSets
   * before making this call.  Return 0 if OK, -1 if error.
   */
  int MergeDataSet(svtkDataSet* set);

  /**
   * Call Finish() after merging last DataSet to free unneeded memory and to
   * make sure the ugrid's GetNumberOfPoints() reflects the actual
   * number of points set, not the number allocated.
   */
  void Finish();

protected:
  svtkMergeCells();
  ~svtkMergeCells() override;

  void FreeLists();
  void StartUGrid(svtkDataSet* set);
  svtkIdType* MapPointsToIdsUsingGlobalIds(svtkDataSet* set);
  svtkIdType* MapPointsToIdsUsingLocator(svtkDataSet* set);
  svtkIdType AddNewCellsUnstructuredGrid(svtkDataSet* set, svtkIdType* idMap);
  svtkIdType AddNewCellsDataSet(svtkDataSet* set, svtkIdType* idMap);

  int TotalNumberOfDataSets;

  svtkIdType TotalNumberOfCells;
  svtkIdType TotalNumberOfPoints;

  svtkIdType NumberOfCells; // so far
  svtkIdType NumberOfPoints;

  int UseGlobalIds;     // point, or node, IDs
  int UseGlobalCellIds; // cell IDs

  float PointMergeTolerance;
  bool MergeDuplicatePoints;

  char InputIsUGrid;
  char InputIsPointSet;

  svtkMergeCellsSTLCloak* GlobalIdMap;
  svtkMergeCellsSTLCloak* GlobalCellIdMap;

  svtkDataSetAttributes::FieldList* PointList;
  svtkDataSetAttributes::FieldList* CellList;

  svtkUnstructuredGrid* UnstructuredGrid;

  int NextGrid;

  svtkSmartPointer<svtkMergePoints> Locator;

private:
  svtkMergeCells(const svtkMergeCells&) = delete;
  void operator=(const svtkMergeCells&) = delete;
};
#endif
