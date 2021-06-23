//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSetBuilderExplicit.h>

namespace svtkm
{
namespace cont
{

SVTKM_CONT
DataSetBuilderExplicitIterative::DataSetBuilderExplicitIterative()
{
}


SVTKM_CONT
void DataSetBuilderExplicitIterative::Begin(const std::string& coordName)
{
  this->coordNm = coordName;
  this->points.resize(0);
  this->shapes.resize(0);
  this->numIdx.resize(0);
  this->connectivity.resize(0);
}

//Define points.
SVTKM_CONT
svtkm::cont::DataSet DataSetBuilderExplicitIterative::Create()
{
  DataSetBuilderExplicit dsb;
  return dsb.Create(points, shapes, numIdx, connectivity, coordNm);
}

SVTKM_CONT
svtkm::Id DataSetBuilderExplicitIterative::AddPoint(const svtkm::Vec3f& pt)
{
  points.push_back(pt);
  svtkm::Id id = static_cast<svtkm::Id>(points.size());
  //ID is zero-based.
  return id - 1;
}

SVTKM_CONT
svtkm::Id DataSetBuilderExplicitIterative::AddPoint(const svtkm::FloatDefault& x,
                                                   const svtkm::FloatDefault& y,
                                                   const svtkm::FloatDefault& z)
{
  points.push_back(svtkm::make_Vec(x, y, z));
  svtkm::Id id = static_cast<svtkm::Id>(points.size());
  //ID is zero-based.
  return id - 1;
}

//Define cells.
SVTKM_CONT
void DataSetBuilderExplicitIterative::AddCell(svtkm::UInt8 shape)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(0);
}

SVTKM_CONT
void DataSetBuilderExplicitIterative::AddCell(const svtkm::UInt8& shape,
                                              const std::vector<svtkm::Id>& conn)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(static_cast<svtkm::IdComponent>(conn.size()));
  connectivity.insert(connectivity.end(), conn.begin(), conn.end());
}

SVTKM_CONT
void DataSetBuilderExplicitIterative::AddCell(const svtkm::UInt8& shape,
                                              const svtkm::Id* conn,
                                              const svtkm::IdComponent& n)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(n);
  for (int i = 0; i < n; i++)
  {
    connectivity.push_back(conn[i]);
  }
}

SVTKM_CONT
void DataSetBuilderExplicitIterative::AddCellPoint(svtkm::Id pointIndex)
{
  SVTKM_ASSERT(this->numIdx.size() > 0);
  this->connectivity.push_back(pointIndex);
  this->numIdx.back() += 1;
}
}
} // end namespace svtkm::cont
