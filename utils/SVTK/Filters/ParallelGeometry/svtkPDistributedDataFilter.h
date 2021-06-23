/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDistributedDataFilter.h

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
 * @class   svtkPDistributedDataFilter
 * @brief   Distribute data among processors
 *
 *
 * This filter redistributes data among processors in a parallel
 * application into spatially contiguous svtkUnstructuredGrids.
 * The execution model anticipated is that all processes read in
 * part of a large svtkDataSet. Each process sets the input of
 * filter to be that DataSet. When executed, this filter builds
 * in parallel a k-d tree, decomposing the space occupied by the
 * distributed DataSet into spatial regions.  It assigns each
 * spatial region to a processor.  The data is then redistributed
 * and the output is a single svtkUnstructuredGrid containing the
 * cells in the process' assigned regions.
 *
 * This filter is sometimes called "D3" for "distributed data decomposition".
 *
 * Enhancement: You can set the k-d tree decomposition, rather than
 * have D3 compute it.  This allows you to divide a dataset using
 * the decomposition computed for another dataset.  Obtain a description
 * of the k-d tree cuts this way:
 *
 * @code{cpp}
 *    svtkBSPCuts *cuts = D3Object1->GetCuts()
 * @endcode
 *
 * And set it this way:
 *
 * @code{cpp}
 *     D3Object2->SetCuts(cuts)
 * @endcode
 *
 *
 * It is desirable to have a field array of global node IDs
 * for two reasons:
 *
 * 1. When merging together sub grids that were distributed
 *    across processors, global node IDs can be used to remove
 *    duplicate points and significantly reduce the size of the
 *    resulting output grid.  If no such array is available,
 *    D3 will use a tolerance to merge points, which is much
 *    slower.
 *
 * 2. If ghost cells have been requested, D3 requires a
 *    global node ID array in order to request and transfer
 *    ghost cells in parallel among the processors.  If there
 *    is no global node ID array, D3 will in parallel create
 *    a global node ID array, and the time to do this can be
 *    significant.
 *
 * D3 uses `svtkPointData::GetGlobalIds` to access global
 * node ids from the input. If none is found,
 * and ghost cells have been requested, D3 will create a
 * temporary global node ID array before acquiring ghost cells.
 *
 * It is also desirable to have global element IDs (svtkCellData::GetGlobalIds).
 * However, if they don't exist D3 can create them relatively quickly.
 *
 * @warning
 * The Execute() method must be called by all processes in the
 * parallel application, or it will hang.  If you are not certain
 * that your pipeline will execute identically on all processors,
 * you may want to use this filter in an explicit execution mode.
 *
 * @sa
 * svtkKdTree svtkPKdTree svtkBSPCuts
 */

#ifndef svtkPDistributedDataFilter_h
#define svtkPDistributedDataFilter_h

#include "svtkDistributedDataFilter.h"
#include "svtkFiltersParallelGeometryModule.h" // For export macro

class svtkBSPCuts;
class svtkDataArray;
class svtkFloatArray;
class svtkIdList;
class svtkIdTypeArray;
class svtkIntArray;
class svtkMultiProcessController;
class svtkPDistributedDataFilterSTLCloak;
class svtkPKdTree;
class svtkUnstructuredGrid;

