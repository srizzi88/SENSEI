/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3HeavyDataHandler.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXdmf3HeavyDataHandler
 * @brief   internal helper for svtkXdmf3Reader
 *
 * svtkXdmf3Reader uses this class to read the heave data from the XDMF
 * file(s).
 *
 * This file is a helper for the svtkXdmf3Reader and not intended to be
 * part of SVTK public API
 */

#ifndef svtkXdmf3HeavyDataHandler_h
#define svtkXdmf3HeavyDataHandler_h

#include "svtkIOXdmf3Module.h" // For export macro

#include "svtk_xdmf3.h"

// clang-format off
#include SVTKXDMF3_HEADER(core/XdmfInformation.hpp)

#include SVTKXDMF3_HEADER(core/XdmfItem.hpp)
#include SVTKXDMF3_HEADER(core/XdmfSharedPtr.hpp)
// clang-format on

#include "svtkXdmf3ArrayKeeper.h"
#include "svtkXdmf3ArraySelection.h"

#include SVTKXDMF3_HEADER(XdmfCurvilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfGraph.hpp)
#include SVTKXDMF3_HEADER(XdmfGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfRectilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfRegularGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfSet.hpp)
#include SVTKXDMF3_HEADER(XdmfUnstructuredGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfDomain.hpp)

class svtkDataObject;
class svtkDataSet;
class svtkImageData;
class svtkMutableDirectedGraph;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkUnstructuredGrid;

class SVTKIOXDMF3_EXPORT svtkXdmf3HeavyDataHandler
{
public:
  /**
   * factory constructor
   */
  static shared_ptr<svtkXdmf3HeavyDataHandler> New(svtkXdmf3ArraySelection* fs,
    svtkXdmf3ArraySelection* cs, svtkXdmf3ArraySelection* ps, svtkXdmf3ArraySelection* gc,
    svtkXdmf3ArraySelection* sc, unsigned int processor, unsigned int nprocessors, bool dt, double t,
    svtkXdmf3ArrayKeeper* keeper, bool asTime);

  /**
   * recursively create and populate svtk data objects for the provided Xdmf item
   */
  svtkDataObject* Populate(shared_ptr<XdmfGrid> item, svtkDataObject* toFill);
  svtkDataObject* Populate(shared_ptr<XdmfDomain> item, svtkDataObject* toFill);
  svtkDataObject* Populate(shared_ptr<XdmfGraph> item, svtkDataObject* toFill);

  svtkXdmf3ArrayKeeper* Keeper;

  shared_ptr<XdmfGrid> testItem1;
  shared_ptr<XdmfDomain> testItem2;

protected:
  /**
   * for parallel partitioning
   */
  bool ShouldRead(unsigned int piece, unsigned int npieces);

  bool GridEnabled(shared_ptr<XdmfGrid> grid);
  bool GridEnabled(shared_ptr<XdmfGraph> graph);
  bool SetEnabled(shared_ptr<XdmfSet> set);

  bool ForThisTime(shared_ptr<XdmfGrid> grid);
  bool ForThisTime(shared_ptr<XdmfGraph> graph);

  svtkDataObject* MakeUnsGrid(shared_ptr<XdmfUnstructuredGrid> grid, svtkUnstructuredGrid* dataSet,
    svtkXdmf3ArrayKeeper* keeper);

  svtkDataObject* MakeRecGrid(
    shared_ptr<XdmfRectilinearGrid> grid, svtkRectilinearGrid* dataSet, svtkXdmf3ArrayKeeper* keeper);

  svtkDataObject* MakeCrvGrid(
    shared_ptr<XdmfCurvilinearGrid> grid, svtkStructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper);

  svtkDataObject* MakeRegGrid(
    shared_ptr<XdmfRegularGrid> grid, svtkImageData* dataSet, svtkXdmf3ArrayKeeper* keeper);
  svtkDataObject* MakeGraph(
    shared_ptr<XdmfGraph> grid, svtkMutableDirectedGraph* dataSet, svtkXdmf3ArrayKeeper* keeper);

  svtkDataObject* ExtractSet(unsigned int setnum, shared_ptr<XdmfGrid> grid, svtkDataSet* dataSet,
    svtkUnstructuredGrid* subSet, svtkXdmf3ArrayKeeper* keeper);

  bool doTime;
  double time;
  unsigned int Rank;
  unsigned int NumProcs;
  svtkXdmf3ArraySelection* FieldArrays;
  svtkXdmf3ArraySelection* CellArrays;
  svtkXdmf3ArraySelection* PointArrays;
  svtkXdmf3ArraySelection* GridsCache;
  svtkXdmf3ArraySelection* SetsCache;
  bool AsTime;
};

#endif // svtkXdmf3HeavyDataHandler_h
// SVTK-HeaderTest-Exclude: svtkXdmf3HeavyDataHandler.h
