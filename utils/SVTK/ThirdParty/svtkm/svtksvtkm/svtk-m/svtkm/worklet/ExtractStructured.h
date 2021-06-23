//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_ExtractStructured_h
#define svtk_m_worklet_ExtractStructured_h

#include <svtkm/RangeId3.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellSetList.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>

#include <svtkm/cont/CellSetStructured.h>

namespace svtkm
{
namespace worklet
{

namespace extractstructured
{
namespace internal
{

class SubArrayPermutePoints
{
public:
  SubArrayPermutePoints() = default;

  SubArrayPermutePoints(svtkm::Id size,
                        svtkm::Id first,
                        svtkm::Id last,
                        svtkm::Id stride,
                        bool includeBoundary)
    : MaxIdx(size - 1)
    , First(first)
    , Last(last)
    , Stride(stride)
    , IncludeBoundary(includeBoundary)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id operator()(svtkm::Id idx) const
  {
    return (this->IncludeBoundary && (idx == this->MaxIdx)) ? (this->Last)
                                                            : (this->First + (idx * this->Stride));
  }

private:
  svtkm::Id MaxIdx;
  svtkm::Id First, Last;
  svtkm::Id Stride;
  bool IncludeBoundary;
};

struct ExtractCopy : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);

  ExtractCopy(const svtkm::Id3& dim)
    : XDim(dim[0])
    , XYDim(dim[0] * dim[1])
  {
  }
  SVTKM_EXEC_CONT
  inline svtkm::Id ToFlat(const svtkm::Id3& index) const
  {
    return index[0] + index[1] * this->XDim + index[2] * this->XYDim;
  }

  template <typename ScalarType, typename WholeFieldIn>
  SVTKM_EXEC void operator()(svtkm::Id3& index,
                            ScalarType& output,
                            const WholeFieldIn& inputField) const
  {
    output = inputField.Get(this->ToFlat(index));
  }

  svtkm::Id XDim;
  svtkm::Id XYDim;
};
}
} // extractstructured::internal

class ExtractStructured
{
public:
  using DynamicCellSetStructured =
    svtkm::cont::DynamicCellSetBase<svtkm::cont::CellSetListStructured>;

private:
  using AxisIndexArrayPoints =
    svtkm::cont::ArrayHandleImplicit<extractstructured::internal::SubArrayPermutePoints>;
  using PointIndexArray = svtkm::cont::ArrayHandleCartesianProduct<AxisIndexArrayPoints,
                                                                  AxisIndexArrayPoints,
                                                                  AxisIndexArrayPoints>;

  using AxisIndexArrayCells = svtkm::cont::ArrayHandleCounting<svtkm::Id>;
  using CellIndexArray = svtkm::cont::ArrayHandleCartesianProduct<AxisIndexArrayCells,
                                                                 AxisIndexArrayCells,
                                                                 AxisIndexArrayCells>;

  inline AxisIndexArrayPoints MakeAxisIndexArrayPoints(svtkm::Id count,
                                                       svtkm::Id first,
                                                       svtkm::Id last,
                                                       svtkm::Id stride,
                                                       bool includeBoundary)
  {
    auto fnctr = extractstructured::internal::SubArrayPermutePoints(
      count, first, last, stride, includeBoundary);
    return svtkm::cont::make_ArrayHandleImplicit(fnctr, count);
  }

  inline AxisIndexArrayCells MakeAxisIndexArrayCells(svtkm::Id count,
                                                     svtkm::Id start,
                                                     svtkm::Id stride)
  {
    return svtkm::cont::make_ArrayHandleCounting(start, stride, count);
  }

