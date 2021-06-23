//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_waveletcompressor_h
#define svtk_m_worklet_waveletcompressor_h

#include <svtkm/worklet/wavelets/WaveletDWT.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayGetValues.h>

namespace svtkm
{
namespace worklet
{

class WaveletCompressor : public svtkm::worklet::wavelets::WaveletDWT
{
public:
  // Constructor
  WaveletCompressor(wavelets::WaveletName name)
    : WaveletDWT(name)
  {
  }

  // Multi-level 1D wavelet decomposition
  template <typename SignalArrayType, typename CoeffArrayType>
  SVTKM_CONT svtkm::Id WaveDecompose(const SignalArrayType& sigIn, // Input
                                   svtkm::Id nLevels,             // n levels of DWT
                                   CoeffArrayType& coeffOut,
                                   std::vector<svtkm::Id>& L)
  {
    svtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if (nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel(sigInLen))
    {
      throw svtkm::cont::ErrorBadValue("Number of levels of transform is not supported! ");
    }
    if (nLevels == 0) //  0 levels means no transform
    {
      svtkm::cont::ArrayCopy(sigIn, coeffOut);
      return 0;
    }

    this->ComputeL(sigInLen, nLevels, L); // memory for L is allocated by ComputeL().
    svtkm::Id CLength = this->ComputeCoeffLength(L, nLevels);
    SVTKM_ASSERT(CLength == sigInLen);

    svtkm::Id sigInPtr = 0; // pseudo pointer for the beginning of input array
    svtkm::Id len = sigInLen;
    svtkm::Id cALen = WaveletBase::GetApproxLength(len);
    svtkm::Id cptr; // pseudo pointer for the beginning of output array
    svtkm::Id tlen = 0;
    std::vector<svtkm::Id> L1d(3, 0);

    // Use an intermediate array
    using OutputValueType = typename CoeffArrayType::ValueType;
    using InterArrayType = svtkm::cont::ArrayHandle<OutputValueType>;

    // Define a few more types
    using IdArrayType = svtkm::cont::ArrayHandleCounting<svtkm::Id>;
    using PermutArrayType = svtkm::cont::ArrayHandlePermutation<IdArrayType, CoeffArrayType>;

    svtkm::cont::ArrayCopy(sigIn, coeffOut);

    for (svtkm::Id i = nLevels; i > 0; i--)
    {
      tlen += L[size_t(i)];
      cptr = 0 + CLength - tlen - cALen;

      // make input array (permutation array)
      IdArrayType inputIndices(sigInPtr, 1, len);
      PermutArrayType input(inputIndices, coeffOut);
      // make output array
      InterArrayType output;

      WaveletDWT::DWT1D(input, output, L1d);

      // move intermediate results to final array
      WaveletBase::DeviceCopyStartX(output, coeffOut, cptr);

      // update pseudo pointers
      len = cALen;
      cALen = WaveletBase::GetApproxLength(cALen);
      sigInPtr = cptr;
    }

    return 0;
  }

  // Multi-level 1D wavelet reconstruction
  template <typename CoeffArrayType, typename SignalArrayType>
  SVTKM_CONT svtkm::Id WaveReconstruct(const CoeffArrayType& coeffIn, // Input
                                     svtkm::Id nLevels,              // n levels of DWT
                                     std::vector<svtkm::Id>& L,
                                     SignalArrayType& sigOut)
  {
    SVTKM_ASSERT(nLevels > 0);
    svtkm::Id LLength = nLevels + 2;
    SVTKM_ASSERT(svtkm::Id(L.size()) == LLength);

    std::vector<svtkm::Id> L1d(3, 0); // three elements
    L1d[0] = L[0];
    L1d[1] = L[1];

    using OutValueType = typename SignalArrayType::ValueType;
    using OutArrayBasic = svtkm::cont::ArrayHandle<OutValueType>;
    using IdArrayType = svtkm::cont::ArrayHandleCounting<svtkm::Id>;
    using PermutArrayType = svtkm::cont::ArrayHandlePermutation<IdArrayType, SignalArrayType>;

    svtkm::cont::ArrayCopy(coeffIn, sigOut);

    for (svtkm::Id i = 1; i <= nLevels; i++)
    {
      L1d[2] = this->GetApproxLengthLevN(L[size_t(LLength - 1)], nLevels - i);

      // Make an input array
      IdArrayType inputIndices(0, 1, L1d[2]);
      PermutArrayType input(inputIndices, sigOut);

      // Make an output array
      OutArrayBasic output;

      WaveletDWT::IDWT1D(input, L1d, output);
      SVTKM_ASSERT(output.GetNumberOfValues() == L1d[2]);

      // Move output to intermediate array
      WaveletBase::DeviceCopyStartX(output, sigOut, 0);

      L1d[0] = L1d[2];
      L1d[1] = L[size_t(i + 1)];
    }

    return 0;
  }

