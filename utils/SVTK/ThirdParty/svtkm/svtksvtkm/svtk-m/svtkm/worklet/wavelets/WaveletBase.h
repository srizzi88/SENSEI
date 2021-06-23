//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_wavelets_waveletbase_h
#define svtk_m_worklet_wavelets_waveletbase_h

#include <svtkm/worklet/wavelets/WaveletFilter.h>
#include <svtkm/worklet/wavelets/WaveletTransforms.h>

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayGetValues.h>

namespace svtkm
{
namespace worklet
{

namespace wavelets
{

// Functionalities are similar to MatWaveBase in VAPoR.
class WaveletBase
{
public:
  // Constructor
  WaveletBase(WaveletName name)
    : wname(name)
    , filter(name)
  {
    if (wname == CDF9_7 || wname == BIOR4_4 || wname == CDF5_3 || wname == BIOR2_2)
    {
      this->wmode = SYMW; // Default extension mode, see MatWaveBase.cpp
    }
    else if (wname == HAAR || wname == BIOR1_1 || wname == CDF8_4 || wname == BIOR3_3)
    {
      this->wmode = SYMH;
    }
  }

  // Returns length of approximation coefficients from a decomposition pass.
  svtkm::Id GetApproxLength(svtkm::Id sigInLen)
  {
    if (sigInLen % 2 != 0)
    {
      return ((sigInLen + 1) / 2);
    }
    else
    {
      return ((sigInLen) / 2);
    }
  }

  // Returns length of detail coefficients from a decomposition pass
  svtkm::Id GetDetailLength(svtkm::Id sigInLen)
  {
    if (sigInLen % 2 != 0)
    {
      return ((sigInLen - 1) / 2);
    }
    else
    {
      return ((sigInLen) / 2);
    }
  }

  // Returns length of coefficients generated in a decomposition pass
  svtkm::Id GetCoeffLength(svtkm::Id sigInLen)
  {
    return (GetApproxLength(sigInLen) + GetDetailLength(sigInLen));
  }
  svtkm::Id GetCoeffLength2(svtkm::Id sigInX, svtkm::Id sigInY)
  {
    return (GetCoeffLength(sigInX) * GetCoeffLength(sigInY));
  }
  svtkm::Id GetCoeffLength3(svtkm::Id sigInX, svtkm::Id sigInY, svtkm::Id sigInZ)
  {
    return (GetCoeffLength(sigInX) * GetCoeffLength(sigInY) * GetCoeffLength(sigInZ));
  }

  // Returns maximum wavelet decomposition level
  svtkm::Id GetWaveletMaxLevel(svtkm::Id sigInLen)
  {
    svtkm::Id filterLen = this->filter.GetFilterLength();
    svtkm::Id level;
    this->WaveLengthValidate(sigInLen, filterLen, level);
    return level;
  }

  // perform a device copy. The whole 1st array to a certain start location of the 2nd array
  template <typename ArrayType1, typename ArrayType2>
  void DeviceCopyStartX(const ArrayType1& srcArray, ArrayType2& dstArray, svtkm::Id startIdx)
  {
    using CopyType = svtkm::worklet::wavelets::CopyWorklet;
    CopyType cp(startIdx);
    svtkm::worklet::DispatcherMapField<CopyType> dispatcher(cp);
    dispatcher.Invoke(srcArray, dstArray);
  }

