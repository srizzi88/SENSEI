//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DataSetBuilderRectilinear_h
#define svtk_m_cont_DataSetBuilderRectilinear_h

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT DataSetBuilderRectilinear
{
  template <typename T, typename U>
  SVTKM_CONT static void CopyInto(const std::vector<T>& input, svtkm::cont::ArrayHandle<U>& output)
  {
    DataSetBuilderRectilinear::CopyInto(svtkm::cont::make_ArrayHandle(input), output);
  }

  template <typename T, typename U>
  SVTKM_CONT static void CopyInto(const svtkm::cont::ArrayHandle<T>& input,
                                 svtkm::cont::ArrayHandle<U>& output)
  {
    svtkm::cont::ArrayCopy(input, output);
  }

  template <typename T, typename U>
  SVTKM_CONT static void CopyInto(const T* input, svtkm::Id len, svtkm::cont::ArrayHandle<U>& output)
  {
    DataSetBuilderRectilinear::CopyInto(svtkm::cont::make_ArrayHandle(input, len), output);
  }

public:
  SVTKM_CONT
  DataSetBuilderRectilinear();

  //1D grids.
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<T>& xvals,
                                              const std::string& coordNm = "coords")
  {
    std::vector<T> yvals(1, 0), zvals(1, 0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(svtkm::Id nx,
                                              T* xvals,
                                              const std::string& coordNm = "coords")
  {
    T yvals = 0, zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(nx, 1, 1, xvals, &yvals, &zvals, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const svtkm::cont::ArrayHandle<T>& xvals,
                                              const std::string& coordNm = "coords")
  {
    svtkm::cont::ArrayHandle<T> yvals, zvals;
    yvals.Allocate(1);
    yvals.GetPortalControl().Set(0, 0.0);
    zvals.Allocate(1);
    zvals.GetPortalControl().Set(0, 0.0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  //2D grids.
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<T>& xvals,
                                              const std::vector<T>& yvals,
                                              const std::string& coordNm = "coords")
  {
    std::vector<T> zvals(1, 0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(svtkm::Id nx,
                                              svtkm::Id ny,
                                              T* xvals,
                                              T* yvals,
                                              const std::string& coordNm = "coords")
  {
    T zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(nx, ny, 1, xvals, yvals, &zvals, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const svtkm::cont::ArrayHandle<T>& xvals,
                                              const svtkm::cont::ArrayHandle<T>& yvals,
                                              const std::string& coordNm = "coords")
  {
    svtkm::cont::ArrayHandle<T> zvals;
    zvals.Allocate(1);
    zvals.GetPortalControl().Set(0, 0.0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  //3D grids.
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(svtkm::Id nx,
                                              svtkm::Id ny,
                                              svtkm::Id nz,
                                              T* xvals,
                                              T* yvals,
                                              T* zvals,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(nx, ny, nz, xvals, yvals, zvals, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<T>& xvals,
                                              const std::vector<T>& yvals,
                                              const std::vector<T>& zvals,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const svtkm::cont::ArrayHandle<T>& xvals,
                                              const svtkm::cont::ArrayHandle<T>& yvals,
                                              const svtkm::cont::ArrayHandle<T>& zvals,
                                              const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

private:
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet BuildDataSet(const std::vector<T>& xvals,
                                                    const std::vector<T>& yvals,
                                                    const std::vector<T>& zvals,
                                                    const std::string& coordNm)
  {
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(Xc, Yc, Zc, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet BuildDataSet(svtkm::Id nx,
                                                    svtkm::Id ny,
                                                    svtkm::Id nz,
                                                    const T* xvals,
                                                    const T* yvals,
                                                    const T* zvals,
                                                    const std::string& coordNm)
  {
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, nx, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, ny, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, nz, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(Xc, Yc, Zc, coordNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet BuildDataSet(const svtkm::cont::ArrayHandle<T>& X,
                                                    const svtkm::cont::ArrayHandle<T>& Y,
                                                    const svtkm::cont::ArrayHandle<T>& Z,
                                                    const std::string& coordNm)
  {
    svtkm::cont::DataSet dataSet;

    //Convert all coordinates to floatDefault.
    svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>>
      coords;

    svtkm::cont::ArrayHandle<svtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(X, Xc);
    DataSetBuilderRectilinear::CopyInto(Y, Yc);
    DataSetBuilderRectilinear::CopyInto(Z, Zc);

    coords = svtkm::cont::make_ArrayHandleCartesianProduct(Xc, Yc, Zc);
    svtkm::cont::CoordinateSystem cs(coordNm, coords);
    dataSet.AddCoordinateSystem(cs);

    // compute the dimensions of the cellset by counting the number of axes
    // with >1 dimension
    int ndims = 0;
    svtkm::Id dims[3];
    if (Xc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Xc.GetNumberOfValues();
    }
    if (Yc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Yc.GetNumberOfValues();
    }
    if (Zc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Zc.GetNumberOfValues();
    }

    if (ndims == 1)
    {
      svtkm::cont::CellSetStructured<1> cellSet;
      cellSet.SetPointDimensions(dims[0]);
      dataSet.SetCellSet(cellSet);
    }
    else if (ndims == 2)
    {
      svtkm::cont::CellSetStructured<2> cellSet;
      cellSet.SetPointDimensions(svtkm::make_Vec(dims[0], dims[1]));
      dataSet.SetCellSet(cellSet);
    }
    else if (ndims == 3)
    {
      svtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(svtkm::make_Vec(dims[0], dims[1], dims[2]));
      dataSet.SetCellSet(cellSet);
    }
    else
    {
      throw svtkm::cont::ErrorBadValue("Invalid cell set dimension");
    }

    return dataSet;
  }
};

} // namespace cont
} // namespace svtkm

#endif //svtk_m_cont_DataSetBuilderRectilinear_h
