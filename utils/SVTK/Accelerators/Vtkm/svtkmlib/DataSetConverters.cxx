//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include "DataSetConverters.h"

#include "ArrayConverters.h"
#include "CellSetConverters.h"
#include "ImageDataConverter.h"
#include "PolyDataConverter.h"
#include "UnstructuredGridConverter.h"

#include "svtkmDataArray.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkDataObject.h"
#include "svtkDataObjectTypes.h"
#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/CoordinateSystem.hxx>
#include <svtkm/cont/Field.h>

namespace tosvtkm
{

namespace
{

template <typename T>
svtkm::cont::CoordinateSystem deduce_container(svtkPoints* points)
{
  typedef svtkm::Vec<T, 3> Vec3;

  svtkAOSDataArrayTemplate<T>* typedIn = svtkAOSDataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn)
  {
    auto p = DataArrayToArrayHandle<svtkAOSDataArrayTemplate<T>, 3>::Wrap(typedIn);
    return svtkm::cont::CoordinateSystem("coords", p);
  }

  svtkSOADataArrayTemplate<T>* typedIn2 =
    svtkSOADataArrayTemplate<T>::FastDownCast(points->GetData());
  if (typedIn2)
  {
    auto p = DataArrayToArrayHandle<svtkSOADataArrayTemplate<T>, 3>::Wrap(typedIn2);
    return svtkm::cont::CoordinateSystem("coords", p);
  }

  svtkmDataArray<T>* typedIn3 = svtkmDataArray<T>::SafeDownCast(points->GetData());
  if (typedIn3)
  {
    return svtkm::cont::CoordinateSystem("coords", typedIn3->GetVtkmVariantArrayHandle());
  }

  typedef svtkm::Vec<T, 3> Vec3;
  Vec3* xyz = nullptr;
  return svtkm::cont::make_CoordinateSystem("coords", xyz, 0);
}
}
//------------------------------------------------------------------------------
// convert a svtkPoints array into a coordinate system
svtkm::cont::CoordinateSystem Convert(svtkPoints* points)
{
  if (points)
  {
    if (points->GetDataType() == SVTK_FLOAT)
    {
      return deduce_container<svtkm::Float32>(points);
    }
    else if (points->GetDataType() == SVTK_DOUBLE)
    {
      return deduce_container<svtkm::Float64>(points);
    }
  }

  // unsupported/null point set
  typedef svtkm::Vec<svtkm::Float32, 3> Vec3;
  Vec3* xyz = nullptr;
  return svtkm::cont::make_CoordinateSystem("coords", xyz, 0);
}

//------------------------------------------------------------------------------
// convert an structured grid type
svtkm::cont::DataSet Convert(svtkStructuredGrid* input, FieldsFlag fields)
{
  const int dimensionality = input->GetDataDimension();
  int dims[3];
  input->GetDimensions(dims);

  svtkm::cont::DataSet dataset;

  // first step convert the points over to an array handle
  svtkm::cont::CoordinateSystem coords = Convert(input->GetPoints());
  dataset.AddCoordinateSystem(coords);

  // second step is to create structured cellset that represe
  if (dimensionality == 1)
  {
    svtkm::cont::CellSetStructured<1> cells;
    cells.SetPointDimensions(dims[0]);
    dataset.SetCellSet(cells);
  }
  else if (dimensionality == 2)
  {
    svtkm::cont::CellSetStructured<2> cells;
    cells.SetPointDimensions(svtkm::make_Vec(dims[0], dims[1]));
    dataset.SetCellSet(cells);
  }
  else
  { // going to presume 3d for everything else
    svtkm::cont::CellSetStructured<3> cells;
    cells.SetPointDimensions(svtkm::make_Vec(dims[0], dims[1], dims[2]));
    dataset.SetCellSet(cells);
  }

  ProcessFields(input, dataset, fields);

  return dataset;
}

//------------------------------------------------------------------------------
// determine the type and call the proper Convert routine
svtkm::cont::DataSet Convert(svtkDataSet* input, FieldsFlag fields)
{
  switch (input->GetDataObjectType())
  {
    case SVTK_UNSTRUCTURED_GRID:
      return Convert(svtkUnstructuredGrid::SafeDownCast(input), fields);
    case SVTK_STRUCTURED_GRID:
      return Convert(svtkStructuredGrid::SafeDownCast(input), fields);
    case SVTK_UNIFORM_GRID:
    case SVTK_IMAGE_DATA:
      return Convert(svtkImageData::SafeDownCast(input), fields);
    case SVTK_POLY_DATA:
      return Convert(svtkPolyData::SafeDownCast(input), fields);

    case SVTK_UNSTRUCTURED_GRID_BASE:
    case SVTK_RECTILINEAR_GRID:
    case SVTK_STRUCTURED_POINTS:
    default:
      return svtkm::cont::DataSet();
  }
}

} // namespace tosvtkm

