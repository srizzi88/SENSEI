//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_wavelets_waveletfilter_h
#define svtk_m_worklet_wavelets_waveletfilter_h

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/worklet/wavelets/FilterBanks.h>

#include <svtkm/Math.h>

namespace svtkm
{
namespace worklet
{

namespace wavelets
{

enum WaveletName
{
  CDF9_7,
  CDF5_3,
  CDF8_4,
  HAAR,
  BIOR4_4, // the same as CDF9_7
  BIOR3_3, // the same as CDF8_4
  BIOR2_2, // the same as CDF5_3
  BIOR1_1  // the same as HAAR
};

// Wavelet filter class;
// functionally equivalent to WaveFiltBase and its subclasses in VAPoR.
class WaveletFilter
{
public:
  // constructor
  WaveletFilter(WaveletName wtype)
    : Symmetricity(true)
    , FilterLength(0)
    , LowDecomposeFilter(nullptr)
    , HighDecomposeFilter(nullptr)
    , LowReconstructFilter(nullptr)
    , HighReconstructFilter(nullptr)
  {
    if (wtype == CDF9_7 || wtype == BIOR4_4)
    {
      this->FilterLength = 9;
      this->AllocateFilterMemory();
      this->wrev(svtkm::worklet::wavelets::hm4_44, LowDecomposeFilter, FilterLength);
      this->qmf_wrev(svtkm::worklet::wavelets::h4, HighDecomposeFilter, FilterLength);
      this->verbatim_copy(svtkm::worklet::wavelets::h4, LowReconstructFilter, FilterLength);
      this->qmf_even(svtkm::worklet::wavelets::hm4_44, HighReconstructFilter, FilterLength);
    }
    else if (wtype == CDF8_4 || wtype == BIOR3_3)
    {
      this->FilterLength = 8;
      this->AllocateFilterMemory();
      this->wrev(svtkm::worklet::wavelets::hm3_33, LowDecomposeFilter, FilterLength);
      this->qmf_wrev(svtkm::worklet::wavelets::h3 + 6, HighDecomposeFilter, FilterLength);
      this->verbatim_copy(svtkm::worklet::wavelets::h3 + 6, LowReconstructFilter, FilterLength);
      this->qmf_even(svtkm::worklet::wavelets::hm3_33, HighReconstructFilter, FilterLength);
    }
    else if (wtype == CDF5_3 || wtype == BIOR2_2)
    {
      this->FilterLength = 5;
      this->AllocateFilterMemory();
      this->wrev(svtkm::worklet::wavelets::hm2_22, LowDecomposeFilter, FilterLength);
      this->qmf_wrev(svtkm::worklet::wavelets::h2 + 6, HighDecomposeFilter, FilterLength);
      this->verbatim_copy(svtkm::worklet::wavelets::h2 + 6, LowReconstructFilter, FilterLength);
      this->qmf_even(svtkm::worklet::wavelets::hm2_22, HighReconstructFilter, FilterLength);
    }
    else if (wtype == HAAR || wtype == BIOR1_1)
    {
      this->FilterLength = 2;
      this->AllocateFilterMemory();
      this->wrev(svtkm::worklet::wavelets::hm1_11, LowDecomposeFilter, FilterLength);
      this->qmf_wrev(svtkm::worklet::wavelets::h1 + 4, HighDecomposeFilter, FilterLength);
      this->verbatim_copy(svtkm::worklet::wavelets::h1 + 4, LowReconstructFilter, FilterLength);
      this->qmf_even(svtkm::worklet::wavelets::hm1_11, HighReconstructFilter, FilterLength);
    }
    this->MakeArrayHandles();
  }

  // destructor
  ~WaveletFilter()
  {
    if (LowDecomposeFilter)
    {
      delete[] LowDecomposeFilter;
      LowDecomposeFilter = HighDecomposeFilter = LowReconstructFilter = HighReconstructFilter =
        nullptr;
    }
  }