  // Multi-level 3D wavelet decomposition
  template <typename InArrayType, typename OutArrayType>
  SVTKM_CONT svtkm::Float64 WaveDecompose3D(InArrayType& sigIn, // Input
                                          svtkm::Id nLevels,   // n levels of DWT
                                          svtkm::Id inX,
                                          svtkm::Id inY,
                                          svtkm::Id inZ,
                                          OutArrayType& coeffOut,
                                          bool discardSigIn) // can we discard sigIn on devices?
  {
    svtkm::Id sigInLen = sigIn.GetNumberOfValues();
    SVTKM_ASSERT(inX * inY * inZ == sigInLen);
    if (nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel(inX) ||
        nLevels > WaveletBase::GetWaveletMaxLevel(inY) ||
        nLevels > WaveletBase::GetWaveletMaxLevel(inZ))
    {
      throw svtkm::cont::ErrorBadValue("Number of levels of transform is not supported! ");
    }
    if (nLevels == 0) //  0 levels means no transform
    {
      svtkm::cont::ArrayCopy(sigIn, coeffOut);
      return 0;
    }

    svtkm::Id currentLenX = inX;
    svtkm::Id currentLenY = inY;
    svtkm::Id currentLenZ = inZ;

    using OutValueType = typename OutArrayType::ValueType;
    using OutBasicArray = svtkm::cont::ArrayHandle<OutValueType>;

    // First level transform writes to the output array
    svtkm::Float64 computationTime = WaveletDWT::DWT3D(
      sigIn, inX, inY, inZ, 0, 0, 0, currentLenX, currentLenY, currentLenZ, coeffOut, discardSigIn);

    // Successor transforms writes to a temporary array
    for (svtkm::Id i = nLevels - 1; i > 0; i--)
    {
      currentLenX = WaveletBase::GetApproxLength(currentLenX);
      currentLenY = WaveletBase::GetApproxLength(currentLenY);
      currentLenZ = WaveletBase::GetApproxLength(currentLenZ);

      OutBasicArray tempOutput;

      computationTime += WaveletDWT::DWT3D(
        coeffOut, inX, inY, inZ, 0, 0, 0, currentLenX, currentLenY, currentLenZ, tempOutput, false);

      // copy results to coeffOut
      WaveletBase::DeviceCubeCopyTo(
        tempOutput, currentLenX, currentLenY, currentLenZ, coeffOut, inX, inY, inZ, 0, 0, 0);
    }

    return computationTime;
  }

