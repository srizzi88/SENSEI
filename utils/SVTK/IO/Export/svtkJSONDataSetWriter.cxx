/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJSONDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkJSONDataSetWriter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkTypeInt32Array.h"
#include "svtkTypeInt64Array.h"
#include "svtkTypeUInt32Array.h"
#include "svtkTypeUInt64Array.h"
#include "svtksys/FStream.hxx"
#include "svtksys/MD5.h"
#include "svtksys/SystemTools.hxx"

#include "svtkArchiver.h"

#include <fstream>
#include <sstream>
#include <string>

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkJSONDataSetWriter);
svtkCxxSetObjectMacro(svtkJSONDataSetWriter, Archiver, svtkArchiver);

//----------------------------------------------------------------------------
svtkJSONDataSetWriter::svtkJSONDataSetWriter()
{
  this->Archiver = svtkArchiver::New();
  this->ValidStringCount = 1;
}

//----------------------------------------------------------------------------
svtkJSONDataSetWriter::~svtkJSONDataSetWriter()
{
  this->SetArchiver(nullptr);
}

//----------------------------------------------------------------------------

#if !defined(SVTK_LEGACY_REMOVE)
void svtkJSONDataSetWriter::SetFileName(const char* archiveName)
{
  this->Archiver->SetArchiveName(archiveName);
}
#endif

//----------------------------------------------------------------------------

#if !defined(SVTK_LEGACY_REMOVE)
char* svtkJSONDataSetWriter::GetFileName()
{
  return this->Archiver->GetArchiveName();
}
#endif

// ----------------------------------------------------------------------------

svtkDataSet* svtkJSONDataSetWriter::GetInput()
{
  return svtkDataSet::SafeDownCast(this->Superclass::GetInput());
}

// ----------------------------------------------------------------------------

svtkDataSet* svtkJSONDataSetWriter::GetInput(int port)
{
  return svtkDataSet::SafeDownCast(this->Superclass::GetInput(port));
}

// ----------------------------------------------------------------------------

std::string svtkJSONDataSetWriter::WriteDataSetAttributes(
  svtkDataSetAttributes* fields, const char* className)
{
  int nbArrayWritten = 0;
  svtkIdType activeTCoords = -1;
  svtkIdType activeScalars = -1;
  svtkIdType activeNormals = -1;
  svtkIdType activeGlobalIds = -1;
  svtkIdType activeTensors = -1;
  svtkIdType activePedigreeIds = -1;
  svtkIdType activeVectors = -1;

  svtkIdType nbFields = fields->GetNumberOfArrays();

  if (nbFields == 0)
  {
    return "";
  }

  std::stringstream jsonSnippet;
  jsonSnippet << "  \"" << className << "\": {"
              << "\n    \"svtkClass\": \"svtkDataSetAttributes\","
              << "\n    \"arrays\": [\n";
  for (svtkIdType idx = 0; idx < nbFields; idx++)
  {
    svtkDataArray* field = fields->GetArray(idx);
    if (field == nullptr)
    {
      continue;
    }

    if (nbArrayWritten)
    {
      jsonSnippet << ",\n";
    }

    jsonSnippet << "      { \"data\": " << this->WriteArray(field, "svtkDataArray") << "}";

    // Update active field if any
    activeTCoords = field == fields->GetTCoords() ? nbArrayWritten : activeTCoords;
    activeScalars = field == fields->GetScalars() ? nbArrayWritten : activeScalars;
    activeNormals = field == fields->GetNormals() ? nbArrayWritten : activeNormals;
    activeGlobalIds = field == fields->GetGlobalIds() ? nbArrayWritten : activeGlobalIds;
    activeTensors = field == fields->GetTensors() ? nbArrayWritten : activeTensors;
    activePedigreeIds = field == fields->GetPedigreeIds() ? nbArrayWritten : activePedigreeIds;
    activeVectors = field == fields->GetVectors() ? nbArrayWritten : activeVectors;

    // Increment the number of array currently in the list
    nbArrayWritten++;
  }
  jsonSnippet << "\n    ],\n"
              << "    \"activeTCoords\": " << activeTCoords << ",\n"
              << "    \"activeScalars\": " << activeScalars << ",\n"
              << "    \"activeNormals\": " << activeNormals << ",\n"
              << "    \"activeGlobalIds\": " << activeGlobalIds << ",\n"
              << "    \"activeTensors\": " << activeTensors << ",\n"
              << "    \"activePedigreeIds\": " << activePedigreeIds << ",\n"
              << "    \"activeVectors\": " << activeVectors << "\n"
              << "  }";

  return jsonSnippet.str();
}

