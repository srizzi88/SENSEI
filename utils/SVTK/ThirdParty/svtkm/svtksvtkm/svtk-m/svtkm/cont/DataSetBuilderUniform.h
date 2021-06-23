//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DataSetBuilderUniform_h
#define svtk_m_cont_DataSetBuilderUniform_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT DataSetBuilderUniform
{
  using VecType = svtkm::Vec3f;

public:
  SVTKM_CONT
  DataSetBuilderUniform();

  //1D uniform grid
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const svtkm::Id& dimension,
                                              const T& origin,
                                              const T& spacing,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(
      svtkm::Id3(dimension, 1, 1),
      VecType(static_cast<svtkm::FloatDefault>(origin), 0, 0),
      VecType(static_cast<svtkm::FloatDefault>(spacing), 1, 1),
      coordNm);
  }

  SVTKM_CONT
  static svtkm::cont::DataSet Create(const svtkm::Id& dimension,
                                    const std::string& coordNm = "coords");

  //2D uniform grids.
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const svtkm::Id2& dimensions,
                                              const svtkm::Vec<T, 2>& origin,
                                              const svtkm::Vec<T, 2>& spacing,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(svtkm::Id3(dimensions[0], dimensions[1], 1),
                                                VecType(static_cast<svtkm::FloatDefault>(origin[0]),
                                                        static_cast<svtkm::FloatDefault>(origin[1]),
                                                        0),
                                                VecType(static_cast<svtkm::FloatDefault>(spacing[0]),
                                                        static_cast<svtkm::FloatDefault>(spacing[1]),
                                                        1),
                                                coordNm);
  }

  SVTKM_CONT
  static svtkm::cont::DataSet Create(const svtkm::Id2& dimensions,
                                    const std::string& coordNm = "coords");

  //3D uniform grids.
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const svtkm::Id3& dimensions,
                                              const svtkm::Vec<T, 3>& origin,
                                              const svtkm::Vec<T, 3>& spacing,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderUniform::CreateDataSet(
      svtkm::Id3(dimensions[0], dimensions[1], dimensions[2]),
      VecType(static_cast<svtkm::FloatDefault>(origin[0]),
              static_cast<svtkm::FloatDefault>(origin[1]),
              static_cast<svtkm::FloatDefault>(origin[2])),
      VecType(static_cast<svtkm::FloatDefault>(spacing[0]),
              static_cast<svtkm::FloatDefault>(spacing[1]),
              static_cast<svtkm::FloatDefault>(spacing[2])),
      coordNm);
  }

  SVTKM_CONT
  static svtkm::cont::DataSet Create(const svtkm::Id3& dimensions,
                                    const std::string& coordNm = "coords");

private:
  SVTKM_CONT
  static svtkm::cont::DataSet CreateDataSet(const svtkm::Id3& dimensions,
                                           const svtkm::Vec3f& origin,
                                           const svtkm::Vec3f& spacing,
                                           const std::string& coordNm);
};

} // namespace cont
} // namespace svtkm

#endif //svtk_m_cont_DataSetBuilderUniform_h
