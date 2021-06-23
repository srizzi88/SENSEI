/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataReader.h"

#include "svtkBitArray.h"
#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationKeyLookup.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationStringVectorKey.h"
#include "svtkInformationUnsignedLongKey.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLegacyReaderVersion.h"
#include "svtkLongArray.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkRectilinearGrid.h"
#include "svtkShortArray.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTypeInt64Array.h"
#include "svtkTypeUInt64Array.h"
#include "svtkUnicodeStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedShortArray.h"
#include "svtkVariantArray.h"

#include "svtksys/FStream.hxx"
#include <svtksys/SystemTools.hxx>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
// This function is also defined in Infovis/svtkDelimitedTextReader.cxx,
// so it would be nice to put this in a common file.
static int my_getline(istream& stream, svtkStdString& output, char delim = '\n');

svtkStandardNewMacro(svtkDataReader);

svtkCxxSetObjectMacro(svtkDataReader, InputArray, svtkCharArray);

// this undef is required on the hp. svtkMutexLock ends up including
// /usr/include/dce/cma_ux.h which has the gall to #define read as cma_read

#ifdef read
#undef read
#endif

// Construct object.
svtkDataReader::svtkDataReader()
{
  this->FileType = SVTK_ASCII;
  this->ScalarsName = nullptr;
  this->VectorsName = nullptr;
  this->TensorsName = nullptr;
  this->NormalsName = nullptr;
  this->TCoordsName = nullptr;
  this->LookupTableName = nullptr;
  this->FieldDataName = nullptr;
  this->ScalarLut = nullptr;
  this->InputString = nullptr;
  this->InputStringLength = 0;
  this->InputStringPos = 0;
  this->ReadFromInputString = 0;
  this->IS = nullptr;
  this->Header = nullptr;

  this->InputArray = nullptr;

  this->NumberOfScalarsInFile = 0;
  this->ScalarsNameInFile = nullptr;
  this->ScalarsNameAllocSize = 0;
  this->NumberOfVectorsInFile = 0;
  this->VectorsNameInFile = nullptr;
  this->VectorsNameAllocSize = 0;
  this->NumberOfTensorsInFile = 0;
  this->TensorsNameInFile = nullptr;
  this->TensorsNameAllocSize = 0;
  this->NumberOfTCoordsInFile = 0;
  this->TCoordsNameInFile = nullptr;
  this->TCoordsNameAllocSize = 0;
  this->NumberOfNormalsInFile = 0;
  this->NormalsNameInFile = nullptr;
  this->NormalsNameAllocSize = 0;
  this->NumberOfFieldDataInFile = 0;
  this->FieldDataNameInFile = nullptr;
  this->FieldDataNameAllocSize = 0;

  this->ReadAllScalars = 0;
  this->ReadAllVectors = 0;
  this->ReadAllNormals = 0;
  this->ReadAllTensors = 0;
  this->ReadAllColorScalars = 0;
  this->ReadAllTCoords = 0;
  this->ReadAllFields = 0;
  this->FileMajorVersion = 0;
  this->FileMinorVersion = 0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

svtkDataReader::~svtkDataReader()
{
  delete[] this->ScalarsName;
  delete[] this->VectorsName;
  delete[] this->TensorsName;
  delete[] this->NormalsName;
  delete[] this->TCoordsName;
  delete[] this->LookupTableName;
  delete[] this->FieldDataName;
  delete[] this->ScalarLut;
  delete[] this->InputString;
  delete[] this->Header;

  this->SetInputArray(nullptr);
  this->InitializeCharacteristics();
  delete this->IS;
}

void svtkDataReader::SetFileName(const char* fname)
{
  if (this->GetNumberOfFileNames() == 1 && this->GetFileName(0) && fname &&
    strcmp(this->GetFileName(0), fname) == 0)
  {
    return;
  }
  this->ClearFileNames();
  if (fname)
  {
    this->AddFileName(fname);
  }
  this->Modified();
}

const char* svtkDataReader::GetFileName() const
{
  if (this->GetNumberOfFileNames() < 1)
  {
    return nullptr;
  }
  return this->svtkSimpleReader::GetFileName(0);
}

int svtkDataReader::ReadTimeDependentMetaData(int timestep, svtkInformation* metadata)
{
  if (this->ReadFromInputString)
  {
    return this->ReadMetaDataSimple(std::string(), metadata);
  }

  return this->Superclass::ReadTimeDependentMetaData(timestep, metadata);
}

int svtkDataReader::ReadMesh(
  int piece, int npieces, int nghosts, int timestep, svtkDataObject* output)
{
  // Not a parallel reader. Cannot handle anything other than the first piece,
  // which will have everything.
  if (piece > 0)
  {
    return 1;
  }

  if (this->ReadFromInputString)
  {
    return this->ReadMeshSimple(std::string(), output);
  }

  return this->Superclass::ReadMesh(piece, npieces, nghosts, timestep, output);
}

void svtkDataReader::SetInputString(const char* in)
{
  int len = 0;
  if (in != nullptr)
  {
    len = static_cast<int>(strlen(in));
  }
  this->SetInputString(in, len);
}

void svtkDataReader::SetBinaryInputString(const char* in, int len)
{
  this->SetInputString(in, len);
}

void svtkDataReader::SetInputString(const char* in, int len)
{
  if (this->Debug)
  {
    svtkDebugMacro(<< "SetInputString len: " << len << " in: " << (in ? in : "(null)"));
  }

  if (this->InputString && in && strncmp(in, this->InputString, len) == 0)
  {
    return;
  }

  delete[] this->InputString;

  if (in && len > 0)
  {
    // Add a nullptr terminator so that GetInputString
    // callers (from wrapped languages) get a valid
    // C string in *ALL* cases...
    //
    this->InputString = new char[len + 1];
    memcpy(this->InputString, in, len);
    this->InputString[len] = 0;
    this->InputStringLength = len;
  }
  else
  {
    this->InputString = nullptr;
    this->InputStringLength = 0;
  }

  this->Modified();
}

// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int svtkDataReader::ReadLine(char result[256])
{
  this->IS->getline(result, 256);
  if (this->IS->fail())
  {
    if (this->IS->eof())
    {
      return 0;
    }
    if (this->IS->gcount() == 255)
    {
      // Read 256 chars; ignoring the rest of the line.
      this->IS->clear();
      this->IS->ignore(SVTK_INT_MAX, '\n');
    }
  }
  // remove '\r', if present.
  size_t slen = strlen(result);
  if (slen > 0 && result[slen - 1] == '\r')
  {
    result[slen - 1] = '\0';
  }
  return 1;
}

// Internal function to read in a string up to 256 characters.
// Returns zero if there was an error.
int svtkDataReader::ReadString(char result[256])
{
  this->IS->width(256);
  *this->IS >> result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

// Internal function to read in an integer value.
// Returns zero if there was an error.
int svtkDataReader::Read(char* result)
{
  int intData;
  *this->IS >> intData;
  if (this->IS->fail())
  {
    return 0;
  }

  *result = (char)intData;
  return 1;
}

int svtkDataReader::Read(unsigned char* result)
{
  int intData;
  *this->IS >> intData;
  if (this->IS->fail())
  {
    return 0;
  }

  *result = (unsigned char)intData;
  return 1;
}

int svtkDataReader::Read(short* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(unsigned short* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(int* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(unsigned int* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(long* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(unsigned long* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(long long* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(unsigned long long* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(float* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

int svtkDataReader::Read(double* result)
{
  *this->IS >> *result;
  if (this->IS->fail())
  {
    return 0;
  }
  return 1;
}

size_t svtkDataReader::Peek(char* str, size_t n)
{
  if (n == 0)
  {
    return 0;
  }

  this->IS->read(str, n);
  std::streamsize len = this->IS->gcount();

  if (!*this->IS)
  {
    this->IS->clear();
  }

  this->IS->seekg(-len, std::ios_base::cur);

  return len;
}

// Open a svtk data file. Returns zero if error.
int svtkDataReader::OpenSVTKFile(const char* fname)
{
  // Save current locale settings and set standard one to
  // avoid locale issues - for instance with the decimal separator.
  this->CurrentLocale = std::locale::global(std::locale::classic());

  if (!fname && this->GetNumberOfFileNames() > 0)
  {
    fname = this->GetFileName(0);
  }
  this->CurrentFileName = fname ? fname : std::string();

  if (this->IS != nullptr)
  {
    this->CloseSVTKFile();
  }
  if (this->ReadFromInputString)
  {
    if (this->InputArray)
    {
      svtkDebugMacro(<< "Reading from InputArray");
      std::string str(this->InputArray->GetPointer(0),
        static_cast<size_t>(
          this->InputArray->GetNumberOfTuples() * this->InputArray->GetNumberOfComponents()));
      this->IS = new std::istringstream(str);
      return 1;
    }
    else if (this->InputString)
    {
      svtkDebugMacro(<< "Reading from InputString");
      std::string str(this->InputString, this->InputStringLength);
      this->IS = new std::istringstream(str);
      return 1;
    }
  }
  else
  {
    svtkDebugMacro(<< "Opening svtk file");

    if (!fname || (strlen(fname) == 0))
    {
      svtkErrorMacro(<< "No file specified!");
      this->SetErrorCode(svtkErrorCode::NoFileNameError);
      return 0;
    }

    // first make sure the file exists, this prevents an empty file from
    // being created on older compilers
    svtksys::SystemTools::Stat_t fs;
    if (svtksys::SystemTools::Stat(fname, &fs) != 0)
    {
      svtkErrorMacro(<< "Unable to open file: " << fname);
      this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
      return 0;
    }

    this->IS = new svtksys::ifstream(fname, ios::in | ios::binary);
    if (this->IS->fail())
    {
      svtkErrorMacro(<< "Unable to open file: " << fname);
      delete this->IS;
      this->IS = nullptr;
      this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
      return 0;
    }
    return 1;
  }

  return 0;
}

// Read the header of a svtk data file. Returns 0 if error.
int svtkDataReader::ReadHeader(const char* fname)
{
  if (!fname && this->GetNumberOfFileNames() > 0)
  {
    fname = this->GetFileName(0);
  }
  char line[256];

  svtkDebugMacro(<< "Reading svtk file header");
  //
  // read header
  //
  if (!this->ReadLine(line))
  {
    svtkErrorMacro(<< "Premature EOF reading first line! "
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
    return 0;
  }
  const int VERSION_PREFIX_LENGTH = 22;
  if (strncmp("# svtk DataFile Version", line, VERSION_PREFIX_LENGTH))
  {
    svtkErrorMacro(<< "Unrecognized file type: " << line
                  << " for file: " << (fname ? fname : "(Null FileName)"));

    this->SetErrorCode(svtkErrorCode::UnrecognizedFileTypeError);
    return 0;
  }
  if (sscanf(line + VERSION_PREFIX_LENGTH, "%d.%d", &this->FileMajorVersion,
        &this->FileMinorVersion) != 2)
  {
    svtkWarningMacro(<< "Cannot read file version: " << line
                    << " for file: " << (fname ? fname : "(Null FileName)"));
    this->FileMajorVersion = 0;
    this->FileMinorVersion = 0;
  }
  if (this->FileMajorVersion > svtkLegacyReaderMajorVersion ||
    (this->FileMajorVersion == svtkLegacyReaderMajorVersion &&
      this->FileMinorVersion > svtkLegacyReaderMinorVersion))
  {
    // newer file than the reader version
    svtkWarningMacro(<< "Reading file version: " << this->FileMajorVersion << "."
                    << this->FileMinorVersion << " with older reader version "
                    << svtkLegacyReaderMajorVersion << "." << svtkLegacyReaderMinorVersion);
  }

  //
  // read title
  //
  if (!this->ReadLine(line))
  {
    svtkErrorMacro(<< "Premature EOF reading title! "
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
    return 0;
  }
  delete[] this->Header;
  this->Header = new char[strlen(line) + 1];
  strcpy(this->Header, line);

  svtkDebugMacro(<< "Reading svtk file entitled: " << line);
  //
  // read type
  //
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Premature EOF reading file type!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
    return 0;
  }

  if (!strncmp(this->LowerCase(line), "ascii", 5))
  {
    this->FileType = SVTK_ASCII;
  }
  else if (!strncmp(line, "binary", 6))
  {
    this->FileType = SVTK_BINARY;
  }
  else
  {
    svtkErrorMacro(<< "Unrecognized file type: " << line
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    this->FileType = 0;
    this->SetErrorCode(svtkErrorCode::UnrecognizedFileTypeError);
    return 0;
  }

  // if this is a binary file we need to make sure that we opened it
  // as a binary file.
  if (this->FileType == SVTK_BINARY && this->ReadFromInputString == 0)
  {
    svtkDebugMacro(<< "Opening svtk file as binary");
    delete this->IS;
    this->IS = nullptr;
#ifdef _WIN32
    this->IS = new svtksys::ifstream(fname, ios::in | ios::binary);
#else
    this->IS = new svtksys::ifstream(fname, ios::in);
#endif
    if (this->IS->fail())
    {
      svtkErrorMacro(<< "Unable to open file: " << fname);
      delete this->IS;
      this->IS = nullptr;
      this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
      return 0;
    }
    // read up to the same point in the file
    this->ReadLine(line);
    this->ReadLine(line);
    this->ReadString(line);
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

int svtkDataReader::IsFileValid(const char* dstype)
{
  char line[1024];

  if (!dstype)
  {
    return 0;
  }

  if (!this->OpenSVTKFile() || !this->ReadHeader())
  {
    this->CloseSVTKFile();
    return 0;
  }

  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
    return 0;
  }

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    if (!this->ReadString(line))
    {
      svtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseSVTKFile();
      this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
      return 0;
    }
    if (strncmp(this->LowerCase(line), dstype, strlen(dstype)))
    {
      this->CloseSVTKFile();
      return 0;
    }
    // everything looks good
    this->CloseSVTKFile();
    return 1;
  }

  this->CloseSVTKFile();
  return 0;
}

// Read the cell data of a svtk data file. The number of cells (from the
// dataset) must match the number of cells defined in cell attributes (unless
// no geometry was defined).
int svtkDataReader::ReadCellData(svtkDataSet* ds, svtkIdType numCells)
{
  char line[256];
  svtkDataSetAttributes* a = ds->GetCellData();

  svtkDebugMacro(<< "Reading svtk cell data");

  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
  {
    //
    // read scalar data
    //
    if (!strncmp(this->LowerCase(line), "scalars", 7))
    {
      if (!this->ReadScalarData(a, numCells))
      {
        return 0;
      }
    }
    //
    // read vector data
    //
    else if (!strncmp(line, "vectors", 7))
    {
      if (!this->ReadVectorData(a, numCells))
      {
        return 0;
      }
    }
    //
    // read 3x2 symmetric tensor data
    //
    else if (!strncmp(line, "tensors6", 8))
    {
      if (!this->ReadTensorData(a, numCells, 6))
      {
        return 0;
      }
    }
    //
    // read 3x3 tensor data
    //
    else if (!strncmp(line, "tensors", 7))
    {
      if (!this->ReadTensorData(a, numCells))
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if (!strncmp(line, "normals", 7))
    {
      if (!this->ReadNormalData(a, numCells))
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if (!strncmp(line, "texture_coordinates", 19))
    {
      if (!this->ReadTCoordsData(a, numCells))
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if (!strncmp(line, "global_ids", 10))
    {
      if (!this->ReadGlobalIds(a, numCells))
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if (!strncmp(line, "pedigree_ids", 12))
    {
      if (!this->ReadPedigreeIds(a, numCells))
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if (!strncmp(line, "color_scalars", 13))
    {
      if (!this->ReadCoScalarData(a, numCells))
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if (!strncmp(line, "lookup_table", 12))
    {
      if (!this->ReadLutData(a))
      {
        return 0;
      }
    }
    //
    // read field of data
    //
    else if (!strncmp(line, "field", 5))
    {
      svtkFieldData* f;
      if (!(f = this->ReadFieldData(CELL_DATA)))
      {
        return 0;
      }
      for (int i = 0; i < f->GetNumberOfArrays(); i++)
      {
        a->AddArray(f->GetAbstractArray(i));
      }
      f->Delete();
    }
    //
    // maybe bumped into point data
    //
    else if (!strncmp(line, "point_data", 10))
    {
      svtkIdType npts;
      if (!this->Read(&npts))
      {
        svtkErrorMacro(<< "Cannot read point data!");
        return 0;
      }

      this->ReadPointData(ds, npts);
    }

    else
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Unsupported cell attribute type: " << line
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
  }

  return 1;
}

// Read the point data of a svtk data file. The number of points (from the
// dataset) must match the number of points defined in point attributes (unless
// no geometry was defined).
int svtkDataReader::ReadPointData(svtkDataSet* ds, svtkIdType numPts)
{
  char line[256];
  svtkDataSetAttributes* a = ds->GetPointData();

  svtkDebugMacro(<< "Reading svtk point data");

  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
  {
    //
    // read scalar data
    //
    if (!strncmp(this->LowerCase(line), "scalars", 7))
    {
      if (!this->ReadScalarData(a, numPts))
      {
        return 0;
      }
    }
    //
    // read vector data
    //
    else if (!strncmp(line, "vectors", 7))
    {
      if (!this->ReadVectorData(a, numPts))
      {
        return 0;
      }
    }
    //
    // read 3x2 symmetric tensor data
    //
    else if (!strncmp(line, "tensors6", 8))
    {
      if (!this->ReadTensorData(a, numPts, 6))
      {
        return 0;
      }
    }
    //
    // read 3x3 tensor data
    //
    else if (!strncmp(line, "tensors", 7))
    {
      if (!this->ReadTensorData(a, numPts))
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if (!strncmp(line, "normals", 7))
    {

      if (!this->ReadNormalData(a, numPts))
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if (!strncmp(line, "texture_coordinates", 19))
    {
      if (!this->ReadTCoordsData(a, numPts))
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if (!strncmp(line, "global_ids", 10))
    {
      if (!this->ReadGlobalIds(a, numPts))
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if (!strncmp(line, "pedigree_ids", 12))
    {
      if (!this->ReadPedigreeIds(a, numPts))
      {
        return 0;
      }
    }
    //
    // read the edge flags data
    //
    else if (!strncmp(line, "edge_flags", 10))
    {
      if (!this->ReadEdgeFlags(a, numPts))
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if (!strncmp(line, "color_scalars", 13))
    {
      if (!this->ReadCoScalarData(a, numPts))
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if (!strncmp(line, "lookup_table", 12))
    {
      if (!this->ReadLutData(a))
      {
        return 0;
      }
    }
    //
    // read field of data
    //
    else if (!strncmp(line, "field", 5))
    {
      svtkFieldData* f;
      if (!(f = this->ReadFieldData(POINT_DATA)))
      {
        return 0;
      }
      for (int i = 0; i < f->GetNumberOfArrays(); i++)
      {
        a->AddArray(f->GetAbstractArray(i));
      }
      f->Delete();
    }
    //
    // maybe bumped into cell data
    //
    else if (!strncmp(line, "cell_data", 9))
    {
      svtkIdType ncells;
      if (!this->Read(&ncells))
      {
        svtkErrorMacro(<< "Cannot read cell data!");
        return 0;
      }

      this->ReadCellData(ds, ncells);
    }

    else
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Unsupported point attribute type: " << line
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
  }
  return 1;
}

// Read the vertex data of a svtk data file. The number of vertices (from the
// graph) must match the number of vertices defined in vertex attributes (unless
// no geometry was defined).
int svtkDataReader::ReadVertexData(svtkGraph* g, svtkIdType numVertices)
{
  char line[256];
  svtkDataSetAttributes* a = g->GetVertexData();

  svtkDebugMacro(<< "Reading svtk vertex data");

  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
  {
    //
    // read scalar data
    //
    if (!strncmp(this->LowerCase(line), "scalars", 7))
    {
      if (!this->ReadScalarData(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read vector data
    //
    else if (!strncmp(line, "vectors", 7))
    {
      if (!this->ReadVectorData(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read 3x2 symmetric tensor data
    //
    else if (!strncmp(line, "tensors6", 8))
    {
      if (!this->ReadTensorData(a, numVertices, 6))
      {
        return 0;
      }
    }
    //
    // read 3x3 tensor data
    //
    else if (!strncmp(line, "tensors", 7))
    {
      if (!this->ReadTensorData(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if (!strncmp(line, "normals", 7))
    {
      if (!this->ReadNormalData(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if (!strncmp(line, "texture_coordinates", 19))
    {
      if (!this->ReadTCoordsData(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if (!strncmp(line, "global_ids", 10))
    {
      if (!this->ReadGlobalIds(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if (!strncmp(line, "pedigree_ids", 12))
    {
      if (!this->ReadPedigreeIds(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if (!strncmp(line, "color_scalars", 13))
    {
      if (!this->ReadCoScalarData(a, numVertices))
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if (!strncmp(line, "lookup_table", 12))
    {
      if (!this->ReadLutData(a))
      {
        return 0;
      }
    }
    //
    // read field of data
    //
    else if (!strncmp(line, "field", 5))
    {
      svtkFieldData* f;
      if (!(f = this->ReadFieldData()))
      {
        return 0;
      }
      for (int i = 0; i < f->GetNumberOfArrays(); i++)
      {
        a->AddArray(f->GetAbstractArray(i));
      }
      f->Delete();
    }
    //
    // maybe bumped into edge data
    //
    else if (!strncmp(line, "edge_data", 10))
    {
      svtkIdType npts;
      if (!this->Read(&npts))
      {
        svtkErrorMacro(<< "Cannot read point data!");
        return 0;
      }

      this->ReadEdgeData(g, npts);
    }

    else
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Unsupported vertex attribute type: " << line
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
  }

  return 1;
}

// Read the edge data of a svtk data file. The number of edges (from the
// graph) must match the number of edges defined in edge attributes (unless
// no geometry was defined).
int svtkDataReader::ReadEdgeData(svtkGraph* g, svtkIdType numEdges)
{
  char line[256];
  svtkDataSetAttributes* a = g->GetEdgeData();

  svtkDebugMacro(<< "Reading svtk edge data");

  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
  {
    //
    // read scalar data
    //
    if (!strncmp(this->LowerCase(line), "scalars", 7))
    {
      if (!this->ReadScalarData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read vector data
    //
    else if (!strncmp(line, "vectors", 7))
    {
      if (!this->ReadVectorData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read 3x2 symmetric tensor data
    //
    else if (!strncmp(line, "tensors6", 8))
    {
      if (!this->ReadTensorData(a, numEdges, 6))
      {
        return 0;
      }
    }
    //
    // read 3x3 tensor data
    //
    else if (!strncmp(line, "tensors", 7))
    {
      if (!this->ReadTensorData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if (!strncmp(line, "normals", 7))
    {
      if (!this->ReadNormalData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if (!strncmp(line, "texture_coordinates", 19))
    {
      if (!this->ReadTCoordsData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if (!strncmp(line, "global_ids", 10))
    {
      if (!this->ReadGlobalIds(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if (!strncmp(line, "pedigree_ids", 12))
    {
      if (!this->ReadPedigreeIds(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if (!strncmp(line, "color_scalars", 13))
    {
      if (!this->ReadCoScalarData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if (!strncmp(line, "lookup_table", 12))
    {
      if (!this->ReadLutData(a))
      {
        return 0;
      }
    }
    //
    // read field of data
    //
    else if (!strncmp(line, "field", 5))
    {
      svtkFieldData* f;
      if (!(f = this->ReadFieldData()))
      {
        return 0;
      }
      for (int i = 0; i < f->GetNumberOfArrays(); i++)
      {
        a->AddArray(f->GetAbstractArray(i));
      }
      f->Delete();
    }
    //
    // maybe bumped into vertex data
    //
    else if (!strncmp(line, "vertex_data", 10))
    {
      svtkIdType npts;
      if (!this->Read(&npts))
      {
        svtkErrorMacro(<< "Cannot read vertex data!");
        return 0;
      }

      this->ReadVertexData(g, npts);
    }

    else
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Unsupported vertex attribute type: " << line
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
  }

  return 1;
}

// Read the row data of a svtk data file.
int svtkDataReader::ReadRowData(svtkTable* t, svtkIdType numEdges)
{
  char line[256];
  svtkDataSetAttributes* a = t->GetRowData();

  svtkDebugMacro(<< "Reading svtk row data");

  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
  {
    //
    // read scalar data
    //
    if (!strncmp(this->LowerCase(line), "scalars", 7))
    {
      if (!this->ReadScalarData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read vector data
    //
    else if (!strncmp(line, "vectors", 7))
    {
      if (!this->ReadVectorData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read 3x2 symmetric tensor data
    //
    else if (!strncmp(line, "tensors6", 8))
    {
      if (!this->ReadTensorData(a, numEdges, 6))
      {
        return 0;
      }
    }
    //
    // read 3x3 tensor data
    //
    else if (!strncmp(line, "tensors", 7))
    {
      if (!this->ReadTensorData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read normals data
    //
    else if (!strncmp(line, "normals", 7))
    {
      if (!this->ReadNormalData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read texture coordinates data
    //
    else if (!strncmp(line, "texture_coordinates", 19))
    {
      if (!this->ReadTCoordsData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read the global id data
    //
    else if (!strncmp(line, "global_ids", 10))
    {
      if (!this->ReadGlobalIds(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read the pedigree id data
    //
    else if (!strncmp(line, "pedigree_ids", 12))
    {
      if (!this->ReadPedigreeIds(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read color scalars data
    //
    else if (!strncmp(line, "color_scalars", 13))
    {
      if (!this->ReadCoScalarData(a, numEdges))
      {
        return 0;
      }
    }
    //
    // read lookup table. Associate with scalar data.
    //
    else if (!strncmp(line, "lookup_table", 12))
    {
      if (!this->ReadLutData(a))
      {
        return 0;
      }
    }
    //
    // read field of data
    //
    else if (!strncmp(line, "field", 5))
    {
      svtkFieldData* f;
      if (!(f = this->ReadFieldData()))
      {
        return 0;
      }
      for (int i = 0; i < f->GetNumberOfArrays(); i++)
      {
        a->AddArray(f->GetAbstractArray(i));
      }
      f->Delete();
    }
    else
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Unsupported row attribute type: " << line
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
  }

  return 1;
}

// General templated function to read data of various types.
template <class T>
int svtkReadBinaryData(istream* IS, T* data, svtkIdType numTuples, svtkIdType numComp)
{
  if (numTuples == 0 || numComp == 0)
  {
    // nothing to read here.
    return 1;
  }
  char line[256];

  // suck up newline
  IS->getline(line, 256);
  IS->read((char*)data, sizeof(T) * numComp * numTuples);
  if (IS->eof())
  {
    svtkGenericWarningMacro(<< "Error reading binary data!");
    return 0;
  }
  return 1;
}

// General templated function to read data of various types.
template <class T>
int svtkReadASCIIData(svtkDataReader* self, T* data, svtkIdType numTuples, svtkIdType numComp)
{
  svtkIdType i, j;

  for (i = 0; i < numTuples; i++)
  {
    for (j = 0; j < numComp; j++)
    {
      if (!self->Read(data++))
      {
        svtkGenericWarningMacro(<< "Error reading ascii data. Possible mismatch of "
                                  "datasize with declaration.");
        return 0;
      }
    }
  }
  return 1;
}

// Description:
// Read data array. Return pointer to array object if successful read;
// otherwise return nullptr. Note: this method instantiates a reference counted
// object with initial count of one; proper protocol is for you to assign
// the data object and then invoke Delete() it to restore proper reference
// count.
svtkAbstractArray* svtkDataReader::ReadArray(
  const char* dataType, svtkIdType numTuples, svtkIdType numComp)
{
  char* type = strdup(dataType);
  type = this->LowerCase(type);

  svtkAbstractArray* array;
  if (!strncmp(type, "bit", 3))
  {
    array = svtkBitArray::New();
    array->SetNumberOfComponents(numComp);
    if (numTuples != 0 && numComp != 0)
    {
      unsigned char* ptr = ((svtkBitArray*)array)->WritePointer(0, numTuples * numComp);
      if (this->FileType == SVTK_BINARY)
      {
        char line[256];
        this->IS->getline(line, 256);
        this->IS->read((char*)ptr, sizeof(unsigned char) * (numTuples * numComp + 7) / 8);
        if (this->IS->eof())
        {
          svtkErrorMacro(<< "Error reading binary bit array!");
          free(type);
          array->Delete();
          return nullptr;
        }
      }
      else
      {
        svtkIdType b;
        for (svtkIdType i = 0; i < numTuples; i++)
        {
          for (svtkIdType j = 0; j < numComp; j++)
          {
            if (!this->Read(&b))
            {
              svtkErrorMacro("Error reading ascii bit array! tuple: " << i << ", component: " << j);
              free(type);
              array->Delete();
              return nullptr;
            }
            else
            {
              ((svtkBitArray*)array)->SetValue(i * numComp + j, b);
            }
          }
        }
      }
    }
  }

  else if (!strcmp(type, "char") || !strcmp(type, "signed_char"))
  {
    array = svtkCharArray::New();
    array->SetNumberOfComponents(numComp);
    char* ptr = ((svtkCharArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "unsigned_char", 13))
  {
    array = svtkUnsignedCharArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned char* ptr = ((svtkUnsignedCharArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "short", 5))
  {
    array = svtkShortArray::New();
    array->SetNumberOfComponents(numComp);
    short* ptr = ((svtkShortArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap2BERange(ptr, numTuples * numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "unsigned_short", 14))
  {
    array = svtkUnsignedShortArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned short* ptr = ((svtkUnsignedShortArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap2BERange((short*)ptr, numTuples * numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "svtkidtype", 9))
  {
    // currently writing svtkIdType as int.
    array = svtkIdTypeArray::New();
    array->SetNumberOfComponents(numComp);
    std::vector<int> buffer(numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, buffer.data(), numTuples, numComp);
      svtkByteSwap::Swap4BERange(buffer.data(), numTuples * numComp);
    }
    else
    {
      svtkReadASCIIData(this, buffer.data(), numTuples, numComp);
    }
    svtkIdType* ptr2 = ((svtkIdTypeArray*)array)->WritePointer(0, numTuples * numComp);
    for (svtkIdType idx = 0; idx < numTuples * numComp; idx++)
    {
      ptr2[idx] = buffer[idx];
    }
  }

  else if (!strncmp(type, "int", 3))
  {
    array = svtkIntArray::New();
    array->SetNumberOfComponents(numComp);
    int* ptr = ((svtkIntArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap4BERange(ptr, numTuples * numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "unsigned_int", 12))
  {
    array = svtkUnsignedIntArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned int* ptr = ((svtkUnsignedIntArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap4BERange((int*)ptr, numTuples * numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "long", 4))
  {
    // svtkDataWriter does not write "long" data anymore
    // as data size is not certain
    // we keep this for retro compatibility
    array = svtkLongArray::New();
    array->SetNumberOfComponents(numComp);
    long* ptr = ((svtkLongArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
#if SVTK_SIZEOF_LONG == 4
      svtkByteSwap::Swap4BERange(ptr, numTuples * numComp);
#else // SVTK_SIZEOF_LONG == 8
      svtkByteSwap::Swap8BERange(ptr, numTuples * numComp);
#endif
    }

    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "unsigned_long", 13))
  {
    array = svtkUnsignedLongArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned long* ptr = ((svtkUnsignedLongArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
#if SVTK_SIZEOF_LONG == 4
      svtkByteSwap::Swap4BERange(ptr, numTuples * numComp);
#else // SVTK_SIZEOF_LONG == 8
      svtkByteSwap::Swap8BERange(ptr, numTuples * numComp);
#endif
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "svtktypeint64", 12))
  {
    array = svtkTypeInt64Array::New();
    array->SetNumberOfComponents(numComp);
    svtkTypeInt64* ptr = ((svtkTypeInt64Array*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap8BERange(ptr, numTuples * numComp);
    }

    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "svtktypeuint64", 13))
  {
    array = svtkTypeUInt64Array::New();
    array->SetNumberOfComponents(numComp);
    svtkTypeUInt64* ptr = ((svtkTypeUInt64Array*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap8BERange(ptr, numTuples * numComp);
    }

    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "float", 5))
  {
    array = svtkFloatArray::New();
    array->SetNumberOfComponents(numComp);
    float* ptr = ((svtkFloatArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap4BERange(ptr, numTuples * numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "double", 6))
  {
    array = svtkDoubleArray::New();
    array->SetNumberOfComponents(numComp);
    double* ptr = ((svtkDoubleArray*)array)->WritePointer(0, numTuples * numComp);
    if (this->FileType == SVTK_BINARY)
    {
      svtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      svtkByteSwap::Swap8BERange(ptr, numTuples * numComp);
    }
    else
    {
      svtkReadASCIIData(this, ptr, numTuples, numComp);
    }
  }

  else if (!strncmp(type, "string", 6))
  {
    array = svtkStringArray::New();
    array->SetNumberOfComponents(numComp);

    if (this->FileType == SVTK_BINARY)
    {
      // read in newline
      char line[256];
      IS->getline(line, 256);

      for (svtkIdType i = 0; i < numTuples; i++)
      {
        for (svtkIdType j = 0; j < numComp; j++)
        {
          svtkTypeUInt8 firstByte;
          svtkTypeUInt8 headerType;
          svtkStdString::size_type stringLength;
          firstByte = IS->peek();
          headerType = firstByte >> 6;
          if (headerType == 3)
          {
            svtkTypeUInt8 length = IS->get();
            length <<= 2;
            length >>= 2;
            stringLength = length;
          }
          else if (headerType == 2)
          {
            svtkTypeUInt16 length;
            IS->read(reinterpret_cast<char*>(&length), 2);
            svtkByteSwap::Swap2BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
          }
          else if (headerType == 1)
          {
            svtkTypeUInt32 length;
            IS->read(reinterpret_cast<char*>(&length), 4);
            svtkByteSwap::Swap4BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
          }
          else
          {
            svtkTypeUInt64 length;
            IS->read(reinterpret_cast<char*>(&length), 8);
            svtkByteSwap::Swap4BE(&length);
            stringLength = length;
          }
          std::vector<char> str(stringLength);
          IS->read(str.data(), stringLength);
          svtkStdString s(str.data(), stringLength);
          ((svtkStringArray*)array)->InsertNextValue(s);
        }
      }
    }
    else
    {
      // read in newline
      svtkStdString s;
      my_getline(*(this->IS), s);

      for (svtkIdType i = 0; i < numTuples; i++)
      {
        for (svtkIdType j = 0; j < numComp; j++)
        {
          my_getline(*(this->IS), s);
          int length = static_cast<int>(s.length());
          std::vector<char> decoded(length + 1);
          int decodedLength = this->DecodeString(decoded.data(), s.c_str());
          svtkStdString decodedStr(decoded.data(), decodedLength);
          ((svtkStringArray*)array)->InsertNextValue(decodedStr);
        }
      }
    }
  }
  else if (!strncmp(type, "utf8_string", 11))
  {
    array = svtkUnicodeStringArray::New();
    array->SetNumberOfComponents(numComp);

    if (this->FileType == SVTK_BINARY)
    {
      // read in newline
      char line[256];
      IS->getline(line, 256);

      for (svtkIdType i = 0; i < numTuples; i++)
      {
        for (svtkIdType j = 0; j < numComp; j++)
        {
          svtkTypeUInt8 firstByte;
          svtkTypeUInt8 headerType;
          svtkStdString::size_type stringLength;
          firstByte = IS->peek();
          headerType = firstByte >> 6;
          if (headerType == 3)
          {
            svtkTypeUInt8 length = IS->get();
            length <<= 2;
            length >>= 2;
            stringLength = length;
          }
          else if (headerType == 2)
          {
            svtkTypeUInt16 length;
            IS->read(reinterpret_cast<char*>(&length), 2);
            svtkByteSwap::Swap2BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
          }
          else if (headerType == 1)
          {
            svtkTypeUInt32 length;
            IS->read(reinterpret_cast<char*>(&length), 4);
            svtkByteSwap::Swap4BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
          }
          else
          {
            svtkTypeUInt64 length;
            IS->read(reinterpret_cast<char*>(&length), 8);
            svtkByteSwap::Swap4BE(&length);
            stringLength = length;
          }
          std::vector<char> str(stringLength);
          IS->read(str.data(), stringLength);
          svtkUnicodeString s = svtkUnicodeString::from_utf8(str.data(), str.data() + stringLength);
          ((svtkUnicodeStringArray*)array)->InsertNextValue(s);
        }
      }
    }
    else
    {
      // read in newline
      svtkStdString s;
      my_getline(*(this->IS), s);

      for (svtkIdType i = 0; i < numTuples; i++)
      {
        for (svtkIdType j = 0; j < numComp; j++)
        {
          my_getline(*(this->IS), s);
          int length = static_cast<int>(s.length());
          std::vector<char> decoded(length + 1);
          int decodedLength = this->DecodeString(decoded.data(), s.c_str());
          svtkUnicodeString decodedStr =
            svtkUnicodeString::from_utf8(decoded.data(), decoded.data() + decodedLength);
          ((svtkUnicodeStringArray*)array)->InsertNextValue(decodedStr);
        }
      }
    }
  }
  else if (!strncmp(type, "variant", 7))
  {
    array = svtkVariantArray::New();
    array->SetNumberOfComponents(numComp);
    for (svtkIdType i = 0; i < numTuples; i++)
    {
      for (svtkIdType j = 0; j < numComp; j++)
      {
        int t;
        svtkStdString str;
        *(this->IS) >> t >> str;
        std::vector<char> decoded(str.length() + 1);
        int decodedLength = this->DecodeString(decoded.data(), str.c_str());
        svtkStdString decodedStr(decoded.data(), decodedLength);
        svtkVariant sv(decodedStr);
        svtkVariant v;
        switch (t)
        {
          case SVTK_CHAR:
            v = sv.ToChar();
            break;
          case SVTK_SIGNED_CHAR:
            v = sv.ToSignedChar();
            break;
          case SVTK_UNSIGNED_CHAR:
            v = sv.ToUnsignedChar();
            break;
          case SVTK_SHORT:
            v = sv.ToShort();
            break;
          case SVTK_UNSIGNED_SHORT:
            v = sv.ToUnsignedShort();
            break;
          case SVTK_INT:
            v = sv.ToInt();
            break;
          case SVTK_UNSIGNED_INT:
            v = sv.ToUnsignedInt();
            break;
          case SVTK_LONG:
            v = sv.ToLong();
            break;
          case SVTK_UNSIGNED_LONG:
            v = sv.ToUnsignedLong();
            break;
          case SVTK_FLOAT:
            v = sv.ToFloat();
            break;
          case SVTK_DOUBLE:
            v = sv.ToDouble();
            break;
          case SVTK_LONG_LONG:
            v = sv.ToLongLong();
            break;
          case SVTK_UNSIGNED_LONG_LONG:
            v = sv.ToUnsignedLongLong();
            break;
          case SVTK_STRING:
            v = sv.ToString();
            break;
          default:
            svtkErrorMacro("Unknown variant type " << t);
        }
        ((svtkVariantArray*)array)->InsertNextValue(v);
      }
    }
  }
  else
  {
    svtkErrorMacro(<< "Unsupported data type: " << type);
    free(type);
    return nullptr;
  }

  free(type);

  // Pop off any blank lines, these get added occasionally by the writer
  // when the data is a certain length:
  bool readyToCheckMetaData = false;
  char line[256];
  size_t peekSize = this->Peek(line, 256);
  bool hasNewData = false;
  do
  {
    hasNewData = false;
    // Strip leading whitespace, check for newlines:
    for (size_t i = 0; i < peekSize; ++i)
    {
      switch (line[i])
      {
        case ' ':
          continue;
        case '\r':
          continue;
        case '\n':
          // pop line, peek at next
          if (!this->ReadLine(line))
          {
            return array;
          }
          peekSize = this->Peek(line, 256);
          hasNewData = true;
          i = peekSize;      // Break outer loop
          if (peekSize == 0) // EOF
          {
            return array;
          }
          break;
        default: // Made it past all of the whitespace.
          readyToCheckMetaData = true;
          i = peekSize; // Break outer loop
          break;
      }
    }
  }
  // The only time peekSize will be less than 256 (the requested size)
  // is if the end of file was reached or an error occurred.  Break this
  // outermost loop if we are past the whitespace (readyToCheckMetaData)
  // or the peekSize hit the end of file but wasn't read in during this
  // time through the loop.  This handles the case of files that end in
  // whitespace without a trailing newline.
  while (!readyToCheckMetaData && (peekSize == 256 || hasNewData));

  // Peek at the next line to see if there's any array metadata:
  if (this->Peek(line, 8) < 8) // looking for "metadata"
  {
    return array; // EOF?
  }

  this->LowerCase(&line[0], 8);
  if (strncmp(line, "metadata", 8) != 0)
  { // No metadata
    return array;
  }

  // Pop off the meta data line:
  if (!this->ReadLine(line))
  {
    return array;
  }
  this->LowerCase(line, 256);
  assert("sanity check" && strncmp(line, "metadata", 8) == 0);

  while (this->ReadLine(line))
  {
    this->LowerCase(line, 256);

    // Blank line indicates end of metadata:
    if (strlen(line) == 0)
    {
      break;
    }

    if (strncmp(line, "component_names", 15) == 0)
    {
      char decoded[256];
      for (int i = 0; i < numComp; ++i)
      {
        if (!this->ReadLine(line))
        {
          svtkErrorMacro(
            "Error reading component name " << i << " for array '" << array->GetName() << "'.");
          continue;
        }

        this->DecodeString(decoded, line);
        array->SetComponentName(static_cast<svtkIdType>(i), decoded);
      }
      continue;
    }

    if (strncmp(line, "information", 11) == 0)
    {
      int numKeys;
      if (sscanf(line, "information %d", &numKeys) != 1)
      {
        svtkWarningMacro("Invalid information header: " << line);
        continue;
      }

      svtkInformation* info = array->GetInformation();
      this->ReadInformation(info, numKeys);
      continue;
    }
  }

  return array;
}

// Read point coordinates. Return 0 if error.
int svtkDataReader::ReadPointCoordinates(svtkPointSet* ps, svtkIdType numPts)
{
  char line[256];
  svtkDataArray* data;

  if (!this->ReadString(line))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read points type!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, 3));
  if (data != nullptr)
  {
    svtkPoints* points = svtkPoints::New();
    points->SetData(data);
    data->Delete();
    ps->SetPoints(points);
    points->Delete();
  }
  else
  {
    return 0;
  }

  svtkDebugMacro(<< "Read " << ps->GetNumberOfPoints() << " points");
  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read point coordinates. Return 0 if error.
int svtkDataReader::ReadPointCoordinates(svtkGraph* g, svtkIdType numPts)
{
  char line[256];
  svtkDataArray* data;

  if (!this->ReadString(line))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read points type!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, 3));
  if (data != nullptr)
  {
    svtkPoints* points = svtkPoints::New();
    points->SetData(data);
    data->Delete();
    g->SetPoints(points);
    points->Delete();
  }
  else
  {
    return 0;
  }

  svtkDebugMacro(<< "Read " << g->GetNumberOfVertices() << " points");
  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read the coordinates for a rectilinear grid. The axes parameter specifies
// which coordinate axes (0,1,2) is being read.
int svtkDataReader::ReadCoordinates(svtkRectilinearGrid* rg, int axes, int numCoords)
{
  char line[256];
  svtkDataArray* data;

  if (!this->ReadString(line))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read coordinates type!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numCoords, 1));
  if (!data)
  {
    return 0;
  }

  if (axes == 0)
  {
    rg->SetXCoordinates(data);
  }
  else if (axes == 1)
  {
    rg->SetYCoordinates(data);
  }
  else
  {
    rg->SetZCoordinates(data);
  }

  svtkDebugMacro(<< "Read " << data->GetNumberOfTuples() << " coordinates");
  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  data->Delete();

  return 1;
}

// Read scalar point attributes. Return 0 if error.
int svtkDataReader::ReadScalarData(svtkDataSetAttributes* a, svtkIdType numPts)
{
  char line[256], name[256], key[256], tableName[256];
  int skipScalar = 0;
  svtkDataArray* data;
  int numComp = 1;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  this->DecodeString(name, buffer);

  if (!this->ReadString(key))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  // the next string could be an integer number of components or a lookup table
  if (strcmp(this->LowerCase(key), "lookup_table"))
  {
    numComp = atoi(key);
    if (numComp < 1 || !this->ReadString(key))
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Cannot read scalar header!"
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
  }

  if (strcmp(this->LowerCase(key), "lookup_table"))
  {
    svtkErrorMacro(<< "Lookup table must be specified with scalar.\n"
                  << "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
  }

  if (!this->ReadString(tableName))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read scalar header!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  // See whether scalar has been already read or scalar name (if specified)
  // matches name in file.
  //
  if (a->GetScalars() != nullptr || (this->ScalarsName && strcmp(name, this->ScalarsName)))
  {
    skipScalar = 1;
  }
  else
  {
    this->SetScalarLut(tableName); // may be "default"
  }

  // Read the data
  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, numComp));
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipScalar)
    {
      a->SetScalars(data);
    }
    else if (this->ReadAllScalars)
    {
      a->AddArray(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read vector point attributes. Return 0 if error.
int svtkDataReader::ReadVectorData(svtkDataSetAttributes* a, svtkIdType numPts)
{
  int skipVector = 0;
  char line[256], name[256];
  svtkDataArray* data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read vector data!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);

  //
  // See whether vector has been already read or vector name (if specified)
  // matches name in file.
  //
  if (a->GetVectors() != nullptr || (this->VectorsName && strcmp(name, this->VectorsName)))
  {
    skipVector = 1;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, 3));
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipVector)
    {
      a->SetVectors(data);
    }
    else if (this->ReadAllVectors)
    {
      a->AddArray(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read normal point attributes. Return 0 if error.
int svtkDataReader::ReadNormalData(svtkDataSetAttributes* a, svtkIdType numPts)
{
  int skipNormal = 0;
  char line[256], name[256];
  svtkDataArray* data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read normal data!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);

  //
  // See whether normal has been already read or normal name (if specified)
  // matches name in file.
  //
  if (a->GetNormals() != nullptr || (this->NormalsName && strcmp(name, this->NormalsName)))
  {
    skipNormal = 1;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, 3));
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipNormal)
    {
      a->SetNormals(data);
    }
    else if (this->ReadAllNormals)
    {
      a->AddArray(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read tensor point attributes. Return 0 if error.
int svtkDataReader::ReadTensorData(svtkDataSetAttributes* a, svtkIdType numPts, svtkIdType numComp)
{
  int skipTensor = 0;
  char line[256], name[256];
  svtkDataArray* data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read tensor data!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);
  //
  // See whether tensor has been already read or tensor name (if specified)
  // matches name in file.
  //
  if (a->GetTensors() != nullptr || (this->TensorsName && strcmp(name, this->TensorsName)))
  {
    skipTensor = 1;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, numComp));
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipTensor)
    {
      a->SetTensors(data);
    }
    else if (this->ReadAllTensors)
    {
      a->AddArray(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read color scalar point attributes. Return 0 if error.
int svtkDataReader::ReadCoScalarData(svtkDataSetAttributes* a, svtkIdType numPts)
{
  int i, j, idx, numComp = 0, skipScalar = 0;
  char name[256];
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->Read(&numComp)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read color scalar data!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);
  //
  // See whether scalar has been already read or scalar name (if specified)
  // matches name in file.
  //
  if (a->GetScalars() != nullptr || (this->ScalarsName && strcmp(name, this->ScalarsName)))
  {
    skipScalar = 1;
  }

  // handle binary different from ASCII since they are stored
  // in a different format float versus uchar
  if (this->FileType == SVTK_BINARY)
  {
    svtkUnsignedCharArray* data;
    char type[14] = "unsigned_char";
    data = (svtkUnsignedCharArray*)this->ReadArray(type, numPts, numComp);

    if (data != nullptr)
    {
      data->SetName(name);
      if (!skipScalar)
      {
        a->SetScalars(data);
      }
      else if (this->ReadAllColorScalars)
      {
        a->AddArray(data);
      }
      data->Delete();
    }
    else
    {
      return 0;
    }
  }
  else
  {
    svtkFloatArray* data;
    char type[6] = "float";
    data = (svtkFloatArray*)this->ReadArray(type, numPts, numComp);

    if (data != nullptr)
    {
      if (!skipScalar || this->ReadAllColorScalars)
      {
        svtkUnsignedCharArray* scalars = svtkUnsignedCharArray::New();
        scalars->SetNumberOfComponents(numComp);
        scalars->SetNumberOfTuples(numPts);
        scalars->SetName(name);
        for (i = 0; i < numPts; i++)
        {
          for (j = 0; j < numComp; j++)
          {
            idx = i * numComp + j;
            scalars->SetValue(idx, (unsigned char)(255.0 * data->GetValue(idx) + 0.5));
          }
        }
        if (!skipScalar)
        {
          a->SetScalars(scalars);
        }
        else if (this->ReadAllColorScalars)
        {
          a->AddArray(scalars);
        }
        scalars->Delete();
      }
      data->Delete();
    }
    else
    {
      return 0;
    }
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read texture coordinates point attributes. Return 0 if error.
int svtkDataReader::ReadTCoordsData(svtkDataSetAttributes* a, svtkIdType numPts)
{
  int dim = 0;
  int skipTCoord = 0;
  char line[256], name[256];
  svtkDataArray* data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->Read(&dim) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read texture data!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);

  if (dim < 1 || dim > 3)
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Unsupported texture coordinates dimension: " << dim
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  //
  // See whether texture coords have been already read or texture coords name
  // (if specified) matches name in file.
  //
  if (a->GetTCoords() != nullptr || (this->TCoordsName && strcmp(name, this->TCoordsName)))
  {
    skipTCoord = 1;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, dim));
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipTCoord)
    {
      a->SetTCoords(data);
    }
    else if (this->ReadAllTCoords)
    {
      a->AddArray(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read texture coordinates point attributes. Return 0 if error.
int svtkDataReader::ReadGlobalIds(svtkDataSetAttributes* a, svtkIdType numPts)
{
  int skipGlobalIds = 0;
  char line[256], name[256];
  svtkDataArray* data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read global id data"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);

  //
  // See whether global ids have been already read
  //
  if (a->GetGlobalIds() != nullptr)
  {
    skipGlobalIds = 1;
  }

  data = svtkArrayDownCast<svtkDataArray>(this->ReadArray(line, numPts, 1));
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipGlobalIds)
    {
      a->SetGlobalIds(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read pedigree ids. Return 0 if error.
int svtkDataReader::ReadPedigreeIds(svtkDataSetAttributes* a, svtkIdType numPts)
{
  int skipPedigreeIds = 0;
  char line[256], name[256];
  svtkAbstractArray* data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read global id data"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);

  //
  // See whether pedigree ids have been already read
  //
  if (a->GetPedigreeIds() != nullptr)
  {
    skipPedigreeIds = 1;
  }

  data = this->ReadArray(line, numPts, 1);
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipPedigreeIds)
    {
      a->SetPedigreeIds(data);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

// Read edge flags. Return 0 if error.
int svtkDataReader::ReadEdgeFlags(svtkDataSetAttributes* a, svtkIdType numPts)
{
  int skipEdgeFlags = 0;
  char line[256], name[256];
  svtkAbstractArray* data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read edge flags data"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }
  this->DecodeString(name, buffer);

  //
  // See whether edge flags have been already read
  //
  if (a->GetAttribute(svtkDataSetAttributes::EDGEFLAG) != nullptr)
  {
    skipEdgeFlags = 1;
  }

  data = this->ReadArray(line, numPts, 1);
  if (data != nullptr)
  {
    data->SetName(name);
    if (!skipEdgeFlags)
    {
      a->SetAttribute(data, svtkDataSetAttributes::EDGEFLAG);
    }
    data->Delete();
  }
  else
  {
    return 0;
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

int svtkDataReader::ReadInformation(svtkInformation* info, svtkIdType numKeys)
{
  // Assuming that the opening INFORMATION line has been read.
  char line[256];
  char name[256];
  char location[256];
  for (int keyIdx = 0; keyIdx < numKeys; ++keyIdx)
  {
    do
    {
      if (!this->ReadLine(line))
      {
        svtkErrorMacro("Unexpected EOF while parsing INFORMATION section.");
        return 0;
      }
    } while (strlen(line) == 0); // Skip empty lines

    if (strncmp("NAME ", line, 5) == 0)
    { // New key
      if (sscanf(line, "NAME %s LOCATION %s", name, location) != 2)
      {
        svtkWarningMacro("Invalid line in information specification: " << line);
        continue;
      }

      svtkInformationKey* key = svtkInformationKeyLookup::Find(name, location);
      if (!key)
      {
        svtkWarningMacro("Could not locate key "
          << location << "::" << name << ". Is the module in which it is defined linked?");
        continue;
      }

      svtkInformationDoubleKey* dKey = nullptr;
      svtkInformationDoubleVectorKey* dvKey = nullptr;
      svtkInformationIdTypeKey* idKey = nullptr;
      svtkInformationIntegerKey* iKey = nullptr;
      svtkInformationIntegerVectorKey* ivKey = nullptr;
      svtkInformationStringKey* sKey = nullptr;
      svtkInformationStringVectorKey* svKey = nullptr;
      svtkInformationUnsignedLongKey* ulKey = nullptr;
      if ((dKey = svtkInformationDoubleKey::SafeDownCast(key)))
      {
        double value;
        if (!this->ReadString(line) || strncmp("DATA", line, 4) != 0 || !this->Read(&value))
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        // Pop off the trailing newline:
        this->ReadLine(line);

        info->Set(dKey, value);
        continue;
      }
      else if ((dvKey = svtkInformationDoubleVectorKey::SafeDownCast(key)))
      {
        int length;
        if (!this->ReadString(line) || strncmp("DATA", line, 4) != 0 || !this->Read(&length))
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        if (length == 0)
        {
          info->Set(dvKey, nullptr, 0);
          continue;
        }

        std::vector<double> values;
        values.reserve(length);
        for (int i = 0; i < length; ++i)
        {
          double value;
          if (!this->Read(&value))
          {
            svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
            break;
          }
          values.push_back(value);
        }
        if (values.size() == static_cast<size_t>(length))
        {
          info->Set(dvKey, &values[0], length);
        }

        // Pop off the trailing newline:
        this->ReadLine(line);

        continue;
      }
      else if ((idKey = svtkInformationIdTypeKey::SafeDownCast(key)))
      {
        svtkIdType value;
        if (!this->ReadString(line) || strncmp("DATA", line, 4) != 0 || !this->Read(&value))
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        // Pop off the trailing newline:
        this->ReadLine(line);

        info->Set(idKey, value);
        continue;
      }
      else if ((iKey = svtkInformationIntegerKey::SafeDownCast(key)))
      {
        int value;
        if (!this->ReadString(line) || strncmp("DATA", line, 4) != 0 || !this->Read(&value))
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        // Pop off the trailing newline:
        this->ReadLine(line);

        info->Set(iKey, value);
        continue;
      }
      else if ((ivKey = svtkInformationIntegerVectorKey::SafeDownCast(key)))
      {
        int length;
        if (!this->ReadString(line) || strncmp("DATA", line, 4) != 0 || !this->Read(&length))
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        if (length == 0)
        {
          info->Set(ivKey, nullptr, 0);
          continue;
        }

        std::vector<int> values;
        values.reserve(length);
        for (int i = 0; i < length; ++i)
        {
          int value;
          if (!this->Read(&value))
          {
            svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
            break;
          }
          values.push_back(value);
        }
        if (values.size() == static_cast<size_t>(length))
        {
          info->Set(ivKey, &values[0], length);
        }

        // Pop off the trailing newline:
        this->ReadLine(line);

        continue;
      }
      else if ((sKey = svtkInformationStringKey::SafeDownCast(key)))
      {
        if (!this->ReadLine(line))
        {
          svtkWarningMacro("Unexpected EOF while parsing key " << location << "::" << name << ".");
          continue;
        }

        char value[256];
        if (sscanf(line, "DATA %s", value) != 1)
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        char decoded[256];
        this->DecodeString(decoded, value);

        info->Set(sKey, decoded);
      }
      else if ((svKey = svtkInformationStringVectorKey::SafeDownCast(key)))
      {
        int length;
        if (!this->ReadString(line) || strncmp("DATA", line, 4) != 0 || !this->Read(&length))
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        // Pop off the trailing newline:
        this->ReadLine(line);

        if (length == 0)
        {
          info->Set(svKey, nullptr, 0);
          continue;
        }

        // Just build the string vector information by appending, as these
        // don't really implement the RequiredLength feature like the other
        // vector keys.
        bool success = true;
        for (int i = 0; i < length; ++i)
        {
          char value[256];
          if (!this->ReadLine(value))
          {
            svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
            success = false;
            break;
          }

          char decoded[256];
          this->DecodeString(decoded, value);

          info->Append(svKey, decoded);
        }
        if (!success)
        {
          info->Remove(svKey);
        }
        continue;
      }
      else if ((ulKey = svtkInformationUnsignedLongKey::SafeDownCast(key)))
      {
        unsigned long value;
        if (!this->ReadString(line) || strncmp("DATA", line, 4) != 0 || !this->Read(&value))
        {
          svtkWarningMacro("Malformed data block for key " << location << "::" << name << ".");
          continue;
        }

        // Pop off the trailing newline:
        this->ReadLine(line);

        info->Set(ulKey, value);
        continue;
      }
      else
      {
        svtkWarningMacro("Could not deserialize information with key "
          << key->GetLocation() << "::" << key->GetName()
          << ": "
             "key type '"
          << key->GetClassName() << "' is not serializable.");
        continue;
      }
    }
    else
    {
      svtkWarningMacro("Ignoring line in INFORMATION block: " << line);
    }
  }

  return 1;
}

// Read lookup table. Return 0 if error.
int svtkDataReader::ReadLutData(svtkDataSetAttributes* a)
{
  int i;
  int size = 0, skipTable = 0;
  svtkLookupTable* lut;
  unsigned char* ptr;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->Read(&size)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read lookup table data!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return 0;
  }

  if (a->GetScalars() == nullptr ||
    (this->LookupTableName && strcmp(name, this->LookupTableName)) ||
    (this->ScalarLut && strcmp(name, this->ScalarLut)))
  {
    skipTable = 1;
  }

  lut = svtkLookupTable::New();
  lut->Allocate(size);
  ptr = lut->WritePointer(0, size);

  if (this->FileType == SVTK_BINARY)
  {
    // suck up newline
    this->IS->getline(line, 256);
    this->IS->read((char*)ptr, sizeof(unsigned char) * 4 * size);
    if (this->IS->eof())
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Error reading binary lookup table!"
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
  }
  else // ascii
  {
    float rgba[4];
    for (i = 0; i < size; i++)
    {
      if (!(this->Read(rgba) && this->Read(rgba + 1) && this->Read(rgba + 2) &&
            this->Read(rgba + 3)))
      {
        const char* fname = this->CurrentFileName.c_str();
        svtkErrorMacro(<< "Error reading lookup table!"
                      << " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
      }
      lut->SetTableValue(i, rgba[0], rgba[1], rgba[2], rgba[3]);
    }
  }

  if (!skipTable)
  {
    a->GetScalars()->SetLookupTable(lut);
  }
  lut->Delete();

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

int svtkDataReader::ReadCells(svtkSmartPointer<svtkCellArray>& cellArray)
{
  svtkIdType offsetsSize{ 0 };
  svtkIdType connSize{ 0 };
  char buffer[256];

  if (!(this->Read(&offsetsSize) && this->Read(&connSize)))
  {
    svtkErrorMacro("Error while reading cell array header.");
    this->CloseSVTKFile();
    return 0;
  }

  if (offsetsSize < 1)
  {
    cellArray = svtkSmartPointer<svtkCellArray>::New();
    return 1;
  }

  if (!this->ReadString(buffer) || // "offsets"
    (strcmp(this->LowerCase(buffer, 256), "offsets") != 0) || !this->ReadString(buffer)) // datatype
  {
    svtkErrorMacro("Error reading cell array offset header.");
    this->CloseSVTKFile();
    return 0;
  }

  this->LowerCase(buffer, 256);

  auto offsets = svtk::TakeSmartPointer(this->ReadArray(buffer, offsetsSize, 1));
  if (!offsets)
  {
    svtkErrorMacro("Error reading cell array offset data.");
    this->CloseSVTKFile();
    return 0;
  }

  if (!this->ReadString(buffer) || // "connectivity"
    (strcmp(this->LowerCase(buffer, 256), "connectivity") != 0) ||
    !this->ReadString(buffer)) // datatype
  {
    svtkErrorMacro("Error reading cell array connectivity header.");
    this->CloseSVTKFile();
    return 0;
  }

  this->LowerCase(buffer, 256);

  auto conn = svtk::TakeSmartPointer(this->ReadArray(buffer, connSize, 1));
  if (!conn)
  {
    svtkErrorMacro("Error reading cell array connectivity data.");
    this->CloseSVTKFile();
    return 0;
  }

  // Check that they are the indicated types and add them to the cell array:

  auto* offDA = svtkArrayDownCast<svtkDataArray>(offsets.Get());
  auto* connDA = svtkArrayDownCast<svtkDataArray>(conn.Get());
  if (!offDA || !connDA)
  {
    svtkErrorMacro("Offsets and connectivity arrays must subclass svtkDataArray.");
    this->CloseSVTKFile();
    return 0;
  }

  cellArray = svtkSmartPointer<svtkCellArray>::New();
  bool succ = cellArray->SetData(offDA, connDA);

  if (!succ)
  { // SetData logs error for us
    cellArray = nullptr;
    this->CloseSVTKFile();
    return 0;
  }

  return 1;
}

// Read lookup table. Return 0 if error.
int svtkDataReader::ReadCellsLegacy(svtkIdType size, int* data)
{
  char line[256];
  int i;

  if (this->FileType == SVTK_BINARY)
  {
    // suck up newline
    this->IS->getline(line, 256);
    this->IS->read((char*)data, sizeof(int) * size);
    if (this->IS->eof())
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Error reading binary cell data!"
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
    svtkByteSwap::Swap4BERange(data, size);
  }
  else // ascii
  {
    for (i = 0; i < size; i++)
    {
      if (!this->Read(data + i))
      {
        const char* fname = this->CurrentFileName.c_str();
        svtkErrorMacro(<< "Error reading ascii cell data!"
                      << " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
      }
    }
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

int svtkDataReader::ReadCellsLegacy(svtkIdType size, int* data, int skip1, int read2, int skip3)
{
  char line[256];
  int i, numCellPts, junk, *tmp, *pTmp;

  std::vector<int> tmpStorage;

  if (this->FileType == SVTK_BINARY)
  {
    // suck up newline
    this->IS->getline(line, 256);
    // first read all the cells as one chunk (each cell has different length).
    if (skip1 == 0 && skip3 == 0)
    {
      tmp = data;
    }
    else
    {
      tmpStorage.resize(size);
      tmp = tmpStorage.data();
    }
    this->IS->read((char*)tmp, sizeof(int) * size);
    if (this->IS->eof())
    {
      const char* fname = this->CurrentFileName.c_str();
      svtkErrorMacro(<< "Error reading binary cell data!"
                    << " for file: " << (fname ? fname : "(Null FileName)"));
      return 0;
    }
    svtkByteSwap::Swap4BERange(tmp, size);
    if (tmp == data)
    {
      return 1;
    }
    // skip cells before the piece
    pTmp = tmp;
    while (skip1 > 0)
    {
      // the first value is the number of point ids
      // skip these plus one for the number itself.
      pTmp += *pTmp + 1;
      --skip1;
    }
    // copy the cells in the piece
    // (ok, I am getting criptic with the loops and increments ...)
    while (read2 > 0)
    {
      // the first value is the number of point ids
      *data++ = i = *pTmp++;
      while (i-- > 0)
      {
        *data++ = *pTmp++;
      }
      --read2;
    }
  }
  else // ascii
  {
    // skip cells before the piece
    for (i = 0; i < skip1; i++)
    {
      if (!this->Read(&numCellPts))
      {
        const char* fname = this->CurrentFileName.c_str();
        svtkErrorMacro(<< "Error reading ascii cell data!"
                      << " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
      }
      while (numCellPts-- > 0)
      {
        this->Read(&junk);
      }
    }
    // read the cells in the piece
    for (i = 0; i < read2; i++)
    {
      if (!this->Read(data))
      {
        const char* fname = this->CurrentFileName.c_str();
        svtkErrorMacro(<< "Error reading ascii cell data!"
                      << " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
      }
      numCellPts = *data++;
      while (numCellPts-- > 0)
      {
        this->Read(data++);
      }
    }
    // skip cells after the piece
    for (i = 0; i < skip3; i++)
    {
      if (!this->Read(&numCellPts))
      {
        const char* fname = this->CurrentFileName.c_str();
        svtkErrorMacro(<< "Error reading ascii cell data!"
                      << " for file: " << (fname ? fname : "(Null FileName)"));
        return 0;
      }
      while (numCellPts-- > 0)
      {
        this->Read(&junk);
      }
    }
  }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5 * (1.0 - progress));

  return 1;
}

void svtkDataReader::ConvertGhostLevelsToGhostType(FieldType fieldType, svtkAbstractArray* data) const
{
  svtkUnsignedCharArray* ucData = svtkArrayDownCast<svtkUnsignedCharArray>(data);
  const char* name = data->GetName();
  int numComp = data->GetNumberOfComponents();
  if (this->FileMajorVersion < 4 && ucData && numComp == 1 &&
    (fieldType == CELL_DATA || fieldType == POINT_DATA) && !strcmp(name, "svtkGhostLevels"))
  {
    // convert ghost levels to ghost type
    unsigned char* ghosts = ucData->GetPointer(0);
    // only CELL_DATA or POINT_DATA are possible at this point.
    unsigned char newValue = svtkDataSetAttributes::DUPLICATEPOINT;
    if (fieldType == CELL_DATA)
    {
      newValue = svtkDataSetAttributes::DUPLICATECELL;
    }
    svtkIdType numTuples = ucData->GetNumberOfTuples();
    for (int i = 0; i < numTuples; ++i)
    {
      if (ghosts[i] > 0)
      {
        ghosts[i] = newValue;
      }
    }
    data->SetName(svtkDataSetAttributes::GhostArrayName());
  }
}

svtkFieldData* svtkDataReader::ReadFieldData(FieldType fieldType)
{
  int i, numArrays = 0, skipField = 0;
  svtkFieldData* f;
  char name[256], type[256];
  svtkIdType numComp, numTuples;
  svtkAbstractArray* data;

  if (!(this->ReadString(name) && this->Read(&numArrays)))
  {
    const char* fname = this->CurrentFileName.c_str();
    svtkErrorMacro(<< "Cannot read field header!"
                  << " for file: " << (fname ? fname : "(Null FileName)"));
    return nullptr;
  }

  // See whether field data name (if specified)
  if ((this->FieldDataName && strcmp(name, this->FieldDataName)))
  {
    skipField = 1;
  }

  f = svtkFieldData::New();
  f->AllocateArrays(numArrays);

  // Read the number of arrays specified
  for (i = 0; i < numArrays; i++)
  {
    char buffer[1024];
    this->ReadString(buffer);
    if (strcmp(buffer, "NULL_ARRAY") == 0)
    {
      continue;
    }
    this->DecodeString(name, buffer);
    this->Read(&numComp);
    this->Read(&numTuples);
    this->ReadString(type);
    data = this->ReadArray(type, numTuples, numComp);
    if (data != nullptr)
    {
      if (!skipField || this->ReadAllFields)
      {
        data->SetName(name);
        this->ConvertGhostLevelsToGhostType(fieldType, data);
        f->AddArray(data);
      }
      data->Delete();
    }
    else
    {
      f->Delete();
      return nullptr;
    }
  }

  if (skipField && !this->ReadAllFields)
  {
    f->Delete();
    return nullptr;
  }
  else
  {
    return f;
  }
}

char* svtkDataReader::LowerCase(char* str, const size_t len)
{
  size_t i;
  char* s;

  for (i = 0, s = str; *s != '\0' && i < len; s++, i++)
  {
    *s = tolower(*s);
  }
  return str;
}

// Close a svtk file.
void svtkDataReader::CloseSVTKFile()
{
  svtkDebugMacro(<< "Closing svtk file");

  // Restore the previous locale settings
  std::locale::global(this->CurrentLocale);

  delete this->IS;
  this->IS = nullptr;
}

void svtkDataReader::InitializeCharacteristics()
{
  int i;

  // Release any old stuff first
  if (this->ScalarsNameInFile)
  {
    for (i = 0; i < this->NumberOfScalarsInFile; i++)
    {
      delete[] this->ScalarsNameInFile[i];
    }
    this->NumberOfScalarsInFile = 0;
    delete[] this->ScalarsNameInFile;
    this->ScalarsNameInFile = nullptr;
  }

  if (this->VectorsNameInFile)
  {
    for (i = 0; i < this->NumberOfVectorsInFile; i++)
    {
      delete[] this->VectorsNameInFile[i];
    }
    this->NumberOfVectorsInFile = 0;
    delete[] this->VectorsNameInFile;
    this->VectorsNameInFile = nullptr;
  }

  if (this->TensorsNameInFile)
  {
    for (i = 0; i < this->NumberOfTensorsInFile; i++)
    {
      delete[] this->TensorsNameInFile[i];
    }
    this->NumberOfTensorsInFile = 0;
    delete[] this->TensorsNameInFile;
    this->TensorsNameInFile = nullptr;
  }

  if (this->NormalsNameInFile)
  {
    for (i = 0; i < this->NumberOfNormalsInFile; i++)
    {
      delete[] this->NormalsNameInFile[i];
    }
    this->NumberOfNormalsInFile = 0;
    delete[] this->NormalsNameInFile;
    this->NormalsNameInFile = nullptr;
  }

  if (this->TCoordsNameInFile)
  {
    for (i = 0; i < this->NumberOfTCoordsInFile; i++)
    {
      delete[] this->TCoordsNameInFile[i];
    }
    this->NumberOfTCoordsInFile = 0;
    delete[] this->TCoordsNameInFile;
    this->TCoordsNameInFile = nullptr;
  }

  if (this->FieldDataNameInFile)
  {
    for (i = 0; i < this->NumberOfFieldDataInFile; i++)
    {
      delete[] this->FieldDataNameInFile[i];
    }
    this->NumberOfFieldDataInFile = 0;
    delete[] this->FieldDataNameInFile;
    this->FieldDataNameInFile = nullptr;
  }
}

// read entire file, storing important characteristics
int svtkDataReader::CharacterizeFile()
{
  if (this->CharacteristicsTime > this->MTime)
  {
    return 1;
  }

  this->InitializeCharacteristics();
  this->CharacteristicsTime.Modified();

  // Open the file
  if (!this->OpenSVTKFile() || !this->ReadHeader())
  {
    this->CloseSVTKFile();
    return 0;
  }

  char line[256];
  while (this->ReadLine(line))
  {
    this->CheckFor("scalars", line, this->NumberOfScalarsInFile, this->ScalarsNameInFile,
      this->ScalarsNameAllocSize);
    this->CheckFor("vectors", line, this->NumberOfVectorsInFile, this->VectorsNameInFile,
      this->VectorsNameAllocSize);
    this->CheckFor("tensors", line, this->NumberOfTensorsInFile, this->TensorsNameInFile,
      this->TensorsNameAllocSize);
    this->CheckFor("normals", line, this->NumberOfNormalsInFile, this->NormalsNameInFile,
      this->NormalsNameAllocSize);
    this->CheckFor("tcoords", line, this->NumberOfTCoordsInFile, this->TCoordsNameInFile,
      this->TCoordsNameAllocSize);
    this->CheckFor("field", line, this->NumberOfFieldDataInFile, this->FieldDataNameInFile,
      this->FieldDataNameAllocSize);
  }

  this->CloseSVTKFile();
  return 1;
}

void svtkDataReader::CheckFor(const char* name, char* line, int& num, char**& array, int& allocSize)
{
  if (!strncmp(this->LowerCase(line, strlen(name)), name, strlen(name)))
  {
    int i;
    int newAllocSize;
    char** newArray;

    // update numbers
    num++;

    if (!array)
    {
      allocSize = 25;
      array = new char*[allocSize];
      for (i = 0; i < allocSize; i++)
      {
        array[i] = nullptr;
      }
    }
    else if (num >= allocSize)
    {
      newAllocSize = 2 * num;
      newArray = new char*[newAllocSize];
      for (i = 0; i < allocSize; i++)
      {
        newArray[i] = array[i];
      }
      for (i = allocSize; i < newAllocSize; i++)
      {
        newArray[i] = nullptr;
      }
      allocSize = newAllocSize;
      delete[] array;
      array = newArray;
    }

    // enter the name
    char nameOfAttribute[256];
    sscanf(line, "%*s %s", nameOfAttribute);
    if (*nameOfAttribute)
    {
      array[num - 1] = new char[strlen(nameOfAttribute) + 1];
      strcpy(array[num - 1], nameOfAttribute);
    }
  } // found one
}

const char* svtkDataReader::GetScalarsNameInFile(int i)
{
  this->CharacterizeFile();
  if (!this->ScalarsNameInFile || i < 0 || i >= this->NumberOfScalarsInFile)
  {
    return nullptr;
  }
  else
  {
    return this->ScalarsNameInFile[i];
  }
}

const char* svtkDataReader::GetVectorsNameInFile(int i)
{
  this->CharacterizeFile();
  if (!this->VectorsNameInFile || i < 0 || i >= this->NumberOfVectorsInFile)
  {
    return nullptr;
  }
  else
  {
    return this->VectorsNameInFile[i];
  }
}
const char* svtkDataReader::GetTensorsNameInFile(int i)
{
  this->CharacterizeFile();
  if (!this->TensorsNameInFile || i < 0 || i >= this->NumberOfTensorsInFile)
  {
    return nullptr;
  }
  else
  {
    return this->TensorsNameInFile[i];
  }
}
const char* svtkDataReader::GetNormalsNameInFile(int i)
{
  this->CharacterizeFile();
  if (!this->NormalsNameInFile || i < 0 || i >= this->NumberOfNormalsInFile)
  {
    return nullptr;
  }
  else
  {
    return this->NormalsNameInFile[i];
  }
}
const char* svtkDataReader::GetTCoordsNameInFile(int i)
{
  this->CharacterizeFile();
  if (!this->TCoordsNameInFile || i < 0 || i >= this->NumberOfTCoordsInFile)
  {
    return nullptr;
  }
  else
  {
    return this->TCoordsNameInFile[i];
  }
}
const char* svtkDataReader::GetFieldDataNameInFile(int i)
{
  this->CharacterizeFile();
  if (!this->FieldDataNameInFile || i < 0 || i >= this->NumberOfFieldDataInFile)
  {
    return nullptr;
  }
  else
  {
    return this->FieldDataNameInFile[i];
  }
}

svtkTypeBool svtkDataReader::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

void svtkDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileType == SVTK_BINARY)
  {
    os << indent << "File Type: BINARY\n";
  }
  else
  {
    os << indent << "File Type: ASCII\n";
  }

  if (this->Header)
  {
    os << indent << "Header: " << this->Header << "\n";
  }
  else
  {
    os << indent << "Header: (None)\n";
  }

  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "On\n" : "Off\n");
  if (this->InputString)
  {
    os << indent << "Input String: " << this->InputString << "\n";
  }
  else
  {
    os << indent << "Input String: (None)\n";
  }

  if (this->InputArray)
  {
    os << indent << "Input Array: "
       << "\n";
    this->InputArray->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Input String: (None)\n";
  }

  os << indent << "Input String Length: " << this->InputStringLength << endl;

  if (this->ScalarsName)
  {
    os << indent << "Scalars Name: " << this->ScalarsName << "\n";
  }
  else
  {
    os << indent << "Scalars Name: (None)\n";
  }
  os << indent << "ReadAllScalars: " << (this->ReadAllScalars ? "On" : "Off") << "\n";

  if (this->VectorsName)
  {
    os << indent << "Vectors Name: " << this->VectorsName << "\n";
  }
  else
  {
    os << indent << "Vectors Name: (None)\n";
  }
  os << indent << "ReadAllVectors: " << (this->ReadAllVectors ? "On" : "Off") << "\n";

  if (this->NormalsName)
  {
    os << indent << "Normals Name: " << this->NormalsName << "\n";
  }
  else
  {
    os << indent << "Normals Name: (None)\n";
  }
  os << indent << "ReadAllNormals: " << (this->ReadAllNormals ? "On" : "Off") << "\n";

  if (this->TensorsName)
  {
    os << indent << "Tensors Name: " << this->TensorsName << "\n";
  }
  else
  {
    os << indent << "Tensors Name: (None)\n";
  }
  os << indent << "ReadAllTensors: " << (this->ReadAllTensors ? "On" : "Off") << "\n";

  if (this->TCoordsName)
  {
    os << indent << "Texture Coords Name: " << this->TCoordsName << "\n";
  }
  else
  {
    os << indent << "Texture Coordinates Name: (None)\n";
  }
  os << indent << "ReadAllTCoords: " << (this->ReadAllTCoords ? "On" : "Off") << "\n";

  if (this->LookupTableName)
  {
    os << indent << "Lookup Table Name: " << this->LookupTableName << "\n";
  }
  else
  {
    os << indent << "Lookup Table Name: (None)\n";
  }
  os << indent << "ReadAllColorScalars: " << (this->ReadAllColorScalars ? "On" : "Off") << "\n";

  if (this->FieldDataName)
  {
    os << indent << "Field Data Name: " << this->FieldDataName << "\n";
  }
  else
  {
    os << indent << "Field Data Name: (None)\n";
  }
  os << indent << "ReadAllFields: " << (this->ReadAllFields ? "On" : "Off") << "\n";

  os << indent << "InputStringLength: " << this->InputStringLength << endl;
}

int svtkDataReader::ReadDataSetData(svtkDataSet* svtkNotUsed(ds))
{
  return 0;
}

int svtkDataReader::DecodeString(char* resname, const char* name)
{
  if (!resname || !name)
  {
    return 0;
  }
  std::ostringstream str;
  size_t cc = 0;
  unsigned int ch;
  size_t len = strlen(name);
  size_t reslen = 0;
  char buffer[10] = "0x";
  while (name[cc])
  {
    if (name[cc] == '%')
    {
      if (cc <= (len - 3))
      {
        buffer[2] = name[cc + 1];
        buffer[3] = name[cc + 2];
        buffer[4] = 0;
        sscanf(buffer, "%x", &ch);
        str << static_cast<char>(ch);
        cc += 2;
        reslen++;
      }
    }
    else
    {
      str << name[cc];
      reslen++;
    }
    cc++;
  }
  strncpy(resname, str.str().c_str(), reslen + 1);
  resname[reslen] = 0;
  return static_cast<int>(reslen);
}

static int my_getline(istream& in, svtkStdString& out, char delimiter)
{
  out = svtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = in.get()) != EOF && numCharactersRead < out.max_size())
  {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter)
    {
      out += downcast;
    }
    else
    {
      return numCharactersRead;
    }
  }

  return numCharactersRead;
}

//----------------------------------------------------------------------------
void svtkDataReader::SetScalarLut(const char* sl)
{
  if (!this->ScalarLut && !sl)
  {
    return;
  }
  if (this->ScalarLut && sl && (strcmp(this->ScalarLut, sl)) == 0)
  {
    return;
  }
  delete[] this->ScalarLut;
  this->ScalarLut = nullptr;
  if (sl)
  {
    size_t n = strlen(sl) + 1;
    char* cp1 = new char[n];
    const char* cp2 = sl;
    this->ScalarLut = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
}