// ----------------------------------------------------------------------------

std::string svtkJSONDataSetWriter::WriteArray(
  svtkDataArray* array, const char* className, const char* arrayName)
{
  bool needConvert;
  std::string id = svtkJSONDataSetWriter::GetUID(array, needConvert);
  std::stringstream arrayPath;
  arrayPath << "data/" << id.c_str();
  bool success = svtkJSONDataSetWriter::WriteArrayContents(array, arrayPath.str().c_str());

  if (!success)
  {
    return "{}";
  }

  const char* INDENT = "    ";
  std::stringstream ss;
  ss << "{\n"
     << INDENT << "  \"svtkClass\": \"" << className << "\",\n"
     << INDENT << "  \"name\": \""
     << this->GetValidString(arrayName == nullptr ? array->GetName() : arrayName) << "\",\n"
     << INDENT << "  \"numberOfComponents\": " << array->GetNumberOfComponents() << ",\n"
     << INDENT << "  \"dataType\": \"" << svtkJSONDataSetWriter::GetShortType(array, needConvert)
     << "Array\",\n"
     << INDENT << "  \"ref\": {\n"
     << INDENT << "     \"encode\": \"LittleEndian\",\n"
     << INDENT << "     \"basepath\": \"data\",\n"
     << INDENT << "     \"id\": \"" << id.c_str() << "\"\n"
     << INDENT << "  },\n"
     << INDENT << "  \"size\": " << array->GetNumberOfValues() << "\n"
     << INDENT << "}";

  return ss.str();
}

//----------------------------------------------------------------------------
void svtkJSONDataSetWriter::Write(svtkDataSet* dataset)
{
  svtkImageData* imageData = svtkImageData::SafeDownCast(dataset);
  svtkPolyData* polyData = svtkPolyData::SafeDownCast(dataset);
  this->ValidDataSet = false;

  // Get input and check data
  if (dataset == nullptr)
  {
    svtkErrorMacro(<< "No data to write!");
    return;
  }

  this->GetArchiver()->OpenArchive();

  // Capture svtkDataSet definition
  std::stringstream metaJsonFile;
  metaJsonFile << "{\n";
  metaJsonFile << "  \"svtkClass\": \"" << dataset->GetClassName() << "\"";

  // ImageData
  if (imageData)
  {
    this->ValidDataSet = true;

    // Spacing
    metaJsonFile << ",\n  \"spacing\": [" << imageData->GetSpacing()[0] << ", "
                 << imageData->GetSpacing()[1] << ", " << imageData->GetSpacing()[2] << "]";

    // Origin
    metaJsonFile << ",\n  \"origin\": [" << imageData->GetOrigin()[0] << ", "
                 << imageData->GetOrigin()[1] << ", " << imageData->GetOrigin()[2] << "]";

    // Extent
    metaJsonFile << ",\n  \"extent\": [" << imageData->GetExtent()[0] << ", "
                 << imageData->GetExtent()[1] << ", " << imageData->GetExtent()[2] << ", "
                 << imageData->GetExtent()[3] << ", " << imageData->GetExtent()[4] << ", "
                 << imageData->GetExtent()[5] << "]";
  }

  // PolyData
  if (polyData && polyData->GetPoints())
  {
    this->ValidDataSet = true;

    svtkPoints* points = polyData->GetPoints();
    metaJsonFile << ",\n  \"points\": "
                 << this->WriteArray(points->GetData(), "svtkPoints", "points").c_str();

    // Verts
    svtkNew<svtkIdTypeArray> cells;
    polyData->GetVerts()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"verts\": "
                   << this->WriteArray(cells, "svtkCellArray", "verts").c_str();
    }

    // Lines
    polyData->GetLines()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"lines\": "
                   << this->WriteArray(cells, "svtkCellArray", "lines").c_str();
    }

    // Strips
    polyData->GetStrips()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"strips\": "
                   << this->WriteArray(cells, "svtkCellArray", "strips").c_str();
    }

    // Polys
    polyData->GetPolys()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"polys\": "
                   << this->WriteArray(cells, "svtkCellArray", "polys").c_str();
    }
  }

  // PointData
  std::string fieldJSON = this->WriteDataSetAttributes(dataset->GetPointData(), "pointData");
  if (!fieldJSON.empty())
  {
    metaJsonFile << ",\n" << fieldJSON.c_str();
  }

  // CellData
  fieldJSON = this->WriteDataSetAttributes(dataset->GetCellData(), "cellData");
  if (!fieldJSON.empty())
  {
    metaJsonFile << ",\n" << fieldJSON.c_str();
  }

  metaJsonFile << "}\n";

  // Write meta-data file
  std::string metaJsonFileStr = metaJsonFile.str();
  this->GetArchiver()->InsertIntoArchive(
    "index.json", metaJsonFileStr.c_str(), metaJsonFileStr.size());

  this->GetArchiver()->CloseArchive();
}

