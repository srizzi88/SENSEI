/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIYUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkDIYUtilities
 * @brief collection of helper functions for working with DIY
 *
 * svtkDIYUtilities provides a set of utility functions when using DIY in a SVTK
 * filters.
 */
#ifndef svtkDIYUtilities_h
#define svtkDIYUtilities_h

#include "svtkObject.h"
#include "svtkParallelDIYModule.h" // for export macros
#include "svtkSmartPointer.h"      // needed for svtkSmartPointer

// clang-format off
#include "svtk_diy2.h" // needed for DIY
#include SVTK_DIY2(diy/mpi.hpp)
#include SVTK_DIY2(diy/serialization.hpp)
#include SVTK_DIY2(diy/types.hpp)
// clang-format on

class svtkBoundingBox;
class svtkDataObject;
class svtkDataSet;
class svtkMultiProcessController;
class svtkPoints;

class SVTKPARALLELDIY_EXPORT svtkDIYUtilities : public svtkObject
{
public:
  svtkTypeMacro(svtkDIYUtilities, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * In MPI-enabled builds, DIY filters need MPI to be initialized.
   * Calling this method in such filters will ensure that that's the case.
   */
  static void InitializeEnvironmentForDIY();

  /**
   * Converts a svtkMultiProcessController to a diy::mpi::communicator.
   * If controller is nullptr or not a svtkMPIController, then
   * diy::mpi::communicator(MPI_COMM_NULL) is created.
   */
  static diy::mpi::communicator GetCommunicator(svtkMultiProcessController* controller);

  //@{
  /**
   * Load/Save a svtkDataSet in a diy::BinaryBuffer.
   */
  static void Save(diy::BinaryBuffer& bb, svtkDataSet*);
  static void Load(diy::BinaryBuffer& bb, svtkDataSet*&);
  //@}

  /**
   * Reduce bounding box.
   */
  static void AllReduce(diy::mpi::communicator& comm, svtkBoundingBox& bbox);

  /**
   * Convert svtkBoundingBox to diy::ContinuousBounds.
   *
   * Note, there is a loss of precision since svtkBoundingBox uses `double` while
   * diy::ContinuousBounds uses float.
   */
  static diy::ContinuousBounds Convert(const svtkBoundingBox& bbox);

  /**
   * Convert diy::ContinuousBounds to svtkBoundingBox.
   *
   * Note, there is a change of precision since svtkBoundingBox uses `double` while
   * diy::ContinuousBounds uses float.
   */
  static svtkBoundingBox Convert(const diy::ContinuousBounds& bds);

  /**
   * Broadcast a vector of bounding boxes. Only the source vector needs to have
   * a valid size.
   */
  static void Broadcast(
    diy::mpi::communicator& comm, std::vector<svtkBoundingBox>& boxes, int source);

  /**
   * Extract datasets from the given data object. This method returns a vector
   * of svtkDataSet* from the `dobj`. If dobj is a svtkDataSet, the returned
   * vector will have just 1 svtkDataSet. If dobj is a svtkCompositeDataSet, then
   * we iterate over it and add all non-null leaf nodes to the returned vector.
   */
  static std::vector<svtkDataSet*> GetDataSets(svtkDataObject* dobj);

  //@{
  /**
   * Extracts points from the input. If input is not a svtkPointSet, it will use
   * an appropriate filter to extract the svtkPoints. If use_cell_centers is
   * true, cell-centers will be computed and extracted instead of the dataset
   * points.
   */
  static std::vector<svtkSmartPointer<svtkPoints> > ExtractPoints(
    const std::vector<svtkDataSet*>& datasets, bool use_cell_centers);
  //@}

  /**
   * Convenience method to get local bounds for the data object.
   */
  static svtkBoundingBox GetLocalBounds(svtkDataObject* dobj);

protected:
  svtkDIYUtilities();
  ~svtkDIYUtilities() override;

private:
  svtkDIYUtilities(const svtkDIYUtilities&) = delete;
  void operator=(const svtkDIYUtilities&) = delete;
};

namespace diy
{
template <>
struct Serialization<svtkDataSet*>
{
  static void save(BinaryBuffer& bb, svtkDataSet* const& p) { svtkDIYUtilities::Save(bb, p); }
  static void load(BinaryBuffer& bb, svtkDataSet*& p) { svtkDIYUtilities::Load(bb, p); }
};
}

// Implementation detail for Schwarz counter idiom.
class SVTKPARALLELDIY_EXPORT svtkDIYUtilitiesCleanup
{
public:
  svtkDIYUtilitiesCleanup();
  ~svtkDIYUtilitiesCleanup();

private:
  svtkDIYUtilitiesCleanup(const svtkDIYUtilitiesCleanup&) = delete;
  void operator=(const svtkDIYUtilitiesCleanup&) = delete;
};
static svtkDIYUtilitiesCleanup svtkDIYUtilitiesCleanupInstance;

#endif
// SVTK-HeaderTest-Exclude: svtkDIYUtilities.h
