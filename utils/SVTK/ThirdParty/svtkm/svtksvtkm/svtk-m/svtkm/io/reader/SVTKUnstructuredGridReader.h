//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_reader_SVTKUnstructuredGridReader_h
#define svtk_m_io_reader_SVTKUnstructuredGridReader_h

#include <svtkm/io/reader/SVTKDataSetReaderBase.h>

namespace svtkm
{
namespace io
{
namespace reader
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

class SVTKUnstructuredGridReader : public SVTKDataSetReaderBase
{
public:
  explicit SVTKUnstructuredGridReader(const char* fileName)
    : SVTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != svtkm::io::internal::DATASET_UNSTRUCTURED_GRID)
    {
      throw svtkm::io::ErrorIO("Incorrect DataSet type");
    }

    //We need to be able to handle VisIt files which dump Field data
    //at the top of a SVTK file
    std::string tag;
    this->DataFile->Stream >> tag;
    if (tag == "FIELD")
    {
      this->ReadGlobalFields();
      this->DataFile->Stream >> tag;
    }

    // Read the points
    internal::parseAssert(tag == "POINTS");
    this->ReadPoints();

    svtkm::Id numPoints = this->DataSet.GetNumberOfPoints();

    // Read the cellset
    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices;
    svtkm::cont::ArrayHandle<svtkm::UInt8> shapes;

    this->DataFile->Stream >> tag;
    internal::parseAssert(tag == "CELLS");

    this->ReadCells(connectivity, numIndices);
    this->ReadShapes(shapes);

    svtkm::cont::ArrayHandle<svtkm::Id> permutation;
    svtkm::io::internal::FixupCellSet(connectivity, numIndices, shapes, permutation);
    this->SetCellsPermutation(permutation);

    //DRP
    if (false) //svtkm::io::internal::IsSingleShape(shapes))
    {
      svtkm::cont::CellSetSingleType<> cellSet;
      cellSet.Fill(numPoints,
                   shapes.GetPortalConstControl().Get(0),
                   numIndices.GetPortalConstControl().Get(0),
                   connectivity);
      this->DataSet.SetCellSet(cellSet);
    }
    else
    {
      auto offsets = svtkm::cont::ConvertNumIndicesToOffsets(numIndices);
      svtkm::cont::CellSetExplicit<> cellSet;
      cellSet.Fill(numPoints, shapes, connectivity, offsets);
      this->DataSet.SetCellSet(cellSet);
    }

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace svtkm::io:reader

#endif // svtk_m_io_reader_SVTKUnstructuredGridReader_h
