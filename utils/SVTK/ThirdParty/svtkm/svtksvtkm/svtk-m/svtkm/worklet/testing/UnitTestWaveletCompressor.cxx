//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/WaveletCompressor.h>

#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/testing/Testing.h>

#include <iomanip>
#include <vector>

namespace svtkm
{
namespace worklet
{
namespace wavelets
{

class GaussianWorklet2D : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  SVTKM_EXEC
  GaussianWorklet2D(svtkm::Id dx,
                    svtkm::Id dy,
                    svtkm::Float64 a,
                    svtkm::Float64 x,
                    svtkm::Float64 y,
                    svtkm::Float64 sx,
                    svtkm::Float64 xy)
    : dimX(dx)
    , amp(a)
    , x0(x)
    , y0(y)
    , sigmaX(sx)
    , sigmaY(xy)
  {
    (void)dy;
    sigmaX2 = 2 * sigmaX * sigmaX;
    sigmaY2 = 2 * sigmaY * sigmaY;
  }

  SVTKM_EXEC
  void Sig1Dto2D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y) const
  {
    x = idx % dimX;
    y = idx / dimX;
  }

  SVTKM_EXEC
  svtkm::Float64 GetGaussian(svtkm::Float64 x, svtkm::Float64 y) const
  {
    svtkm::Float64 power = (x - x0) * (x - x0) / sigmaX2 + (y - y0) * (y - y0) / sigmaY2;
    return svtkm::Exp(power * -1.0) * amp;
  }

  template <typename T>
  SVTKM_EXEC void operator()(T& val, const svtkm::Id& workIdx) const
  {
    svtkm::Id x, y;
    Sig1Dto2D(workIdx, x, y);
    val = GetGaussian(static_cast<svtkm::Float64>(x), static_cast<svtkm::Float64>(y));
  }

private:                              // see wikipedia page
  const svtkm::Id dimX;                // 2D extent
  const svtkm::Float64 amp;            // amplitude
  const svtkm::Float64 x0, y0;         // center
  const svtkm::Float64 sigmaX, sigmaY; // spread
  svtkm::Float64 sigmaX2, sigmaY2;     // 2 * sigma * sigma
};

template <typename T>
class GaussianWorklet3D : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  SVTKM_EXEC
  GaussianWorklet3D(svtkm::Id dx, svtkm::Id dy, svtkm::Id dz)
    : dimX(dx)
    , dimY(dy)
    , dimZ(dz)
  {
    amp = (T)20.0;
    sigmaX = (T)dimX / (T)4.0;
    sigmaX2 = sigmaX * sigmaX * (T)2.0;
    sigmaY = (T)dimY / (T)4.0;
    sigmaY2 = sigmaY * sigmaY * (T)2.0;
    sigmaZ = (T)dimZ / (T)4.0;
    sigmaZ2 = sigmaZ * sigmaZ * (T)2.0;
  }

  SVTKM_EXEC
  void Sig1Dto3D(svtkm::Id idx, svtkm::Id& x, svtkm::Id& y, svtkm::Id& z) const
  {
    z = idx / (dimX * dimY);
    y = (idx - z * dimX * dimY) / dimX;
    x = idx % dimX;
  }

  SVTKM_EXEC
  T GetGaussian(T x, T y, T z) const
  {
    x -= (T)dimX / (T)2.0; // translate to center at (0, 0, 0)
    y -= (T)dimY / (T)2.0;
    z -= (T)dimZ / (T)2.0;
    T power = x * x / sigmaX2 + y * y / sigmaY2 + z * z / sigmaZ2;

    return svtkm::Exp(power * (T)-1.0) * amp;
  }

  SVTKM_EXEC
  void operator()(T& val, const svtkm::Id& workIdx) const
  {
    svtkm::Id x, y, z;
    Sig1Dto3D(workIdx, x, y, z);
    val = GetGaussian((T)x, (T)y, (T)z);
  }

private:
  const svtkm::Id dimX, dimY, dimZ; // extent
  T amp;                           // amplitude
  T sigmaX, sigmaY, sigmaZ;        // spread
  T sigmaX2, sigmaY2, sigmaZ2;     // sigma * sigma * 2
};
}
}
}

template <typename ArrayType>
void FillArray2D(ArrayType& array, svtkm::Id dimX, svtkm::Id dimY)
{
  using WorkletType = svtkm::worklet::wavelets::GaussianWorklet2D;
  WorkletType worklet(dimX,
                      dimY,
                      100.0,
                      static_cast<svtkm::Float64>(dimX) / 2.0,  // center
                      static_cast<svtkm::Float64>(dimY) / 2.0,  // center
                      static_cast<svtkm::Float64>(dimX) / 4.0,  // spread
                      static_cast<svtkm::Float64>(dimY) / 4.0); // spread
  svtkm::worklet::DispatcherMapField<WorkletType> dispatcher(worklet);
  dispatcher.Invoke(array);
}
template <typename ArrayType>
void FillArray3D(ArrayType& array, svtkm::Id dimX, svtkm::Id dimY, svtkm::Id dimZ)
{
  using WorkletType = svtkm::worklet::wavelets::GaussianWorklet3D<typename ArrayType::ValueType>;
  WorkletType worklet(dimX, dimY, dimZ);
  svtkm::worklet::DispatcherMapField<WorkletType> dispatcher(worklet);
  dispatcher.Invoke(array);
}

