//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_wavelets_waveletdwt_h
#define svtk_m_worklet_wavelets_waveletdwt_h

#include <svtkm/worklet/wavelets/WaveletBase.h>

#include <svtkm/worklet/wavelets/WaveletTransforms.h>

#include <svtkm/cont/ArrayHandleConcatenate.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandlePermutation.h>

#include <svtkm/Math.h>
#include <svtkm/cont/Timer.h>

namespace svtkm
{
namespace worklet
{
namespace wavelets
{

class WaveletDWT : public WaveletBase
{
public:
  // Constructor
  WaveletDWT(WaveletName name)
    : WaveletBase(name)
  {
  }

  // Function: extend a cube in X direction
  template <typename SigInArrayType, typename ExtensionArrayType>
  svtkm::Id Extend3DLeftRight(const SigInArrayType& sigIn, // input
                             svtkm::Id sigDimX,
                             svtkm::Id sigDimY,
                             svtkm::Id sigDimZ,
                             svtkm::Id sigStartX,
                             svtkm::Id sigStartY,
                             svtkm::Id sigStartZ,
                             svtkm::Id sigPretendDimX,
                             svtkm::Id sigPretendDimY,
                             svtkm::Id sigPretendDimZ,
                             ExtensionArrayType& ext1, // output
                             ExtensionArrayType& ext2, // output
                             svtkm::Id addLen,
                             svtkm::worklet::wavelets::DWTMode ext1Method,
                             svtkm::worklet::wavelets::DWTMode ext2Method,
                             bool pretendSigPaddedZero,
                             bool padZeroAtExt2)
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    SVTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.Allocate(0);                          // No extension on the left side
      if (pretendSigPaddedZero || padZeroAtExt2) // plane of size 1*dimY*dimZ
      {
        ext2.Allocate(sigPretendDimY * sigPretendDimZ);
        WaveletBase::DeviceAssignZero3DPlaneX(ext2, 1, sigPretendDimY, sigPretendDimZ, 0);
      }
      else
      {
        ext2.Allocate(0); // No extension on the right side
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = svtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = svtkm::worklet::wavelets::ExtensionWorklet3D;
    using DispatcherType = typename svtkm::worklet::DispatcherMapField<ExtensionWorklet>;
    svtkm::Id extDimX, extDimY, extDimZ;
    svtkm::worklet::wavelets::ExtensionDirection dir;

    { // First work on left extension
      dir = LEFT;
      extDimX = addLen;
      extDimY = sigPretendDimY;
      extDimZ = sigPretendDimZ;

      ext1.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext1Method,
                               dir,
                               false); // not treating input signal as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Then work on right extension
    dir = RIGHT;
    extDimY = sigPretendDimY;
    extDimZ = sigPretendDimZ;
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      extDimX = addLen;
      ext2.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    { // This case is not exactly padding a zero at the end of Ext2.
      // Rather, it is to increase extension length by one and fill it
      //         to be whatever mirrored.
      extDimX = addLen + 1;
      ext2.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      extDimX = addLen;
      ext2Temp.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      // Give ext2 one layer thicker to hold the pretend zeros from signal.
      ext2.Allocate((extDimX + 1) * extDimY * extDimZ);
      WaveletBase::DeviceCubeCopyTo(
        ext2Temp, extDimX, extDimY, extDimZ, ext2, extDimX + 1, extDimY, extDimZ, 1, 0, 0);
      WaveletBase::DeviceAssignZero3DPlaneX(ext2, extDimX + 1, extDimY, extDimZ, 0);
    }
    return 0;
  }

  // Function: extend a cube in Y direction
  template <typename SigInArrayType, typename ExtensionArrayType>
  svtkm::Id Extend3DTopDown(const SigInArrayType& sigIn, // input
                           svtkm::Id sigDimX,
                           svtkm::Id sigDimY,
                           svtkm::Id sigDimZ,
                           svtkm::Id sigStartX,
                           svtkm::Id sigStartY,
                           svtkm::Id sigStartZ,
                           svtkm::Id sigPretendDimX,
                           svtkm::Id sigPretendDimY,
                           svtkm::Id sigPretendDimZ,
                           ExtensionArrayType& ext1, // output
                           ExtensionArrayType& ext2, // output
                           svtkm::Id addLen,
                           svtkm::worklet::wavelets::DWTMode ext1Method,
                           svtkm::worklet::wavelets::DWTMode ext2Method,
                           bool pretendSigPaddedZero,
                           bool padZeroAtExt2)
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    SVTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.Allocate(0);                          // No extension on the top side
      if (pretendSigPaddedZero || padZeroAtExt2) // plane of size dimX*dimZ
      {
        ext2.Allocate(sigPretendDimX * 1 * sigPretendDimZ);
        WaveletBase::DeviceAssignZero3DPlaneY(ext2, sigPretendDimX, 1, sigPretendDimZ, 0);
      }
      else
      {
        ext2.Allocate(0); // No extension on the right side
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = svtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = svtkm::worklet::wavelets::ExtensionWorklet3D;
    using DispatcherType = typename svtkm::worklet::DispatcherMapField<ExtensionWorklet>;
    svtkm::Id extDimX, extDimY, extDimZ;
    svtkm::worklet::wavelets::ExtensionDirection dir;

    { // First work on top extension
      dir = TOP;
      extDimX = sigPretendDimX;
      extDimY = addLen;
      extDimZ = sigPretendDimZ;

      ext1.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext1Method,
                               dir,
                               false); // not treating input signal as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Then work on bottom extension
    dir = BOTTOM;
    extDimX = sigPretendDimX;
    extDimZ = sigPretendDimZ;
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      extDimY = addLen;
      ext2.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    { // This case is not exactly padding a zero at the end of Ext2.
      // Rather, it is to increase extension length by one and fill it
      //         to be whatever mirrored.
      extDimY = addLen + 1;
      ext2.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      extDimY = addLen;
      ext2Temp.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      // Give ext2 one layer thicker to hold the pretend zeros from signal.
      ext2.Allocate(extDimX * (extDimY + 1) * extDimZ);
      WaveletBase::DeviceCubeCopyTo(
        ext2Temp, extDimX, extDimY, extDimZ, ext2, extDimX, extDimY + 1, extDimZ, 0, 1, 0);
      WaveletBase::DeviceAssignZero3DPlaneY(ext2, extDimX, extDimY + 1, extDimZ, 0);
    }
    return 0;
  }

  // Function: extend a cube in Z direction
  template <typename SigInArrayType, typename ExtensionArrayType>
  svtkm::Id Extend3DFrontBack(const SigInArrayType& sigIn, // input
                             svtkm::Id sigDimX,
                             svtkm::Id sigDimY,
                             svtkm::Id sigDimZ,
                             svtkm::Id sigStartX,
                             svtkm::Id sigStartY,
                             svtkm::Id sigStartZ,
                             svtkm::Id sigPretendDimX,
                             svtkm::Id sigPretendDimY,
                             svtkm::Id sigPretendDimZ,
                             ExtensionArrayType& ext1, // output
                             ExtensionArrayType& ext2, // output
                             svtkm::Id addLen,
                             svtkm::worklet::wavelets::DWTMode ext1Method,
                             svtkm::worklet::wavelets::DWTMode ext2Method,
                             bool pretendSigPaddedZero,
                             bool padZeroAtExt2)
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    SVTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.Allocate(0);                          // No extension on the front side
      if (pretendSigPaddedZero || padZeroAtExt2) // plane of size dimX * dimY
      {
        ext2.Allocate(sigPretendDimX * sigPretendDimY * 1);
        WaveletBase::DeviceAssignZero3DPlaneZ(ext2, sigPretendDimX, sigPretendDimY, 1, 0);
      }
      else
      {
        ext2.Allocate(0); // No extension on the right side
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = svtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = svtkm::worklet::wavelets::ExtensionWorklet3D;
    using DispatcherType = typename svtkm::worklet::DispatcherMapField<ExtensionWorklet>;
    svtkm::Id extDimX, extDimY, extDimZ;
    svtkm::worklet::wavelets::ExtensionDirection dir;

    { // First work on front extension
      dir = FRONT;
      extDimX = sigPretendDimX;
      extDimY = sigPretendDimY;
      extDimZ = addLen;

      ext1.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext1Method,
                               dir,
                               false); // not treating input signal as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Then work on back extension
    dir = BACK;
    extDimX = sigPretendDimX;
    extDimY = sigPretendDimY;
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      extDimZ = addLen;
      ext2.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    { // This case is not exactly padding a zero at the end of Ext2.
      // Rather, it is to increase extension length by one and fill it
      //         to be whatever mirrored.
      extDimZ = addLen + 1;
      ext2.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      extDimZ = addLen;
      ext2Temp.Allocate(extDimX * extDimY * extDimZ);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               extDimZ,
                               sigDimX,
                               sigDimY,
                               sigDimZ,
                               sigStartX,
                               sigStartY,
                               sigStartZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      // Give ext2 one layer thicker to hold the pretend zeros from signal.
      ext2.Allocate(extDimX * extDimY * (extDimZ + 1));
      WaveletBase::DeviceCubeCopyTo(
        ext2Temp, extDimX, extDimY, extDimZ, ext2, extDimX, extDimY, extDimZ + 1, 0, 0, 1);
      WaveletBase::DeviceAssignZero3DPlaneZ(ext2, extDimX, extDimY, extDimZ + 1, 0);
    }
    return 0;
  }

