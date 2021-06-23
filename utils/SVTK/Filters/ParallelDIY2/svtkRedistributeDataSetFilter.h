/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRedistributeDataSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkRedistributeDataSetFilter
 * @brief redistributes input dataset into requested number of partitions
 *
 * svtkRedistributeDataSetFilter is intended for redistributing data in a load
 * balanced fashion. The load balancing attempts to balance the number of cells
 * per target partition approximately. It uses a DIY-based kdtree implementation
 * that builds balances the cell centers among requested number of partitions.
 * Current implementation only supports power-of-2 target partition. If a
 * non-power of two value is specified for `NumberOfPartitions`, then the load
 * balancing simply uses the power-of-two greater than the requested value. The
 * bounding boxes for the kdtree leaf nodes are then used to redistribute the
 * data.
 *
 * Alternatively a collection of bounding boxes may be provided that can be used
 * to distribute the data instead of computing them (see `UseExplicitCuts` and
 * `SetExplicitCuts`). When explicit cuts are specified, it is possible use
 * those cuts strictly or to expand boxes on the edge to fit the domain of the
 * input dataset. This can be controlled by `ExpandExplicitCutsForInputDomain`.
 *
 * The filter allows users to pick how cells along the boundary of the cuts
 * either automatically generated or explicitly specified are to be distributed
 * using `BoundaryMode`. One can choose to assign those cells uniquely to one of
 * those regions or duplicate then on all regions or split the cells (using
 * svtkTableBasedClipDataSet filter). When cells are
 * duplicated along the boundary,  the filter will mark the duplicated cells as
 * `svtkDataSetAttributes::DUPLICATECELL` correctly on all but one of the
 * partitions using the ghost cell array (@sa `svtkDataSetAttributes::GhostArrayName`).
 *
 * Besides redistributing the data, the filter can optionally generate global
 * cell ids. This is provided since it relative easy to generate these
 * on when it is known that the data is spatially partitioned as is the case
 * after this filter has executed.
 *
 */
#ifndef svtkRedistributeDataSetFilter_h
#define svtkRedistributeDataSetFilter_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersParallelDIY2Module.h" // for export macros
#include "svtkSmartPointer.h"              // for svtkSmartPointer
#include <vector>                         // for std::vector

class svtkMultiProcessController;
class svtkBoundingBox;
class svtkPartitionedDataSet;
class svtkMultiBlockDataSet;
class svtkMultiPieceDataSet;

