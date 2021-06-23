//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_reader_SVTKRectilinearGridReader_h
#define svtk_m_io_reader_SVTKRectilinearGridReader_h

#include <svtkm/io/reader/SVTKDataSetReaderBase.h>

namespace svtkm
{
namespace io
{
namespace reader
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

class SVTKRectilinearGridReader : public SVTKDataSetReaderBase
{
public:
  explicit SVTKRectilinearGridReader(const char* fileName)
    : SVTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != svtkm::io::internal::DATASET_RECTILINEAR_GRID)
      throw svtkm::io::ErrorIO("Incorrect DataSet type");

    //We need to be able to handle VisIt files which dump Field data
    //at the top of a SVTK file
    std::string tag;
    this->DataFile->Stream >> tag;
    if (tag == "FIELD")
    {
      this->ReadGlobalFields();
      this->DataFile->Stream >> tag;
    }

    // Read structured grid specific meta-data
    internal::parseAssert(tag == "DIMENSIONS");
    svtkm::Id3 dim;
    this->DataFile->Stream >> dim[0] >> dim[1] >> dim[2] >> std::ws;

    //Read the points.
    std::string dataType;
    std::size_t numPoints[3];
    svtkm::cont::VariantArrayHandle X, Y, Z;

    // Always read coordinates as svtkm::FloatDefault
    std::string readDataType = svtkm::io::internal::DataTypeName<svtkm::FloatDefault>::Name();

    this->DataFile->Stream >> tag >> numPoints[0] >> dataType >> std::ws;
    if (tag != "X_COORDINATES")
      throw svtkm::io::ErrorIO("X_COORDINATES tag not found");
    X =
      this->DoReadArrayVariant(svtkm::cont::Field::Association::ANY, readDataType, numPoints[0], 1);

    this->DataFile->Stream >> tag >> numPoints[1] >> dataType >> std::ws;
    if (tag != "Y_COORDINATES")
      throw svtkm::io::ErrorIO("Y_COORDINATES tag not found");
    Y =
      this->DoReadArrayVariant(svtkm::cont::Field::Association::ANY, readDataType, numPoints[1], 1);

    this->DataFile->Stream >> tag >> numPoints[2] >> dataType >> std::ws;
    if (tag != "Z_COORDINATES")
      throw svtkm::io::ErrorIO("Z_COORDINATES tag not found");
    Z =
      this->DoReadArrayVariant(svtkm::cont::Field::Association::ANY, readDataType, numPoints[2], 1);

    if (dim != svtkm::Id3(static_cast<svtkm::Id>(numPoints[0]),
                         static_cast<svtkm::Id>(numPoints[1]),
                         static_cast<svtkm::Id>(numPoints[2])))
      throw svtkm::io::ErrorIO("DIMENSIONS not equal to number of points");

    svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>,
                                            svtkm::cont::ArrayHandle<svtkm::FloatDefault>>
      coords;

    svtkm::cont::ArrayHandle<svtkm::FloatDefault> Xc, Yc, Zc;
    X.CopyTo(Xc);
    Y.CopyTo(Yc);
    Z.CopyTo(Zc);
    coords = svtkm::cont::make_ArrayHandleCartesianProduct(Xc, Yc, Zc);
    svtkm::cont::CoordinateSystem coordSys("coordinates", coords);
    this->DataSet.AddCoordinateSystem(coordSys);

    this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace svtkm::io:reader

#endif // svtk_m_io_reader_SVTKRectilinearGridReader_h