  // Multi-level 3D wavelet reconstruction
  template <typename InArrayType, typename OutArrayType>
  SVTKM_CONT svtkm::Float64 WaveReconstruct3D(
    InArrayType& arrIn, // Input
    svtkm::Id nLevels,   // n levels of DWT
    svtkm::Id inX,
    svtkm::Id inY,
    svtkm::Id inZ,
    OutArrayType& arrOut,
    bool discardArrIn) // can we discard input for more memory?
  {
    svtkm::Id arrInLen = arrIn.GetNumberOfValues();
    SVTKM_ASSERT(inX * inY * inZ == arrInLen);
    if (nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel(inX) ||
        nLevels > WaveletBase::GetWaveletMaxLevel(inY) ||
        nLevels > WaveletBase::GetWaveletMaxLevel(inZ))
    {
      throw svtkm::cont::ErrorBadValue("Number of levels of transform is not supported! ");
    }
    using OutValueType = typename OutArrayType::ValueType;
    using OutBasicArray = svtkm::cont::ArrayHandle<OutValueType>;
    svtkm::Float64 computationTime = 0.0;

    OutBasicArray outBuffer;
    if (nLevels == 0) //  0 levels means no transform
    {
      svtkm::cont::ArrayCopy(arrIn, arrOut);
      return 0;
    }
    else if (discardArrIn)
    {
      outBuffer = arrIn;
    }
    else
    {
      svtkm::cont::ArrayCopy(arrIn, outBuffer);
    }

    std::vector<svtkm::Id> L;
    this->ComputeL3(inX, inY, inZ, nLevels, L);
    std::vector<svtkm::Id> L3d(27, 0);

    // All transforms but the last level operate on temporary arrays
    for (size_t i = 0; i < 24; i++)
    {
      L3d[i] = L[i];
    }
    for (size_t i = 1; i < static_cast<size_t>(nLevels); i++)
    {
      L3d[24] = L3d[0] + L3d[12]; // Total X dim; this is always true for Biorthogonal wavelets
      L3d[25] = L3d[1] + L3d[7];  // Total Y dim
      L3d[26] = L3d[2] + L3d[5];  // Total Z dim

      OutBasicArray tempOutput;

      // IDWT
      computationTime +=
        WaveletDWT::IDWT3D(outBuffer, inX, inY, inZ, 0, 0, 0, L3d, tempOutput, false);

      // copy back reconstructed block
      WaveletBase::DeviceCubeCopyTo(
        tempOutput, L3d[24], L3d[25], L3d[26], outBuffer, inX, inY, inZ, 0, 0, 0);

      // update L3d array
      L3d[0] = L3d[24];
      L3d[1] = L3d[25];
      L3d[2] = L3d[26];
      for (size_t j = 3; j < 24; j++)
      {
        L3d[j] = L[21 * i + j];
      }
    }

    // The last transform outputs to the final output
    L3d[24] = L3d[0] + L3d[12];
    L3d[25] = L3d[1] + L3d[7];
    L3d[26] = L3d[2] + L3d[5];
    computationTime += WaveletDWT::IDWT3D(outBuffer, inX, inY, inZ, 0, 0, 0, L3d, arrOut, true);

    return computationTime;
  }

  // Multi-level 2D wavelet decomposition
  template <typename InArrayType, typename OutArrayType>
  SVTKM_CONT svtkm::Float64 WaveDecompose2D(const InArrayType& sigIn, // Input
                                          svtkm::Id nLevels,         // n levels of DWT
                                          svtkm::Id inX,             // Input X dim
                                          svtkm::Id inY,             // Input Y dim
                                          OutArrayType& coeffOut,
                                          std::vector<svtkm::Id>& L)
  {
    svtkm::Id sigInLen = sigIn.GetNumberOfValues();
    SVTKM_ASSERT(inX * inY == sigInLen);
    if (nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel(inX) ||
        nLevels > WaveletBase::GetWaveletMaxLevel(inY))
    {
      throw svtkm::cont::ErrorBadValue("Number of levels of transform is not supported! ");
    }
    if (nLevels == 0) //  0 levels means no transform
    {
      svtkm::cont::ArrayCopy(sigIn, coeffOut);
      return 0;
    }

    this->ComputeL2(inX, inY, nLevels, L);
    svtkm::Id CLength = this->ComputeCoeffLength2(L, nLevels);
    SVTKM_ASSERT(CLength == sigInLen);

    svtkm::Id currentLenX = inX;
    svtkm::Id currentLenY = inY;
    std::vector<svtkm::Id> L2d(10, 0);
    svtkm::Float64 computationTime = 0.0;

    using OutValueType = typename OutArrayType::ValueType;
    using OutBasicArray = svtkm::cont::ArrayHandle<OutValueType>;

    // First level transform operates writes to the output array
    computationTime += WaveletDWT::DWT2D(
      sigIn, currentLenX, currentLenY, 0, 0, currentLenX, currentLenY, coeffOut, L2d);
    SVTKM_ASSERT(coeffOut.GetNumberOfValues() == currentLenX * currentLenY);
    currentLenX = WaveletBase::GetApproxLength(currentLenX);
    currentLenY = WaveletBase::GetApproxLength(currentLenY);

    // Successor transforms writes to a temporary array
    for (svtkm::Id i = nLevels - 1; i > 0; i--)
    {
      OutBasicArray tempOutput;

      computationTime +=
        WaveletDWT::DWT2D(coeffOut, inX, inY, 0, 0, currentLenX, currentLenY, tempOutput, L2d);

      // copy results to coeffOut
      WaveletBase::DeviceRectangleCopyTo(
        tempOutput, currentLenX, currentLenY, coeffOut, inX, inY, 0, 0);

      // update currentLen
      currentLenX = WaveletBase::GetApproxLength(currentLenX);
      currentLenY = WaveletBase::GetApproxLength(currentLenY);
    }

    return computationTime;
  }