class SVTKFILTERSPARALLELDIY2_EXPORT svtkRedistributeDataSetFilter : public svtkDataObjectAlgorithm
{
public:
  static svtkRedistributeDataSetFilter* New();
  svtkTypeMacro(svtkRedistributeDataSetFilter, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default
   * svtkMultiProcessController::GlobalController will be used.
   */
  void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  enum BoundaryModes
  {
    ASSIGN_TO_ONE_REGION = 0,
    ASSIGN_TO_ALL_INTERSECTING_REGIONS = 1,
    SPLIT_BOUNDARY_CELLS = 2
  };

  //@{
  /**
   * Specify how cells on the boundaries are handled.
   *
   * \li `ASSIGN_TO_ONE_REGION` results in a cell on the boundary uniquely added
   *      to one of the ranks containing the region intersecting the cell.
   * \li `ASSIGN_TO_ALL_INTERSECTING_REGIONS` results in a cell on the boundary
   *      added to all ranks containing the region intersecting the cell.
   * \li `SPLIT_BOUNDARY_CELLS` results in cells along the boundary being
   *      clipped along the region boundaries.
   *
   * Default is `ASSIGN_TO_ONE_REGION`.
   */
  svtkSetClampMacro(BoundaryMode, int, ASSIGN_TO_ONE_REGION, SPLIT_BOUNDARY_CELLS);
  svtkGetMacro(BoundaryMode, int);
  void SetBoundaryModeToAssignToOneRegion() { this->SetBoundaryMode(ASSIGN_TO_ONE_REGION); }
  void SetBoundaryModeToAssignToAllIntersectingRegions()
  {
    this->SetBoundaryMode(ASSIGN_TO_ALL_INTERSECTING_REGIONS);
  }
  void SetBoundaryModeToSplitBoundaryCells() { this->SetBoundaryMode(SPLIT_BOUNDARY_CELLS); }
  //@}

  //@{
  /**
   * Specify whether to compute the load balancing automatically or use
   * explicitly provided cuts. Set to false (default) to automatically compute
   * the cuts to use for redistributing the dataset.
   */
  svtkSetMacro(UseExplicitCuts, bool);
  svtkGetMacro(UseExplicitCuts, bool);
  svtkBooleanMacro(UseExplicitCuts, bool);
  //@}

  //@{
  /**
   * Specify the cuts to use when `UseExplicitCuts` is true.
   */
  void SetExplicitCuts(const std::vector<svtkBoundingBox>& boxes);
  const std::vector<svtkBoundingBox>& GetExplicitCuts() const { return this->ExplicitCuts; }
  void RemoveAllExplicitCuts();
  void AddExplicitCut(const svtkBoundingBox& bbox);
  void AddExplicitCut(const double bbox[6]);
  int GetNumberOfExplicitCuts() const;
  const svtkBoundingBox& GetExplicitCut(int index) const;
  //@}

  //@{
  /**
   * When using explicit cuts, it possible that the bounding box defined by all
   * the cuts is smaller than the input's bounds. In that case, the filter can
   * automatically expand the edge boxes to include the input bounds to avoid
   * clipping of the input dataset on the external faces of the combined
   * bounding box.
   *
   * Default is true, that is explicit cuts will automatically be expanded.
   *
   */
  svtkSetMacro(ExpandExplicitCuts, bool);
  svtkGetMacro(ExpandExplicitCuts, bool);
  svtkBooleanMacro(ExpandExplicitCuts, bool);
  //@}

  //@}
  /**
   * Returns the cuts used by the most recent `RequestData` call. This is only
   * valid after a successful `Update` request.
   */
  const std::vector<svtkBoundingBox>& GetCuts() const { return this->Cuts; }

  //@{
  /**
   * Specify the number of partitions to split the input dataset into.
   * Set to 0 to indicate that the partitions should match the number of
   * ranks (processes) determined using svtkMultiProcessController provided.
   * Setting to a non-zero positive number will result in the filter generating at
   * least as many partitions.
   *
   * This is simply a hint and not an exact number of partitions the data will be
   * split into. Current implementation results in number of partitions equal to
   * the power of 2 greater than or equal to the chosen value.
   *
   * Default is 0.
   *
   * This has no effect when `UseExplicitCuts` is set to true. In that case, the
   * number of partitions is dictated by the number of cuts provided.
   *
   * @sa PreservePartitionsInOutput, UseExplicitCuts
   */
  svtkSetClampMacro(NumberOfPartitions, int, 0, SVTK_INT_MAX);
  svtkGetMacro(NumberOfPartitions, int);
  //@}

  //@{
  /**
   * When set to true (default is false), this filter will generate a svtkPartitionedDataSet as the
   * output. The advantage of doing that is each partition that the input dataset was split
   * into can be individually accessed. Otherwise, when the number of partitions generated is
   * greater than the number of ranks, a rank with more than one partition will use
   * `svtkAppendFilter` to merge the multiple partitions into a single unstructured grid.
   *
   * The output dataset type is always svtkUnstructuredGrid when
   * PreservePartitionsInOutput is false and always a svtkPartitionedDataSet when
   * PreservePartitionsInOutput is true.
   *
   * Default is false i.e. the filter will generate a single svtkUnstructuredGrid.
   */
  svtkSetMacro(PreservePartitionsInOutput, bool);
  svtkGetMacro(PreservePartitionsInOutput, bool);
  svtkBooleanMacro(PreservePartitionsInOutput, bool);
  //@}

  //@{
  /**
   * Generate global cell ids if none present in the input. If global cell ids are present
   * in the input then this flag is ignored. Default is true.
   */
  svtkSetMacro(GenerateGlobalCellIds, bool);
  svtkGetMacro(GenerateGlobalCellIds, bool);
  svtkBooleanMacro(GenerateGlobalCellIds, bool);
  //@}

  /**
   * Helper function to expand a collection of bounding boxes to include the
   * `bounds` specified. This will expand any boxes in the `cuts` that abut any
   * of the external faces of the bounding box formed by all the `cuts` to
   * touch the external faces of the `bounds`.
   */
  std::vector<svtkBoundingBox> ExpandCuts(
    const std::vector<svtkBoundingBox>& cuts, const svtkBoundingBox& bounds);

  //@{
  /**
   * Enable/disable debugging mode. In this mode internal arrays are preserved
   * and ghost cells are not explicitly marked as such so that they can be inspected
   * without risk of being dropped or removed by the pipeline.
   *
   * Default is false.
   */
  svtkSetMacro(EnableDebugging, bool);
  svtkGetMacro(EnableDebugging, bool);
  svtkBooleanMacro(EnableDebugging, bool);
  //@}

protected:
  svtkRedistributeDataSetFilter();
  ~svtkRedistributeDataSetFilter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * This method is called to generate the partitions for the input dataset.
   * Subclasses should override this to generate partitions using preferred data
   * redistribution strategy.
   *
   * The `data` will either be a `svtkPartitionedDataSet` or a `svtkDataSet`. In
   * case of `svtkPartitionedDataSet`, the method is expected to redistribute all
   * datasets (partitions) in the `svtkPartitionedDataSet` taken as a whole.
   */
  virtual std::vector<svtkBoundingBox> GenerateCuts(svtkDataObject* data);

  /**
   * This method is called to split a svtkDataSet into multiple datasets by the
   * vector of `svtkBoundingBox` passed in. The returned svtkPartitionedDataSet
   * must have exactly as many partitions as the number of svtkBoundingBoxes
   * in the `cuts` vector with each partition matching the bounding box at the
   * matching index.
   *
   * Note, this method duplicates cells that lie on the boundaries and adds cell
   * arrays that indicate cell ownership and flags boundary cells.
   */
  virtual svtkSmartPointer<svtkPartitionedDataSet> SplitDataSet(
    svtkDataSet* dataset, const std::vector<svtkBoundingBox>& cuts);

private:
  svtkRedistributeDataSetFilter(const svtkRedistributeDataSetFilter&) = delete;
  void operator=(const svtkRedistributeDataSetFilter&) = delete;

  bool Redistribute(svtkDataObject* inputDO, svtkPartitionedDataSet* outputPDS,
    const std::vector<svtkBoundingBox>& cuts, svtkIdType* mb_offset = nullptr);
  bool RedistributeDataSet(
    svtkDataSet* inputDS, svtkPartitionedDataSet* outputPDS, const std::vector<svtkBoundingBox>& cuts);
  int RedistributeMultiBlockDataSet(
    svtkMultiBlockDataSet* input, svtkMultiBlockDataSet* output, svtkIdType* mb_offset = nullptr);
  int RedistributeMultiPieceDataSet(
    svtkMultiPieceDataSet* input, svtkMultiPieceDataSet* output, svtkIdType* mb_offset = nullptr);
  svtkSmartPointer<svtkDataSet> ClipDataSet(svtkDataSet* dataset, const svtkBoundingBox& bbox);

  void MarkGhostCells(svtkPartitionedDataSet* pieces);

  svtkSmartPointer<svtkPartitionedDataSet> AssignGlobalCellIds(
    svtkPartitionedDataSet* input, svtkIdType* mb_offset = nullptr);
  svtkSmartPointer<svtkDataSet> AssignGlobalCellIds(
    svtkDataSet* input, svtkIdType* mb_offset = nullptr);

  void MarkValidDimensions(svtkDataObject* inputDO);

  std::vector<svtkBoundingBox> ExplicitCuts;
  std::vector<svtkBoundingBox> Cuts;

  svtkMultiProcessController* Controller;
  int BoundaryMode;
  int NumberOfPartitions;
  bool PreservePartitionsInOutput;
  bool GenerateGlobalCellIds;
  bool UseExplicitCuts;
  bool ExpandExplicitCuts;
  bool EnableDebugging;
  bool ValidDim[3];
};

#endif