void TestDecomposeReconstruct3D(svtkm::Float64 cratio)
{
  svtkm::Id sigX = 99;
  svtkm::Id sigY = 99;
  svtkm::Id sigZ = 99;
  svtkm::Id sigLen = sigX * sigY * sigZ;
  std::cout << "Testing 3D wavelet compressor on a (99x99x99) cube..." << std::endl;

  // make input data array handle
  svtkm::cont::ArrayHandle<svtkm::Float32> inputArray;
  inputArray.Allocate(sigLen);
  FillArray3D(inputArray, sigX, sigY, sigZ);

  svtkm::cont::ArrayHandle<svtkm::Float32> outputArray;

  // Use a WaveletCompressor
  svtkm::worklet::wavelets::WaveletName wname = svtkm::worklet::wavelets::BIOR4_4;
  if (wname == svtkm::worklet::wavelets::BIOR1_1)
    std::cout << "Using wavelet kernel   = Bior1.1 (HAAR)" << std::endl;
  else if (wname == svtkm::worklet::wavelets::BIOR2_2)
    std::cout << "Using wavelet kernel   = Bior2.2 (CDF 5/3)" << std::endl;
  else if (wname == svtkm::worklet::wavelets::BIOR3_3)
    std::cout << "Using wavelet kernel   = Bior3.3 (CDF 8/4)" << std::endl;
  else if (wname == svtkm::worklet::wavelets::BIOR4_4)
    std::cout << "Using wavelet kernel   = Bior4.4 (CDF 9/7)" << std::endl;
  svtkm::worklet::WaveletCompressor compressor(wname);

  svtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  svtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  svtkm::Id ZMaxLevel = compressor.GetWaveletMaxLevel(sigZ);
  svtkm::Id nLevels = svtkm::Min(svtkm::Min(XMaxLevel, YMaxLevel), ZMaxLevel);
  std::cout << "Decomposition levels   = " << nLevels << std::endl;
  svtkm::Float64 computationTime = 0.0;
  svtkm::Float64 elapsedTime1, elapsedTime2, elapsedTime3;

  // Decompose

  svtkm::cont::Timer timer;
  timer.Start();
  computationTime =
    compressor.WaveDecompose3D(inputArray, nLevels, sigX, sigY, sigZ, outputArray, false);
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Decompose time         = " << elapsedTime1 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;

  // Squash small coefficients
  timer.Start();
  compressor.SquashCoefficients(outputArray, cratio);
  elapsedTime2 = timer.GetElapsedTime();
  std::cout << "Squash time            = " << elapsedTime2 << std::endl;

  // Reconstruct
  svtkm::cont::ArrayHandle<svtkm::Float32> reconstructArray;
  timer.Start();
  computationTime =
    compressor.WaveReconstruct3D(outputArray, nLevels, sigX, sigY, sigZ, reconstructArray, false);
  elapsedTime3 = timer.GetElapsedTime();
  std::cout << "Reconstruction time    = " << elapsedTime3 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;
  std::cout << "Total time             = " << (elapsedTime1 + elapsedTime2 + elapsedTime3)
            << std::endl;

  outputArray.ReleaseResources();

  compressor.EvaluateReconstruction(inputArray, reconstructArray);

  timer.Start();
  for (svtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(reconstructArray.GetPortalConstControl().Get(i),
                                inputArray.GetPortalConstControl().Get(i)),
                     "WaveletCompressor 3D failed...");
  }
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Verification time      = " << elapsedTime1 << std::endl;
}