//----------------------------------------------------------------------------
void svtkJSONDataSetWriter::WriteData()
{
  svtkDataSet* dataset = this->GetInput();
  this->Write(dataset);
}

// ----------------------------------------------------------------------------
bool svtkJSONDataSetWriter::WriteArrayContents(svtkDataArray* input, const char* filePath)
{
  if (input->GetDataTypeSize() == 0)
  {
    // Skip BIT arrays
    return false;
  }

  // Check if we need to convert the (u)int64 to (u)int32
  svtkSmartPointer<svtkDataArray> arrayToWrite = input;
  svtkIdType arraySize = input->GetNumberOfTuples() * input->GetNumberOfComponents();
  switch (input->GetDataType())
  {
    case SVTK_UNSIGNED_CHAR:
    case SVTK_UNSIGNED_LONG:
    case SVTK_UNSIGNED_LONG_LONG:
      if (input->GetDataTypeSize() > 4)
      {
        svtkNew<svtkTypeUInt64Array> srcUInt64;
        srcUInt64->ShallowCopy(input);
        svtkNew<svtkTypeUInt32Array> uint32;
        uint32->SetNumberOfValues(arraySize);
        uint32->SetName(input->GetName());
        for (svtkIdType i = 0; i < arraySize; i++)
        {
          uint32->SetValue(i, srcUInt64->GetValue(i));
        }
        arrayToWrite = uint32;
      }
      break;
    case SVTK_LONG:
    case SVTK_LONG_LONG:
    case SVTK_ID_TYPE:
      if (input->GetDataTypeSize() > 4)
      {
        svtkNew<svtkTypeInt64Array> srcInt64;
        srcInt64->ShallowCopy(input);
        svtkNew<svtkTypeInt32Array> int32;
        int32->SetNumberOfTuples(arraySize);
        int32->SetName(input->GetName());
        for (svtkIdType i = 0; i < arraySize; i++)
        {
          int32->SetValue(i, srcInt64->GetValue(i));
        }
        arrayToWrite = int32;
      }
      break;
  }

  const char* content = (const char*)arrayToWrite->GetVoidPointer(0);
  size_t size = arrayToWrite->GetNumberOfValues() * arrayToWrite->GetDataTypeSize();

  this->GetArchiver()->InsertIntoArchive(filePath, content, size);
  return true;
}

//----------------------------------------------------------------------------
namespace
{
class svtkSingleFileArchiver : public svtkArchiver
{
public:
  static svtkSingleFileArchiver* New();
  svtkTypeMacro(svtkSingleFileArchiver, svtkArchiver);