class SVTKFILTERSPARALLELGEOMETRY_EXPORT svtkPDistributedDataFilter : public svtkDistributedDataFilter
{
public:
  svtkTypeMacro(svtkPDistributedDataFilter, svtkDistributedDataFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPDistributedDataFilter* New();

protected:
  svtkPDistributedDataFilter();
  ~svtkPDistributedDataFilter() override;

  /**
   * Build a svtkUnstructuredGrid for a spatial region from the
   * data distributed across processes.  Execute() must be called
   * by all processes, or it will hang.
   */

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void SingleProcessExecute(svtkDataSet* input, svtkUnstructuredGrid* output);

  /**
   * Implementation for request data.
   */
  int RequestDataInternal(svtkDataSet* input, svtkUnstructuredGrid* output);

private:
  enum
  {
    DeleteNo = 0,
    DeleteYes = 1
  };

  enum
  {
    DuplicateCellsNo = 0,
    DuplicateCellsYes = 1
  };

  enum
  {
    GhostCellsNo = 0,
    GhostCellsYes = 1
  };

  enum
  {
    UnsetGhostLevel = 99
  };

  /**
   * ?
   */
  int PartitionDataAndAssignToProcesses(svtkDataSet* set);

  /**
   * ?
   */
  svtkUnstructuredGrid* RedistributeDataSet(
    svtkDataSet* set, svtkDataSet* input, int filterOutDuplicateCells);

  /**
   * ?
   */
  int ClipGridCells(svtkUnstructuredGrid* grid);

  /**
   * ?
   */
  svtkUnstructuredGrid* AcquireGhostCells(svtkUnstructuredGrid* grid);

  /**
   * ?
   */
  void ComputeMyRegionBounds();

  /**
   * ?
   */
  int CheckFieldArrayTypes(svtkDataSet* set);

  /**
   * If any processes have 0 cell input data sets, then
   * spread the input data sets around (quickly) before formal
   * redistribution.
   */
  svtkDataSet* TestFixTooFewInputFiles(svtkDataSet* input, int& duplicateCells);

  /**
   * ?
   */
  svtkUnstructuredGrid* MPIRedistribute(
    svtkDataSet* in, svtkDataSet* input, int filterOutDuplicateCells);

  /**
   * ?
   */
  svtkIdList** GetCellIdsForProcess(int proc, int* nlists);

  /**
   * Fills in the Source and Target arrays which contain a schedule to allow
   * each processor to talk to every other.
   */
  void SetUpPairWiseExchange();

  //@{
  /**
   * ?
   */
  void FreeIntArrays(svtkIdTypeArray** ar);
  static void FreeIdLists(svtkIdList** lists, int nlists);
  static svtkIdType GetIdListSize(svtkIdList** lists, int nlists);
  //@}

  //@{
  /**
   * This transfers counts (array sizes) between processes.
   */
  svtkIdTypeArray* ExchangeCounts(svtkIdType myCount, int tag);
  svtkIdTypeArray* ExchangeCountsLean(svtkIdType myCount, int tag);
  svtkIdTypeArray* ExchangeCountsFast(svtkIdType myCount, int tag);
  //@}

  //@{
  /**
   * This transfers id valued data arrays between processes.
   */
  svtkIdTypeArray** ExchangeIdArrays(svtkIdTypeArray** arIn, int deleteSendArrays, int tag);
  svtkIdTypeArray** ExchangeIdArraysLean(svtkIdTypeArray** arIn, int deleteSendArrays, int tag);
  svtkIdTypeArray** ExchangeIdArraysFast(svtkIdTypeArray** arIn, int deleteSendArrays, int tag);
  //@}

  //@{
  /**
   * This transfers float valued data arrays between processes.
   */
  svtkFloatArray** ExchangeFloatArrays(svtkFloatArray** myArray, int deleteSendArrays, int tag);
  svtkFloatArray** ExchangeFloatArraysLean(svtkFloatArray** myArray, int deleteSendArrays, int tag);
  svtkFloatArray** ExchangeFloatArraysFast(svtkFloatArray** myArray, int deleteSendArrays, int tag);
  //@}

  //@{
  /**
   * ?
   */
  svtkUnstructuredGrid* ExchangeMergeSubGrids(svtkIdList** cellIds, int deleteCellIds,
    svtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells, int ghostCellFlag, int tag);
  svtkUnstructuredGrid* ExchangeMergeSubGrids(svtkIdList*** cellIds, int* numLists, int deleteCellIds,
    svtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells, int ghostCellFlag, int tag);
  svtkUnstructuredGrid* ExchangeMergeSubGridsLean(svtkIdList*** cellIds, int* numLists,
    int deleteCellIds, svtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells,
    int ghostCellFlag, int tag);
  svtkUnstructuredGrid* ExchangeMergeSubGridsFast(svtkIdList*** cellIds, int* numLists,
    int deleteCellIds, svtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells,
    int ghostCellFlag, int tag);
  //@}

  //@{
  /**
   * ?
   */
  char* MarshallDataSet(svtkUnstructuredGrid* extractedGrid, svtkIdType& size);
  svtkUnstructuredGrid* UnMarshallDataSet(char* buf, svtkIdType size);
  //@}

  //@{
  /**
   * ?
   */
  void ClipCellsToSpatialRegion(svtkUnstructuredGrid* grid);
#if 0
  void ClipWithVtkClipDataSet(svtkUnstructuredGrid *grid, double *bounds,
           svtkUnstructuredGrid **outside, svtkUnstructuredGrid **inside);
#endif
  //@}

  void ClipWithBoxClipDataSet(svtkUnstructuredGrid* grid, double* bounds,
    svtkUnstructuredGrid** outside, svtkUnstructuredGrid** inside);

  //@{
  /**
   * Accessors to the "GLOBALID" point and cell arrays of the dataset.
   * Global ids are used by D3 to uniquely name all points and cells
   * so that after shuffling data between processors, redundant information
   * can be quickly eliminated.
   */
  svtkIdTypeArray* GetGlobalNodeIdArray(svtkDataSet* set);
  svtkIdType* GetGlobalNodeIds(svtkDataSet* set);
  svtkIdTypeArray* GetGlobalElementIdArray(svtkDataSet* set);
  svtkIdType* GetGlobalElementIds(svtkDataSet* set);
  int AssignGlobalNodeIds(svtkUnstructuredGrid* grid);
  int AssignGlobalElementIds(svtkDataSet* in);
  svtkIdTypeArray** FindGlobalPointIds(svtkFloatArray** ptarray, svtkIdTypeArray* ids,
    svtkUnstructuredGrid* grid, svtkIdType& numUniqueMissingPoints);
  //@}

  /**
   * ?
   */
  svtkIdTypeArray** MakeProcessLists(
    svtkIdTypeArray** pointIds, svtkPDistributedDataFilterSTLCloak* procs);

  /**
   * ?
   */
  svtkIdList** BuildRequestedGrids(svtkIdTypeArray** globalPtIds, svtkUnstructuredGrid* grid,
    svtkPDistributedDataFilterSTLCloak* ptIdMap);

  //@{
  /**
   * ?
   */
  int InMySpatialRegion(float x, float y, float z);
  int InMySpatialRegion(double x, double y, double z);
  int StrictlyInsideMyBounds(float x, float y, float z);
  int StrictlyInsideMyBounds(double x, double y, double z);
  //@}

  //@{
  /**
   * ?
   */
  svtkIdTypeArray** GetGhostPointIds(
    int ghostLevel, svtkUnstructuredGrid* grid, int AddCellsIAlreadyHave);
  svtkUnstructuredGrid* AddGhostCellsUniqueCellAssignment(
    svtkUnstructuredGrid* myGrid, svtkPDistributedDataFilterSTLCloak* globalToLocalMap);
  svtkUnstructuredGrid* AddGhostCellsDuplicateCellAssignment(
    svtkUnstructuredGrid* myGrid, svtkPDistributedDataFilterSTLCloak* globalToLocalMap);
  svtkUnstructuredGrid* SetMergeGhostGrid(svtkUnstructuredGrid* ghostCellGrid,
    svtkUnstructuredGrid* incomingGhostCells, int ghostLevel,
    svtkPDistributedDataFilterSTLCloak* idMap);
  //@}

  //@{
  /**
   * ?
   */
  svtkUnstructuredGrid* ExtractCells(svtkIdList* list, int deleteCellLists, svtkDataSet* in);
  svtkUnstructuredGrid* ExtractCells(
    svtkIdList** lists, int nlists, int deleteCellLists, svtkDataSet* in);
  svtkUnstructuredGrid* ExtractZeroCellGrid(svtkDataSet* in);
  //@}

  //@{
  /**
   * ?
   */
  static int GlobalPointIdIsUsed(
    svtkUnstructuredGrid* grid, int ptId, svtkPDistributedDataFilterSTLCloak* globalToLocal);
  static int LocalPointIdIsUsed(svtkUnstructuredGrid* grid, int ptId);
  static svtkIdType FindId(svtkIdTypeArray* ids, svtkIdType gid, svtkIdType startLoc);
  //@}

  /**
   * ?
   */
  static svtkIdTypeArray* AddPointAndCells(svtkIdType gid, svtkIdType localId,
    svtkUnstructuredGrid* grid, svtkIdType* gidCells, svtkIdTypeArray* ids);

  //@{
  /**
   * ?
   */
  static void AddConstantUnsignedCharPointArray(
    svtkUnstructuredGrid* grid, const char* arrayName, unsigned char val);
  static void AddConstantUnsignedCharCellArray(
    svtkUnstructuredGrid* grid, const char* arrayName, unsigned char val);
  //@}

  /**
   * ?
   */
  static void RemoveRemoteCellsFromList(
    svtkIdList* cellList, svtkIdType* gidCells, svtkIdType* remoteCells, svtkIdType nRemoteCells);

  /**
   * ?
   */
  static svtkUnstructuredGrid* MergeGrids(svtkDataSet** sets, int nsets, int deleteDataSets,
    int useGlobalNodeIds, float pointMergeTolerance, int useGlobalCellIds);

private:
  svtkPDistributedDataFilter(const svtkPDistributedDataFilter&) = delete;
  void operator=(const svtkPDistributedDataFilter&) = delete;
};
#endif