void TestDecomposeReconstruct2D(svtkm::Float64 cratio)
{
  std::cout << "Testing 2D wavelet compressor on a (1000x1000) square... " << std::endl;
  svtkm::Id sigX = 1000;
  svtkm::Id sigY = 1000;
  svtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  svtkm::cont::ArrayHandle<svtkm::Float64> inputArray;
  inputArray.Allocate(sigLen);
  FillArray2D(inputArray, sigX, sigY);

  svtkm::cont::ArrayHandle<svtkm::Float64> outputArray;

  // Use a WaveletCompressor
  svtkm::worklet::wavelets::WaveletName wname = svtkm::worklet::wavelets::CDF9_7;
  std::cout << "Wavelet kernel         = CDF 9/7" << std::endl;
  svtkm::worklet::WaveletCompressor compressor(wname);

  svtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  svtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  svtkm::Id nLevels = svtkm::Min(XMaxLevel, YMaxLevel);
  std::cout << "Decomposition levels   = " << nLevels << std::endl;
  std::vector<svtkm::Id> L;
  svtkm::Float64 computationTime = 0.0;
  svtkm::Float64 elapsedTime1, elapsedTime2, elapsedTime3;

  // Decompose
  svtkm::cont::Timer timer;
  timer.Start();
  computationTime = compressor.WaveDecompose2D(inputArray, nLevels, sigX, sigY, outputArray, L);
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Decompose time         = " << elapsedTime1 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;

  // Squash small coefficients
  timer.Start();
  compressor.SquashCoefficients(outputArray, cratio);
  elapsedTime2 = timer.GetElapsedTime();
  std::cout << "Squash time            = " << elapsedTime2 << std::endl;

  // Reconstruct
  svtkm::cont::ArrayHandle<svtkm::Float64> reconstructArray;
  timer.Start();
  computationTime =
    compressor.WaveReconstruct2D(outputArray, nLevels, sigX, sigY, reconstructArray, L);
  elapsedTime3 = timer.GetElapsedTime();
  std::cout << "Reconstruction time    = " << elapsedTime3 << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;
  std::cout << "Total time             = " << (elapsedTime1 + elapsedTime2 + elapsedTime3)
            << std::endl;

  outputArray.ReleaseResources();

  compressor.EvaluateReconstruction(inputArray, reconstructArray);

  timer.Start();
  for (svtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(reconstructArray.GetPortalConstControl().Get(i),
                                inputArray.GetPortalConstControl().Get(i)),
                     "WaveletCompressor 2D failed...");
  }
  elapsedTime1 = timer.GetElapsedTime();
  std::cout << "Verification time      = " << elapsedTime1 << std::endl;
}

void TestDecomposeReconstruct1D(svtkm::Float64 cratio)
{
  std::cout << "Testing 1D wavelet compressor on a 1 million sized array... " << std::endl;
  svtkm::Id sigLen = 1000000;

  // make input data array handle
  std::vector<svtkm::Float64> tmpVector;
  for (svtkm::Id i = 0; i < sigLen; i++)
  {
    tmpVector.push_back(100.0 * svtkm::Sin(static_cast<svtkm::Float64>(i) / 100.0));
  }
  svtkm::cont::ArrayHandle<svtkm::Float64> inputArray = svtkm::cont::make_ArrayHandle(tmpVector);

  svtkm::cont::ArrayHandle<svtkm::Float64> outputArray;

  // Use a WaveletCompressor
  svtkm::worklet::wavelets::WaveletName wname = svtkm::worklet::wavelets::CDF9_7;
  std::cout << "Wavelet kernel         = CDF 9/7" << std::endl;
  svtkm::worklet::WaveletCompressor compressor(wname);

  // User maximum decompose levels
  svtkm::Id maxLevel = compressor.GetWaveletMaxLevel(sigLen);
  svtkm::Id nLevels = maxLevel;
  std::cout << "Decomposition levels   = " << nLevels << std::endl;

  std::vector<svtkm::Id> L;

  // Decompose
  svtkm::cont::Timer timer;
  timer.Start();
  compressor.WaveDecompose(inputArray, nLevels, outputArray, L);

  svtkm::Float64 elapsedTime = timer.GetElapsedTime();
  std::cout << "Decompose time         = " << elapsedTime << std::endl;

  // Squash small coefficients
  timer.Start();
  compressor.SquashCoefficients(outputArray, cratio);
  elapsedTime = timer.GetElapsedTime();
  std::cout << "Squash time            = " << elapsedTime << std::endl;

  // Reconstruct
  svtkm::cont::ArrayHandle<svtkm::Float64> reconstructArray;
  timer.Start();
  compressor.WaveReconstruct(outputArray, nLevels, L, reconstructArray);
  elapsedTime = timer.GetElapsedTime();
  std::cout << "Reconstruction time    = " << elapsedTime << std::endl;

  compressor.EvaluateReconstruction(inputArray, reconstructArray);

  timer.Start();
  for (svtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    SVTKM_TEST_ASSERT(test_equal(reconstructArray.GetPortalConstControl().Get(i),
                                inputArray.GetPortalConstControl().Get(i)),
                     "WaveletCompressor 1D failed...");
  }
  elapsedTime = timer.GetElapsedTime();
  std::cout << "Verification time      = " << elapsedTime << std::endl;
}

void TestWaveletCompressor()
{
  svtkm::Float64 cratio = 2.0; // X:1 compression, where X >= 1
  std::cout << "Compression ratio       = " << cratio << ":1 ";
  std::cout
    << "(Reconstruction using higher compression ratios may result in failure in verification)"
    << std::endl;

  TestDecomposeReconstruct1D(cratio);
  std::cout << std::endl;
  TestDecomposeReconstruct2D(cratio);
  std::cout << std::endl;
  TestDecomposeReconstruct3D(cratio);
}

int UnitTestWaveletCompressor(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestWaveletCompressor, argc, argv);
}
