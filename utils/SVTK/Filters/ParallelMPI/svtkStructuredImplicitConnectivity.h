/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredImplicitConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStructuredImplicitConnectivity
 * a distributed structured dataset that is implicitly connected among
 * partitions without abutting. This creates a gap between partitions and
 * introduces a cell that spans multiple zones. This typically arises with
 * finite difference grids, which are partitioned with respect to the
 * nodes of the grid, or, when a filter samples the grid, e.g., to get a
 * lower resolution representation.
 *
 *
 * This class is intended as a lower-level helper for higher level SVTK filters
 * that provides functionality for resolving the implicit connectivity (gap)
 * between two or more partitions of a distributed structured dataset.
 *
 * @warning
 * The present implementation requires:
 * <ul>
 *  <li> one block/grid per rank. </li>
 *  <li> 2-D (XY,YZ or XZ planes) or 3-D datasets. </li>
 *  <li> node-center fields must match across processes. </li>
 * </ul>
 *
 */

#ifndef svtkStructuredImplicitConnectivity_h
#define svtkStructuredImplicitConnectivity_h

#include "svtkFiltersParallelMPIModule.h" // For export macro
#include "svtkObject.h"

// Forward declarations
class svtkDataArray;
class svtkImageData;
class svtkMPIController;
class svtkMultiProcessStream;
class svtkPointData;
class svtkPoints;
class svtkRectilinearGrid;
class svtkStructuredGrid;

namespace svtk
{
namespace detail
{

class CommunicationManager;
struct DomainMetaData;
struct StructuredGrid;

} // END namespace detail
} // END namespace svtk

class SVTKFILTERSPARALLELMPI_EXPORT svtkStructuredImplicitConnectivity : public svtkObject
{
public:
  static svtkStructuredImplicitConnectivity* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkStructuredImplicitConnectivity, svtkObject);

  /**
   * \brief Sets the whole extent for the distributed structured domain.
   * \param wholeExt the extent of the entire domain (in).
   * \note All ranks must call this method with the same whole extent.
   * \post this->DomainInfo != nullptr
   */
  void SetWholeExtent(int wholeExt[6]);

  // \brief Registers the structured grid dataset belonging to this process.
  // \param gridID the ID of the grid in this rank.
  // \param extent the [imin,imax,jmin,jmax,kmin,kmax] of the grid.
  // \param gridPnts pointer to the points of the grid (nullptr for uniform grid).
  // \param pointData pointer to the node-centered fields of the grid.
  // \pre gridID >= 0. The code uses values of gridID < -1 as flag internally.
  // \pre svtkStructuredExtent::Smaller(extent,wholeExtent) == true.
  // \note A rank with no or an empty grid, should not call this method.
  void RegisterGrid(const int gridID, int extent[6], svtkPoints* gridPnts, svtkPointData* pointData);

  // \brief Registers the rectilinear grid dataset belonging to this process.
  // \param gridID the ID of the in this rank.
  // \param extent the [imin,imax,jmin,jmax,kmin,kmax] of the grid.
  // \param xcoords the x-coordinates array of the rectilinear grid.
  // \param ycoords the y-coordinates array of the rectilinear grid.
  // \param zcoords the z-coordinates array of the rectilinear grid.
  // \param pointData pointer to the node-centered fields of the grid.
  // \pre gridID >= 0. The code uses values of gridID < -1 as flag internally.
  // \pre svtkStructuredExtent::Smaller(extent,wholeExtent) == true.
  // \note A rank with no or an empty grid, should not call this method.
  void RegisterRectilinearGrid(const int gridID, int extent[6], svtkDataArray* xcoords,
    svtkDataArray* ycoords, svtkDataArray* zcoords, svtkPointData* pointData);

  /**
   * \brief Finds implicit connectivity for a distributed structured dataset.
   * \note This is a collective operation, all ranks must call this method.
   * \pre this->Controller != nullptr
   * \pre this->DomainInfo != nullptr
   */
  void EstablishConnectivity();

  /**
   * \brief Checks if there is implicit connectivity.
   * \return status true if implicit connectivity in one or more dimensions.
   */
  bool HasImplicitConnectivity();

  /**
   * \brief Exchanges one layer (row or column) of data between neighboring
   * grids to fix the implicit connectivity.
   * \note This is a collective operation, all ranks must call this method.
   * \pre this->Controller != nullptr
   * \pre this->DomainInfo != nullptr
   */
  void ExchangeData();

  /**
   * \brief Gets the output structured grid instance on this process.
   * \param gridID the ID of the grid
   * \param grid pointer to data-structure where to store the output.
   */
  void GetOutputStructuredGrid(const int gridID, svtkStructuredGrid* grid);

  /**
   * \brief Gets the output uniform grid instance on this process.
   * \param gridID the ID of the grid.
   * \param grid pointer to data-structure where to store the output.
   */
  void GetOutputImageData(const int gridID, svtkImageData* grid);

  /**
   * \brief Gets the output rectilinear grid instance on this process.
   * \param gridID the ID of the grid.
   * \param grid pointer to data-structure where to store the output.
   */
  void GetOutputRectilinearGrid(const int gridID, svtkRectilinearGrid* grid);

protected:
  svtkStructuredImplicitConnectivity();
  virtual ~svtkStructuredImplicitConnectivity();

  svtkMPIController* Controller;

  svtk::detail::DomainMetaData* DomainInfo;
  svtk::detail::StructuredGrid* InputGrid;
  svtk::detail::StructuredGrid* OutputGrid;
  svtk::detail::CommunicationManager* CommManager;

  /**
   * \brief Checks if the data description matches globally.
   */
  bool GlobalDataDescriptionMatch();

  /**
   * \brief Packs the data to send into a bytestream
   */
  void PackData(int ext[6], svtkMultiProcessStream& bytestream);

  /**
   * \brief Unpacks the data to the output grid
   */
  void UnPackData(unsigned char* buffer, unsigned int size);

  /**
   * \brief Allocates send/rcv buffers needed to carry out the communication.
   */
  void AllocateBuffers(const int dim);

  /**
   * \brief Computes the neighbors with implicit connectivity.
   */
  void ComputeNeighbors();

  /**
   * \brief Constructs the output data-structures.
   */
  void ConstructOutput();

  /**
   * \brief Grows grid along a given dimension.
   * \param dim the dimension in query.
   */
  void GrowGrid(const int dim);

  /**
   * \brief Updates the list of neighbors after growing the grid along the
   * given dimension dim.
   * \param dim the dimension in query.
   */
  void UpdateNeighborList(const int dim);

  /**
   * \brief Gets whether there is implicit connectivity across all processes.
   */
  void GetGlobalImplicitConnectivityState();

  /**
   * \brief Exchanges extents among processes.
   * \pre this->Controller != nullptr.
   * \note This method is collective operation. All ranks must call it.
   */
  void ExchangeExtents();

private:
  svtkStructuredImplicitConnectivity(const svtkStructuredImplicitConnectivity&) = delete;
  void operator=(const svtkStructuredImplicitConnectivity&) = delete;
};
#endif
