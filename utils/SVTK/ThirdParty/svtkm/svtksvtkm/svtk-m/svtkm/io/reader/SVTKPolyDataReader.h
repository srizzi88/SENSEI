//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_reader_SVTKPolyDataReader_h
#define svtk_m_io_reader_SVTKPolyDataReader_h

#include <svtkm/io/reader/SVTKDataSetReaderBase.h>

#include <svtkm/cont/ArrayPortalToIterators.h>

#include <iterator>

namespace svtkm
{
namespace io
{
namespace reader
{

namespace internal
{

template <typename T>
inline svtkm::cont::ArrayHandle<T> ConcatinateArrayHandles(
  const std::vector<svtkm::cont::ArrayHandle<T>>& arrays)
{
  svtkm::Id size = 0;
  for (std::size_t i = 0; i < arrays.size(); ++i)
  {
    size += arrays[i].GetNumberOfValues();
  }

  svtkm::cont::ArrayHandle<T> out;
  out.Allocate(size);

  auto outp = svtkm::cont::ArrayPortalToIteratorBegin(out.GetPortalControl());
  for (std::size_t i = 0; i < arrays.size(); ++i)
  {
    std::copy(svtkm::cont::ArrayPortalToIteratorBegin(arrays[i].GetPortalConstControl()),
              svtkm::cont::ArrayPortalToIteratorEnd(arrays[i].GetPortalConstControl()),
              outp);
    using DifferenceType = typename std::iterator_traits<decltype(outp)>::difference_type;
    std::advance(outp, static_cast<DifferenceType>(arrays[i].GetNumberOfValues()));
  }

  return out;
}

} // namespace internal

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

class SVTKPolyDataReader : public SVTKDataSetReaderBase
{
public:
  explicit SVTKPolyDataReader(const char* fileName)
    : SVTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != svtkm::io::internal::DATASET_POLYDATA)
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
    std::vector<svtkm::cont::ArrayHandle<svtkm::Id>> connectivityArrays;
    std::vector<svtkm::cont::ArrayHandle<svtkm::IdComponent>> numIndicesArrays;
    std::vector<svtkm::UInt8> shapesBuffer;
    while (!this->DataFile->Stream.eof())
    {
      svtkm::UInt8 shape = svtkm::CELL_SHAPE_EMPTY;
      this->DataFile->Stream >> tag;
      if (tag == "VERTICES")
      {
        shape = svtkm::io::internal::CELL_SHAPE_POLY_VERTEX;
      }
      else if (tag == "LINES")
      {
        shape = svtkm::io::internal::CELL_SHAPE_POLY_LINE;
      }
      else if (tag == "POLYGONS")
      {
        shape = svtkm::CELL_SHAPE_POLYGON;
      }
      else if (tag == "TRIANGLE_STRIPS")
      {
        shape = svtkm::io::internal::CELL_SHAPE_TRIANGLE_STRIP;
      }
      else
      {
        this->DataFile->Stream.seekg(-static_cast<std::streamoff>(tag.length()),
                                     std::ios_base::cur);
        break;
      }

      svtkm::cont::ArrayHandle<svtkm::Id> cellConnectivity;
      svtkm::cont::ArrayHandle<svtkm::IdComponent> cellNumIndices;
      this->ReadCells(cellConnectivity, cellNumIndices);

      connectivityArrays.push_back(cellConnectivity);
      numIndicesArrays.push_back(cellNumIndices);
      shapesBuffer.insert(
        shapesBuffer.end(), static_cast<std::size_t>(cellNumIndices.GetNumberOfValues()), shape);
    }

    svtkm::cont::ArrayHandle<svtkm::Id> connectivity =
      internal::ConcatinateArrayHandles(connectivityArrays);
    svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices =
      internal::ConcatinateArrayHandles(numIndicesArrays);
    svtkm::cont::ArrayHandle<svtkm::UInt8> shapes;
    shapes.Allocate(static_cast<svtkm::Id>(shapesBuffer.size()));
    std::copy(shapesBuffer.begin(),
              shapesBuffer.end(),
              svtkm::cont::ArrayPortalToIteratorBegin(shapes.GetPortalControl()));

    svtkm::cont::ArrayHandle<svtkm::Id> permutation;
    svtkm::io::internal::FixupCellSet(connectivity, numIndices, shapes, permutation);
    this->SetCellsPermutation(permutation);

    if (svtkm::io::internal::IsSingleShape(shapes))
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

#endif // svtk_m_io_reader_SVTKPolyDataReader_h
