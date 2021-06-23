/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPUnstructuredGridGhostCellsGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPUnstructuredGridGhostCellsGenerator
 * @brief   Builds ghost cells for a
 *  distributed unstructured grid dataset.
 *
 * This filter generate ghost cells for distributed a unstructured grid in
 * parallel - using MPI asynchronous communications.
 * The filter can take benefit of the input grid point global ids to perform.
 *
 * @warning
 *  <ul>
 *    <li> A step of 'all reduce' (each process send/receive grid bounds to/from
 *         all other processes).
 *    <li> The code currently assumes one grid per rank. </li>
 *    <li> PointData and CellData must match across partitions/processes. </li>
 *  </ul>
 *
 * @sa
 * svtkDistributedDataFilter
 *
 * @par Thanks:
 * This filter has been developed by Joachim Pouderoux, Kitware SAS 2015.
 *
 * @par Thanks:
 * This filter was expanded to compute multiple ghost layers by Boonthanome
 * Nouanesengsy and John Patchett, Los Alamos National Laboratory 2016.
 * These changes are based on the paper: M. Patchett, John & Nouanesengesy,
 * Boonthanome & Pouderoux, Joachim & Ahrens, James & Hagen, Hans. (2017).
 * "Parallel Multi-Level Ghost Cell Generation for Distributed Unstructured Grids"
 * which was presented at LDAV 2017 (The 7th IEEE Symposium on Large Data
 * Analysis and Visualization), At Phoenix, AZ, USA.
 *
 * @par Thanks:
 * ************************************************
 *
 * @par Thanks:
 * This filter uses different algorithms when obtaining the first layer of
 * ghost cells and getting subsequent layers.
 *
 * @par Thanks:
 * First ghost cell layer algorithm:
 *   - each proc obtains surface points using the surface filter
 *   - share bounds to determine potential neighbor processes
 *   - share surface points with each potential neighbors
 *   - for each neighbor proc, look at their points, and see if any points
 *     match any of your local points
 *   - for each matching point, find all local cells which use those points,
 *     and send those cells to that proc. mark the cells that were sent
 *     (used for later ghost layers)
 *   - receive all cells sent to you, and merge everything together
 *
 * @par Thanks:
 * Subsequent ghost layers
 *   - for each cell that was sent last round, find all other local cells
 *     which border these cells. 'local cells' also includes all ghost cells
 *     which i have. send these cells to the same proc, and mark them as sent
 *     last round
 *   - receive all cells sent to you, and merge everything together
 *   - if another layer is needed, repeat
 *
 */

#ifndef svtkPUnstructuredGridGhostCellsGenerator_h
#define svtkPUnstructuredGridGhostCellsGenerator_h

#include "svtkFiltersParallelGeometryModule.h" // For export macro
#include "svtkUnstructuredGridGhostCellsGenerator.h"
#include <vector> // For passing data between methods

class svtkMultiProcessController;

class SVTKFILTERSPARALLELGEOMETRY_EXPORT svtkPUnstructuredGridGhostCellsGenerator
  : public svtkUnstructuredGridGhostCellsGenerator
{
  svtkTypeMacro(svtkPUnstructuredGridGhostCellsGenerator, svtkUnstructuredGridGhostCellsGenerator);

public:
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPUnstructuredGridGhostCellsGenerator* New();

  //@{
  /**
   * Set/Get the MPI multi process controller object.
   */
  void SetController(svtkMultiProcessController* c);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPUnstructuredGridGhostCellsGenerator();
  ~svtkPUnstructuredGridGhostCellsGenerator() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void GetFirstGhostLayer(int, svtkUnstructuredGrid*);

  void ExchangeBoundsAndDetermineNeighbors(std::vector<double>&);
  void ExtractAndReduceSurfacePointsShareData(std::vector<double>&);
  void ComputeSharedPoints();

  void ExtractAndSendGhostCells(svtkUnstructuredGridBase*);

  void ReceiveAndMergeGhostCells(int, int, svtkUnstructuredGridBase*, svtkUnstructuredGrid*);

  void AddGhostLayer(int ghostLevel, int maxGhostLevel);

  void FindGhostCells();

  void AddGlobalCellIds();

  void RemoveGlobalCellIds();

  svtkMultiProcessController* Controller;

private:
  struct svtkInternals;
  svtkInternals* Internals;

  svtkPUnstructuredGridGhostCellsGenerator(const svtkPUnstructuredGridGhostCellsGenerator&) = delete;
  void operator=(const svtkPUnstructuredGridGhostCellsGenerator&) = delete;
};

#endif