namespace fromsvtkm
{

namespace
{

struct ComputeExtents
{
  template <svtkm::IdComponent Dim>
  void operator()(const svtkm::cont::CellSetStructured<Dim>& cs,
    const svtkm::Id3& structuredCoordsDims, int extent[6]) const
  {
    auto extStart = cs.GetGlobalPointIndexStart();
    for (int i = 0, ii = 0; i < 3; ++i)
    {
      if (structuredCoordsDims[i] > 1)
      {
        extent[2 * i] = svtkm::VecTraits<decltype(extStart)>::GetComponent(extStart, ii++);
        extent[(2 * i) + 1] = extent[2 * i] + structuredCoordsDims[i] - 1;
      }
      else
      {
        extent[2 * i] = extent[(2 * i) + 1] = 0;
      }
    }
  }

  template <svtkm::IdComponent Dim>
  void operator()(const svtkm::cont::CellSetStructured<Dim>& cs, int extent[6]) const
  {
    auto extStart = cs.GetGlobalPointIndexStart();
    auto csDim = cs.GetPointDimensions();
    for (int i = 0; i < Dim; ++i)
    {
      extent[2 * i] = svtkm::VecTraits<decltype(extStart)>::GetComponent(extStart, i);
      extent[(2 * i) + 1] =
        extent[2 * i] + svtkm::VecTraits<decltype(csDim)>::GetComponent(csDim, i) - 1;
    }
    for (int i = Dim; i < 3; ++i)
    {
      extent[2 * i] = extent[(2 * i) + 1] = 0;
    }
  }
};
} // anonymous namespace

void PassAttributesInformation(svtkDataSetAttributes* input, svtkDataSetAttributes* output)
{
  for (int attribType = 0; attribType < svtkDataSetAttributes::NUM_ATTRIBUTES; attribType++)
  {
    svtkDataArray* attribute = input->GetAttribute(attribType);
    if (attribute == nullptr)
    {
      continue;
    }
    output->SetActiveAttribute(attribute->GetName(), attribType);
  }
}

bool Convert(const svtkm::cont::DataSet& svtkmOut, svtkRectilinearGrid* output, svtkDataSet* input)
{
  using ListCellSetStructured = svtkm::List<svtkm::cont::CellSetStructured<1>,
    svtkm::cont::CellSetStructured<2>, svtkm::cont::CellSetStructured<3> >;
  auto cellSet = svtkmOut.GetCellSet().ResetCellSetList(ListCellSetStructured{});

  using coordType =
    svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
      svtkm::cont::ArrayHandle<svtkm::FloatDefault>, svtkm::cont::ArrayHandle<svtkm::FloatDefault> >;
  auto coordsArray = svtkm::cont::Cast<coordType>(svtkmOut.GetCoordinateSystem().GetData());

  svtkSmartPointer<svtkDataArray> xArray =
    Convert(svtkm::cont::make_FieldPoint("xArray", coordsArray.GetStorage().GetFirstArray()));
  svtkSmartPointer<svtkDataArray> yArray =
    Convert(svtkm::cont::make_FieldPoint("yArray", coordsArray.GetStorage().GetSecondArray()));
  svtkSmartPointer<svtkDataArray> zArray =
    Convert(svtkm::cont::make_FieldPoint("zArray", coordsArray.GetStorage().GetThirdArray()));

  if (!xArray || !yArray || !zArray)
  {
    return false;
  }

  svtkm::Id3 dims(
    xArray->GetNumberOfValues(), yArray->GetNumberOfValues(), zArray->GetNumberOfValues());

  int extents[6];
  svtkm::cont::CastAndCall(cellSet, ComputeExtents{}, dims, extents);

  output->SetExtent(extents);
  output->SetXCoordinates(xArray);
  output->SetYCoordinates(yArray);
  output->SetZCoordinates(zArray);

  // Next we need to convert any extra fields from svtkm over to svtk
  if (!fromsvtkm::ConvertArrays(svtkmOut, output))
  {
    return false;
  }

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return true;
}

bool Convert(const svtkm::cont::DataSet& svtkmOut, svtkStructuredGrid* output, svtkDataSet* input)
{
  using ListCellSetStructured = svtkm::List<svtkm::cont::CellSetStructured<1>,
    svtkm::cont::CellSetStructured<2>, svtkm::cont::CellSetStructured<3> >;
  auto cellSet = svtkmOut.GetCellSet().ResetCellSetList(ListCellSetStructured{});

  int extents[6];
  svtkm::cont::CastAndCall(cellSet, ComputeExtents{}, extents);

  svtkSmartPointer<svtkPoints> points = Convert(svtkmOut.GetCoordinateSystem());
  if (!points)
  {
    return false;
  }

  output->SetExtent(extents);
  output->SetPoints(points);

  // Next we need to convert any extra fields from svtkm over to svtk
  if (!fromsvtkm::ConvertArrays(svtkmOut, output))
  {
    return false;
  }

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return true;
}

} // namespace fromsvtkm
