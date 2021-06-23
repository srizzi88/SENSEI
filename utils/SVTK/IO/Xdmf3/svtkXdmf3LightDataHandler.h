/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3LightDataHandler.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXdmf3LightDataHandler
 * @brief   internal helper for svtkXdmf3Reader
 *
 * svtkXdmf3Reader uses this class to inspect the light data in the XDMF
 * file(s) and determine meta-information about the svtkDataObjects it
 * needs to produce.
 *
 * This file is a helper for the svtkXdmf3Reader and not intended to be
 * part of SVTK public API
 */

#ifndef svtkXdmf3LightDataHandler_h
#define svtkXdmf3LightDataHandler_h

#include "svtkIOXdmf3Module.h" // For export macro
#include "svtkType.h"

// clang-format off
#include "svtk_xdmf3.h"
#include SVTKXDMF3_HEADER(core/XdmfItem.hpp)
// clang-format on

#include <set>

class svtkXdmf3SILBuilder;
class svtkXdmf3ArraySelection;
class XdmfItem;
class XdmfGraph;
class XdmfGrid;

class SVTKIOXDMF3_EXPORT svtkXdmf3LightDataHandler
{
public:
  /**
   * factory constructor
   */
  static shared_ptr<svtkXdmf3LightDataHandler> New(svtkXdmf3SILBuilder* sb, svtkXdmf3ArraySelection* f,
    svtkXdmf3ArraySelection* ce, svtkXdmf3ArraySelection* pn, svtkXdmf3ArraySelection* gc,
    svtkXdmf3ArraySelection* sc, unsigned int processor, unsigned int nprocessors);

  /**
   * recursively inspect XDMF data hierarchy to determine
   * times that we can provide data at
   * name of arrays to select from
   * name and hierarchical relationship of blocks to select from
   */
  void InspectXDMF(shared_ptr<XdmfItem> item, svtkIdType parentVertex, unsigned int depth = 0);

  /**
   * called to make sure overflown SIL doesn't give nonsensical results
   */
  void ClearGridsIfNeeded(shared_ptr<XdmfItem> domain);

  /**
   * return the list of times that the xdmf file can provide data at
   * only valid after InspectXDMF
   */
  std::set<double> getTimes();

private:
  /**
   * constructor
   */
  svtkXdmf3LightDataHandler();

  /**
   * remembers array names from the item
   */
  void InspectArrays(shared_ptr<XdmfItem> item);

  /**
   * Used in SIL creation.
   */
  bool TooDeep(unsigned int depth);

  /**
   * Used in SIL creation.
   */
  std::string UniqueName(const std::string& name, bool ForGrid);

  /**
   * Used in SIL creation.
   */
  void AddNamedBlock(svtkIdType parentVertex, std::string originalName, std::string uniqueName);

  /**
   * Used in SIL creation.
   */
  void AddNamedSet(std::string uniqueName);

  //@{
  /**
   * records times that xdmf grids supply data at
   * if timespecs are only implied we add them to make things simpler later on
   */
  void InspectTime(shared_ptr<XdmfItem> item);
  void GetSetTime(shared_ptr<XdmfGrid> child, unsigned int& cnt);
  void GetSetTime(shared_ptr<XdmfGraph> child, unsigned int& cnt);
  //@}

  /**
   * for parallel partitioning
   */
  bool ShouldRead(unsigned int piece, unsigned int npieces);

  svtkXdmf3SILBuilder* SILBuilder;
  svtkXdmf3ArraySelection* FieldArrays;
  svtkXdmf3ArraySelection* CellArrays;  // ie EdgeArrays for Graphs
  svtkXdmf3ArraySelection* PointArrays; // ie NodeArrays for Graphs
  svtkXdmf3ArraySelection* GridsCache;
  svtkXdmf3ArraySelection* SetsCache;
  unsigned int MaxDepth;
  unsigned int Rank;
  unsigned int NumProcs;
  std::set<double> times; // relying on implicit sort from set<double>
};

#endif // svtkXdmf3LightDataHandler_h
// SVTK-HeaderTest-Exclude: svtkXdmf3LightDataHandler.h