  DynamicCellSetStructured MakeCellSetStructured(const svtkm::Id3& inputPointDims,
                                                 const svtkm::Id3& inputOffsets,
                                                 svtkm::IdComponent forcedDimensionality = 0)
  {
    // when the point dimension for a given axis is 1 we
    // need to lower the dimensonality by 1. So a Plane
    // in XZ space would have a dimensonality of 2.
    // likewise the global offsets need to also
    // be updated when this occurs
    svtkm::IdComponent dimensionality = forcedDimensionality;
    svtkm::Id3 dimensions = inputPointDims;
    svtkm::Id3 offset = inputOffsets;
    for (int i = 0; i < 3 && (forcedDimensionality == 0); ++i)
    {
      if (inputPointDims[i] > 1)
      {
        dimensions[dimensionality] = inputPointDims[i];
        offset[dimensionality] = inputOffsets[i];
        ++dimensionality;
      }
    }

    switch (dimensionality)
    {
      case 1:
      {
        svtkm::cont::CellSetStructured<1> outCs;
        outCs.SetPointDimensions(dimensions[0]);
        outCs.SetGlobalPointIndexStart(offset[0]);
        return outCs;
      }
      case 2:
      {
        svtkm::cont::CellSetStructured<2> outCs;
        outCs.SetPointDimensions(svtkm::Id2(dimensions[0], dimensions[1]));
        outCs.SetGlobalPointIndexStart(svtkm::Id2(offset[0], offset[1]));
        return outCs;
      }
      case 3:
      {
        svtkm::cont::CellSetStructured<3> outCs;
        outCs.SetPointDimensions(dimensions);
        outCs.SetGlobalPointIndexStart(offset);
        return outCs;
      }
      default:
        return DynamicCellSetStructured();
    }
  }

public:
  inline DynamicCellSetStructured Run(const svtkm::cont::CellSetStructured<1>& cellset,
                                      const svtkm::RangeId3& voi,
                                      const svtkm::Id3& sampleRate,
                                      bool includeBoundary,
                                      bool includeOffset)
  {
    svtkm::Id pdims = cellset.GetPointDimensions();
    svtkm::Id offsets = cellset.GetGlobalPointIndexStart();
    return this->Compute(1,
                         svtkm::Id3{ pdims, 1, 1 },
                         svtkm::Id3{ offsets, 0, 0 },
                         voi,
                         sampleRate,
                         includeBoundary,
                         includeOffset);
  }

  inline DynamicCellSetStructured Run(const svtkm::cont::CellSetStructured<2>& cellset,
                                      const svtkm::RangeId3& voi,
                                      const svtkm::Id3& sampleRate,
                                      bool includeBoundary,
                                      bool includeOffset)
  {
    svtkm::Id2 pdims = cellset.GetPointDimensions();
    svtkm::Id2 offsets = cellset.GetGlobalPointIndexStart();
    return this->Compute(2,
                         svtkm::Id3{ pdims[0], pdims[1], 1 },
                         svtkm::Id3{ offsets[0], offsets[1], 0 },
                         voi,
                         sampleRate,
                         includeBoundary,
                         includeOffset);
  }

  inline DynamicCellSetStructured Run(const svtkm::cont::CellSetStructured<3>& cellset,
                                      const svtkm::RangeId3& voi,
                                      const svtkm::Id3& sampleRate,
                                      bool includeBoundary,
                                      bool includeOffset)
  {
    svtkm::Id3 pdims = cellset.GetPointDimensions();
    svtkm::Id3 offsets = cellset.GetGlobalPointIndexStart();
    return this->Compute(3, pdims, offsets, voi, sampleRate, includeBoundary, includeOffset);
  }