  //                  L[3]        L[15]
  //               -----------------------
  //              /          /          /|
  //        L[5] /          /          / |
  //            /  LLH     /  HLH     /  |
  //           /          /          /   | L[16]
  //          -----------------------    |
  //         /          /          /|    |
  //   L[2] /          /          / |   /|
  //       /          /          /  |  / |
  //      /___L[0]___/___L[12]__/   | /  | L[22]
  //      |          |          |   |/   |
  // L[1] |          |          |   /HHH /
  //      |   LLL    |   HLL    |  /|   /
  //      |          |          | / |  / L[23]
  //      |---------------------|/  | /
  //      |          |          |   |/
  //      |          |          |   /
  // L[7] |   LHL    |   HHL    |  /
  //      |          |          | / L[20]
  //      |__________|__________|/
  //          L[6]       L[18]
  //
  // Performs one level of 3D discrete wavelet transform on a small cube of input array
  // The output has the same size as the small cube
  template <typename ArrayInType, typename ArrayOutType>
  svtkm::Float64 DWT3D(ArrayInType& sigIn,
                      svtkm::Id sigDimX,
                      svtkm::Id sigDimY,
                      svtkm::Id sigDimZ,
                      svtkm::Id sigStartX,
                      svtkm::Id sigStartY,
                      svtkm::Id sigStartZ,
                      svtkm::Id sigPretendDimX,
                      svtkm::Id sigPretendDimY,
                      svtkm::Id sigPretendDimZ,
                      ArrayOutType& coeffOut,
                      bool discardSigIn) // discard sigIn on devices
  {
    std::vector<svtkm::Id> L(27, 0);

    // LLL
    L[0] = WaveletBase::GetApproxLength(sigPretendDimX);
    L[1] = WaveletBase::GetApproxLength(sigPretendDimY);
    L[2] = WaveletBase::GetApproxLength(sigPretendDimZ);
    // LLH
    L[3] = L[0];
    L[4] = L[1];
    L[5] = WaveletBase::GetDetailLength(sigPretendDimZ);
    // LHL
    L[6] = L[0];
    L[7] = WaveletBase::GetDetailLength(sigPretendDimY);
    L[8] = L[2];
    // LHH
    L[9] = L[0];
    L[10] = L[7];
    L[11] = L[5];
    // HLL
    L[12] = WaveletBase::GetDetailLength(sigPretendDimX);
    L[13] = L[1];
    L[14] = L[2];
    // HLH
    L[15] = L[12];
    L[16] = L[1];
    L[17] = L[5];
    // HHL
    L[18] = L[12];
    L[19] = L[7];
    L[20] = L[2];
    // HHH
    L[21] = L[12];
    L[22] = L[7];
    L[23] = L[5];

    L[24] = sigPretendDimX;
    L[25] = sigPretendDimY;
    L[26] = sigPretendDimZ;

    svtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool oddLow = true;
    if (filterLen % 2 != 0)
    {
      oddLow = false;
    }
    svtkm::Id addLen = filterLen / 2;

    using ValueType = typename ArrayInType::ValueType;
    using ArrayType = svtkm::cont::ArrayHandle<ValueType>;
    using LeftRightXFormType = svtkm::worklet::wavelets::ForwardTransform3DLeftRight;
    using TopDownXFormType = svtkm::worklet::wavelets::ForwardTransform3DTopDown;
    using FrontBackXFormType = svtkm::worklet::wavelets::ForwardTransform3DFrontBack;
    using LeftRightDispatcherType = svtkm::worklet::DispatcherMapField<LeftRightXFormType>;
    using TopDownDispatcherType = svtkm::worklet::DispatcherMapField<TopDownXFormType>;
    using FrontBackDispatcherType = svtkm::worklet::DispatcherMapField<FrontBackXFormType>;

    svtkm::cont::Timer timer;
    svtkm::Float64 computationTime = 0.0;

    // First transform in X direction
    ArrayType afterX;
    afterX.Allocate(sigPretendDimX * sigPretendDimY * sigPretendDimZ);
    {
      ArrayType leftExt, rightExt;
      this->Extend3DLeftRight(sigIn,
                              sigDimX,
                              sigDimY,
                              sigDimZ,
                              sigStartX,
                              sigStartY,
                              sigStartZ,
                              sigPretendDimX,
                              sigPretendDimY,
                              sigPretendDimZ,
                              leftExt,
                              rightExt,
                              addLen,
                              WaveletBase::wmode,
                              WaveletBase::wmode,
                              false,
                              false);
      LeftRightXFormType worklet(filterLen,
                                 L[0],
                                 oddLow,
                                 addLen,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 sigDimX,
                                 sigDimY,
                                 sigDimZ,
                                 sigStartX,
                                 sigStartY,
                                 sigStartZ,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 addLen,
                                 sigPretendDimY,
                                 sigPretendDimZ);
      LeftRightDispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(leftExt,
                        sigIn,
                        rightExt,
                        WaveletBase::filter.GetLowDecomposeFilter(),
                        WaveletBase::filter.GetHighDecomposeFilter(),
                        afterX);
      computationTime += timer.GetElapsedTime();
    }

    if (discardSigIn)
    {
      sigIn.ReleaseResourcesExecution();
    }

    // Then do transform in Y direction
    ArrayType afterY;
    afterY.Allocate(sigPretendDimX * sigPretendDimY * sigPretendDimZ);
    {
      ArrayType topExt, bottomExt;
      this->Extend3DTopDown(afterX,
                            sigPretendDimX,
                            sigPretendDimY,
                            sigPretendDimZ,
                            0,
                            0,
                            0,
                            sigPretendDimX,
                            sigPretendDimY,
                            sigPretendDimZ,
                            topExt,
                            bottomExt,
                            addLen,
                            WaveletBase::wmode,
                            WaveletBase::wmode,
                            false,
                            false);
      TopDownXFormType worklet(filterLen,
                               L[1],
                               oddLow,
                               sigPretendDimX,
                               addLen,
                               sigPretendDimZ,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               0,
                               0,
                               0,
                               sigPretendDimX,
                               sigPretendDimY,
                               sigPretendDimZ,
                               sigPretendDimX,
                               addLen,
                               sigPretendDimZ);
      TopDownDispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(topExt,
                        afterX,
                        bottomExt,
                        WaveletBase::filter.GetLowDecomposeFilter(),
                        WaveletBase::filter.GetHighDecomposeFilter(),
                        afterY);
      computationTime += timer.GetElapsedTime();
    }

    // Then do transform in Z direction
    afterX.ReleaseResources(); // release afterX
    {
      ArrayType frontExt, backExt;
      coeffOut.Allocate(sigPretendDimX * sigPretendDimY * sigPretendDimZ);
      this->Extend3DFrontBack(afterY,
                              sigPretendDimX,
                              sigPretendDimY,
                              sigPretendDimZ,
                              0,
                              0,
                              0,
                              sigPretendDimX,
                              sigPretendDimY,
                              sigPretendDimZ,
                              frontExt,
                              backExt,
                              addLen,
                              WaveletBase::wmode,
                              WaveletBase::wmode,
                              false,
                              false);
      FrontBackXFormType worklet(filterLen,
                                 L[1],
                                 oddLow,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 addLen,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 0,
                                 0,
                                 0,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 sigPretendDimZ,
                                 sigPretendDimX,
                                 sigPretendDimY,
                                 addLen);
      FrontBackDispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(frontExt,
                        afterY,
                        backExt,
                        WaveletBase::filter.GetLowDecomposeFilter(),
                        WaveletBase::filter.GetHighDecomposeFilter(),
                        coeffOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  // Performs one level of IDWT on a small cube of a big cube
  // The output array has the same dimensions as the small cube.
  template <typename ArrayInType, typename ArrayOutType>
  svtkm::Float64 IDWT3D(ArrayInType& coeffIn,
                       svtkm::Id inDimX,
                       svtkm::Id inDimY,
                       svtkm::Id inDimZ,
                       svtkm::Id inStartX,
                       svtkm::Id inStartY,
                       svtkm::Id inStartZ,
                       const std::vector<svtkm::Id>& L,
                       ArrayOutType& sigOut,
                       bool discardCoeffIn) // can we discard coeffIn?
  {
    //SVTKM_ASSERT( L.size() == 27 );
    //SVTKM_ASSERT( inDimX * inDimY * inDimZ == coeffIn.GetNumberOfValues() );
    svtkm::Id inPretendDimX = L[0] + L[12];
    svtkm::Id inPretendDimY = L[1] + L[7];
    svtkm::Id inPretendDimZ = L[2] + L[5];

    svtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    using BasicArrayType = svtkm::cont::ArrayHandle<typename ArrayInType::ValueType>;
    using LeftRightXFormType = svtkm::worklet::wavelets::InverseTransform3DLeftRight;
    using TopDownXFormType = svtkm::worklet::wavelets::InverseTransform3DTopDown;
    using FrontBackXFormType = svtkm::worklet::wavelets::InverseTransform3DFrontBack;
    using LeftRightDispatcherType = svtkm::worklet::DispatcherMapField<LeftRightXFormType>;
    using TopDownDispatcherType = svtkm::worklet::DispatcherMapField<TopDownXFormType>;
    using FrontBackDispatcherType = svtkm::worklet::DispatcherMapField<FrontBackXFormType>;

    svtkm::cont::Timer timer;
    svtkm::Float64 computationTime = 0.0;

    // First, inverse transform in Z direction
    BasicArrayType afterZ;
    afterZ.Allocate(inPretendDimX * inPretendDimY * inPretendDimZ);
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      svtkm::Id extDimX = inPretendDimX;
      svtkm::Id extDimY = inPretendDimY;
      svtkm::Id ext1DimZ = 0, ext2DimZ = 0, ext3DimZ = 0, ext4DimZ = 0;
      this->IDWTHelper3DFrontBack(coeffIn,
                                  inDimX,
                                  inDimY,
                                  inDimZ,
                                  inStartX,
                                  inStartY,
                                  inStartZ,
                                  inPretendDimX,
                                  inPretendDimY,
                                  inPretendDimZ,
                                  L[2],
                                  L[5],
                                  ext1,
                                  ext2,
                                  ext3,
                                  ext4,
                                  ext1DimZ,
                                  ext2DimZ,
                                  ext3DimZ,
                                  ext4DimZ,
                                  filterLen,
                                  wmode);
      FrontBackXFormType worklet(filterLen,
                                 extDimX,
                                 extDimY,
                                 ext1DimZ, // ext1
                                 extDimX,
                                 extDimY,
                                 ext2DimZ, // ext2
                                 extDimX,
                                 extDimY,
                                 ext3DimZ, // ext3
                                 extDimX,
                                 extDimY,
                                 ext4DimZ, // ext4
                                 inPretendDimX,
                                 inPretendDimY,
                                 L[2], // cA
                                 inPretendDimX,
                                 inPretendDimY,
                                 L[5], // cD
                                 inDimX,
                                 inDimY,
                                 inDimZ, // coeffIn
                                 inStartX,
                                 inStartY,
                                 inStartZ); // coeffIn
      FrontBackDispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(ext1,
                        ext2,
                        ext3,
                        ext4,
                        coeffIn,
                        WaveletBase::filter.GetLowReconstructFilter(),
                        WaveletBase::filter.GetHighReconstructFilter(),
                        afterZ);
      computationTime += timer.GetElapsedTime();
    }

    if (discardCoeffIn)
    {
      coeffIn.ReleaseResourcesExecution();
    }

    // Second, inverse transform in Y direction
    BasicArrayType afterY;
    afterY.Allocate(inPretendDimX * inPretendDimY * inPretendDimZ);
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      svtkm::Id extDimX = inPretendDimX;
      svtkm::Id extDimZ = inPretendDimZ;
      svtkm::Id ext1DimY = 0, ext2DimY = 0, ext3DimY = 0, ext4DimY = 0;
      this->IDWTHelper3DTopDown(afterZ,
                                inPretendDimX,
                                inPretendDimY,
                                inPretendDimZ,
                                0,
                                0,
                                0,
                                inPretendDimX,
                                inPretendDimY,
                                inPretendDimZ,
                                L[1],
                                L[7],
                                ext1,
                                ext2,
                                ext3,
                                ext4,
                                ext1DimY,
                                ext2DimY,
                                ext3DimY,
                                ext4DimY,
                                filterLen,
                                wmode);
      TopDownXFormType worklet(filterLen,
                               extDimX,
                               ext1DimY,
                               extDimZ, // ext1
                               extDimX,
                               ext2DimY,
                               extDimZ, // ext2
                               extDimX,
                               ext3DimY,
                               extDimZ, // ext3
                               extDimX,
                               ext4DimY,
                               extDimZ, // ext4
                               inPretendDimX,
                               L[1],
                               inPretendDimZ, // cA
                               inPretendDimX,
                               L[7],
                               inPretendDimZ, // cD
                               inPretendDimX,
                               inPretendDimY,
                               inPretendDimZ, // actual signal
                               0,
                               0,
                               0);
      TopDownDispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(ext1,
                        ext2,
                        ext3,
                        ext4,
                        afterZ,
                        WaveletBase::filter.GetLowReconstructFilter(),
                        WaveletBase::filter.GetHighReconstructFilter(),
                        afterY);
      computationTime += timer.GetElapsedTime();
    }

    // Lastly, inverse transform in X direction
    afterZ.ReleaseResources();
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      svtkm::Id extDimY = inPretendDimY;
      svtkm::Id extDimZ = inPretendDimZ;
      svtkm::Id ext1DimX = 0, ext2DimX = 0, ext3DimX = 0, ext4DimX = 0;
      this->IDWTHelper3DLeftRight(afterY,
                                  inPretendDimX,
                                  inPretendDimY,
                                  inPretendDimZ,
                                  0,
                                  0,
                                  0,
                                  inPretendDimX,
                                  inPretendDimY,
                                  inPretendDimZ,
                                  L[0],
                                  L[12],
                                  ext1,
                                  ext2,
                                  ext3,
                                  ext4,
                                  ext1DimX,
                                  ext2DimX,
                                  ext3DimX,
                                  ext4DimX,
                                  filterLen,
                                  wmode);
      sigOut.Allocate(inPretendDimX * inPretendDimY * inPretendDimZ);
      LeftRightXFormType worklet(filterLen,
                                 ext1DimX,
                                 extDimY,
                                 extDimZ, // ext1
                                 ext2DimX,
                                 extDimY,
                                 extDimZ, // ext2
                                 ext3DimX,
                                 extDimY,
                                 extDimZ, // ext3
                                 ext4DimX,
                                 extDimY,
                                 extDimZ, // ext4
                                 L[0],
                                 inPretendDimY,
                                 inPretendDimZ, // cA
                                 L[12],
                                 inPretendDimY,
                                 inPretendDimZ, // cD
                                 inPretendDimX,
                                 inPretendDimY,
                                 inPretendDimZ, // actual signal
                                 0,
                                 0,
                                 0);
      LeftRightDispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(ext1,
                        ext2,
                        ext3,
                        ext4,
                        afterY,
                        WaveletBase::filter.GetLowReconstructFilter(),
                        WaveletBase::filter.GetHighReconstructFilter(),
                        sigOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  //=============================================================================

  template <typename SigInArrayType, typename ExtensionArrayType>
  svtkm::Id Extend2D(const SigInArrayType& sigIn, // Input
                    svtkm::Id sigDimX,
                    svtkm::Id sigDimY,
                    svtkm::Id sigStartX,
                    svtkm::Id sigStartY,
                    svtkm::Id sigPretendDimX,
                    svtkm::Id sigPretendDimY,
                    ExtensionArrayType& ext1, // left/top extension
                    ExtensionArrayType& ext2, // right/bottom extension
                    svtkm::Id addLen,
                    svtkm::worklet::wavelets::DWTMode ext1Method,
                    svtkm::worklet::wavelets::DWTMode ext2Method,
                    bool pretendSigPaddedZero,
                    bool padZeroAtExt2,
                    bool modeLR) // true = left-right, false = top-down
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    SVTKM_ASSERT(!pretendSigPaddedZero || !padZeroAtExt2);

    if (addLen == 0) // Haar kernel
    {
      ext1.Allocate(0); // no need to extend on left/top
      if (pretendSigPaddedZero || padZeroAtExt2)
      {
        if (modeLR) // right extension
        {
          ext2.Allocate(sigPretendDimY);
          WaveletBase::DeviceAssignZero2DColumn(ext2, 1, sigPretendDimY, 0);
        }
        else // bottom extension
        {
          ext2.Allocate(sigPretendDimX);
          WaveletBase::DeviceAssignZero2DRow(ext2, sigPretendDimX, 1, 0);
        }
      }
      else
      {
        ext2.Allocate(0);
      }
      return 0;
    }

    using ValueType = typename SigInArrayType::ValueType;
    using ExtendArrayType = svtkm::cont::ArrayHandle<ValueType>;
    using ExtensionWorklet = svtkm::worklet::wavelets::ExtensionWorklet2D;
    using DispatcherType = typename svtkm::worklet::DispatcherMapField<ExtensionWorklet>;
    svtkm::Id extDimX, extDimY;
    svtkm::worklet::wavelets::ExtensionDirection dir;

    { // Work on left/top extension
      if (modeLR)
      {
        dir = LEFT;
        extDimX = addLen;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = TOP;
        extDimX = sigPretendDimX;
        extDimY = addLen;
      }
      ext1.Allocate(extDimX * extDimY);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY, // use this area
                               ext1Method,
                               dir,
                               false); // not treating sigIn as having zeros
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext1, sigIn);
    }

    // Work on right/bottom extension
    if (!pretendSigPaddedZero && !padZeroAtExt2)
    {
      if (modeLR)
      {
        dir = RIGHT;
        extDimX = addLen;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = BOTTOM;
        extDimX = sigPretendDimX;
        extDimY = addLen;
      }
      ext2.Allocate(extDimX * extDimY);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY, // use this area
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
    }
    else if (!pretendSigPaddedZero && padZeroAtExt2)
    {
      if (modeLR)
      {
        dir = RIGHT;
        extDimX = addLen + 1;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = BOTTOM;
        extDimX = sigPretendDimX;
        extDimY = addLen + 1;
      }
      ext2.Allocate(extDimX * extDimY);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY,
                               ext2Method,
                               dir,
                               false);
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2, sigIn);
      /* Pad a zero at the end of cDTemp, when cDTemp is forced to have the same
         length as cATemp. For example, with odd length signal, cA is 1 element
         longer than cD.
       */
      /* Update 10/24/2016: the extra element of cD shouldn't be zero, just be
       * whatever it extends to be.
       * if( modeLR )
       *   WaveletBase::DeviceAssignZero2DColumn( ext2, extDimX, extDimY,
       *                                          extDimX-1 );
       * else
       *   WaveletBase::DeviceAssignZero2DRow( ext2, extDimX, extDimY,
       *                                       extDimY-1 );
       */
    }
    else // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      if (modeLR)
      {
        dir = RIGHT;
        extDimX = addLen;
        extDimY = sigPretendDimY;
      }
      else
      {
        dir = BOTTOM;
        extDimX = sigPretendDimX;
        extDimY = addLen;
      }
      ext2Temp.Allocate(extDimX * extDimY);
      ExtensionWorklet worklet(extDimX,
                               extDimY,
                               sigDimX,
                               sigDimY,
                               sigStartX,
                               sigStartY,
                               sigPretendDimX,
                               sigPretendDimY,
                               ext2Method,
                               dir,
                               true); // pretend sig is padded a zero
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke(ext2Temp, sigIn);

