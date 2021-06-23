//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_writer_DataSetWriter_h
#define svtk_m_io_writer_DataSetWriter_h

#include <svtkm/CellShape.h>

#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Field.h>

#include <svtkm/io/ErrorIO.h>

#include <svtkm/io/internal/SVTKDataSetTypes.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace svtkm
{
namespace io
{
namespace writer
{

namespace detail
{

struct OutputPointsFunctor
{
private:
  std::ostream& out;

  template <typename PortalType>
  SVTKM_CONT void Output(const PortalType& portal) const
  {
    for (svtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      const int SVTKDims = 3; // SVTK files always require 3 dims for points

      using ValueType = typename PortalType::ValueType;
      using VecType = typename svtkm::VecTraits<ValueType>;

      const ValueType& value = portal.Get(index);

      svtkm::IdComponent numComponents = VecType::GetNumberOfComponents(value);
      for (svtkm::IdComponent c = 0; c < numComponents && c < SVTKDims; c++)
      {
        out << (c == 0 ? "" : " ") << VecType::GetComponent(value, c);
      }
      for (svtkm::IdComponent c = numComponents; c < SVTKDims; c++)
      {
        out << " 0";
      }
      out << std::endl;
    }
  }

public:
  SVTKM_CONT
  OutputPointsFunctor(std::ostream& o)
    : out(o)
  {
  }

  template <typename T, typename Storage>
  SVTKM_CONT void operator()(const svtkm::cont::ArrayHandle<T, Storage>& array) const
  {
    this->Output(array.GetPortalConstControl());
  }
};

struct OutputFieldFunctor
{
private:
  std::ostream& out;

  template <typename PortalType>
  SVTKM_CONT void Output(const PortalType& portal) const
  {
    for (svtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      using ValueType = typename PortalType::ValueType;
      using VecType = typename svtkm::VecTraits<ValueType>;

      const ValueType& value = portal.Get(index);

      svtkm::IdComponent numComponents = VecType::GetNumberOfComponents(value);
      for (svtkm::IdComponent c = 0; c < numComponents; c++)
      {
        out << (c == 0 ? "" : " ") << VecType::GetComponent(value, c);
      }
      out << std::endl;
    }
  }

public:
  SVTKM_CONT
  OutputFieldFunctor(std::ostream& o)
    : out(o)
  {
  }

  template <typename T, typename Storage>
  SVTKM_CONT void operator()(const svtkm::cont::ArrayHandle<T, Storage>& array) const
  {
    this->Output(array.GetPortalConstControl());
  }
};

class GetDataTypeName
{
public:
  GetDataTypeName(std::string& name)
    : Name(&name)
  {
  }

  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType&) const
  {
    using DataType = typename svtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType;
    *this->Name = svtkm::io::internal::DataTypeName<DataType>::Name();
  }

private:
  std::string* Name;
};

} // namespace detail

struct SVTKDataSetWriter
{
private:
  static void WritePoints(std::ostream& out, const svtkm::cont::DataSet& dataSet)
  {
    ///\todo: support other coordinate systems
    int cindex = 0;
    auto cdata = dataSet.GetCoordinateSystem(cindex).GetData();

    svtkm::Id npoints = cdata.GetNumberOfValues();
    out << "POINTS " << npoints << " "
        << svtkm::io::internal::DataTypeName<svtkm::FloatDefault>::Name() << " " << std::endl;

    detail::OutputPointsFunctor{ out }(cdata);
  }

  template <class CellSetType>
  static void WriteExplicitCells(std::ostream& out, const CellSetType& cellSet)
  {
    svtkm::Id nCells = cellSet.GetNumberOfCells();

    svtkm::Id conn_length = 0;
    for (svtkm::Id i = 0; i < nCells; ++i)
    {
      conn_length += 1 + cellSet.GetNumberOfPointsInCell(i);
    }

    out << "CELLS " << nCells << " " << conn_length << std::endl;

    for (svtkm::Id i = 0; i < nCells; ++i)
    {
      svtkm::cont::ArrayHandle<svtkm::Id> ids;
      svtkm::Id nids = cellSet.GetNumberOfPointsInCell(i);
      cellSet.GetIndices(i, ids);
      out << nids;
      auto IdPortal = ids.GetPortalConstControl();
      for (int j = 0; j < nids; ++j)
        out << " " << IdPortal.Get(j);
      out << std::endl;
    }

    out << "CELL_TYPES " << nCells << std::endl;
    for (svtkm::Id i = 0; i < nCells; ++i)
    {
      svtkm::Id shape = cellSet.GetCellShape(i);
      out << shape << std::endl;
    }
  }

  static void WriteVertexCells(std::ostream& out, const svtkm::cont::DataSet& dataSet)
  {
    svtkm::Id nCells = dataSet.GetCoordinateSystem(0).GetNumberOfPoints();

    out << "CELLS " << nCells << " " << nCells * 2 << std::endl;
    for (int i = 0; i < nCells; i++)
    {
      out << "1 " << i << std::endl;
    }
    out << "CELL_TYPES " << nCells << std::endl;
    for (int i = 0; i < nCells; i++)
    {
      out << svtkm::CELL_SHAPE_VERTEX << std::endl;
    }
  }

  static void WritePointFields(std::ostream& out, const svtkm::cont::DataSet& dataSet)
  {
    bool wrote_header = false;
    for (svtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
    {
      const svtkm::cont::Field field = dataSet.GetField(f);

      if (field.GetAssociation() != svtkm::cont::Field::Association::POINTS)
      {
        continue;
      }

      svtkm::Id npoints = field.GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4)
      {
        continue;
      }

      if (!wrote_header)
      {
        out << "POINT_DATA " << npoints << std::endl;
        wrote_header = true;
      }

      std::string typeName;
      svtkm::cont::CastAndCall(field, detail::GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() << " " << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      svtkm::cont::CastAndCall(field, detail::OutputFieldFunctor(out));
    }
  }

  static void WriteCellFields(std::ostream& out, const svtkm::cont::DataSet& dataSet)
  {
    bool wrote_header = false;
    for (svtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
    {
      const svtkm::cont::Field field = dataSet.GetField(f);
      if (!field.IsFieldCell())
      {
        continue;
      }


      svtkm::Id ncells = field.GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4)
        continue;

      if (!wrote_header)
      {
        out << "CELL_DATA " << ncells << std::endl;
        wrote_header = true;
      }

      std::string typeName;
      svtkm::cont::CastAndCall(field, detail::GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() << " " << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      svtkm::cont::CastAndCall(field, detail::OutputFieldFunctor(out));
    }
  }

  static void WriteDataSetAsPoints(std::ostream& out, const svtkm::cont::DataSet& dataSet)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, dataSet);
    WriteVertexCells(out, dataSet);
  }

  template <class CellSetType>
  static void WriteDataSetAsUnstructured(std::ostream& out,
                                         const svtkm::cont::DataSet& dataSet,
                                         const CellSetType& cellSet)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, dataSet);
    WriteExplicitCells(out, cellSet);
  }

  template <svtkm::IdComponent DIM>
  static void WriteDataSetAsStructured(std::ostream& out,
                                       const svtkm::cont::DataSet& dataSet,
                                       const svtkm::cont::CellSetStructured<DIM>& cellSet)
  {
    ///\todo: support uniform/rectilinear
    out << "DATASET STRUCTURED_GRID" << std::endl;

    auto pointDimensions = cellSet.GetPointDimensions();
    using VTraits = svtkm::VecTraits<decltype(pointDimensions)>;

    out << "DIMENSIONS ";
    out << VTraits::GetComponent(pointDimensions, 0) << " ";
    out << (DIM > 1 ? VTraits::GetComponent(pointDimensions, 1) : 1) << " ";
    out << (DIM > 2 ? VTraits::GetComponent(pointDimensions, 2) : 1) << " ";

    WritePoints(out, dataSet);
  }

  static void Write(std::ostream& out, const svtkm::cont::DataSet& dataSet, bool just_points = false)
  {
    out << "# svtk DataFile Version 3.0" << std::endl;
    out << "svtk output" << std::endl;
    out << "ASCII" << std::endl;

    if (just_points)
    {
      WriteDataSetAsPoints(out, dataSet);
      WritePointFields(out, dataSet);
    }
    else
    {
      svtkm::cont::DynamicCellSet cellSet = dataSet.GetCellSet();
      if (cellSet.IsType<svtkm::cont::CellSetExplicit<>>())
      {
        WriteDataSetAsUnstructured(out, dataSet, cellSet.Cast<svtkm::cont::CellSetExplicit<>>());
      }
      else if (cellSet.IsType<svtkm::cont::CellSetStructured<1>>())
      {
        WriteDataSetAsStructured(out, dataSet, cellSet.Cast<svtkm::cont::CellSetStructured<1>>());
      }
      else if (cellSet.IsType<svtkm::cont::CellSetStructured<2>>())
      {
        WriteDataSetAsStructured(out, dataSet, cellSet.Cast<svtkm::cont::CellSetStructured<2>>());
      }
      else if (cellSet.IsType<svtkm::cont::CellSetStructured<3>>())
      {
        WriteDataSetAsStructured(out, dataSet, cellSet.Cast<svtkm::cont::CellSetStructured<3>>());
      }
      else if (cellSet.IsType<svtkm::cont::CellSetSingleType<>>())
      {
        // these function just like explicit cell sets
        WriteDataSetAsUnstructured(out, dataSet, cellSet.Cast<svtkm::cont::CellSetSingleType<>>());
      }
      else
      {
        throw svtkm::cont::ErrorBadType("Could not determine type to write out.");
      }

      WritePointFields(out, dataSet);
      WriteCellFields(out, dataSet);
    }
  }

public:
  SVTKM_CONT
  explicit SVTKDataSetWriter(const std::string& filename)
    : FileName(filename)
  {
  }

  SVTKM_CONT
  void WriteDataSet(const svtkm::cont::DataSet& dataSet, bool just_points = false) const
  {
    if (dataSet.GetNumberOfCoordinateSystems() < 1)
    {
      throw svtkm::cont::ErrorBadValue(
        "DataSet has no coordinate system, which is not supported by SVTK file format.");
    }
    try
    {
      std::ofstream fileStream(this->FileName.c_str(), std::fstream::trunc);
      this->Write(fileStream, dataSet, just_points);
      fileStream.close();
    }
    catch (std::ofstream::failure& error)
    {
      throw svtkm::io::ErrorIO(error.what());
    }
  }

private:
  std::string FileName;

}; //struct SVTKDataSetWriter
}
}
} //namespace svtkm::io::writer

#endif //svtk_m_io_writer_DataSetWriter_h