  // Assign zero value to a certain location of an array
  template <typename ArrayType>
  void DeviceAssignZero(ArrayType& array, svtkm::Id index)
  {
    using ZeroWorklet = svtkm::worklet::wavelets::AssignZeroWorklet;
    ZeroWorklet worklet(index);
    svtkm::worklet::DispatcherMapField<ZeroWorklet> dispatcher(worklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a certain row to a matrix
  template <typename ArrayType>
  void DeviceAssignZero2DRow(ArrayType& array,
                             svtkm::Id dimX,
                             svtkm::Id dimY, // input
                             svtkm::Id rowIdx)
  {
    using AssignZero2DType = svtkm::worklet::wavelets::AssignZero2DWorklet;
    AssignZero2DType zeroWorklet(dimX, dimY, -1, rowIdx);
    svtkm::worklet::DispatcherMapField<AssignZero2DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a certain column to a matrix
  template <typename ArrayType>
  void DeviceAssignZero2DColumn(ArrayType& array,
                                svtkm::Id dimX,
                                svtkm::Id dimY, // input
                                svtkm::Id colIdx)
  {
    using AssignZero2DType = svtkm::worklet::wavelets::AssignZero2DWorklet;
    AssignZero2DType zeroWorklet(dimX, dimY, colIdx, -1);
    svtkm::worklet::DispatcherMapField<AssignZero2DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the X axis (Left-Right direction)
  template <typename ArrayType>
  void DeviceAssignZero3DPlaneX(ArrayType& array, // input array
                                svtkm::Id dimX,
                                svtkm::Id dimY,
                                svtkm::Id dimZ,  // dims of input
                                svtkm::Id zeroX) // X idx to set zero
  {
    using AssignZero3DType = svtkm::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, zeroX, -1, -1);
    svtkm::worklet::DispatcherMapField<AssignZero3DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the Y axis (Top-Down direction)
  template <typename ArrayType>
  void DeviceAssignZero3DPlaneY(ArrayType& array, // input array
                                svtkm::Id dimX,
                                svtkm::Id dimY,
                                svtkm::Id dimZ,  // dims of input
                                svtkm::Id zeroY) // Y idx to set zero
  {
    using AssignZero3DType = svtkm::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, -1, zeroY, -1);
    svtkm::worklet::DispatcherMapField<AssignZero3DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the Z axis (Front-Back direction)
  template <typename ArrayType>
  void DeviceAssignZero3DPlaneZ(ArrayType& array, // input array
                                svtkm::Id dimX,
                                svtkm::Id dimY,
                                svtkm::Id dimZ,  // dims of input
                                svtkm::Id zeroZ) // Y idx to set zero
  {
    using AssignZero3DType = svtkm::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, -1, -1, zeroZ);
    svtkm::worklet::DispatcherMapField<AssignZero3DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Sort by the absolute value on device
  struct SortLessAbsFunctor
  {
    template <typename T>
    SVTKM_EXEC bool operator()(const T& x, const T& y) const
    {
      return svtkm::Abs(x) < svtkm::Abs(y);
    }
  };
  template <typename ArrayType>
  void DeviceSort(ArrayType& array)
  {
    svtkm::cont::Algorithm::Sort(array, SortLessAbsFunctor());
  }

  // Reduce to the sum of all values on device
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceSum(const ArrayType& array)
  {
    return svtkm::cont::Algorithm::Reduce(array, static_cast<typename ArrayType::ValueType>(0.0));
  }

  // Helper functors for finding the max and min of an array
  struct minFunctor
  {
    template <typename FieldType>
    SVTKM_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return Min(x, y);
    }
  };
  struct maxFunctor
  {
    template <typename FieldType>
    SVTKM_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return svtkm::Max(x, y);
    }
  };

  // Device Min and Max functions
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceMax(const ArrayType& array)
  {
    typename ArrayType::ValueType initVal = svtkm::cont::ArrayGetValue(0, array);
    return svtkm::cont::Algorithm::Reduce(array, initVal, maxFunctor());
  }
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceMin(const ArrayType& array)
  {
    typename ArrayType::ValueType initVal = svtkm::cont::ArrayGetValue(0, array);
    return svtkm::cont::Algorithm::Reduce(array, initVal, minFunctor());
  }

  // Max absolute value of an array
  struct maxAbsFunctor
  {
    template <typename FieldType>
    SVTKM_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return svtkm::Max(svtkm::Abs(x), svtkm::Abs(y));
    }
  };
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceMaxAbs(const ArrayType& array)
  {
    typename ArrayType::ValueType initVal = array.GetPortalConstControl().Get(0);
    return svtkm::cont::Algorithm::Reduce(array, initVal, maxAbsFunctor());
  }

  // Calculate variance of an array
  template <typename ArrayType>
  svtkm::Float64 DeviceCalculateVariance(ArrayType& array)
  {
    svtkm::Float64 mean = static_cast<svtkm::Float64>(this->DeviceSum(array)) /
      static_cast<svtkm::Float64>(array.GetNumberOfValues());

    svtkm::cont::ArrayHandle<svtkm::Float64> squaredDeviation;

    // Use a worklet
    using SDWorklet = svtkm::worklet::wavelets::SquaredDeviation;
    SDWorklet sdw(mean);
    svtkm::worklet::DispatcherMapField<SDWorklet> dispatcher(sdw);
    dispatcher.Invoke(array, squaredDeviation);

    svtkm::Float64 sdMean = this->DeviceSum(squaredDeviation) /
      static_cast<svtkm::Float64>(squaredDeviation.GetNumberOfValues());

    return sdMean;
  }

  // Copy a small rectangle to a big rectangle
  template <typename SmallArrayType, typename BigArrayType>
  void DeviceRectangleCopyTo(const SmallArrayType& smallRect,
                             svtkm::Id smallX,
                             svtkm::Id smallY,
                             BigArrayType& bigRect,
                             svtkm::Id bigX,
                             svtkm::Id bigY,
                             svtkm::Id startX,
                             svtkm::Id startY)
  {
    using CopyToWorklet = svtkm::worklet::wavelets::RectangleCopyTo;
    CopyToWorklet cp(smallX, smallY, bigX, bigY, startX, startY);
    svtkm::worklet::DispatcherMapField<CopyToWorklet> dispatcher(cp);
    dispatcher.Invoke(smallRect, bigRect);
  }

  // Copy a small cube to a big cube
  template <typename SmallArrayType, typename BigArrayType>
  void DeviceCubeCopyTo(const SmallArrayType& smallCube,
                        svtkm::Id smallX,
                        svtkm::Id smallY,
                        svtkm::Id smallZ,
                        BigArrayType& bigCube,
                        svtkm::Id bigX,
                        svtkm::Id bigY,
                        svtkm::Id bigZ,
                        svtkm::Id startX,
                        svtkm::Id startY,
                        svtkm::Id startZ)
  {
    using CopyToWorklet = svtkm::worklet::wavelets::CubeCopyTo;
    CopyToWorklet cp(smallX, smallY, smallZ, bigX, bigY, bigZ, startX, startY, startZ);
    svtkm::worklet::DispatcherMapField<CopyToWorklet> dispatcher(cp);
    dispatcher.Invoke(smallCube, bigCube);
  }

  template <typename ArrayType>
  void Print2DArray(const std::string& str, const ArrayType& arr, svtkm::Id dimX)
  {
    std::cerr << str << std::endl;
    for (svtkm::Id i = 0; i < arr.GetNumberOfValues(); i++)
    {
      std::cerr << arr.GetPortalConstControl().Get(i) << "  ";
      if (i % dimX == dimX - 1)
      {
        std::cerr << std::endl;
      }
    }
  }

protected:
  WaveletName wname;
  DWTMode wmode;
  WaveletFilter filter;

  void WaveLengthValidate(svtkm::Id sigInLen, svtkm::Id filterLength, svtkm::Id& level)
  {
    if (sigInLen < filterLength)
    {
      level = 0;
    }
    else
    {
      level = static_cast<svtkm::Id>(
        svtkm::Floor(1.0 + svtkm::Log2(static_cast<svtkm::Float64>(sigInLen) /
                                     static_cast<svtkm::Float64>(filterLength))));
    }
  }

}; // class WaveletBase.

} // namespace wavelets

} // namespace worklet
} // namespace svtkm

#endif
