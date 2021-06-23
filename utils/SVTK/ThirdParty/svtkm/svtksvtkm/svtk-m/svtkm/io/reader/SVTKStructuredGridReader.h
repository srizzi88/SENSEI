//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_reader_SVTKStructuredGridReader_h
#define svtk_m_io_reader_SVTKStructuredGridReader_h

#include <svtkm/io/reader/SVTKDataSetReaderBase.h>

namespace svtkm
{
namespace io
{
namespace reader
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

class SVTKStructuredGridReader : public SVTKDataSetReaderBase
{
public:
  explicit SVTKStructuredGridReader(const char* fileName)
    : SVTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != svtkm::io::internal::DATASET_STRUCTURED_GRID)
    {
      throw svtkm::io::ErrorIO("Incorrect DataSet type");
    }

    std::string tag;

    //We need to be able to handle VisIt files which dump Field data
    //at the top of a SVTK file
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

    this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));

    // Read the points
    this->DataFile->Stream >> tag;
    internal::parseAssert(tag == "POINTS");
    this->ReadPoints();

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace svtkm::io:reader

#endif // svtk_m_io_reader_SVTKStructuredGridReader_h