      if (modeLR)
      {
        ext2.Allocate((extDimX + 1) * extDimY);
        WaveletBase::DeviceRectangleCopyTo(
          ext2Temp, extDimX, extDimY, ext2, extDimX + 1, extDimY, 1, 0);
        WaveletBase::DeviceAssignZero2DColumn(ext2, extDimX + 1, extDimY, 0);
      }
      else
      {
        ext2.Allocate(extDimX * (extDimY + 1));
        WaveletBase::DeviceRectangleCopyTo(
          ext2Temp, extDimX, extDimY, ext2, extDimX, extDimY + 1, 0, 1);
        WaveletBase::DeviceAssignZero2DRow(ext2, extDimX, extDimY + 1, 0);
      }
    }
    return 0;
  }

  // Extend 1D signal
  template <typename SigInArrayType, typename SigExtendedArrayType>
  svtkm::Id Extend1D(const SigInArrayType& sigIn,  // Input
                    SigExtendedArrayType& sigOut, // Output
                    svtkm::Id addLen,
                    svtkm::worklet::wavelets::DWTMode leftExtMethod,
                    svtkm::worklet::wavelets::DWTMode rightExtMethod,
                    bool attachZeroRightLeft,
                    bool attachZeroRightRight)
  {
    // "right extension" can be attached a zero on either end, but not both ends.
    SVTKM_ASSERT(!attachZeroRightRight || !attachZeroRightLeft);

    using ValueType = typename SigInArrayType::ValueType;
    using ExtensionArrayType = svtkm::cont::ArrayHandle<ValueType>;
    using ArrayConcat = svtkm::cont::ArrayHandleConcatenate<ExtensionArrayType, SigInArrayType>;

    ExtensionArrayType leftExtend, rightExtend;

    if (addLen == 0) // Haar kernel
    {
      if (attachZeroRightLeft || attachZeroRightRight)
      {
        leftExtend.Allocate(0);
        rightExtend.Allocate(1);
        WaveletBase::DeviceAssignZero(rightExtend, 0);
      }
      else
      {
        leftExtend.Allocate(0);
        rightExtend.Allocate(0);
      }
      ArrayConcat leftOn(leftExtend, sigIn);
      sigOut = svtkm::cont::make_ArrayHandleConcatenate(leftOn, rightExtend);
      return 0;
    }

    leftExtend.Allocate(addLen);
    svtkm::Id sigInLen = sigIn.GetNumberOfValues();

    using LeftSYMH = svtkm::worklet::wavelets::LeftSYMHExtentionWorklet;
    using LeftSYMW = svtkm::worklet::wavelets::LeftSYMWExtentionWorklet;
    using RightSYMH = svtkm::worklet::wavelets::RightSYMHExtentionWorklet;
    using RightSYMW = svtkm::worklet::wavelets::RightSYMWExtentionWorklet;
    using LeftASYMH = svtkm::worklet::wavelets::LeftASYMHExtentionWorklet;
    using LeftASYMW = svtkm::worklet::wavelets::LeftASYMWExtentionWorklet;
    using RightASYMH = svtkm::worklet::wavelets::RightASYMHExtentionWorklet;
    using RightASYMW = svtkm::worklet::wavelets::RightASYMWExtentionWorklet;

    switch (leftExtMethod)
    {
      case SYMH:
      {
        LeftSYMH worklet(addLen);
        svtkm::worklet::DispatcherMapField<LeftSYMH> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      case SYMW:
      {
        LeftSYMW worklet(addLen);
        svtkm::worklet::DispatcherMapField<LeftSYMW> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      case ASYMH:
      {
        LeftASYMH worklet(addLen);
        svtkm::worklet::DispatcherMapField<LeftASYMH> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      case ASYMW:
      {
        LeftASYMW worklet(addLen);
        svtkm::worklet::DispatcherMapField<LeftASYMW> dispatcher(worklet);
        dispatcher.Invoke(leftExtend, sigIn);
        break;
      }
      default:
      {
        svtkm::cont::ErrorInternal("Left extension mode not supported!");
        return 1;
      }
    }

    if (!attachZeroRightLeft) // no attach zero, or only attach on RightRight
    {
      // Allocate memory
      if (attachZeroRightRight)
      {
        rightExtend.Allocate(addLen + 1);
      }
      else
      {
        rightExtend.Allocate(addLen);
      }

      switch (rightExtMethod)
      {
        case SYMH:
        {
          RightSYMH worklet(sigInLen);
          svtkm::worklet::DispatcherMapField<RightSYMH> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        case SYMW:
        {
          RightSYMW worklet(sigInLen);
          svtkm::worklet::DispatcherMapField<RightSYMW> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        case ASYMH:
        {
          RightASYMH worklet(sigInLen);
          svtkm::worklet::DispatcherMapField<RightASYMH> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        case ASYMW:
        {
          RightASYMW worklet(sigInLen);
          svtkm::worklet::DispatcherMapField<RightASYMW> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigIn);
          break;
        }
        default:
        {
          svtkm::cont::ErrorInternal("Right extension mode not supported!");
          return 1;
        }
      }
      if (attachZeroRightRight)
      {
        WaveletBase::DeviceAssignZero(rightExtend, addLen);
      }
    }
    else // attachZeroRightLeft mode
    {
      using ConcatArray = svtkm::cont::ArrayHandleConcatenate<SigInArrayType, ExtensionArrayType>;
      // attach a zero at the end of sigIn
      ExtensionArrayType singleValArray;
      singleValArray.Allocate(1);
      WaveletBase::DeviceAssignZero(singleValArray, 0);
      ConcatArray sigInPlusOne(sigIn, singleValArray);

      // allocate memory for extension
      rightExtend.Allocate(addLen);

      switch (rightExtMethod)
      {
        case SYMH:
        {
          RightSYMH worklet(sigInLen + 1);
          svtkm::worklet::DispatcherMapField<RightSYMH> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        case SYMW:
        {
          RightSYMW worklet(sigInLen + 1);
          svtkm::worklet::DispatcherMapField<RightSYMW> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        case ASYMH:
        {
          RightASYMH worklet(sigInLen + 1);
          svtkm::worklet::DispatcherMapField<RightASYMH> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        case ASYMW:
        {
          RightASYMW worklet(sigInLen + 1);
          svtkm::worklet::DispatcherMapField<RightASYMW> dispatcher(worklet);
          dispatcher.Invoke(rightExtend, sigInPlusOne);
          break;
        }
        default:
        {
          svtkm::cont::ErrorInternal("Right extension mode not supported!");
          return 1;
        }
      }

      // make a copy of rightExtend with a zero attached to the left
      ExtensionArrayType rightExtendPlusOne;
      rightExtendPlusOne.Allocate(addLen + 1);
      WaveletBase::DeviceCopyStartX(rightExtend, rightExtendPlusOne, 1);
      WaveletBase::DeviceAssignZero(rightExtendPlusOne, 0);
      rightExtend = rightExtendPlusOne;
    }

    ArrayConcat leftOn(leftExtend, sigIn);
    sigOut = svtkm::cont::make_ArrayHandleConcatenate(leftOn, rightExtend);

    return 0;
  }

  // Performs one level of 1D discrete wavelet transform
  // It takes care of boundary conditions, etc.
  template <typename SignalArrayType, typename CoeffArrayType>
  svtkm::Float64 DWT1D(const SignalArrayType& sigIn, // Input
                      CoeffArrayType& coeffOut,     // Output: cA followed by cD
                      std::vector<svtkm::Id>& L)     // Output: how many cA and cD.
  {
    svtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if (GetWaveletMaxLevel(sigInLen) < 1)
    {
      svtkm::cont::ErrorInternal("Signal is too short to perform DWT!");
      return -1;
    }

    //SVTKM_ASSERT( L.size() == 3 );
    L[0] = WaveletBase::GetApproxLength(sigInLen);
    L[1] = WaveletBase::GetDetailLength(sigInLen);
    L[2] = sigInLen;

    //SVTKM_ASSERT( L[0] + L[1] == L[2] );

    svtkm::Id filterLen = WaveletBase::filter.GetFilterLength();

    bool doSymConv = false;
    if (WaveletBase::filter.isSymmetric())
    {
      if ((WaveletBase::wmode == SYMW && (filterLen % 2 != 0)) ||
          (WaveletBase::wmode == SYMH && (filterLen % 2 == 0)))
      {
        doSymConv = true;
      }
    }

    svtkm::Id sigConvolvedLen = L[0] + L[1]; // approx + detail coeffs
    svtkm::Id addLen;                        // for extension
    bool oddLow = true;
    bool oddHigh = true;
    if (filterLen % 2 != 0)
    {
      oddLow = false;
    }
    if (doSymConv)
    {
      addLen = filterLen / 2;
      if (sigInLen % 2 != 0)
      {
        sigConvolvedLen += 1;
      }
    }
    else
    {
      addLen = filterLen - 1;
    }

    svtkm::Id sigExtendedLen = sigInLen + 2 * addLen;

    using SigInValueType = typename SignalArrayType::ValueType;
    using SigInBasic = svtkm::cont::ArrayHandle<SigInValueType>;

    using ConcatType1 = svtkm::cont::ArrayHandleConcatenate<SigInBasic, SignalArrayType>;
    using ConcatType2 = svtkm::cont::ArrayHandleConcatenate<ConcatType1, SigInBasic>;

    ConcatType2 sigInExtended;

    this->Extend1D(
      sigIn, sigInExtended, addLen, WaveletBase::wmode, WaveletBase::wmode, false, false);
    //SVTKM_ASSERT( sigInExtended.GetNumberOfValues() == sigExtendedLen );

    // initialize a worklet for forward transform
    svtkm::worklet::wavelets::ForwardTransform forwardTransform(
      filterLen, L[0], L[1], oddLow, oddHigh);

    coeffOut.Allocate(sigExtendedLen);
    svtkm::worklet::DispatcherMapField<svtkm::worklet::wavelets::ForwardTransform> dispatcher(
      forwardTransform);
    // put a timer
    svtkm::cont::Timer timer;
    timer.Start();
    dispatcher.Invoke(sigInExtended,
                      WaveletBase::filter.GetLowDecomposeFilter(),
                      WaveletBase::filter.GetHighDecomposeFilter(),
                      coeffOut);
    svtkm::Float64 elapsedTime = timer.GetElapsedTime();

    //SVTKM_ASSERT( L[0] + L[1] <= coeffOut.GetNumberOfValues() );
    coeffOut.Shrink(L[0] + L[1]);

    return elapsedTime;
  }

  // Performs one level of inverse wavelet transform
  // It takes care of boundary conditions, etc.
  template <typename CoeffArrayType, typename SignalArrayType>
  svtkm::Float64 IDWT1D(const CoeffArrayType& coeffIn, // Input, cA followed by cD
                       std::vector<svtkm::Id>& L,      // Input, how many cA and cD
                       SignalArrayType& sigOut)       // Output
  {
    svtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool doSymConv = false;
    svtkm::worklet::wavelets::DWTMode cALeftMode = WaveletBase::wmode;
    svtkm::worklet::wavelets::DWTMode cARightMode = WaveletBase::wmode;
    svtkm::worklet::wavelets::DWTMode cDLeftMode = WaveletBase::wmode;
    svtkm::worklet::wavelets::DWTMode cDRightMode = WaveletBase::wmode;

    if (WaveletBase::filter.isSymmetric()) // this is always true with the 1st 4 filters.
    {
      if ((WaveletBase::wmode == SYMW && (filterLen % 2 != 0)) ||
          (WaveletBase::wmode == SYMH && (filterLen % 2 == 0)))
      {
        doSymConv = true; // doSymConv is always true with the 1st 4 filters.

        if (WaveletBase::wmode == SYMH)
        {
          cDLeftMode = ASYMH;
          if (L[2] % 2 != 0)
          {
            cARightMode = SYMW;
            cDRightMode = ASYMW;
          }
          else
          {
            cDRightMode = ASYMH;
          }
        }
        else
        {
          cDLeftMode = SYMH;
          if (L[2] % 2 != 0)
          {
            cARightMode = SYMW;
            cDRightMode = SYMH;
          }
          else
          {
            cARightMode = SYMH;
          }
        }
      }
    }

    svtkm::Id cATempLen, cDTempLen; //, reconTempLen;
    svtkm::Id addLen = 0;
    svtkm::Id cDPadLen = 0;
    if (doSymConv) // extend cA and cD
    {
      addLen = filterLen / 4; // addLen == 0 for Haar kernel
      if ((L[0] > L[1]) && (WaveletBase::wmode == SYMH))
      {
        cDPadLen = L[0];
      }
      cATempLen = L[0] + 2 * addLen;
      cDTempLen = cATempLen; // same length
    }
    else // not extend cA and cD
    {    //  (biorthogonal kernels won't come into this case)
      cATempLen = L[0];
      cDTempLen = L[1];
    }

    using IdArrayType = svtkm::cont::ArrayHandleCounting<svtkm::Id>;
    using PermutArrayType = svtkm::cont::ArrayHandlePermutation<IdArrayType, CoeffArrayType>;

    // Separate cA and cD
    IdArrayType approxIndices(0, 1, L[0]);
    IdArrayType detailIndices(L[0], 1, L[1]);
    PermutArrayType cA(approxIndices, coeffIn);
    PermutArrayType cD(detailIndices, coeffIn);

    using CoeffValueType = typename CoeffArrayType::ValueType;
    using ExtensionArrayType = svtkm::cont::ArrayHandle<CoeffValueType>;
    using Concat1 = svtkm::cont::ArrayHandleConcatenate<ExtensionArrayType, PermutArrayType>;
    using Concat2 = svtkm::cont::ArrayHandleConcatenate<Concat1, ExtensionArrayType>;

    Concat2 cATemp, cDTemp;

    if (doSymConv) // Actually extend cA and cD
    {
      // first extend cA to be cATemp
      this->Extend1D(cA, cATemp, addLen, cALeftMode, cARightMode, false, false);

      // Then extend cD based on extension needs
      if (cDPadLen > 0)
      {
        // Add back the missing final cD, 0.0, before doing extension
        this->Extend1D(cD, cDTemp, addLen, cDLeftMode, cDRightMode, true, false);
      }
      else
      {
        svtkm::Id cDTempLenWouldBe = L[1] + 2 * addLen;
        if (cDTempLenWouldBe == cDTempLen)
        {
          this->Extend1D(cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, false);
        }
        else if (cDTempLenWouldBe == cDTempLen - 1)
        {
          this->Extend1D(cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, true);
        }
        else
        {
          svtkm::cont::ErrorInternal("cDTemp Length not match!");
          return 1;
        }
      }
    }
    else
    {
      // make cATemp
      ExtensionArrayType dummyArray;
      dummyArray.Allocate(0);
      Concat1 cALeftOn(dummyArray, cA);
      cATemp =
        svtkm::cont::make_ArrayHandleConcatenate<Concat1, ExtensionArrayType>(cALeftOn, dummyArray);

      // make cDTemp
      Concat1 cDLeftOn(dummyArray, cD);
      cDTemp =
        svtkm::cont::make_ArrayHandleConcatenate<Concat1, ExtensionArrayType>(cDLeftOn, dummyArray);
    }

    svtkm::cont::ArrayHandleConcatenate<Concat2, Concat2> coeffInExtended(cATemp, cDTemp);

    // Allocate memory for sigOut
    sigOut.Allocate(cATempLen + cDTempLen);

    svtkm::Float64 elapsedTime = 0;
    if (filterLen % 2 != 0)
    {
      svtkm::worklet::wavelets::InverseTransformOdd inverseXformOdd(filterLen, L[0], cATempLen);
      svtkm::worklet::DispatcherMapField<svtkm::worklet::wavelets::InverseTransformOdd> dispatcher(
        inverseXformOdd);
      // use a timer
      svtkm::cont::Timer timer;
      timer.Start();
      dispatcher.Invoke(coeffInExtended,
                        WaveletBase::filter.GetLowReconstructFilter(),
                        WaveletBase::filter.GetHighReconstructFilter(),
                        sigOut);
      elapsedTime = timer.GetElapsedTime();
    }
    else
    {
      svtkm::worklet::wavelets::InverseTransformEven inverseXformEven(
        filterLen, L[0], cATempLen, !doSymConv);
      svtkm::worklet::DispatcherMapField<svtkm::worklet::wavelets::InverseTransformEven> dispatcher(
        inverseXformEven);
      // use a timer
      svtkm::cont::Timer timer;
      timer.Start();
      dispatcher.Invoke(coeffInExtended,
                        WaveletBase::filter.GetLowReconstructFilter(),
                        WaveletBase::filter.GetHighReconstructFilter(),
                        sigOut);
      elapsedTime = timer.GetElapsedTime();
    }

    sigOut.Shrink(L[2]);

    return elapsedTime;
  }

  // Performs one level of 2D discrete wavelet transform
  // It takes care of boundary conditions, etc.
  // N.B.
  //  L[0] == L[2]
  //  L[1] == L[5]
  //  L[3] == L[7]
  //  L[4] == L[6]
  //
  //      ____L[0]_______L[4]____
  //      |          |          |
  // L[1] |  cA      |  cDv     | L[5]
  //      |  (LL)    |  (HL)    |
  //      |          |          |
  //      |---------------------|
  //      |          |          |
  //      |  cDh     |  cDd     | L[7]
  // L[3] |  (LH)    |  (HH)    |
  //      |          |          |
  //      |__________|__________|
  //         L[2]       L[6]
  //
  // Performs one level of 2D discrete wavelet transform on a small rectangle of input array
  // The output has the same size as the small rectangle
  template <typename ArrayInType, typename ArrayOutType>
  svtkm::Float64 DWT2D(const ArrayInType& sigIn,
                      svtkm::Id sigDimX,
                      svtkm::Id sigDimY,
                      svtkm::Id sigStartX,
                      svtkm::Id sigStartY,
                      svtkm::Id sigPretendDimX,
                      svtkm::Id sigPretendDimY,
                      ArrayOutType& coeffOut,
                      std::vector<svtkm::Id>& L)
  {
    L[0] = WaveletBase::GetApproxLength(sigPretendDimX);
    L[2] = L[0];
    L[1] = WaveletBase::GetApproxLength(sigPretendDimY);
    L[5] = L[1];
    L[3] = WaveletBase::GetDetailLength(sigPretendDimY);
    L[7] = L[3];
    L[4] = WaveletBase::GetDetailLength(sigPretendDimX);
    L[6] = L[4];
    L[8] = sigPretendDimX;
    L[9] = sigPretendDimY;

    svtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool oddLow = true;
    if (filterLen % 2 != 0)
    {
      oddLow = false;
    }
    svtkm::Id addLen = filterLen / 2;

    using ValueType = typename ArrayInType::ValueType;
    using ArrayType = svtkm::cont::ArrayHandle<ValueType>;
    using ForwardXForm = svtkm::worklet::wavelets::ForwardTransform2D;
    using DispatcherType = typename svtkm::worklet::DispatcherMapField<ForwardXForm>;

    svtkm::cont::Timer timer;
    svtkm::Float64 computationTime = 0.0;

    ArrayType afterX;
    afterX.Allocate(sigPretendDimX * sigPretendDimY);

    // First transform on rows
    {
      ArrayType leftExt, rightExt;
      this->Extend2D(sigIn,
                     sigDimX,
                     sigDimY,
                     sigStartX,
                     sigStartY,
                     sigPretendDimX,
                     sigPretendDimY,
                     leftExt,
                     rightExt,
                     addLen,
                     WaveletBase::wmode,
                     WaveletBase::wmode,
                     false,
                     false,
                     true); // Extend in left-right direction
      ForwardXForm worklet(filterLen,
                           L[0],
                           oddLow,
                           true, // left-right
                           addLen,
                           sigPretendDimY,
                           sigDimX,
                           sigDimY,
                           sigStartX,
                           sigStartY,
                           sigPretendDimX,
                           sigPretendDimY,
                           addLen,
                           sigPretendDimY);
      DispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(leftExt,
                        sigIn,
                        rightExt,
                        WaveletBase::filter.GetLowDecomposeFilter(),
                        WaveletBase::filter.GetHighDecomposeFilter(),
                        afterX);
      computationTime += timer.GetElapsedTime();
    }

    // Then do transform in Y direction
    {
      ArrayType topExt, bottomExt;
      coeffOut.Allocate(sigPretendDimX * sigPretendDimY);
      this->Extend2D(afterX,
                     sigPretendDimX,
                     sigPretendDimY,
                     0,
                     0,
                     sigPretendDimX,
                     sigPretendDimY,
                     topExt,
                     bottomExt,
                     addLen,
                     WaveletBase::wmode,
                     WaveletBase::wmode,
                     false,
                     false,
                     false); // Extend in top-down direction
      ForwardXForm worklet(filterLen,
                           L[1],
                           oddLow,
                           false, // top-down
                           sigPretendDimX,
                           addLen,
                           sigPretendDimX,
                           sigPretendDimY,
                           0,
                           0,
                           sigPretendDimX,
                           sigPretendDimY,
                           sigPretendDimX,
                           addLen);
      DispatcherType dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(topExt,
                        afterX,
                        bottomExt,
                        WaveletBase::filter.GetLowDecomposeFilter(),
                        WaveletBase::filter.GetHighDecomposeFilter(),
                        coeffOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  // Performs one level of IDWT.
  // The output array has the same dimensions as the small rectangle.
  template <typename ArrayInType, typename ArrayOutType>
  svtkm::Float64 IDWT2D(const ArrayInType& coeffIn,
                       svtkm::Id inDimX,
                       svtkm::Id inDimY,
                       svtkm::Id inStartX,
                       svtkm::Id inStartY,
                       const std::vector<svtkm::Id>& L,
                       ArrayOutType& sigOut)
  {
    svtkm::Id inPretendDimX = L[0] + L[4];
    svtkm::Id inPretendDimY = L[1] + L[3];

    svtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    using BasicArrayType = svtkm::cont::ArrayHandle<typename ArrayInType::ValueType>;
    using IDWT2DWorklet = svtkm::worklet::wavelets::InverseTransform2D;
    using Dispatcher = svtkm::worklet::DispatcherMapField<IDWT2DWorklet>;
    svtkm::cont::Timer timer;
    svtkm::Float64 computationTime = 0.0;

    // First inverse transform on columns
    BasicArrayType afterY;
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      svtkm::Id extDimX = inPretendDimX;
      svtkm::Id ext1DimY = 0, ext2DimY = 0, ext3DimY = 0, ext4DimY = 0;
      this->IDWTHelper2DTopDown(coeffIn,
                                inDimX,
                                inDimY,
                                inStartX,
                                inStartY,
                                inPretendDimX,
                                inPretendDimY,
                                L[1],
                                L[3],
                                ext1,
                                ext2,
                                ext3,
                                ext4,
                                ext1DimY,
                                ext2DimY,
                                ext3DimY,
                                ext4DimY,
                                filterLen,
                                wmode);

      afterY.Allocate(inPretendDimX * inPretendDimY);
      IDWT2DWorklet worklet(filterLen,
                            extDimX,
                            ext1DimY, // ext1
                            inPretendDimX,
                            L[1], // cA
                            extDimX,
                            ext2DimY, // ext2
                            extDimX,
                            ext3DimY, // ext3
                            inPretendDimX,
                            L[3], // cD
                            extDimX,
                            ext4DimY, // ext4
                            inDimX,
                            inDimY, // coeffIn
                            inStartX,
                            inStartY, // coeffIn
                            false);   // top-down
      Dispatcher dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(ext1,
                        ext2,
                        ext3,
                        ext4,
                        coeffIn,
                        WaveletBase::filter.GetLowReconstructFilter(),
                        WaveletBase::filter.GetHighReconstructFilter(),
                        afterY);
      computationTime += timer.GetElapsedTime();
    }

    // Then inverse transform on rows
    {
      BasicArrayType ext1, ext2, ext3, ext4;
      svtkm::Id extDimY = inPretendDimY;
      svtkm::Id ext1DimX = 0, ext2DimX = 0, ext3DimX = 0, ext4DimX = 0;
      this->IDWTHelper2DLeftRight(afterY,
                                  inPretendDimX,
                                  inPretendDimY,
                                  0,
                                  0,
                                  inPretendDimX,
                                  inPretendDimY,
                                  L[0],
                                  L[4],
                                  ext1,
                                  ext2,
                                  ext3,
                                  ext4,
                                  ext1DimX,
                                  ext2DimX,
                                  ext3DimX,
                                  ext4DimX,
                                  filterLen,
                                  wmode);
      sigOut.Allocate(inPretendDimX * inPretendDimY);
      IDWT2DWorklet worklet(filterLen,
                            ext1DimX,
                            extDimY, // ext1
                            L[0],
                            inPretendDimY, // cA
                            ext2DimX,
                            extDimY, // ext2
                            ext3DimX,
                            extDimY, // ext3
                            L[4],
                            inPretendDimY, // cA
                            ext4DimX,
                            extDimY, // ext4
                            inPretendDimX,
                            inPretendDimY,
                            0,
                            0,
                            true); // left-right
      Dispatcher dispatcher(worklet);
      timer.Start();
      dispatcher.Invoke(ext1,
                        ext2,
                        ext3,
                        ext4,
                        afterY,
                        WaveletBase::filter.GetLowReconstructFilter(),
                        WaveletBase::filter.GetHighReconstructFilter(),
                        sigOut);
      computationTime += timer.GetElapsedTime();
    }

    return computationTime;
  }

  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions (2D matrices)
  template <typename ArrayInType, typename ArrayOutType>
  void IDWTHelper2DLeftRight(const ArrayInType& coeffIn,
                             svtkm::Id inDimX,
                             svtkm::Id inDimY,
                             svtkm::Id inStartX,
                             svtkm::Id inStartY,
                             svtkm::Id inPretendDimX,
                             svtkm::Id inPretendDimY,
                             svtkm::Id cADimX,
                             svtkm::Id cDDimX,
                             ArrayOutType& ext1,
                             ArrayOutType& ext2, // output
                             ArrayOutType& ext3,
                             ArrayOutType& ext4, // output
                             svtkm::Id& ext1DimX,
                             svtkm::Id& ext2DimX, // output
                             svtkm::Id& ext3DimX,
                             svtkm::Id& ext4DimX, // output
                             svtkm::Id filterLen,
                             DWTMode mode)
  {
    SVTKM_ASSERT(inPretendDimX == (cADimX + cDDimX));

    // determine extension modes
    DWTMode cALeft, cARight, cDLeft, cDRight;
    cALeft = cARight = cDLeft = cDRight = mode;
    if (mode == SYMH)
    {
      cDLeft = ASYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARight = SYMW;
        cDRight = ASYMW;
      }
      else
      {
        cDRight = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDLeft = SYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARight = SYMW;
        cDRight = SYMH;
      }
      else
      {
        cARight = SYMH;
      }
    }
    // determine length after extension
    svtkm::Id cAExtendedDimX, cDExtendedDimX;
    svtkm::Id cDPadLen = 0;
    svtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimX > cDDimX) && (mode == SYMH))
    {
      cDPadLen = cADimX;
    }
    cAExtendedDimX = cADimX + 2 * addLen;
    cDExtendedDimX = cAExtendedDimX;

    // extend cA
    svtkm::Id cADimY = inPretendDimY;
    this->Extend2D(coeffIn,
                   inDimX,
                   inDimY,
                   inStartX,
                   inStartY,
                   cADimX,
                   cADimY,
                   ext1,
                   ext2,
                   addLen,
                   cALeft,
                   cARight,
                   false,
                   false,
                   true);

    ext1DimX = ext2DimX = addLen;

    // extend cD
    svtkm::Id cDDimY = inPretendDimY;
    if (cDPadLen > 0)
    {
      this->Extend2D(coeffIn,
                     inDimX,
                     inDimY,
                     inStartX + cADimX,
                     inStartY,
                     cDDimX,
                     cDDimY,
                     ext3,
                     ext4,
                     addLen,
                     cDLeft,
                     cDRight,
                     true,
                     false,
                     true);
      ext3DimX = addLen;
      ext4DimX = addLen + 1;
    }
    else
    {
      svtkm::Id cDExtendedWouldBe = cDDimX + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimX)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX + cADimX,
                       inStartY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDLeft,
                       cDRight,
                       false,
                       false,
                       true);
        ext3DimX = ext4DimX = addLen;
      }
      else if (cDExtendedWouldBe == cDExtendedDimX - 1)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX + cADimX,
                       inStartY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDLeft,
                       cDRight,
                       false,
                       true,
                       true);
        ext3DimX = addLen;
        ext4DimX = addLen + 1;
      }
      else
      {
        svtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
  }

  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions (2D matrices)
  template <typename ArrayInType, typename ArrayOutType>
  void IDWTHelper2DTopDown(const ArrayInType& coeffIn,
                           svtkm::Id inDimX,
                           svtkm::Id inDimY,
                           svtkm::Id inStartX,
                           svtkm::Id inStartY,
                           svtkm::Id inPretendDimX,
                           svtkm::Id inPretendDimY,
                           svtkm::Id cADimY,
                           svtkm::Id cDDimY,
                           ArrayOutType& ext1,
                           ArrayOutType& ext2, // output
                           ArrayOutType& ext3,
                           ArrayOutType& ext4, // output
                           svtkm::Id& ext1DimY,
                           svtkm::Id& ext2DimY, // output
                           svtkm::Id& ext3DimY,
                           svtkm::Id& ext4DimY, // output
                           svtkm::Id filterLen,
                           DWTMode mode)
  {
    SVTKM_ASSERT(inPretendDimY == (cADimY + cDDimY));

    // determine extension modes
    DWTMode cATopMode, cADownMode, cDTopMode, cDDownMode;
    cATopMode = cADownMode = cDTopMode = cDDownMode = mode;
    if (mode == SYMH)
    {
      cDTopMode = ASYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = ASYMW;
      }
      else
      {
        cDDownMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDTopMode = SYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = SYMH;
      }
      else
      {
        cADownMode = SYMH;
      }
    }
    // determine length after extension
    svtkm::Id cAExtendedDimY, cDExtendedDimY;
    svtkm::Id cDPadLen = 0;
    svtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimY > cDDimY) && (mode == SYMH))
      cDPadLen = cADimY;
    cAExtendedDimY = cADimY + 2 * addLen;
    cDExtendedDimY = cAExtendedDimY;

    // extend cA
    svtkm::Id cADimX = inPretendDimX;
    this->Extend2D(coeffIn,
                   inDimX,
                   inDimY,
                   inStartX,
                   inStartY,
                   cADimX,
                   cADimY,
                   ext1,
                   ext2,
                   addLen,
                   cATopMode,
                   cADownMode,
                   false,
                   false,
                   false);
    ext1DimY = ext2DimY = addLen;

    // extend cD
    svtkm::Id cDDimX = inPretendDimX;
    if (cDPadLen > 0)
    {
      this->Extend2D(coeffIn,
                     inDimX,
                     inDimY,
                     inStartX,
                     inStartY + cADimY,
                     cDDimX,
                     cDDimY,
                     ext3,
                     ext4,
                     addLen,
                     cDTopMode,
                     cDDownMode,
                     true,
                     false,
                     false);
      ext3DimY = addLen;
      ext4DimY = addLen + 1;
    }
    else
    {
      svtkm::Id cDExtendedWouldBe = cDDimY + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimY)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX,
                       inStartY + cADimY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDTopMode,
                       cDDownMode,
                       false,
                       false,
                       false);
        ext3DimY = ext4DimY = addLen;
      }
      else if (cDExtendedWouldBe == cDExtendedDimY - 1)
      {
        this->Extend2D(coeffIn,
                       inDimX,
                       inDimY,
                       inStartX,
                       inStartY + cADimY,
                       cDDimX,
                       cDDimY,
                       ext3,
                       ext4,
                       addLen,
                       cDTopMode,
                       cDDownMode,
                       false,
                       true,
                       false);
        ext3DimY = addLen;
        ext4DimY = addLen + 1;
      }
      else
      {
        svtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
  }

  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions (3D cubes)
  template <typename ArrayInType, typename ArrayOutType>
  void IDWTHelper3DLeftRight(const ArrayInType& coeffIn,
                             svtkm::Id inDimX,
                             svtkm::Id inDimY,
                             svtkm::Id inDimZ,
                             svtkm::Id inStartX,
                             svtkm::Id inStartY,
                             svtkm::Id inStartZ,
                             svtkm::Id inPretendDimX,
                             svtkm::Id inPretendDimY,
                             svtkm::Id inPretendDimZ,
                             svtkm::Id cADimX,
                             svtkm::Id cDDimX,
                             ArrayOutType& ext1,
                             ArrayOutType& ext2, // output
                             ArrayOutType& ext3,
                             ArrayOutType& ext4, // output
                             svtkm::Id& ext1DimX,
                             svtkm::Id& ext2DimX, // output
                             svtkm::Id& ext3DimX,
                             svtkm::Id& ext4DimX, // output
                             svtkm::Id filterLen,
                             DWTMode mode)
  {
    SVTKM_ASSERT(inPretendDimX == (cADimX + cDDimX));

    // determine extension modes
    DWTMode cALeftMode, cARightMode, cDLeftMode, cDRightMode;
    cALeftMode = cARightMode = cDLeftMode = cDRightMode = mode;
    if (mode == SYMH)
    {
      cDLeftMode = ASYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARightMode = SYMW;
        cDRightMode = ASYMW;
      }
      else
      {
        cDRightMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDLeftMode = SYMH;
      if (inPretendDimX % 2 != 0)
      {
        cARightMode = SYMW;
        cDRightMode = SYMH;
      }
      else
      {
        cARightMode = SYMH;
      }
    }

    // determine length after extension
    svtkm::Id cAExtendedDimX, cDExtendedDimX;
    svtkm::Id cDPadLen = 0;
    svtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimX > cDDimX) && (mode == SYMH))
    {
      cDPadLen = cADimX;
    }
    cAExtendedDimX = cADimX + 2 * addLen;
    cDExtendedDimX = cAExtendedDimX;

    // extend cA
    svtkm::Id cADimY = inPretendDimY;
    svtkm::Id cADimZ = inPretendDimZ;
    this->Extend3DLeftRight(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX,
                            inStartY,
                            inStartZ,
                            cADimX,
                            cADimY,
                            cADimZ,
                            ext1,
                            ext2,
                            addLen,
                            cALeftMode,
                            cARightMode,
                            false,
                            false);
    ext1DimX = ext2DimX = addLen;

    // extend cD
    svtkm::Id cDDimY = inPretendDimY;
    svtkm::Id cDDimZ = inPretendDimZ;
    bool pretendSigPaddedZero, padZeroAtExt2;
    if (cDPadLen > 0)
    {
      ext3DimX = addLen;
      ext4DimX = addLen + 1;
      pretendSigPaddedZero = true;
      padZeroAtExt2 = false;
    }
    else
    {
      svtkm::Id cDExtendedWouldBe = cDDimX + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimX)
      {
        ext3DimX = ext4DimX = addLen;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = false;
      }
      else if (cDExtendedWouldBe == cDExtendedDimX - 1)
      {
        ext3DimX = addLen;
        ext4DimX = addLen + 1;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = true;
      }
      else
      {
        pretendSigPaddedZero = padZeroAtExt2 = false; // so the compiler doesn't complain
        svtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
    this->Extend3DLeftRight(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX + cADimX,
                            inStartY,
                            inStartZ,
                            cDDimX,
                            cDDimY,
                            cDDimZ,
                            ext3,
                            ext4,
                            addLen,
                            cDLeftMode,
                            cDRightMode,
                            pretendSigPaddedZero,
                            padZeroAtExt2);
  }

  template <typename ArrayInType, typename ArrayOutType>
  void IDWTHelper3DTopDown(const ArrayInType& coeffIn,
                           svtkm::Id inDimX,
                           svtkm::Id inDimY,
                           svtkm::Id inDimZ,
                           svtkm::Id inStartX,
                           svtkm::Id inStartY,
                           svtkm::Id inStartZ,
                           svtkm::Id inPretendDimX,
                           svtkm::Id inPretendDimY,
                           svtkm::Id inPretendDimZ,
                           svtkm::Id cADimY,
                           svtkm::Id cDDimY,
                           ArrayOutType& ext1,
                           ArrayOutType& ext2, // output
                           ArrayOutType& ext3,
                           ArrayOutType& ext4, // output
                           svtkm::Id& ext1DimY,
                           svtkm::Id& ext2DimY, // output
                           svtkm::Id& ext3DimY,
                           svtkm::Id& ext4DimY, // output
                           svtkm::Id filterLen,
                           DWTMode mode)
  {
    SVTKM_ASSERT(inPretendDimY == (cADimY + cDDimY));

    // determine extension modes
    DWTMode cATopMode, cADownMode, cDTopMode, cDDownMode;
    cATopMode = cADownMode = cDTopMode = cDDownMode = mode;
    if (mode == SYMH)
    {
      cDTopMode = ASYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = ASYMW;
      }
      else
      {
        cDDownMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDTopMode = SYMH;
      if (inPretendDimY % 2 != 0)
      {
        cADownMode = SYMW;
        cDDownMode = SYMH;
      }
      else
      {
        cADownMode = SYMH;
      }
    }

    // determine length after extension
    svtkm::Id cAExtendedDimY, cDExtendedDimY;
    svtkm::Id cDPadLen = 0;
    svtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimY > cDDimY) && (mode == SYMH))
    {
      cDPadLen = cADimY;
    }
    cAExtendedDimY = cADimY + 2 * addLen;
    cDExtendedDimY = cAExtendedDimY;

    // extend cA
    svtkm::Id cADimX = inPretendDimX;
    svtkm::Id cADimZ = inPretendDimZ;
    this->Extend3DTopDown(coeffIn,
                          inDimX,
                          inDimY,
                          inDimZ,
                          inStartX,
                          inStartY,
                          inStartZ,
                          cADimX,
                          cADimY,
                          cADimZ,
                          ext1,
                          ext2,
                          addLen,
                          cATopMode,
                          cADownMode,
                          false,
                          false);
    ext1DimY = ext2DimY = addLen;

    // extend cD
    svtkm::Id cDDimX = inPretendDimX;
    svtkm::Id cDDimZ = inPretendDimZ;
    bool pretendSigPaddedZero, padZeroAtExt2;
    if (cDPadLen > 0)
    {
      ext3DimY = addLen;
      ext4DimY = addLen + 1;
      pretendSigPaddedZero = true;
      padZeroAtExt2 = false;
    }
    else
    {
      svtkm::Id cDExtendedWouldBe = cDDimY + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimY)
      {
        ext3DimY = ext4DimY = addLen;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = false;
      }
      else if (cDExtendedWouldBe == cDExtendedDimY - 1)
      {
        ext3DimY = addLen;
        ext4DimY = addLen + 1;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = true;
      }
      else
      {
        pretendSigPaddedZero = padZeroAtExt2 = false; // so the compiler doesn't complain
        svtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
    this->Extend3DTopDown(coeffIn,
                          inDimX,
                          inDimY,
                          inDimZ,
                          inStartX,
                          inStartY + cADimY,
                          inStartZ,
                          cDDimX,
                          cDDimY,
                          cDDimZ,
                          ext3,
                          ext4,
                          addLen,
                          cDTopMode,
                          cDDownMode,
                          pretendSigPaddedZero,
                          padZeroAtExt2);
  }

  template <typename ArrayInType, typename ArrayOutType>
  void IDWTHelper3DFrontBack(const ArrayInType& coeffIn,
                             svtkm::Id inDimX,
                             svtkm::Id inDimY,
                             svtkm::Id inDimZ,
                             svtkm::Id inStartX,
                             svtkm::Id inStartY,
                             svtkm::Id inStartZ,
                             svtkm::Id inPretendDimX,
                             svtkm::Id inPretendDimY,
                             svtkm::Id inPretendDimZ,
                             svtkm::Id cADimZ,
                             svtkm::Id cDDimZ,
                             ArrayOutType& ext1,
                             ArrayOutType& ext2, // output
                             ArrayOutType& ext3,
                             ArrayOutType& ext4, // output
                             svtkm::Id& ext1DimZ,
                             svtkm::Id& ext2DimZ, // output
                             svtkm::Id& ext3DimZ,
                             svtkm::Id& ext4DimZ, // output
                             svtkm::Id filterLen,
                             DWTMode mode)
  {
    SVTKM_ASSERT(inPretendDimZ == (cADimZ + cDDimZ));

    // determine extension modes
    DWTMode cAFrontMode, cABackMode, cDFrontMode, cDBackMode;
    cAFrontMode = cABackMode = cDFrontMode = cDBackMode = mode;
    if (mode == SYMH)
    {
      cDFrontMode = ASYMH;
      if (inPretendDimZ % 2 != 0)
      {
        cABackMode = SYMW;
        cDBackMode = ASYMW;
      }
      else
      {
        cDBackMode = ASYMH;
      }
    }
    else // mode == SYMW
    {
      cDFrontMode = SYMH;
      if (inPretendDimZ % 2 != 0)
      {
        cABackMode = SYMW;
        cDBackMode = SYMH;
      }
      else
      {
        cABackMode = SYMH;
      }
    }

    // determine length after extension
    svtkm::Id cAExtendedDimZ, cDExtendedDimZ;
    svtkm::Id cDPadLen = 0;
    svtkm::Id addLen = filterLen / 4; // addLen == 0 for Haar kernel
    if ((cADimZ > cDDimZ) && (mode == SYMH))
    {
      cDPadLen = cADimZ;
    }
    cAExtendedDimZ = cADimZ + 2 * addLen;
    cDExtendedDimZ = cAExtendedDimZ;

    // extend cA
    svtkm::Id cADimX = inPretendDimX;
    svtkm::Id cADimY = inPretendDimY;
    this->Extend3DFrontBack(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX,
                            inStartY,
                            inStartZ,
                            cADimX,
                            cADimY,
                            cADimZ,
                            ext1,
                            ext2,
                            addLen,
                            cAFrontMode,
                            cABackMode,
                            false,
                            false);
    ext1DimZ = ext2DimZ = addLen;

    // extend cD
    svtkm::Id cDDimX = inPretendDimX;
    svtkm::Id cDDimY = inPretendDimY;
    bool pretendSigPaddedZero, padZeroAtExt2;
    if (cDPadLen > 0)
    {
      ext3DimZ = addLen;
      ext4DimZ = addLen + 1;
      pretendSigPaddedZero = true;
      padZeroAtExt2 = false;
    }
    else
    {
      svtkm::Id cDExtendedWouldBe = cDDimZ + 2 * addLen;
      if (cDExtendedWouldBe == cDExtendedDimZ)
      {
        ext3DimZ = ext4DimZ = addLen;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = false;
      }
      else if (cDExtendedWouldBe == cDExtendedDimZ - 1)
      {
        ext3DimZ = addLen;
        ext4DimZ = addLen + 1;
        pretendSigPaddedZero = false;
        padZeroAtExt2 = true;
      }
      else
      {
        pretendSigPaddedZero = padZeroAtExt2 = false; // so the compiler doesn't complain
        svtkm::cont::ErrorInternal("cDTemp Length not match!");
      }
    }
    this->Extend3DFrontBack(coeffIn,
                            inDimX,
                            inDimY,
                            inDimZ,
                            inStartX,
                            inStartY,
                            inStartZ + cADimZ,
                            cDDimX,
                            cDDimY,
                            cDDimZ,
                            ext3,
                            ext4,
                            addLen,
                            cDFrontMode,
                            cDBackMode,
                            pretendSigPaddedZero,
                            padZeroAtExt2);
  }
};

} // namespace wavelets
} // namespace worklet
} // namespace svtkm

#endif
