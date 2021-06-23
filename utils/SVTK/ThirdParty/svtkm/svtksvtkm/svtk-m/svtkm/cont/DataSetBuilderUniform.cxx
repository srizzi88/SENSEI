//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSetBuilderUniform.h>

namespace svtkm
{
namespace cont
{

SVTKM_CONT
DataSetBuilderUniform::DataSetBuilderUniform()
{
}

SVTKM_CONT
svtkm::cont::DataSet DataSetBuilderUniform::Create(const svtkm::Id& dimension,
                                                  const std::string& coordNm)
{
  return CreateDataSet(svtkm::Id3(dimension, 1, 1), VecType(0), VecType(1), coordNm);
}

SVTKM_CONT
svtkm::cont::DataSet DataSetBuilderUniform::Create(const svtkm::Id2& dimensions,
                                                  const std::string& coordNm)
{
  return CreateDataSet(svtkm::Id3(dimensions[0], dimensions[1], 1), VecType(0), VecType(1), coordNm);
}

SVTKM_CONT
svtkm::cont::DataSet DataSetBuilderUniform::Create(const svtkm::Id3& dimensions,
                                                  const std::string& coordNm)
{
  return CreateDataSet(
    svtkm::Id3(dimensions[0], dimensions[1], dimensions[2]), VecType(0), VecType(1), coordNm);
}

SVTKM_CONT
svtkm::cont::DataSet DataSetBuilderUniform::CreateDataSet(const svtkm::Id3& dimensions,
                                                         const svtkm::Vec3f& origin,
                                                         const svtkm::Vec3f& spacing,
                                                         const std::string& coordNm)
{
  svtkm::Id dims[3] = { 1, 1, 1 };
  int ndims = 0;
  for (int i = 0; i < 3; ++i)
  {
    if (dimensions[i] > 1)
    {
      if (spacing[i] <= 0.0f)
      {
        throw svtkm::cont::ErrorBadValue("spacing must be > 0.0");
      }
      dims[ndims++] = dimensions[i];
    }
  }

  svtkm::cont::DataSet dataSet;
  svtkm::cont::ArrayHandleUniformPointCoordinates coords(dimensions, origin, spacing);
  svtkm::cont::CoordinateSystem cs(coordNm, coords);
  dataSet.AddCoordinateSystem(cs);

  if (ndims == 1)
  {
    svtkm::cont::CellSetStructured<1> cellSet;
    cellSet.SetPointDimensions(dims[0]);
    dataSet.SetCellSet(cellSet);
  }
  else if (ndims == 2)
  {
    svtkm::cont::CellSetStructured<2> cellSet;
    cellSet.SetPointDimensions(svtkm::Id2(dims[0], dims[1]));
    dataSet.SetCellSet(cellSet);
  }
  else if (ndims == 3)
  {
    svtkm::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(svtkm::Id3(dims[0], dims[1], dims[2]));
    dataSet.SetCellSet(cellSet);
  }
  else
  {
    throw svtkm::cont::ErrorBadValue("Invalid cell set dimension");
  }

  return dataSet;
}
}
} // end namespace svtkm::cont