  // Multi-level 2D wavelet reconstruction
  template <typename InArrayType, typename OutArrayType>
  SVTKM_CONT svtkm::Float64 WaveReconstruct2D(const InArrayType& arrIn, // Input
                                            svtkm::Id nLevels,         // n levels of DWT
                                            svtkm::Id inX,             // Input X dim
                                            svtkm::Id inY,             // Input Y dim
                                            OutArrayType& arrOut,
                                            std::vector<svtkm::Id>& L)
  {
    svtkm::Id arrInLen = arrIn.GetNumberOfValues();
    SVTKM_ASSERT(inX * inY == arrInLen);
    if (nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel(inX) ||
        nLevels > WaveletBase::GetWaveletMaxLevel(inY))
    {
      throw svtkm::cont::ErrorBadValue("Number of levels of transform is not supported! ");
    }
    using OutValueType = typename OutArrayType::ValueType;
    using OutBasicArray = svtkm::cont::ArrayHandle<OutValueType>;
    svtkm::Float64 computationTime = 0.0;

    OutBasicArray outBuffer;
    if (nLevels == 0) //  0 levels means no transform
    {
      svtkm::cont::ArrayCopy(arrIn, arrOut);
      return 0;
    }
    else
    {
      svtkm::cont::ArrayCopy(arrIn, outBuffer);
    }

    SVTKM_ASSERT(svtkm::Id(L.size()) == 6 * nLevels + 4);

    std::vector<svtkm::Id> L2d(10, 0);
    L2d[0] = L[0];
    L2d[1] = L[1];
    L2d[2] = L[2];
    L2d[3] = L[3];
    L2d[4] = L[4];
    L2d[5] = L[5];
    L2d[6] = L[6];
    L2d[7] = L[7];

    // All transforms but the last operate on temporary arrays
    for (size_t i = 1; i < static_cast<size_t>(nLevels); i++)
    {
      L2d[8] = L2d[0] + L2d[4]; // This is always true for Biorthogonal wavelets
      L2d[9] = L2d[1] + L2d[3]; // (same above)

      OutBasicArray tempOutput;

      // IDWT
      computationTime += WaveletDWT::IDWT2D(outBuffer, inX, inY, 0, 0, L2d, tempOutput);

      // copy back reconstructed block
      WaveletBase::DeviceRectangleCopyTo(tempOutput, L2d[8], L2d[9], outBuffer, inX, inY, 0, 0);

      // update L2d array
      L2d[0] = L2d[8];
      L2d[1] = L2d[9];
      L2d[2] = L[6 * i + 2];
      L2d[3] = L[6 * i + 3];
      L2d[4] = L[6 * i + 4];
      L2d[5] = L[6 * i + 5];
      L2d[6] = L[6 * i + 6];
      L2d[7] = L[6 * i + 7];
    }

    // The last transform outputs to the final output
    L2d[8] = L2d[0] + L2d[4];
    L2d[9] = L2d[1] + L2d[3];
    computationTime += WaveletDWT::IDWT2D(outBuffer, inX, inY, 0, 0, L2d, arrOut);

    return computationTime;
  }

  // Squash coefficients smaller than a threshold
  template <typename CoeffArrayType>
  svtkm::Id SquashCoefficients(CoeffArrayType& coeffIn, svtkm::Float64 ratio)
  {
    if (ratio > 1.0)
    {
      svtkm::Id coeffLen = coeffIn.GetNumberOfValues();
      using ValueType = typename CoeffArrayType::ValueType;
      using CoeffArrayBasic = svtkm::cont::ArrayHandle<ValueType>;
      CoeffArrayBasic sortedArray;
      svtkm::cont::ArrayCopy(coeffIn, sortedArray);

      WaveletBase::DeviceSort(sortedArray);

      svtkm::Id n = coeffLen - static_cast<svtkm::Id>(static_cast<svtkm::Float64>(coeffLen) / ratio);
      svtkm::Float64 nthVal = static_cast<svtkm::Float64>(svtkm::cont::ArrayGetValue(n, sortedArray));
      if (nthVal < 0.0)
      {
        nthVal *= -1.0;
      }

      using ThresholdType = svtkm::worklet::wavelets::ThresholdWorklet;
      ThresholdType thresholdWorklet(nthVal);
      svtkm::worklet::DispatcherMapField<ThresholdType> dispatcher(thresholdWorklet);
      dispatcher.Invoke(coeffIn);
    }

    return 0;
  }

