/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIYKdTreeUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkDIYKdTreeUtilities
 * @brief collection of utility functions for DIY-based KdTree algorithm
 *
 * svtkDIYKdTreeUtilities is intended for use by svtkRedistributeDataSetFilter. It
 * encapsulates invocation of DIY algorithms for various steps in the
 * svtkRedistributeDataSetFilter.
 */

#ifndef svtkDIYKdTreeUtilities_h
#define svtkDIYKdTreeUtilities_h

#include "svtkBoundingBox.h"               // for svtkBoundingBox
#include "svtkDIYExplicitAssigner.h"       // for svtkDIYExplicitAssigner
#include "svtkFiltersParallelDIY2Module.h" // for export macros
#include "svtkObject.h"
#include "svtkSmartPointer.h" // for svtkSmartPointer
#include <vector>            // for std::vector

class svtkDataObject;
class svtkDataSet;
class svtkIntArray;
class svtkMultiProcessController;
class svtkPartitionedDataSet;
class svtkPoints;
class svtkUnstructuredGrid;

class SVTKFILTERSPARALLELDIY2_EXPORT svtkDIYKdTreeUtilities : public svtkObject
{
public:
  svtkTypeMacro(svtkDIYKdTreeUtilities, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Given a dataset (or a composite dataset), this method will generate box
   * cuts in the domain to approximately load balance the points (or
   * cell-centers) into `number_of_partitions` requested. If `controller` is non-null,
   * the operation will be performed taking points on the multiple ranks into consideration.
   *
   * `local_bounds` provides the local domain bounds. If not specified, domain
   * bounds will be computed using the `dobj`.

   * Returns a vector a bounding boxes that can be used to partition the points
   * into load balanced chunks. The size of the vector is greater than or equal
   * to the `number_of_partitions`.
   */
  static std::vector<svtkBoundingBox> GenerateCuts(svtkDataObject* dobj, int number_of_partitions,
    bool use_cell_centers, svtkMultiProcessController* controller = nullptr,
    const double* local_bounds = nullptr);

  /**
   * Given a collection of points, this method will generate box cuts in the
   * domain to approximately load balance the points into `number_of_partitions`
   * requested. If `controller` is non-null, the operation will be performed
   * taking points on the multiple ranks into consideration.
   *
   * `local_bounds` provides the local domain bounds. If not specified, domain
   * bounds will be computed using the points provided.
   *
   * Returns a vector a bounding boxes that can be used to partition the points
   * into load balanced chunks. The size of the vector is greater than or equal
   * to the `number_of_partitions`.
   */
  static std::vector<svtkBoundingBox> GenerateCuts(
    const std::vector<svtkSmartPointer<svtkPoints> >& points, int number_of_partitions,
    svtkMultiProcessController* controller = nullptr, const double* local_bounds = nullptr);

  /**
   * Exchange parts in the partitioned dataset among ranks in the parallel group
   * defined by the `controller`. The parts are assigned to ranks in a
   * contiguous fashion.
   *
   * This method assumes that the input svtkPartitionedDataSet will have exactly
   * same number of partitions on all ranks. This is assumed since the
   * partitions' index is what dictates which rank it is assigned to.
   *
   * The returned svtkPartitionedDataSet will also have exactly as many
   * partitions as the input svtkPartitionedDataSet, however only the partitions
   * assigned to this current rank may be non-null.
   */
  static svtkSmartPointer<svtkPartitionedDataSet> Exchange(
    svtkPartitionedDataSet* parts, svtkMultiProcessController* controller);

  /**
   * Generates and adds global cell ids to datasets in `parts`. One this to note
   * that this method does not assign valid global ids to ghost cells. This may
   * not be adequate for general use, however for svtkRedistributeDataSetFilter
   * this is okay since the ghost cells in the input are anyways discarded when
   * the dataset is being split based on the cuts provided. This simplifies the
   * implementation and reduces communication.
   */
  static bool GenerateGlobalCellIds(svtkPartitionedDataSet* parts,
    svtkMultiProcessController* controller, svtkIdType* mb_offset = nullptr);

  /**
   * `GenerateCuts` returns a kd-tree with power of 2 nodes. Oftentimes, we want
   * to generate rank assignments for a fewer number of ranks for the nodes such
   * that each rank gets assigned a complete sub-tree. Use this function to
   * generate such an assignment.  This has following constraints:
   * 1. `num_blocks` must be a power of two.
   * 2. `num_ranks` cannot be greater than num_blocks.
   */
  static std::vector<int> ComputeAssignments(int num_blocks, int num_ranks);

  /**
   * Returns an assigner that assigns power-of-two blocks to an arbitrary number
   * of ranks such that each rank with a non-empty assignment gets a subtree --
   * thus preserving the kd-tree ordering between ranks.
   */
  static svtkDIYExplicitAssigner CreateAssigner(diy::mpi::communicator& comm, int num_blocks);

protected:
  svtkDIYKdTreeUtilities();
  ~svtkDIYKdTreeUtilities() override;

private:
  svtkDIYKdTreeUtilities(const svtkDIYKdTreeUtilities&) = delete;
  void operator=(const svtkDIYKdTreeUtilities&) = delete;
};

#endif