  DynamicCellSetStructured Compute(const int dimensionality,
                                   const svtkm::Id3& ptdim,
                                   const svtkm::Id3& offsets,
                                   const svtkm::RangeId3& voi,
                                   const svtkm::Id3& sampleRate,
                                   bool includeBoundary,
                                   bool includeOffset)
  {
    // Verify input parameters
    svtkm::Id3 offset_vec(0, 0, 0);
    svtkm::Id3 globalOffset(0, 0, 0);

    this->InputDimensions = ptdim;
    this->InputDimensionality = dimensionality;
    this->SampleRate = sampleRate;

    if (sampleRate[0] < 1 || sampleRate[1] < 1 || sampleRate[2] < 1)
    {
      throw svtkm::cont::ErrorBadValue("Bad sampling rate");
    }
    if (includeOffset)
    {
      svtkm::Id3 tmpDims = ptdim;
      offset_vec = offsets;
      for (int i = 0; i < dimensionality; ++i)
      {
        if (dimensionality > i)
        {
          if (offset_vec[i] >= voi[i].Min)
          {
            globalOffset[i] = offset_vec[i];
            this->VOI[i].Min = offset_vec[i];
            if (globalOffset[i] + ptdim[i] < voi[i].Max)
            {
              // Start from our GPIS (start point) up to the length of the
              // dimensions (if that is within VOI)
              this->VOI[i].Max = globalOffset[i] + ptdim[i];
            }
            else
            {
              // If it isn't within the voi we set our dimensions from the
              // GPIS up to the VOI.
              tmpDims[i] = voi[i].Max - globalOffset[i];
            }
          }
          else if (offset_vec[i] < voi[i].Min)
          {
            if (offset_vec[i] + ptdim[i] < voi[i].Min)
            {
              // If we're out of bounds we set the dimensions to 0. This
              // causes a return of DynamicCellSetStructured
              tmpDims[i] = 0;
            }
            else
            {
              // If our GPIS is less than VOI min, but our dimensions
              // include the VOI we go from the minimal value that we
              // can up to how far has been specified.
              globalOffset[i] = voi[i].Min;
              this->VOI[i].Min = voi[i].Min;
              if (globalOffset[i] + ptdim[i] < voi[i].Max)
              {
                this->VOI[i].Max = globalOffset[i] + ptdim[i];
              }
              else
              {
                tmpDims[i] = voi[i].Max - globalOffset[i];
              }
            }
          }
        }
      }
      this->OutputDimensions = svtkm::Id3(tmpDims[0], tmpDims[1], tmpDims[2]);
    }

    this->VOI.X.Min = svtkm::Max(svtkm::Id(0), voi.X.Min);
    this->VOI.X.Max = svtkm::Min(this->InputDimensions[0] + globalOffset[0], voi.X.Max);
    this->VOI.Y.Min = svtkm::Max(svtkm::Id(0), voi.Y.Min);
    this->VOI.Y.Max = svtkm::Min(this->InputDimensions[1] + globalOffset[1], voi.Y.Max);
    this->VOI.Z.Min = svtkm::Max(svtkm::Id(0), voi.Z.Min);
    this->VOI.Z.Max = svtkm::Min(this->InputDimensions[2] + globalOffset[2], voi.Z.Max);

    if (!this->VOI.IsNonEmpty())
    {
      svtkm::Id3 empty = { 0, 0, 0 };
      return MakeCellSetStructured(empty, empty, dimensionality);
    }
    if (!includeOffset)
    {
      // compute output dimensions
      this->OutputDimensions = svtkm::Id3(1, 1, 1);
      svtkm::Id3 voiDims = this->VOI.Dimensions();
      for (int i = 0; i < dimensionality; ++i)
      {
        this->OutputDimensions[i] = ((voiDims[i] + this->SampleRate[i] - 1) / this->SampleRate[i]) +
          ((includeBoundary && ((voiDims[i] - 1) % this->SampleRate[i])) ? 1 : 0);
      }
      this->ValidPoints = svtkm::cont::make_ArrayHandleCartesianProduct(
        MakeAxisIndexArrayPoints(this->OutputDimensions[0],
                                 this->VOI.X.Min,
                                 this->VOI.X.Max - 1,
                                 this->SampleRate[0],
                                 includeBoundary),
        MakeAxisIndexArrayPoints(this->OutputDimensions[1],
                                 this->VOI.Y.Min,
                                 this->VOI.Y.Max - 1,
                                 this->SampleRate[1],
                                 includeBoundary),
        MakeAxisIndexArrayPoints(this->OutputDimensions[2],
                                 this->VOI.Z.Min,
                                 this->VOI.Z.Max - 1,
                                 this->SampleRate[2],
                                 includeBoundary));

      this->ValidCells = svtkm::cont::make_ArrayHandleCartesianProduct(
        MakeAxisIndexArrayCells(svtkm::Max(svtkm::Id(1), this->OutputDimensions[0] - 1),
                                this->VOI.X.Min,
                                this->SampleRate[0]),
        MakeAxisIndexArrayCells(svtkm::Max(svtkm::Id(1), this->OutputDimensions[1] - 1),
                                this->VOI.Y.Min,
                                this->SampleRate[1]),
        MakeAxisIndexArrayCells(svtkm::Max(svtkm::Id(1), this->OutputDimensions[2] - 1),
                                this->VOI.Z.Min,
                                this->SampleRate[2]));
    }

    return MakeCellSetStructured(this->OutputDimensions, globalOffset);
  }


private:
  class CallRun
  {
  public:
    CallRun(ExtractStructured* worklet,
            const svtkm::RangeId3& voi,
            const svtkm::Id3& sampleRate,
            bool includeBoundary,
            bool includeOffset,
            DynamicCellSetStructured& output)
      : Worklet(worklet)
      , VOI(&voi)
      , SampleRate(&sampleRate)
      , IncludeBoundary(includeBoundary)
      , IncludeOffset(includeOffset)
      , Output(&output)
    {
    }