  // Report statistics on reconstructed array
  template <typename ArrayType>
  svtkm::Id EvaluateReconstruction(const ArrayType& original, const ArrayType& reconstruct)
  {
#define VAL svtkm::Float64
#define MAKEVAL(a) (static_cast<VAL>(a))
    VAL VarOrig = WaveletBase::DeviceCalculateVariance(original);

    using ValueType = typename ArrayType::ValueType;
    using ArrayBasic = svtkm::cont::ArrayHandle<ValueType>;
    ArrayBasic errorArray, errorSquare;

    // Use a worklet to calculate point-wise error, and its square
    using DifferencerWorklet = svtkm::worklet::wavelets::Differencer;
    DifferencerWorklet dw;
    svtkm::worklet::DispatcherMapField<DifferencerWorklet> dwDispatcher(dw);
    dwDispatcher.Invoke(original, reconstruct, errorArray);

    using SquareWorklet = svtkm::worklet::wavelets::SquareWorklet;
    SquareWorklet sw;
    svtkm::worklet::DispatcherMapField<SquareWorklet> swDispatcher(sw);
    swDispatcher.Invoke(errorArray, errorSquare);

    VAL varErr = WaveletBase::DeviceCalculateVariance(errorArray);
    VAL snr, decibels;
    if (varErr != 0.0)
    {
      snr = VarOrig / varErr;
      decibels = 10 * svtkm::Log10(snr);
    }
    else
    {
      snr = svtkm::Infinity64();
      decibels = svtkm::Infinity64();
    }

    VAL origMax = WaveletBase::DeviceMax(original);
    VAL origMin = WaveletBase::DeviceMin(original);
    VAL errorMax = WaveletBase::DeviceMaxAbs(errorArray);
    VAL range = origMax - origMin;

    VAL squareSum = WaveletBase::DeviceSum(errorSquare);
    VAL rmse = svtkm::Sqrt(MAKEVAL(squareSum) / MAKEVAL(errorArray.GetNumberOfValues()));

    std::cout << "Data range             = " << range << std::endl;
    std::cout << "SNR                    = " << snr << std::endl;
    std::cout << "SNR in decibels        = " << decibels << std::endl;
    std::cout << "L-infy norm            = " << errorMax
              << ", after normalization  = " << errorMax / range << std::endl;
    std::cout << "RMSE                   = " << rmse << ", after normalization  = " << rmse / range
              << std::endl;
#undef MAKEVAL
#undef VAL

    return 0;
  }

  // Compute the book keeping array L for 1D DWT
  void ComputeL(svtkm::Id sigInLen, svtkm::Id nLev, std::vector<svtkm::Id>& L)
  {
    size_t nLevels = static_cast<size_t>(nLev); // cast once
    L.resize(nLevels + 2);
    L[nLevels + 1] = sigInLen;
    L[nLevels] = sigInLen;
    for (size_t i = nLevels; i > 0; i--)
    {
      L[i - 1] = WaveletBase::GetApproxLength(L[i]);
      L[i] = WaveletBase::GetDetailLength(L[i]);
    }
  }

  // Compute the book keeping array L for 2D DWT
  void ComputeL2(svtkm::Id inX, svtkm::Id inY, svtkm::Id nLev, std::vector<svtkm::Id>& L)
  {
    size_t nLevels = static_cast<size_t>(nLev);
    L.resize(nLevels * 6 + 4);
    L[nLevels * 6] = inX;
    L[nLevels * 6 + 1] = inY;
    L[nLevels * 6 + 2] = inX;
    L[nLevels * 6 + 3] = inY;

    for (size_t i = nLevels; i > 0; i--)
    {
      // cA
      L[i * 6 - 6] = WaveletBase::GetApproxLength(L[i * 6 + 0]);
      L[i * 6 - 5] = WaveletBase::GetApproxLength(L[i * 6 + 1]);

      // cDh
      L[i * 6 - 4] = WaveletBase::GetApproxLength(L[i * 6 + 0]);
      L[i * 6 - 3] = WaveletBase::GetDetailLength(L[i * 6 + 1]);

      // cDv
      L[i * 6 - 2] = WaveletBase::GetDetailLength(L[i * 6 + 0]);
      L[i * 6 - 1] = WaveletBase::GetApproxLength(L[i * 6 + 1]);

      // cDv - overwrites previous value!
      L[i * 6 - 0] = WaveletBase::GetDetailLength(L[i * 6 + 0]);
      L[i * 6 + 1] = WaveletBase::GetDetailLength(L[i * 6 + 1]);
    }
  }