  svtkm::Id GetFilterLength() { return this->FilterLength; }

  bool isSymmetric() { return this->Symmetricity; }

  using FilterType = svtkm::cont::ArrayHandle<svtkm::Float64>;

  const FilterType& GetLowDecomposeFilter() const { return this->LowDecomType; }
  const FilterType& GetHighDecomposeFilter() const { return this->HighDecomType; }
  const FilterType& GetLowReconstructFilter() const { return this->LowReconType; }
  const FilterType& GetHighReconstructFilter() const { return this->HighReconType; }

private:
  bool Symmetricity;
  svtkm::Id FilterLength;
  svtkm::Float64* LowDecomposeFilter;
  svtkm::Float64* HighDecomposeFilter;
  svtkm::Float64* LowReconstructFilter;
  svtkm::Float64* HighReconstructFilter;
  FilterType LowDecomType;
  FilterType HighDecomType;
  FilterType LowReconType;
  FilterType HighReconType;

  void AllocateFilterMemory()
  {
    LowDecomposeFilter = new svtkm::Float64[static_cast<std::size_t>(FilterLength * 4)];
    HighDecomposeFilter = LowDecomposeFilter + FilterLength;
    LowReconstructFilter = HighDecomposeFilter + FilterLength;
    HighReconstructFilter = LowReconstructFilter + FilterLength;
  }

  void MakeArrayHandles()
  {
    LowDecomType = svtkm::cont::make_ArrayHandle(LowDecomposeFilter, FilterLength);
    HighDecomType = svtkm::cont::make_ArrayHandle(HighDecomposeFilter, FilterLength);
    LowReconType = svtkm::cont::make_ArrayHandle(LowReconstructFilter, FilterLength);
    HighReconType = svtkm::cont::make_ArrayHandle(HighReconstructFilter, FilterLength);
  }

  // Flipping operation; helper function to initialize a filter.
  void wrev(const svtkm::Float64* arrIn, svtkm::Float64* arrOut, svtkm::Id length)
  {
    for (svtkm::Id count = 0; count < length; count++)
    {
      arrOut[count] = arrIn[length - count - 1];
    }
  }

  // Quadrature mirror filtering operation: helper function to initialize a filter.
  void qmf_even(const svtkm::Float64* arrIn, svtkm::Float64* arrOut, svtkm::Id length)
  {
    if (length % 2 == 0)
    {
      for (svtkm::Id count = 0; count < length; count++)
      {
        arrOut[count] = arrIn[length - count - 1];
        if (count % 2 != 0)
        {
          arrOut[count] = -1.0 * arrOut[count];
        }
      }
    }
    else
    {
      for (svtkm::Id count = 0; count < length; count++)
      {
        arrOut[count] = arrIn[length - count - 1];
        if (count % 2 == 0)
        {
          arrOut[count] = -1.0 * arrOut[count];
        }
      }
    }
  }

  // Flipping and QMF at the same time: helper function to initialize a filter.
  void qmf_wrev(const svtkm::Float64* arrIn, svtkm::Float64* arrOut, svtkm::Id length)
  {
    qmf_even(arrIn, arrOut, length);

    svtkm::Float64 tmp;
    for (svtkm::Id count = 0; count < length / 2; count++)
    {
      tmp = arrOut[count];
      arrOut[count] = arrOut[length - count - 1];
      arrOut[length - count - 1] = tmp;
    }
  }

  // Verbatim Copying: helper function to initialize a filter.
  void verbatim_copy(const svtkm::Float64* arrIn, svtkm::Float64* arrOut, svtkm::Id length)
  {
    for (svtkm::Id count = 0; count < length; count++)
    {
      arrOut[count] = arrIn[count];
    }
  }

}; // class WaveletFilter.
} // namespace wavelets.

} // namespace worklet
} // namespace svtkm

#endif