  virtual void OpenArchive() override {}
  virtual void CloseArchive() override {}
  virtual void InsertIntoArchive(
    const std::string& filePath, const char* data, std::size_t size) override
  {
    svtksys::ofstream file;
    file.open(filePath.c_str(), ios::out | ios::binary);
    file.write(data, size);
    file.close();
  }

private:
  svtkSingleFileArchiver() = default;
  virtual ~svtkSingleFileArchiver() override = default;
};
svtkStandardNewMacro(svtkSingleFileArchiver);
}

//----------------------------------------------------------------------------
bool svtkJSONDataSetWriter::WriteArrayAsRAW(svtkDataArray* array, const char* filePath)
{
  svtkNew<svtkJSONDataSetWriter> writer;
  svtkNew<svtkSingleFileArchiver> archiver;
  writer->SetArchiver(archiver);
  return writer->WriteArrayContents(array, filePath);
}

//----------------------------------------------------------------------------
void svtkJSONDataSetWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
int svtkJSONDataSetWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

// ----------------------------------------------------------------------------
// Static helper functions
// ----------------------------------------------------------------------------

void svtkJSONDataSetWriter::ComputeMD5(const unsigned char* content, int size, std::string& hash)
{
  unsigned char digest[16];
  char md5Hash[33];
  md5Hash[32] = '\0';

  svtksysMD5* md5 = svtksysMD5_New();
  svtksysMD5_Initialize(md5);
  svtksysMD5_Append(md5, content, size);
  svtksysMD5_Finalize(md5, digest);
  svtksysMD5_DigestToHex(digest, md5Hash);
  svtksysMD5_Delete(md5);

  hash = md5Hash;
}

// ----------------------------------------------------------------------------

std::string svtkJSONDataSetWriter::GetShortType(svtkDataArray* input, bool& needConversion)
{
  needConversion = false;
  std::stringstream ss;
  switch (input->GetDataType())
  {
    case SVTK_UNSIGNED_CHAR:
    case SVTK_UNSIGNED_SHORT:
    case SVTK_UNSIGNED_INT:
    case SVTK_UNSIGNED_LONG:
    case SVTK_UNSIGNED_LONG_LONG:
      ss << "Uint";
      if (input->GetDataTypeSize() <= 4)
      {
        ss << (input->GetDataTypeSize() * 8);
      }
      else
      {
        needConversion = true;
        ss << "32";
      }

      break;

    case SVTK_CHAR:
    case SVTK_SIGNED_CHAR:
    case SVTK_SHORT:
    case SVTK_INT:
    case SVTK_LONG:
    case SVTK_LONG_LONG:
    case SVTK_ID_TYPE:
      ss << "Int";
      if (input->GetDataTypeSize() <= 4)
      {
        ss << (input->GetDataTypeSize() * 8);
      }
      else
      {
        needConversion = true;
        ss << "32";
      }
      break;

    case SVTK_FLOAT:
    case SVTK_DOUBLE:
      ss << "Float";
      ss << (input->GetDataTypeSize() * 8);
      break;

    case SVTK_BIT:
    case SVTK_STRING:
    case SVTK_UNICODE_STRING:
    case SVTK_VARIANT:
    default:
      ss << "xxx";
      break;
  }

  return ss.str();
}

// ----------------------------------------------------------------------------

std::string svtkJSONDataSetWriter::GetUID(svtkDataArray* input, bool& needConversion)
{
  const unsigned char* content = (const unsigned char*)input->GetVoidPointer(0);
  int size = input->GetNumberOfValues() * input->GetDataTypeSize();
  std::string hash;
  svtkJSONDataSetWriter::ComputeMD5(content, size, hash);

  std::stringstream ss;
  ss << svtkJSONDataSetWriter::GetShortType(input, needConversion) << "_"
     << input->GetNumberOfValues() << "-" << hash.c_str();

  return ss.str();
}

// ----------------------------------------------------------------------------

std::string svtkJSONDataSetWriter::GetValidString(const char* name)
{
  if (name != nullptr && strlen(name))
  {
    return name;
  }
  std::stringstream ss;
  ss << "invalid_" << this->ValidStringCount++;

  return ss.str();
}
