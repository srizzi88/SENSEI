//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_RayTracingTypeDefs_h
#define svtk_m_rendering_raytracing_RayTracingTypeDefs_h

#include <type_traits>
#include <svtkm/List.h>
#include <svtkm/Math.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/DeviceAdapterList.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/cont/VariantArrayHandle.h>

namespace svtkm
{
namespace rendering
{
// A more useful  bounds check that tells you where it happened
#ifndef NDEBUG
#define BOUNDS_CHECK(HANDLE, INDEX)                                                                \
  {                                                                                                \
    BoundsCheck((HANDLE), (INDEX), __FILE__, __LINE__);                                            \
  }
#else
#define BOUNDS_CHECK(HANDLE, INDEX)
#endif

template <typename ArrayHandleType>
SVTKM_EXEC inline void BoundsCheck(const ArrayHandleType& handle,
                                  const svtkm::Id& index,
                                  const char* file,
                                  int line)
{
  if (index < 0 || index >= handle.GetNumberOfValues())
    printf("Bad Index %d  at file %s line %d\n", (int)index, file, line);
}

namespace raytracing
{
template <typename T>
SVTKM_EXEC_CONT inline void GetInfinity(T& svtkmNotUsed(infinity));

template <>
SVTKM_EXEC_CONT inline void GetInfinity<svtkm::Float32>(svtkm::Float32& infinity)
{
  infinity = svtkm::Infinity32();
}

template <>
SVTKM_EXEC_CONT inline void GetInfinity<svtkm::Float64>(svtkm::Float64& infinity)
{
  infinity = svtkm::Infinity64();
}

template <typename Device>
inline std::string GetDeviceString(Device);

template <>
inline std::string GetDeviceString<svtkm::cont::DeviceAdapterTagSerial>(
  svtkm::cont::DeviceAdapterTagSerial)
{
  return "serial";
}

template <>
inline std::string GetDeviceString<svtkm::cont::DeviceAdapterTagTBB>(svtkm::cont::DeviceAdapterTagTBB)
{
  return "tbb";
}

template <>
inline std::string GetDeviceString<svtkm::cont::DeviceAdapterTagOpenMP>(
  svtkm::cont::DeviceAdapterTagOpenMP)
{
  return "openmp";
}

template <>
inline std::string GetDeviceString<svtkm::cont::DeviceAdapterTagCuda>(
  svtkm::cont::DeviceAdapterTagCuda)
{
  return "cuda";
}

struct DeviceStringFunctor
{
  std::string result;
  DeviceStringFunctor()
    : result("")
  {
  }

  template <typename Device>
  SVTKM_CONT bool operator()(Device)
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = GetDeviceString(Device());
    return true;
  }
};

inline std::string GetDeviceString()
{
  DeviceStringFunctor functor;
  svtkm::cont::TryExecute(functor);
  return functor.result;
}

using ColorBuffer4f = svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;
using ColorBuffer4b = svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>;

//Defining types supported by the rendering

//vec3s
using Vec3F = svtkm::Vec3f_32;
using Vec3D = svtkm::Vec3f_64;
using Vec3RenderingTypes = svtkm::List<Vec3F, Vec3D>;

// Scalars Types
using ScalarF = svtkm::Float32;
using ScalarD = svtkm::Float64;

using RayStatusType = svtkm::List<svtkm::UInt8>;

using ScalarRenderingTypes = svtkm::List<ScalarF, ScalarD>;
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_RayTracingTypeDefs_h
