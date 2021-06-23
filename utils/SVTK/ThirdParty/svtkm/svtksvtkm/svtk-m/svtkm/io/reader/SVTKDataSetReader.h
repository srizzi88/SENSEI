//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_reader_SVTKDataSetReader_h
#define svtk_m_io_reader_SVTKDataSetReader_h

#include <svtkm/io/reader/SVTKDataSetReaderBase.h>
#include <svtkm/io/reader/SVTKPolyDataReader.h>
#include <svtkm/io/reader/SVTKRectilinearGridReader.h>
#include <svtkm/io/reader/SVTKStructuredGridReader.h>
#include <svtkm/io/reader/SVTKStructuredPointsReader.h>
#include <svtkm/io/reader/SVTKUnstructuredGridReader.h>

#include <memory>

namespace svtkm
{
namespace io
{
namespace reader
{

SVTKM_SILENCE_WEAK_VTABLE_WARNING_START

class SVTKDataSetReader : public SVTKDataSetReaderBase
{
public:
  explicit SVTKDataSetReader(const char* fileName)
    : SVTKDataSetReaderBase(fileName)
  {
  }

  explicit SVTKDataSetReader(const std::string& fileName)
    : SVTKDataSetReaderBase(fileName)
  {
  }

  virtual void PrintSummary(std::ostream& out) const
  {
    if (this->Reader)
    {
      this->Reader->PrintSummary(out);
    }
    else
    {
      SVTKDataSetReaderBase::PrintSummary(out);
    }
  }

private:
  virtual void CloseFile()
  {
    if (this->Reader)
    {
      this->Reader->CloseFile();
    }
    else
    {
      SVTKDataSetReaderBase::CloseFile();
    }
  }

  virtual void Read()
  {
    switch (this->DataFile->Structure)
    {
      case svtkm::io::internal::DATASET_STRUCTURED_POINTS:
        this->Reader.reset(new SVTKStructuredPointsReader(""));
        break;
      case svtkm::io::internal::DATASET_STRUCTURED_GRID:
        this->Reader.reset(new SVTKStructuredGridReader(""));
        break;
      case svtkm::io::internal::DATASET_RECTILINEAR_GRID:
        this->Reader.reset(new SVTKRectilinearGridReader(""));
        break;
      case svtkm::io::internal::DATASET_POLYDATA:
        this->Reader.reset(new SVTKPolyDataReader(""));
        break;
      case svtkm::io::internal::DATASET_UNSTRUCTURED_GRID:
        this->Reader.reset(new SVTKUnstructuredGridReader(""));
        break;
      default:
        throw svtkm::io::ErrorIO("Unsupported DataSet type.");
    }

    this->TransferDataFile(*this->Reader.get());
    this->Reader->Read();
    this->DataSet = this->Reader->GetDataSet();
  }

  std::unique_ptr<SVTKDataSetReaderBase> Reader;
};

SVTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // svtkm::io::reader

#endif // svtk_m_io_reader_SVTKReader_h
