#ifndef SVTKUtils_h
#define SVTKUtils_h

#include "MeshMetadata.h"

class svtkDataSet;
class svtkDataObject;
class svtkFieldData;
class svtkDataSetAttributes;
class svtkCompositeDataSet;

#include <svtkSmartPointer.h>
#include <functional>
#include <vector>
#include <mpi.h>

using svtkCompositeDataSetPtr = svtkSmartPointer<svtkCompositeDataSet>;

namespace sensei
{

/// A collection of generally useful funcitons implementing
/// common access patterns or operations on SVTK data structures
namespace SVTKUtils
{

/// given a SVTK POD data type enum returns the size
unsigned int Size(int svtkt);

/// given a SVTK data object enum returns true if it a legacy object
int IsLegacyDataObject(int code);

/// givne a SVTK data object enum constructs an instance
svtkDataObject *NewDataObject(int code);

/// returns the enum value given an association name. where name
/// can be one of: point, cell or, field
int GetAssociation(std::string assocStr, int &assoc);

/// returns the name of the association, point, cell or field
const char *GetAttributesName(int association);

/// returns the container for the associations: svtkPointData,
/// svtkCellData, or svtkFieldData
svtkFieldData *GetAttributes(svtkDataSet *dobj, int association);

/// callback that processes input and output datasets
/// return 0 for success, > zero to stop without error, < zero to stop with error
using BinaryDatasetFunction = std::function<int(svtkDataSet*, svtkDataSet*)>;

/// Applies the function to leaves of the structurally equivalent
/// input and output data objects.
int Apply(svtkDataObject *input, svtkDataObject *output,
  BinaryDatasetFunction &func);

/// callback that processes input and output datasets
/// return 0 for success, > zero to stop without error, < zero to stop with error
using DatasetFunction = std::function<int(svtkDataSet*)>;

/// Applies the function to the data object
/// The function is called once for each leaf dataset
int Apply(svtkDataObject *dobj, DatasetFunction &func);

/// Store ghost layer metadata in the mesh
int SetGhostLayerMetadata(svtkDataObject *mesh,
  int nGhostCellLayers, int nGhostNodeLayers);

/// Retreive ghost layer metadata from the mesh. returns non-zero if
/// no such metadata is found.
int GetGhostLayerMetadata(svtkDataObject *mesh,
  int &nGhostCellLayers, int &nGhostNodeLayers);

/// Get  metadata, note that data set variant is not meant to
/// be used on blocks of a multi-block
int GetMetadata(MPI_Comm comm, svtkDataSet *ds, MeshMetadataPtr);
int GetMetadata(MPI_Comm comm, svtkCompositeDataSet *cd, MeshMetadataPtr);

/// Given a data object ensure that it is a composite data set
/// If it already is, then the call is a no-op, if it is not
/// then it is converted to a multiblock. The flag take determines
/// if the smart pointer takes ownership or adds a reference.
svtkCompositeDataSetPtr AsCompositeData(MPI_Comm comm,
  svtkDataObject *dobj, bool take = true);

/// Return true if the mesh or block type is AMR
inline bool AMR(const MeshMetadataPtr &md)
{
  return (md->MeshType == SVTK_OVERLAPPING_AMR) ||
    (md->MeshType == SVTK_NON_OVERLAPPING_AMR);
}

/// Return true if the mesh or block type is logically Cartesian
inline bool Structured(const MeshMetadataPtr &md)
{
  return (md->BlockType == SVTK_STRUCTURED_GRID) ||
    (md->MeshType == SVTK_STRUCTURED_GRID);
}

/// Return true if the mesh or block type is polydata
inline bool Polydata(const MeshMetadataPtr &md)
{
  return (md->BlockType == SVTK_POLY_DATA) || (md->MeshType == SVTK_POLY_DATA);
}

/// Return true if the mesh or block type is unstructured
inline bool Unstructured(const MeshMetadataPtr &md)
{
  return (md->BlockType == SVTK_UNSTRUCTURED_GRID) ||
    (md->MeshType == SVTK_UNSTRUCTURED_GRID);
}

/// Return true if the mesh or block type is stretched Cartesian
inline bool StretchedCartesian(const MeshMetadataPtr &md)
{
  return (md->BlockType == SVTK_RECTILINEAR_GRID) ||
    (md->MeshType == SVTK_RECTILINEAR_GRID);
}

/// Return true if the mesh or block type is uniform Cartesian
inline bool UniformCartesian(const MeshMetadataPtr &md)
{
  return (md->BlockType == SVTK_IMAGE_DATA) || (md->MeshType == SVTK_IMAGE_DATA)
    || (md->BlockType == SVTK_UNIFORM_GRID) || (md->MeshType == SVTK_UNIFORM_GRID);
}

/// Return true if the mesh or block type is logically Cartesian
inline bool LogicallyCartesian(const MeshMetadataPtr &md)
{
  return Structured(md) || UniformCartesian(md) || StretchedCartesian(md);
}

// rank 0 writes a dataset for visualizing the domain decomp
int WriteDomainDecomp(MPI_Comm comm, const sensei::MeshMetadataPtr &md,
  const std::string fileName);

}
}

#endif