    template <int N>
    void operator()(const svtkm::cont::CellSetStructured<N>& cellset) const
    {
      *this->Output = this->Worklet->Run(
        cellset, *this->VOI, *this->SampleRate, this->IncludeBoundary, this->IncludeOffset);
    }

    template <typename CellSetType>
    void operator()(const CellSetType&) const
    {
      throw svtkm::cont::ErrorBadType("ExtractStructured only works with structured datasets");
    }

  private:
    ExtractStructured* Worklet;
    const svtkm::RangeId3* VOI;
    const svtkm::Id3* SampleRate;
    bool IncludeBoundary;
    bool IncludeOffset;
    DynamicCellSetStructured* Output;
  };

public:
  template <typename CellSetList>
  DynamicCellSetStructured Run(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellset,
                               const svtkm::RangeId3& voi,
                               const svtkm::Id3& sampleRate,
                               bool includeBoundary,
                               bool includeOffset)
  {
    DynamicCellSetStructured output;
    CallRun cr(this, voi, sampleRate, includeBoundary, includeOffset, output);
    svtkm::cont::CastAndCall(cellset, cr);
    return output;
  }

private:
  using UniformCoordinatesArrayHandle = svtkm::cont::ArrayHandleUniformPointCoordinates::Superclass;

  using RectilinearCoordinatesArrayHandle = svtkm::cont::ArrayHandleCartesianProduct<
    svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
    svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
    svtkm::cont::ArrayHandle<svtkm::FloatDefault>>::Superclass;


  svtkm::cont::ArrayHandleVirtualCoordinates MapCoordinatesUniform(
    const UniformCoordinatesArrayHandle& coords)
  {
    using CoordsArray = svtkm::cont::ArrayHandleUniformPointCoordinates;
    using CoordType = CoordsArray::ValueType;
    using ValueType = CoordType::ComponentType;

    const auto& portal = coords.GetPortalConstControl();
    CoordType inOrigin = portal.GetOrigin();
    CoordType inSpacing = portal.GetSpacing();

    CoordType outOrigin =
      svtkm::make_Vec(inOrigin[0] + static_cast<ValueType>(this->VOI.X.Min) * inSpacing[0],
                     inOrigin[1] + static_cast<ValueType>(this->VOI.Y.Min) * inSpacing[1],
                     inOrigin[2] + static_cast<ValueType>(this->VOI.Z.Min) * inSpacing[2]);
    CoordType outSpacing = inSpacing * static_cast<CoordType>(this->SampleRate);

    auto out = CoordsArray(this->OutputDimensions, outOrigin, outSpacing);
    return svtkm::cont::ArrayHandleVirtualCoordinates(out);
  }

