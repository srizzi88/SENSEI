/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXsvtkVTU.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXsvtkVTU.h : class that supports UnstructuredMesh schema in SVTK XML
 * format .vtu extends abstract VTXsvtkBase
 *
 *  Created on: June 24, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef SVTK_IO_ADIOS2_VTX_SCHEMA_SVTK_VTXxmlVTU_h
#define SVTK_IO_ADIOS2_VTX_SCHEMA_SVTK_VTXxmlVTU_h

#include <map>
#include <string>
#include <vector>

#include "svtkNew.h"
#include "svtkUnstructuredGrid.h"

#include "VTX/schema/svtk/VTXsvtkBase.h"

namespace vtx
{
namespace schema
{
class VTXsvtkVTU : public VTXsvtkBase
{
public:
  VTXsvtkVTU(const std::string& schema, adios2::IO& io, adios2::Engine& engine);
  ~VTXsvtkVTU();

private:
  /** Could be extended in a container, this is a per-rank ImageData */
  svtkNew<svtkUnstructuredGrid> UnstructuredGrid;

  /** BlockIDs carried by current rank */
  std::vector<size_t> BlockIDs;

  void DoFill(svtkMultiBlockDataSet* multiBlock, const size_t step) final;
  void ReadPiece(const size_t step, const size_t pieceID) final;

  void Init() final;

#define declare_type(T)                                                                            \
  void SetBlocks(adios2::Variable<T> variable, types::DataArray& dataArray, const size_t step)     \
    final;
  SVTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

  template <class T>
  void SetBlocksCommon(
    adios2::Variable<T> variable, types::DataArray& dataArray, const size_t step);
};

} // end namespace schema
} // end namespace vtx

#endif /* SVTK_IO_ADIOS2_VTX_SCHEMA_SVTK_VTXxmlVTU_h */
