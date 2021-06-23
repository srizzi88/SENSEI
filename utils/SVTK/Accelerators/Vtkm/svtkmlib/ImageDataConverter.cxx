/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageDataConverter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "ImageDataConverter.h"

#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/DataSetBuilderUniform.h>

#include "ArrayConverters.h"
#include "DataSetConverters.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkPointData.h"

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
};

struct SetGlobalPointIndexStart
{
  template <svtkm::IdComponent Dim, typename DynamicCellSetType>
  void operator()(const svtkm::cont::CellSetStructured<Dim>&, const svtkm::Id3& structuredCoordsDims,
    const int extent[6], DynamicCellSetType& dcs) const
  {
    typename svtkm::cont::CellSetStructured<Dim>::SchedulingRangeType extStart{};
    for (int i = 0, ii = 0; i < 3; ++i)
    {
      if (structuredCoordsDims[i] > 1)
      {
        svtkm::VecTraits<decltype(extStart)>::SetComponent(extStart, ii++, extent[2 * i]);
      }
    }
    svtkm::cont::Cast<svtkm::cont::CellSetStructured<Dim> >(dcs).SetGlobalPointIndexStart(extStart);
  }
};

} // anonymous namespace

namespace tosvtkm
{

//------------------------------------------------------------------------------
// convert an image data type
svtkm::cont::DataSet Convert(svtkImageData* input, FieldsFlag fields)
{
  int extent[6];
  input->GetExtent(extent);
  double vorigin[3];
  input->GetOrigin(vorigin);
  double vspacing[3];
  input->GetSpacing(vspacing);
  int vdims[3];
  input->GetDimensions(vdims);

  svtkm::Vec<svtkm::FloatDefault, 3> origin(
    static_cast<svtkm::FloatDefault>((static_cast<double>(extent[0]) * vspacing[0]) + vorigin[0]),
    static_cast<svtkm::FloatDefault>((static_cast<double>(extent[2]) * vspacing[1]) + vorigin[1]),
    static_cast<svtkm::FloatDefault>((static_cast<double>(extent[4]) * vspacing[2]) + vorigin[2]));
  svtkm::Vec<svtkm::FloatDefault, 3> spacing(static_cast<svtkm::FloatDefault>(vspacing[0]),
    static_cast<svtkm::FloatDefault>(vspacing[1]), static_cast<svtkm::FloatDefault>(vspacing[2]));
  svtkm::Id3 dims(vdims[0], vdims[1], vdims[2]);

  svtkm::cont::DataSet dataset = svtkm::cont::DataSetBuilderUniform::Create(dims, origin, spacing);

  using ListCellSetStructured = svtkm::List<svtkm::cont::CellSetStructured<1>,
    svtkm::cont::CellSetStructured<2>, svtkm::cont::CellSetStructured<3> >;
  auto cellSet = dataset.GetCellSet().ResetCellSetList(ListCellSetStructured{});
  svtkm::cont::CastAndCall(cellSet, SetGlobalPointIndexStart{}, dims, extent, dataset.GetCellSet());

  ProcessFields(input, dataset, fields);

  return dataset;
}

} // tosvtkm

namespace fromsvtkm
{

bool Convert(
  const svtkm::cont::DataSet& voutput, int extents[6], svtkImageData* output, svtkDataSet* input)
{
  svtkm::cont::CoordinateSystem cs = voutput.GetCoordinateSystem();
  if (!cs.GetData().IsType<svtkm::cont::ArrayHandleUniformPointCoordinates>())
  {
    return false;
  }

  auto points = cs.GetData().Cast<svtkm::cont::ArrayHandleUniformPointCoordinates>();
  auto portal = points.GetPortalConstControl();

  auto origin = portal.GetOrigin();
  auto spacing = portal.GetSpacing();
  auto dim = portal.GetDimensions();
  SVTKM_ASSERT((extents[1] - extents[0] + 1) == dim[0] && (extents[3] - extents[2] + 1) == dim[1] &&
    (extents[5] - extents[4] + 1) == dim[2]);

  origin[0] -= static_cast<svtkm::FloatDefault>(extents[0]) * spacing[0];
  origin[1] -= static_cast<svtkm::FloatDefault>(extents[2]) * spacing[1];
  origin[2] -= static_cast<svtkm::FloatDefault>(extents[4]) * spacing[2];

  output->SetExtent(extents);
  output->SetOrigin(origin[0], origin[1], origin[2]);
  output->SetSpacing(spacing[0], spacing[1], spacing[2]);

  // Next we need to convert any extra fields from svtkm over to svtk
  bool arraysConverted = fromsvtkm::ConvertArrays(voutput, output);

  // Pass information about attributes.
  PassAttributesInformation(input->GetPointData(), output->GetPointData());
  PassAttributesInformation(input->GetCellData(), output->GetCellData());

  return arraysConverted;
}

bool Convert(const svtkm::cont::DataSet& voutput, svtkImageData* output, svtkDataSet* input)
{
  svtkm::cont::CoordinateSystem cs = voutput.GetCoordinateSystem();
  if (!cs.GetData().IsType<svtkm::cont::ArrayHandleUniformPointCoordinates>())
  {
    return false;
  }

  auto points = cs.GetData().Cast<svtkm::cont::ArrayHandleUniformPointCoordinates>();
  auto portal = points.GetPortalConstControl();

  auto dim = portal.GetDimensions();
  int extents[6];
  using ListCellSetStructured = svtkm::List<svtkm::cont::CellSetStructured<1>,
    svtkm::cont::CellSetStructured<2>, svtkm::cont::CellSetStructured<3> >;
  auto cellSet = voutput.GetCellSet().ResetCellSetList(ListCellSetStructured{});
  svtkm::cont::CastAndCall(cellSet, ComputeExtents{}, dim, extents);

  return Convert(voutput, extents, output, input);
}

} // fromsvtkm