  svtkm::cont::ArrayHandleVirtualCoordinates MapCoordinatesRectilinear(
    const RectilinearCoordinatesArrayHandle& coords)
  {
    // For structured datasets, the cellsets are of different types based on
    // its dimensionality, but the coordinates are always 3 dimensional.
    // We can map the axis of the cellset to the coordinates by looking at the
    // length of a coordinate axis array.
    AxisIndexArrayPoints validIds[3] = { this->ValidPoints.GetStorage().GetFirstArray(),
                                         this->ValidPoints.GetStorage().GetSecondArray(),
                                         this->ValidPoints.GetStorage().GetThirdArray() };

    svtkm::cont::ArrayHandle<svtkm::FloatDefault> arrays[3] = { coords.GetStorage().GetFirstArray(),
                                                              coords.GetStorage().GetSecondArray(),
                                                              coords.GetStorage().GetThirdArray() };

    svtkm::cont::ArrayHandle<svtkm::FloatDefault> xyzs[3];
    int dim = 0;
    for (int i = 0; i < 3; ++i)
    {
      if (arrays[i].GetNumberOfValues() == 1)
      {
        xyzs[i].Allocate(1);
        xyzs[i].GetPortalControl().Set(0, svtkm::cont::ArrayGetValue(0, arrays[i]));
      }
      else
      {
        svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandlePermutation(validIds[i], arrays[i]),
                              xyzs[i]);
        ++dim;
      }
    }
    SVTKM_ASSERT(dim == this->InputDimensionality);

    auto out = svtkm::cont::make_ArrayHandleCartesianProduct(xyzs[0], xyzs[1], xyzs[2]);
    return svtkm::cont::ArrayHandleVirtualCoordinates(out);
  }

public:
  svtkm::cont::ArrayHandleVirtualCoordinates MapCoordinates(
    const svtkm::cont::CoordinateSystem& coordinates)
  {
    auto coArray = coordinates.GetData();
    if (coArray.IsType<UniformCoordinatesArrayHandle>())
    {
      return this->MapCoordinatesUniform(coArray.Cast<UniformCoordinatesArrayHandle>());
    }
    else if (coArray.IsType<RectilinearCoordinatesArrayHandle>())
    {
      return this->MapCoordinatesRectilinear(coArray.Cast<RectilinearCoordinatesArrayHandle>());
    }
    else
    {
      auto out = this->ProcessPointField(coArray);
      return svtkm::cont::ArrayHandleVirtualCoordinates(out);
    }
  }

public:
  template <typename T, typename Storage>
  svtkm::cont::ArrayHandle<T> ProcessPointField(
    const svtkm::cont::ArrayHandle<T, Storage>& field) const
  {
    using namespace extractstructured::internal;
    svtkm::cont::ArrayHandle<T> result;
    result.Allocate(this->ValidPoints.GetNumberOfValues());

    ExtractCopy worklet(this->InputDimensions);
    DispatcherMapField<ExtractCopy> dispatcher(worklet);
    dispatcher.Invoke(this->ValidPoints, result, field);

    return result;
  }

  template <typename T, typename Storage>
  svtkm::cont::ArrayHandle<T> ProcessCellField(
    const svtkm::cont::ArrayHandle<T, Storage>& field) const
  {
    using namespace extractstructured::internal;
    svtkm::cont::ArrayHandle<T> result;
    result.Allocate(this->ValidCells.GetNumberOfValues());

    auto inputCellDimensions = this->InputDimensions - svtkm::Id3(1);
    ExtractCopy worklet(inputCellDimensions);
    DispatcherMapField<ExtractCopy> dispatcher(worklet);
    dispatcher.Invoke(this->ValidCells, result, field);

    return result;
  }

private:
  svtkm::RangeId3 VOI;
  svtkm::Id3 SampleRate = { 1, 1, 1 };

  int InputDimensionality;
  svtkm::Id3 InputDimensions;
  svtkm::Id3 OutputDimensions;

  PointIndexArray ValidPoints;
  CellIndexArray ValidCells;
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_ExtractStructured_h