  // Compute the bookkeeping array L for 3D DWT
  void ComputeL3(svtkm::Id inX, svtkm::Id inY, svtkm::Id inZ, svtkm::Id nLev, std::vector<svtkm::Id>& L)
  {
    size_t n = static_cast<size_t>(nLev);
    L.resize(n * 21 + 6);
    L[n * 21 + 0] = inX;
    L[n * 21 + 1] = inY;
    L[n * 21 + 2] = inZ;
    L[n * 21 + 3] = inX;
    L[n * 21 + 4] = inY;
    L[n * 21 + 5] = inZ;

    for (size_t i = n; i > 0; i--)
    {
      // cLLL
      L[i * 21 - 21] = WaveletBase::GetApproxLength(L[i * 21 + 0]);
      L[i * 21 - 20] = WaveletBase::GetApproxLength(L[i * 21 + 1]);
      L[i * 21 - 19] = WaveletBase::GetApproxLength(L[i * 21 + 2]);

      // cLLH
      L[i * 21 - 18] = L[i * 21 - 21];
      L[i * 21 - 17] = L[i * 21 - 20];
      L[i * 21 - 16] = WaveletBase::GetDetailLength(L[i * 21 + 2]);

      // cLHL
      L[i * 21 - 15] = L[i * 21 - 21];
      L[i * 21 - 14] = WaveletBase::GetDetailLength(L[i * 21 + 1]);
      L[i * 21 - 13] = L[i * 21 - 19];

      // cLHH
      L[i * 21 - 12] = L[i * 21 - 21];
      L[i * 21 - 11] = L[i * 21 - 14];
      L[i * 21 - 10] = L[i * 21 - 16];

      // cHLL
      L[i * 21 - 9] = WaveletBase::GetDetailLength(L[i * 21 + 0]);
      L[i * 21 - 8] = L[i * 21 - 20];
      L[i * 21 - 7] = L[i * 21 - 19];

      // cHLH
      L[i * 21 - 6] = L[i * 21 - 9];
      L[i * 21 - 5] = L[i * 21 - 20];
      L[i * 21 - 3] = L[i * 21 - 16];

      // cHHL
      L[i * 21 - 3] = L[i * 21 - 9];
      L[i * 21 - 2] = L[i * 21 - 14];
      L[i * 21 - 1] = L[i * 21 - 19];

      // cHHH - overwrites previous value
      L[i * 21 + 0] = L[i * 21 - 9];
      L[i * 21 + 1] = L[i * 21 - 14];
      L[i * 21 + 2] = L[i * 21 - 16];
    }
  }

  // Compute the length of coefficients for 1D transforms
  svtkm::Id ComputeCoeffLength(std::vector<svtkm::Id>& L, svtkm::Id nLevels)
  {
    svtkm::Id sum = L[0]; // 1st level cA
    for (size_t i = 1; i <= size_t(nLevels); i++)
    {
      sum += L[i];
    }
    return sum;
  }
  // Compute the length of coefficients for 2D transforms
  svtkm::Id ComputeCoeffLength2(std::vector<svtkm::Id>& L, svtkm::Id nLevels)
  {
    svtkm::Id sum = (L[0] * L[1]); // 1st level cA
    for (size_t i = 1; i <= size_t(nLevels); i++)
    {
      sum += L[i * 6 - 4] * L[i * 6 - 3]; // cDh
      sum += L[i * 6 - 2] * L[i * 6 - 1]; // cDv
      sum += L[i * 6 - 0] * L[i * 6 + 1]; // cDd
    }
    return sum;
  }

  // Compute approximate coefficient length at a specific level
  svtkm::Id GetApproxLengthLevN(svtkm::Id sigInLen, svtkm::Id levN)
  {
    svtkm::Id cALen = sigInLen;
    for (svtkm::Id i = 0; i < levN; i++)
    {
      cALen = WaveletBase::GetApproxLength(cALen);
      if (cALen == 0)
      {
        return cALen;
      }
    }

    return cALen;
  }

}; // class WaveletCompressor

} // namespace worklet
} // namespace svtkm

#endif
