/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXsvtkVTI.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXsvtkVTI.h : class that supports ImageData schema in SVTK XML format .vti
 *                  extends abstract ADIOS2xmlSVTK
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef SVTK_IO_ADIOS2_VTX_SCHEMA_SVTK_VTXsvtkVTI_h
#define SVTK_IO_ADIOS2_VTX_SCHEMA_SVTK_VTXsvtkVTI_h

#include <map>
#include <string>
#include <vector>

#include "VTX/schema/svtk/VTXsvtkBase.h"

#include "svtkImageData.h"
#include "svtkNew.h"

namespace vtx
{
namespace schema
{
class VTXsvtkVTI : public VTXsvtkBase
{
public:
  VTXsvtkVTI(const std::string& schema, adios2::IO& io, adios2::Engine& engine);
  ~VTXsvtkVTI();

private:
  /** Could be extended in a container, this is a per-rank ImageData */
  svtkNew<svtkImageData> ImageData;
  /** Store the Whole Extent in physical dimensions, row-major */
  adios2::Dims WholeExtent;

  adios2::Dims GetShape(const types::DataSetType type);
  adios2::Box<adios2::Dims> GetSelection(const types::DataSetType type);

  void DoFill(svtkMultiBlockDataSet* multiBlock, const size_t step) final;
  void ReadPiece(const size_t step, const size_t pieceID) final;

  void Init() final;

#define declare_type(T)                                                                            \
  void SetDimensions(                                                                              \
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step) final;
  SVTK_IO_ADIOS2_VTX_ARRAY_TYPE(declare_type)
#undef declare_type

  template <class T>
  void SetDimensionsCommon(
    adios2::Variable<T> variable, const types::DataArray& dataArray, const size_t step);
};

} // end namespace schema
} // end namespace vtx

#endif /* SVTK_IO_ADIOS2_VTX_SCHEMA_SVTK_VTXsvtkVTI_h */
