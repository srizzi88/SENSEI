/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLReader.h"

#include "svtkArrayIteratorIncludes.h"
#include "svtkCallbackCommand.h"
#include "svtkDataArray.h"
#include "svtkDataArraySelection.h"
#include "svtkDataCompressor.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationKeyLookup.h"
#include "svtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationStringVectorKey.h"
#include "svtkInformationUnsignedLongKey.h"
#include "svtkInformationVector.h"
#include "svtkLZ4DataCompressor.h"
#include "svtkLZMADataCompressor.h"
#include "svtkObjectFactory.h"
#include "svtkQuadratureSchemeDefinition.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataParser.h"
#include "svtkXMLFileReadTester.h"
#include "svtkXMLReaderVersion.h"
#include "svtkZLibDataCompressor.h"

#include "svtksys/Encoding.hxx"
#include "svtksys/FStream.hxx"
#include <svtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <locale> // C++ locale
#include <sstream>
#include <vector>

svtkCxxSetObjectMacro(svtkXMLReader, ReaderErrorObserver, svtkCommand);
svtkCxxSetObjectMacro(svtkXMLReader, ParserErrorObserver, svtkCommand);

//-----------------------------------------------------------------------------
#define CaseIdTypeMacro(type, size)                                                                \
  case type:                                                                                       \
    if (size == SVTK_SIZEOF_ID_TYPE)                                                                \
    {                                                                                              \
      dataType = SVTK_ID_TYPE;                                                                      \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      if (size > SVTK_SIZEOF_ID_TYPE)                                                               \
      {                                                                                            \
        svtkWarningMacro("An array named " << da->GetAttribute("Name")                              \
                                          << " was tagged as an IdType array with a type size of " \
                                          << size                                                  \
                                          << " which is bigger then the IdType size on this SVTK "  \
                                             "build. The IdType tag has been ignored.");           \
      }                                                                                            \
      else                                                                                         \
      {                                                                                            \
        svtkDebugMacro("An array named " << da->GetAttribute("Name")                                \
                                        << " was tagged as an IdType array with a type size of "   \
                                        << size                                                    \
                                        << " which is smaller then the IdType size on this SVTK "   \
                                           "build. The IdType tag has been ignored.");             \
      }                                                                                            \
    }                                                                                              \
    break

//-----------------------------------------------------------------------------
static void ReadStringVersion(const char* version, int& major, int& minor)
{
  if (!version)
  {
    major = -1;
    minor = -1;
    return;
  }
  // Extract the major and minor version numbers.
  size_t length = strlen(version);
  const char* begin = version;
  const char* end = version + length;
  const char* s;

  for (s = begin; (s != end) && (*s != '.'); ++s)
  {
    ;
  }

  if (s > begin)
  {
    std::stringstream str;
    str.write(begin, s - begin);
    str >> major;
    if (!str)
    {
      major = 0;
    }
  }
  if (++s < end)
  {
    std::stringstream str;
    str.write(s, end - s);
    str >> minor;
    if (!str)
    {
      minor = 0;
    }
  }
}
//----------------------------------------------------------------------------
svtkXMLReader::svtkXMLReader()
{
  this->FileName = nullptr;
  this->Stream = nullptr;
  this->FileStream = nullptr;
  this->StringStream = nullptr;
  this->ReadFromInputString = 0;
  this->InputString = "";
  this->XMLParser = nullptr;
  this->ReaderErrorObserver = nullptr;
  this->ParserErrorObserver = nullptr;
  this->FieldDataElement = nullptr;
  this->PointDataArraySelection = svtkDataArraySelection::New();
  this->CellDataArraySelection = svtkDataArraySelection::New();
  this->ColumnArraySelection = svtkDataArraySelection::New();
  this->InformationError = 0;
  this->DataError = 0;
  this->ReadError = 0;
  this->ProgressRange[0] = 0;
  this->ProgressRange[1] = 1;

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = svtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&svtkXMLReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(svtkCommand::ModifiedEvent, this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(svtkCommand::ModifiedEvent, this->SelectionObserver);
  this->ColumnArraySelection->AddObserver(svtkCommand::ModifiedEvent, this->SelectionObserver);
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Lower dimensional cell data support.
  this->AxesEmpty[0] = 0;
  this->AxesEmpty[1] = 0;
  this->AxesEmpty[2] = 0;

  // Time support:
  this->TimeStep = 0; // By default the file does not have timestep
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->NumberOfTimeSteps = 0;
  this->TimeSteps = nullptr;
  this->CurrentTimeStep = 0;
  this->TimeStepWasReadOnce = 0;

  this->FileMinorVersion = -1;
  this->FileMajorVersion = -1;

  this->CurrentOutput = nullptr;
  this->InReadData = 0;
}

//----------------------------------------------------------------------------
svtkXMLReader::~svtkXMLReader()
{
  this->SetFileName(nullptr);
  if (this->XMLParser)
  {
    this->DestroyXMLParser();
  }
  this->CloseStream();
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->ColumnArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
  this->ColumnArraySelection->Delete();
  if (this->ReaderErrorObserver)
  {
    this->ReaderErrorObserver->Delete();
  }
  if (this->ParserErrorObserver)
  {
    this->ParserErrorObserver->Delete();
  }
  delete[] this->TimeSteps;
}

//----------------------------------------------------------------------------
void svtkXMLReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "CellDataArraySelection: " << this->CellDataArraySelection << "\n";
  os << indent << "PointDataArraySelection: " << this->PointDataArraySelection << "\n";
  os << indent << "ColumnArraySelection: " << this->PointDataArraySelection << "\n";
  if (this->Stream)
  {
    os << indent << "Stream: " << this->Stream << "\n";
  }
  else
  {
    os << indent << "Stream: (none)\n";
  }
  os << indent << "TimeStep:" << this->TimeStep << "\n";
  os << indent << "NumberOfTimeSteps:" << this->NumberOfTimeSteps << "\n";
  os << indent << "TimeStepRange:(" << this->TimeStepRange[0] << "," << this->TimeStepRange[1]
     << ")\n";
}

//----------------------------------------------------------------------------
svtkDataSet* svtkXMLReader::GetOutputAsDataSet()
{
  return this->GetOutputAsDataSet(0);
}

//----------------------------------------------------------------------------
svtkDataSet* svtkXMLReader::GetOutputAsDataSet(int index)
{
  return svtkDataSet::SafeDownCast(this->GetOutputDataObject(index));
}

//----------------------------------------------------------------------------
// Major version should be incremented when older readers can no longer
// read files written for this reader. Minor versions are for added
// functionality that can be safely ignored by older readers.
int svtkXMLReader::CanReadFileVersion(int major, int svtkNotUsed(minor))
{
  return (major > svtkXMLReaderMajorVersion) ? 0 : 1;
}

//----------------------------------------------------------------------------
int svtkXMLReader::OpenStream()
{
  if (this->ReadFromInputString)
  {
    return this->OpenSVTKString();
  }
  else
  {
    return this->OpenSVTKFile();
  }
}

//----------------------------------------------------------------------------
int svtkXMLReader::OpenSVTKFile()
{
  if (this->FileStream)
  {
    svtkErrorMacro("File already open.");
    return 1;
  }

  if (!this->Stream && !this->FileName)
  {
    svtkErrorMacro("File name not specified");
    return 0;
  }

  if (this->Stream)
  {
    // Use user-provided stream.
    return 1;
  }

  // Need to open a file.  First make sure it exists.  This prevents
  // an empty file from being created on older compilers.
  svtksys::SystemTools::Stat_t fs;
  if (svtksys::SystemTools::Stat(this->FileName, &fs) != 0)
  {
    svtkErrorMacro("Error opening file " << this->FileName);
    return 0;
  }

  std::ios_base::openmode mode = ios::in;
#ifdef _WIN32
  mode |= ios::binary;
#endif
  this->FileStream = new svtksys::ifstream(this->FileName, mode);
  if (!this->FileStream || !(*this->FileStream))
  {
    svtkErrorMacro("Error opening file " << this->FileName);
    delete this->FileStream;
    this->FileStream = nullptr;
    return 0;
  }

  // Use the file stream.
  this->Stream = this->FileStream;

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLReader::OpenSVTKString()
{
  if (this->StringStream)
  {
    svtkErrorMacro("string already open.");
    return 1;
  }

  if (!this->Stream && this->InputString.compare("") == 0)
  {
    svtkErrorMacro("Input string not specified");
    return 0;
  }

  if (this->Stream)
  {
    // Use user-provided stream.
    return 1;
  }

  // Open the string stream
  this->StringStream = new std::istringstream(this->InputString);
  if (!this->StringStream || !(*this->StringStream))
  {
    svtkErrorMacro("Error opening string stream");
    delete this->StringStream;
    this->StringStream = nullptr;
    return 0;
  }

  // Use the string stream.
  this->Stream = this->StringStream;

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLReader::CloseStream()
{
  if (this->Stream)
  {
    if (this->ReadFromInputString)
    {
      this->CloseSVTKString();
    }
    else
    {
      this->CloseSVTKFile();
    }
    this->Stream = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLReader::CloseSVTKFile()
{
  if (!this->Stream)
  {
    svtkErrorMacro("File not open.");
    return;
  }
  if (this->Stream == this->FileStream)
  {
    delete this->FileStream;
    this->FileStream = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLReader::CloseSVTKString()
{
  if (!this->Stream)
  {
    svtkErrorMacro("String not open.");
    return;
  }
  if (this->Stream == this->StringStream)
  {
    // We opened the string.  Close it.
    delete this->StringStream;
    this->StringStream = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkXMLReader::CreateXMLParser()
{
  if (this->XMLParser)
  {
    svtkErrorMacro("CreateXMLParser() called with existing XMLParser.");
    this->DestroyXMLParser();
  }
  this->XMLParser = svtkXMLDataParser::New();
}

//----------------------------------------------------------------------------
void svtkXMLReader::DestroyXMLParser()
{
  if (!this->XMLParser)
  {
    svtkErrorMacro("DestroyXMLParser() called with no current XMLParser.");
    return;
  }
  this->XMLParser->Delete();
  this->XMLParser = nullptr;
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetupCompressor(const char* type)
{
  // Instantiate a compressor of the given type.
  if (!type)
  {
    svtkErrorMacro("Compressor has no type.");
    return;
  }
  svtkObject* object = nullptr;
  svtkDataCompressor* compressor = svtkDataCompressor::SafeDownCast(object);

  if (!compressor)
  {
    if (strcmp(type, "svtkZLibDataCompressor") == 0)
    {
      compressor = svtkZLibDataCompressor::New();
    }
    else if (strcmp(type, "svtkLZ4DataCompressor") == 0)
    {
      compressor = svtkLZ4DataCompressor::New();
    }
    else if (strcmp(type, "svtkLZMADataCompressor") == 0)
    {
      compressor = svtkLZMADataCompressor::New();
    }
  }

  if (!compressor)
  {
    svtkErrorMacro("Error creating " << type);
    if (object)
    {
      object->Delete();
    }
    return;
  }
  this->XMLParser->SetCompressor(compressor);
  compressor->Delete();
}

//----------------------------------------------------------------------------
int svtkXMLReader::ReadXMLInformation()
{
  // only Parse if something has changed
  if (this->GetMTime() > this->ReadMTime)
  {
    // Destroy any old information that was parsed.
    if (this->XMLParser)
    {
      this->DestroyXMLParser();
    }

    // Open the input file.  If it fails, the error was already
    // reported by OpenStream.
    if (!this->OpenStream())
    {
      return 0;
    }

    // Create the svtkXMLParser instance used to parse the file.
    this->CreateXMLParser();

    // Configure the parser for this file.
    this->XMLParser->SetStream(this->Stream);

    // Parse the input file.
    if (this->XMLParser->Parse())
    {
      // Let the subclasses read the information they want.
      if (!this->ReadSVTKFile(this->XMLParser->GetRootElement()))
      {
        // There was an error reading the file.
        this->ReadError = 1;
      }
      else
      {
        this->ReadError = 0;
      }
    }
    else
    {
      svtkErrorMacro("Error parsing input file.  ReadXMLInformation aborting.");
      // The output should be empty to prevent the rest of the pipeline
      // from executing.
      this->ReadError = 1;
    }

    if (this->FieldDataElement) // read the field data information
    {
      for (int i = 0; i < this->FieldDataElement->GetNumberOfNestedElements(); i++)
      {
        svtkXMLDataElement* eNested = this->FieldDataElement->GetNestedElement(i);
        const char* name = eNested->GetAttribute("Name");
        if (name && strncmp(name, "TimeValue", 9) == 0)
        {
          svtkAbstractArray* array = this->CreateArray(eNested);
          array->SetNumberOfTuples(1);
          if (!this->ReadArrayValues(eNested, 0, array, 0, 1))
          {
            this->DataError = 1;
          }
          svtkDataArray* da = svtkDataArray::SafeDownCast(array);
          if (da)
          {
            double val = da->GetComponent(0, 0);
            svtkInformation* info = this->GetCurrentOutputInformation();
            info->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &val, 1);
            double range[2] = { val, val };
            info->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
          }
          array->Delete();
        }
      }
    }

    // Close the input stream to prevent resource leaks.
    this->CloseStream();

    this->ReadMTime.Modified();
  }
  return !this->ReadError;
}

//----------------------------------------------------------------------------
int svtkXMLReader::RequestInformation(svtkInformation* request,
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (this->ReadXMLInformation())
  {
    this->InformationError = 0;
    // Let the subclasses read the information they want.
    int outputPort = request->Get(svtkDemandDrivenPipeline::FROM_OUTPUT_PORT());
    outputPort = outputPort >= 0 ? outputPort : 0;
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    this->SetupOutputInformation(outInfo);

    if (!outInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
      // this->NumberOfTimeSteps has been set during the
      // this->ReadXMLInformation()
      int numTimesteps = this->GetNumberOfTimeSteps();
      this->TimeStepRange[0] = 0;
      this->TimeStepRange[1] = (numTimesteps > 0 ? numTimesteps - 1 : 0);
      if (numTimesteps != 0)
      {
        std::vector<double> timeSteps(numTimesteps);
        for (int i = 0; i < numTimesteps; i++)
        {
          timeSteps[i] = i;
        }
        outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps[0], numTimesteps);
        double timeRange[2];
        timeRange[0] = timeSteps[0];
        timeRange[1] = timeSteps[numTimesteps - 1];
        outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
      }
    }
  }
  else
  {
    this->InformationError = 1;
  }

  return !this->InformationError;
}

//----------------------------------------------------------------------------
int svtkXMLReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  this->CurrentTimeStep = this->TimeStep;

  // Get the output pipeline information and data object.
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  this->CurrentOutput = output;

  // Save the time value in the output data information.
  double* steps = outInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());

  // Check if a particular time was requested.
  if (steps && outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeStep = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    int length = outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());

    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < length - 1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    this->CurrentTimeStep = cnt;

    // Clamp the requested time step to be in bounds.
    if (this->CurrentTimeStep < this->TimeStepRange[0])
    {
      this->CurrentTimeStep = this->TimeStepRange[0];
    }
    else if (this->CurrentTimeStep > this->TimeStepRange[1])
    {
      this->CurrentTimeStep = this->TimeStepRange[1];
    }

    output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), steps[this->CurrentTimeStep]);
  }

  // Re-open the input file.  If it fails, the error was already
  // reported by OpenStream.
  if (!this->OpenStream())
  {
    this->SetupEmptyOutput();
    this->CurrentOutput = nullptr;
    return 0;
  }
  if (!this->XMLParser)
  {
    svtkErrorMacro("ExecuteData called with no current XMLParser.");
  }

  // Give the svtkXMLParser instance its file back so that data section
  // reads will work.
  (*this->Stream).imbue(std::locale::classic());
  this->XMLParser->SetStream(this->Stream);

  // We are just starting to read.  Do not call UpdateProgressDiscrete
  // because we want a 0 progress callback the first time.
  this->UpdateProgress(0.);

  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = { 0.f, 1.f };
  this->SetProgressRange(wholeProgressRange, 0, 1);

  if (!this->InformationError)
  {
    // We are just starting to execute.  No errors have yet occurred.
    this->XMLParser->SetAbort(0);
    this->DataError = 0;

    // Let the subclasses read the data they want.
    this->ReadXMLData();

    // If we aborted or there was an error, provide empty output.
    if (this->DataError || this->AbortExecute)
    {
      this->SetupEmptyOutput();
    }
  }
  else
  {
    // There was an error reading the file.  Provide empty output.
    this->SetupEmptyOutput();
  }

  // We have finished reading.
  this->UpdateProgressDiscrete(1);

  // Close the input stream to prevent resource leaks.
  this->CloseStream();
  if (this->TimeSteps)
  {
    // The SetupOutput should not reallocate this should be done only in a TimeStep case
    this->TimeStepWasReadOnce = 1;
  }

  this->SqueezeOutputArrays(output);

  this->CurrentOutput = nullptr;
  return 1;
}

namespace
{
//----------------------------------------------------------------------------
template <class iterT>
int svtkXMLDataReaderReadArrayValues(svtkXMLDataElement* da, svtkXMLDataParser* xmlparser,
  svtkIdType arrayIndex, iterT* iter, svtkIdType startIndex, svtkIdType numValues)
{
  if (!iter)
  {
    return 0;
  }
  svtkAbstractArray* array = iter->GetArray();
  // Number of expected words:
  size_t numWords = array->GetDataType() != SVTK_BIT ? numValues : ((numValues + 7) / 8);
  int result;
  void* data = array->GetVoidPointer(arrayIndex);
  if (da->GetAttribute("offset"))
  {
    svtkTypeInt64 offset = 0;
    da->GetScalarAttribute("offset", offset);
    result = (xmlparser->ReadAppendedData(
                offset, data, startIndex, numWords, array->GetDataType()) == numWords);
  }
  else
  {
    int isAscii = 1;
    const char* format = da->GetAttribute("format");
    if (format && (strcmp(format, "binary") == 0))
    {
      isAscii = 0;
    }
    result = (xmlparser->ReadInlineData(
                da, isAscii, data, startIndex, numWords, array->GetDataType()) == numWords);
  }
  return result;
}

//----------------------------------------------------------------------------
template <>
int svtkXMLDataReaderReadArrayValues(svtkXMLDataElement* da, svtkXMLDataParser* xmlparser,
  svtkIdType arrayIndex, svtkArrayIteratorTemplate<svtkStdString>* iter, svtkIdType startIndex,
  svtkIdType numValues)
{
  // now, for strings, we have to read from the start, as we don't have
  // support for index array yet.
  // So this specialization will read all strings starting from the beginning,
  // start putting the strings at the requested indices into the array
  // until the request numValues are put into the array.
  svtkIdType bufstart = 0;
  svtkIdType actualNumValues = startIndex + numValues;

  int size = 1024;
  char* buffer = new char[size + 1 + 7]; // +7 is leeway.
  buffer[1024] = 0;                      // to avoid string reads beyond buffer size.

  int inline_data = (da->GetAttribute("offset") == nullptr);

  svtkTypeInt64 offset = 0;
  if (inline_data == 0)
  {
    da->GetScalarAttribute("offset", offset);
  }

  int isAscii = 1;
  const char* format = da->GetAttribute("format");
  if (format && (strcmp(format, "binary") == 0))
  {
    isAscii = 0;
  }

  // Now read a buffer full of data,
  // create strings out of it.
  int result = 1;
  svtkIdType inIndex = 0;
  svtkIdType outIndex = arrayIndex;
  svtkStdString prev_string;
  while (result && inIndex < actualNumValues)
  {
    size_t chars_read = 0;
    if (inline_data)
    {
      chars_read = xmlparser->ReadInlineData(da, isAscii, buffer, bufstart, size, SVTK_CHAR);
    }
    else
    {
      chars_read = xmlparser->ReadAppendedData(offset, buffer, bufstart, size, SVTK_CHAR);
    }
    if (!chars_read)
    {
      // failed.
      result = 0;
      break;
    }
    bufstart += static_cast<svtkIdType>(chars_read);
    // now read strings
    const char* ptr = buffer;
    const char* end_ptr = &buffer[chars_read];
    buffer[chars_read] = 0;

    while (ptr < end_ptr)
    {
      svtkStdString temp_string = ptr; // will read in string until 0x0;
      ptr += temp_string.size() + 1;
      if (!prev_string.empty())
      {
        temp_string = prev_string + temp_string;
        prev_string = "";
      }
      // now decide if the string terminated or buffer was full.
      if (ptr > end_ptr)
      {
        // buffer ended -- string is incomplete.
        // keep the prefix in temp_string.
        prev_string = temp_string;
      }
      else
      {
        // string read fully.
        if (inIndex >= startIndex)
        {
          // add string to the array.
          iter->GetValue(outIndex) = temp_string; // copy the value.
          outIndex++;
        }
        inIndex++;
      }
    }
  }
  delete[] buffer;
  return result;
}

}

//----------------------------------------------------------------------------
int svtkXMLReader::ReadArrayValues(svtkXMLDataElement* da, svtkIdType arrayIndex,
  svtkAbstractArray* array, svtkIdType startIndex, svtkIdType numValues, FieldType fieldType)
{
  // Skip real read if aborting.
  if (this->AbortExecute)
  {
    return 0;
  }
  this->InReadData = 1;
  int result;
  svtkArrayIterator* iter = array->NewIterator();
  switch (array->GetDataType())
  {
    svtkArrayIteratorTemplateMacro(result = svtkXMLDataReaderReadArrayValues(da, this->XMLParser,
                                    arrayIndex, static_cast<SVTK_TT*>(iter), startIndex, numValues));
    default:
      result = 0;
  }
  if (iter)
  {
    iter->Delete();
  }

  this->ConvertGhostLevelsToGhostType(fieldType, array, startIndex, numValues);
  // Marking the array modified is essential, since otherwise, when reading
  // multiple time-steps, the array does not realize that its contents may have
  // changed and does not recompute the array ranges.
  // This becomes an issue only because we reuse the svtkAbstractArray* instance
  // when reading time-steps. The array is allocated only for the first timestep
  // read (see svtkXMLReader::ReadXMLData() and its use of
  // this->TimeStepWasReadOnce flag).
  array->Modified();
  this->InReadData = 0;
  return result;
}

//----------------------------------------------------------------------------
void svtkXMLReader::ReadXMLData()
{
  // Initialize the output's data.
  if (!this->TimeStepWasReadOnce)
  {
    this->SetupOutputData();
  }
}

//----------------------------------------------------------------------------
int svtkXMLReader::ReadSVTKFile(svtkXMLDataElement* eSVTKFile)
{
  // Check if the file version is one we support.
  const char* version = eSVTKFile->GetAttribute("version");
  if (version && !this->CanReadFileVersionString(version))
  {
    svtkWarningMacro("File version: " << version
                                     << " is higher than "
                                        "this reader supports "
                                     << svtkXMLReaderMajorVersion << "."
                                     << svtkXMLReaderMinorVersion);
  }

  ::ReadStringVersion(version, this->FileMajorVersion, this->FileMinorVersion);

  // Setup the compressor if there is one.
  const char* compressor = eSVTKFile->GetAttribute("compressor");
  if (compressor)
  {
    this->SetupCompressor(compressor);
  }

  // Get the primary element.
  const char* name = this->GetDataSetName();
  svtkXMLDataElement* ePrimary = nullptr;
  for (int i = 0; i < eSVTKFile->GetNumberOfNestedElements(); ++i)
  {
    svtkXMLDataElement* eNested = eSVTKFile->GetNestedElement(i);
    if (strcmp(eNested->GetName(), name) == 0)
    {
      ePrimary = eNested;
      break;
    }
  }
  if (!ePrimary)
  {
    svtkErrorMacro("Cannot find " << name << " element in file.");
    return 0;
  }

  // Read the primary element.
  return this->ReadPrimaryElement(ePrimary);
}

//----------------------------------------------------------------------------
int svtkXMLReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
{
  int numTimeSteps =
    ePrimary->GetVectorAttribute("TimeValues", SVTK_INT_MAX, static_cast<double*>(nullptr));
  this->SetNumberOfTimeSteps(numTimeSteps);

  // See if there is a FieldData element
  int numNested = ePrimary->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    svtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "FieldData") == 0)
    {
      this->FieldDataElement = eNested;
      return 1;
    }
  }

  this->FieldDataElement = nullptr;

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetupOutputData()
{
  // Initialize the output.
  this->CurrentOutput->Initialize();
}

void svtkXMLReader::ReadFieldData()
{
  if (this->FieldDataElement) // read the field data information
  {
    svtkIdType numTuples;
    svtkFieldData* fieldData = this->GetCurrentOutput()->GetFieldData();
    for (int i = 0; i < this->FieldDataElement->GetNumberOfNestedElements() && !this->AbortExecute;
         i++)
    {
      svtkXMLDataElement* eNested = this->FieldDataElement->GetNestedElement(i);
      svtkAbstractArray* array = this->CreateArray(eNested);
      if (array)
      {
        if (eNested->GetScalarAttribute("NumberOfTuples", numTuples))
        {
          array->SetNumberOfTuples(numTuples);
        }
        else
        {
          numTuples = 0;
        }
        fieldData->AddArray(array);
        array->Delete();
        if (!this->ReadArrayValues(
              eNested, 0, array, 0, numTuples * array->GetNumberOfComponents()) &&
          numTuples)
        {
          this->DataError = 1;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// Methods used for deserializing svtkInformation. ----------------------------
namespace
{

void ltrim(std::string& s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

void rtrim(std::string& s)
{
  s.erase(
    std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

void trim(std::string& s)
{
  ltrim(s);
  rtrim(s);
}

// Handle type extraction where needed, but trim and pass-through strings.
template <class ValueType>
bool extractValue(const char* valueStr, ValueType& value)
{
  if (!valueStr)
  {
    return false;
  }

  std::istringstream str;
  str.str(valueStr);
  str >> value;
  return !str.fail();
}
bool extractValue(const char* valueStr, std::string& value)
{
  value = std::string(valueStr ? valueStr : "");
  trim(value); // svtkXMLDataElement adds newlines before/after character data.
  return true;
}

template <class ValueType, class KeyType>
bool readScalarInfo(KeyType* key, svtkInformation* info, svtkXMLDataElement* element)
{
  const char* valueStr = element->GetCharacterData();

  // backwards-compat: Old versions of the writer used to store data in
  // a 'value' attribute, but this causes problems with strings (e.g. the
  // XML parser removes newlines from attribute values).
  // Note that this is only for the non-vector information keys, as there were
  // no serialized vector keys in the old writer.
  // If there's no character data, check for a value attribute:
  if ((!valueStr || strlen(valueStr) == 0))
  {
    valueStr = element->GetAttribute("value");
  }

  ValueType value;
  if (!extractValue(valueStr, value))
  {
    return false;
  }
  info->Set(key, value);
  return true;
}

// Generic vector key reader. Stores in a temporary vector and calls Set to
// make sure that keys with RequiredLength work properly.
template <class ValueType, class KeyType>
bool readVectorInfo(KeyType* key, svtkInformation* info, svtkXMLDataElement* element)
{
  const char* lengthData = element->GetAttribute("length");
  int length;
  if (!extractValue(lengthData, length))
  {
    return false;
  }

  if (length == 0)
  {
    info->Set(key, nullptr, 0);
  }

  std::vector<ValueType> values;
  for (int i = 0; i < length; ++i)
  {
    std::ostringstream indexStr;
    indexStr << i;
    svtkXMLDataElement* valueElement =
      element->FindNestedElementWithNameAndAttribute("Value", "index", indexStr.str().c_str());
    if (!valueElement)
    {
      return false;
    }

    const char* valueData = valueElement->GetCharacterData();
    ValueType value;
    if (!extractValue(valueData, value))
    {
      return false;
    }
    values.push_back(value);
  }
  info->Set(key, &values[0], length);

  return true;
}

// Overload for string vector keys. There is no API for 'set all at once',
// so we'll need to use Append (which can't work with RequiredLength vector
// keys, hence the need for a specialization).
template <typename ValueType>
bool readVectorInfo(
  svtkInformationStringVectorKey* key, svtkInformation* info, svtkXMLDataElement* element)
{
  const char* lengthData = element->GetAttribute("length");
  int length;
  if (!extractValue(lengthData, length))
  {
    return false;
  }

  for (int i = 0; i < length; ++i)
  {
    std::ostringstream indexStr;
    indexStr << i;
    svtkXMLDataElement* valueElement =
      element->FindNestedElementWithNameAndAttribute("Value", "index", indexStr.str().c_str());
    if (!valueElement)
    {
      return false;
    }

    const char* valueData = valueElement->GetCharacterData();
    ValueType value;
    if (!extractValue(valueData, value))
    {
      return false;
    }
    info->Append(key, value);
  }

  return true;
}

} // end anon namespace
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
int svtkXMLReader::CreateInformationKey(svtkXMLDataElement* element, svtkInformation* info)
{
  const char* name = element->GetAttribute("name");
  const char* location = element->GetAttribute("location");
  if (!name || !location)
  {
    svtkWarningMacro("InformationKey element missing name and/or location "
                    "attributes.");
    return 0;
  }

  svtkInformationKey* key = svtkInformationKeyLookup::Find(name, location);
  if (!key)
  {
    svtkWarningMacro("Could not locate key " << location << "::" << name
                                            << ". Is the module in which it is defined linked?");
    return 0;
  }

  svtkInformationDoubleKey* dKey = nullptr;
  svtkInformationDoubleVectorKey* dvKey = nullptr;
  svtkInformationIdTypeKey* idKey = nullptr;
  svtkInformationIntegerKey* iKey = nullptr;
  svtkInformationIntegerVectorKey* ivKey = nullptr;
  svtkInformationStringKey* sKey = nullptr;
  svtkInformationStringVectorKey* svKey = nullptr;
  svtkInformationUnsignedLongKey* ulKey = nullptr;
  typedef svtkInformationQuadratureSchemeDefinitionVectorKey QuadDictKey;
  QuadDictKey* qdKey = nullptr;
  if ((dKey = svtkInformationDoubleKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<double>(dKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((dvKey = svtkInformationDoubleVectorKey::SafeDownCast(key)))
  {
    if (!readVectorInfo<double>(dvKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((idKey = svtkInformationIdTypeKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<svtkIdType>(idKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((iKey = svtkInformationIntegerKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<int>(iKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((ivKey = svtkInformationIntegerVectorKey::SafeDownCast(key)))
  {
    if (!readVectorInfo<int>(ivKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((sKey = svtkInformationStringKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<std::string>(sKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((svKey = svtkInformationStringVectorKey::SafeDownCast(key)))
  {
    if (!readVectorInfo<std::string>(svKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((ulKey = svtkInformationUnsignedLongKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<unsigned long>(ulKey, info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((qdKey = QuadDictKey::SafeDownCast(key)))
  { // Special case:
    if (!qdKey->RestoreState(info, element))
    {
      svtkErrorMacro("Error reading InformationKey element for "
        << location << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else
  {
    svtkErrorMacro("Could not deserialize information with key "
      << key->GetLocation() << "::" << key->GetName()
      << ": "
         "key type '"
      << key->GetClassName() << "' is not serializable.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
bool svtkXMLReader::ReadInformation(svtkXMLDataElement* infoRoot, svtkInformation* info)
{
  int numChildren = infoRoot->GetNumberOfNestedElements();
  for (int child = 0; child < numChildren; ++child)
  {
    svtkXMLDataElement* element = infoRoot->GetNestedElement(child);
    if (strncmp("InformationKey", element->GetName(), 14) != 0)
    { // Not an element we care about.
      continue;
    }

    if (!this->CreateInformationKey(element, info))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
int svtkXMLReader::GetLocalDataType(svtkXMLDataElement* da, int dataType)
{
  int idType;
  if (da->GetScalarAttribute("IdType", idType) && idType == 1)
  {
    // For now, only uses svtkIdTypeArray when the size of the data is
    // consistent with the SVTK build.
    // TODO create a smaller size array before converting to svtkIdTypeArray
    // TODO potentially truncate bigger size array into svtkIdTypeArray
    switch (dataType)
    {
      CaseIdTypeMacro(SVTK_SHORT, SVTK_SIZEOF_SHORT);
      CaseIdTypeMacro(SVTK_INT, SVTK_SIZEOF_INT);
      CaseIdTypeMacro(SVTK_LONG, SVTK_SIZEOF_LONG);
      CaseIdTypeMacro(SVTK_LONG_LONG, SVTK_SIZEOF_LONG_LONG);
      default:
        svtkWarningMacro("An array named " << da->GetAttribute("Name")
                                          << " was tagged as an IdType array with an invalid type."
                                             "The IdType tag has been ignored.");
        break;
    }
  }
  return dataType;
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkXMLReader::CreateArray(svtkXMLDataElement* da)
{
  int dataType = 0;
  if (!da->GetWordTypeAttribute("type", dataType))
  {
    return nullptr;
  }

  dataType = this->GetLocalDataType(da, dataType);
  svtkAbstractArray* array = svtkAbstractArray::CreateArray(dataType);

  array->SetName(da->GetAttribute("Name"));

  // if NumberOfComponents fails, we have 1 component
  int components = 1;

  if (da->GetScalarAttribute("NumberOfComponents", components))
  {
    array->SetNumberOfComponents(components);
  }

  // determine what component names have been saved in the file.
  const char* compName = nullptr;
  std::ostringstream buff;
  for (int i = 0; i < components && i < 10; ++i)
  {
    // get the component names
    buff << "ComponentName" << i;
    compName = da->GetAttribute(buff.str().c_str());
    if (compName)
    {
      // detected a component name, add it
      array->SetComponentName(i, compName);
      compName = nullptr;
    }
    buff.str("");
    buff.clear();
  }

  // Scan/load for svtkInformationKey data.
  int nElements = da->GetNumberOfNestedElements();
  for (int i = 0; i < nElements; ++i)
  {
    svtkXMLDataElement* eInfoKeyData = da->GetNestedElement(i);
    if (strcmp(eInfoKeyData->GetName(), "InformationKey") == 0)
    {
      svtkInformation* info = array->GetInformation();
      this->CreateInformationKey(eInfoKeyData, info);
    }
  }

  return array;
}

//----------------------------------------------------------------------------
int svtkXMLReader::CanReadFile(const char* name)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  svtksys::SystemTools::Stat_t fs;
  if (svtksys::SystemTools::Stat(name, &fs) != 0)
  {
    return 0;
  }

  // Test if the file with the given name is a SVTKFile with the given
  // type.
  svtkXMLFileReadTester* tester = svtkXMLFileReadTester::New();
  tester->SetFileName(name);

  int result = 0;
  if (tester->TestReadFile() && tester->GetFileDataType())
  {
    if (this->CanReadFileWithDataType(tester->GetFileDataType()))
    {
      result = 1;
    }
  }

  tester->Delete();
  return result;
}

//----------------------------------------------------------------------------
int svtkXMLReader::CanReadFileWithDataType(const char* dsname)
{
  return (dsname && strcmp(dsname, this->GetDataSetName()) == 0) ? 1 : 0;
}

//----------------------------------------------------------------------------
int svtkXMLReader::CanReadFileVersionString(const char* version)
{
  int major = 0;
  int minor = 0;
  ::ReadStringVersion(version, major, minor);
  return this->CanReadFileVersion(major, minor);
}

//----------------------------------------------------------------------------
int svtkXMLReader::IntersectExtents(int* extent1, int* extent2, int* result)
{
  if ((extent1[0] > extent2[1]) || (extent1[2] > extent2[3]) || (extent1[4] > extent2[5]) ||
    (extent1[1] < extent2[0]) || (extent1[3] < extent2[2]) || (extent1[5] < extent2[4]))
  {
    // No intersection of extents.
    return 0;
  }

  // Get the intersection of the extents.
  result[0] = this->Max(extent1[0], extent2[0]);
  result[1] = this->Min(extent1[1], extent2[1]);
  result[2] = this->Max(extent1[2], extent2[2]);
  result[3] = this->Min(extent1[3], extent2[3]);
  result[4] = this->Max(extent1[4], extent2[4]);
  result[5] = this->Min(extent1[5], extent2[5]);

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLReader::Min(int a, int b)
{
  return (a < b) ? a : b;
}

//----------------------------------------------------------------------------
int svtkXMLReader::Max(int a, int b)
{
  return (a > b) ? a : b;
}

//----------------------------------------------------------------------------
void svtkXMLReader::ComputePointDimensions(int* extent, int* dimensions)
{
  dimensions[0] = extent[1] - extent[0] + 1;
  dimensions[1] = extent[3] - extent[2] + 1;
  dimensions[2] = extent[5] - extent[4] + 1;
}

//----------------------------------------------------------------------------
void svtkXMLReader::ComputePointIncrements(int* extent, svtkIdType* increments)
{
  increments[0] = 1;
  increments[1] = increments[0] * (extent[1] - extent[0] + 1);
  increments[2] = increments[1] * (extent[3] - extent[2] + 1);
}

//----------------------------------------------------------------------------
void svtkXMLReader::ComputeCellDimensions(int* extent, int* dimensions)
{
  // For structured cells, axes that are empty of cells are treated as
  // having one cell when computing cell counts.  This allows cell
  // dimensions lower than 3.
  for (int a = 0; a < 3; ++a)
  {
    if (this->AxesEmpty[a] && extent[2 * a + 1] == extent[2 * a])
    {
      dimensions[a] = 1;
    }
    else
    {
      dimensions[a] = extent[2 * a + 1] - extent[2 * a];
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLReader::ComputeCellIncrements(int* extent, svtkIdType* increments)
{
  // For structured cells, axes that are empty of cells do not
  // contribute to the memory layout of cell data.
  svtkIdType nextIncrement = 1;
  for (int a = 0; a < 3; ++a)
  {
    if (this->AxesEmpty[a] && extent[2 * a + 1] == extent[2 * a])
    {
      increments[a] = 0;
    }
    else
    {
      increments[a] = nextIncrement;
      nextIncrement *= extent[2 * a + 1] - extent[2 * a];
    }
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkXMLReader::GetStartTuple(int* extent, svtkIdType* increments, int i, int j, int k)
{
  svtkIdType offset = (i - extent[0]) * increments[0];
  offset += (j - extent[2]) * increments[1];
  offset += (k - extent[4]) * increments[2];
  return offset;
}

//----------------------------------------------------------------------------
void svtkXMLReader::ReadAttributeIndices(svtkXMLDataElement* eDSA, svtkDataSetAttributes* dsa)
{
  // Setup attribute indices.
  for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
  {
    const char* attrName = svtkDataSetAttributes::GetAttributeTypeAsString(i);
    if (eDSA && eDSA->GetAttribute(attrName))
    {
      dsa->SetActiveAttribute(eDSA->GetAttribute(attrName), i);
    }
  }
}

//----------------------------------------------------------------------------
char** svtkXMLReader::CreateStringArray(int numStrings)
{
  char** strings = new char*[numStrings];
  for (int i = 0; i < numStrings; ++i)
  {
    strings[i] = nullptr;
  }
  return strings;
}

//----------------------------------------------------------------------------
void svtkXMLReader::DestroyStringArray(int numStrings, char** strings)
{
  for (int i = 0; i < numStrings; ++i)
  {
    delete[] strings[i];
  }
  delete[] strings;
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetDataArraySelections(svtkXMLDataElement* eDSA, svtkDataArraySelection* sel)
{
  if (!eDSA)
  {
    sel->SetArrays(nullptr, 0);
    return;
  }

  int numArrays = eDSA->GetNumberOfNestedElements();
  if (!numArrays)
  {
    sel->SetArrays(nullptr, 0);
    return;
  }

  for (int i = 0; i < numArrays; ++i)
  {
    svtkXMLDataElement* eNested = eDSA->GetNestedElement(i);
    const char* name = eNested->GetAttribute("Name");
    if (name)
    {
      sel->AddArray(name);
    }
    else
    {
      std::ostringstream s;
      s << "Array " << i;
      sel->AddArray(s.str().c_str());
    }
  }
}

//----------------------------------------------------------------------------
int svtkXMLReader::SetFieldDataInfo(
  svtkXMLDataElement* eDSA, int association, svtkIdType numTuples, svtkInformationVector*(&infoVector))
{
  if (!eDSA)
  {
    return 1;
  }

  char* attributeName[svtkDataSetAttributes::NUM_ATTRIBUTES];

  for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    const char* attrName = svtkDataSetAttributes::GetAttributeTypeAsString(i);
    const char* name = eDSA->GetAttribute(attrName);
    if (name)
    {
      attributeName[i] = new char[strlen(name) + 1];
      strcpy(attributeName[i], name);
    }
    else
    {
      attributeName[i] = nullptr;
    }
  }

  if (!infoVector)
  {
    infoVector = svtkInformationVector::New();
  }

  svtkInformation* info = nullptr;

  // Cycle through each data array
  for (int i = 0; i < eDSA->GetNumberOfNestedElements(); i++)
  {
    svtkXMLDataElement* eNested = eDSA->GetNestedElement(i);
    int components, dataType, activeFlag = 0;

    info = svtkInformation::New();
    info->Set(svtkDataObject::FIELD_ASSOCIATION(), association);
    info->Set(svtkDataObject::FIELD_NUMBER_OF_TUPLES(), numTuples);

    const char* name = eNested->GetAttribute("Name");
    if (!name)
    {
      this->InformationError = 1;
      break;
    }
    info->Set(svtkDataObject::FIELD_NAME(), name);

    // Search for matching attribute name
    for (int j = 0; j < svtkDataSetAttributes::NUM_ATTRIBUTES; j++)
    {
      if (attributeName[j] && !strcmp(name, attributeName[j]))
      {
        // set appropriate bit to indicate an active attribute type
        activeFlag |= 1 << j;
        break;
      }
    }

    if (!eNested->GetWordTypeAttribute("type", dataType))
    {
      this->InformationError = 1;
      break;
    }
    dataType = this->GetLocalDataType(eNested, dataType);
    info->Set(svtkDataObject::FIELD_ARRAY_TYPE(), dataType);

    if (eNested->GetScalarAttribute("NumberOfComponents", components))
    {
      info->Set(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), components);
    }
    else
    {
      info->Set(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), 1);
    }

    double range[2];
    if (eNested->GetScalarAttribute("RangeMin", range[0]) &&
      eNested->GetScalarAttribute("RangeMax", range[1]))
    {
      info->Set(svtkDataObject::FIELD_RANGE(), range, 2);
    }

    info->Set(svtkDataObject::FIELD_ACTIVE_ATTRIBUTE(), activeFlag);
    infoVector->Append(info);
    info->Delete();
  }

  for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    delete[] attributeName[i];
  }

  if (this->InformationError)
  {
    info->Delete();
    infoVector->Delete();
    infoVector = nullptr;
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLReader::PointDataArrayIsEnabled(svtkXMLDataElement* ePDA)
{
  const char* name = ePDA->GetAttribute("Name");
  return (name && this->PointDataArraySelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
int svtkXMLReader::CellDataArrayIsEnabled(svtkXMLDataElement* eCDA)
{
  const char* name = eCDA->GetAttribute("Name");
  return (name && this->CellDataArraySelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
void svtkXMLReader::SelectionModifiedCallback(svtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<svtkXMLReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
int svtkXMLReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* svtkXMLReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int svtkXMLReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
int svtkXMLReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* svtkXMLReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int svtkXMLReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
int svtkXMLReader::GetNumberOfColumnArrays()
{
  return this->ColumnArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* svtkXMLReader::GetColumnArrayName(int index)
{
  return this->ColumnArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int svtkXMLReader::GetColumnArrayStatus(const char* name)
{
  return this->ColumnArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetColumnArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->ColumnArraySelection->EnableArray(name);
  }
  else
  {
    this->ColumnArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
void svtkXMLReader::GetProgressRange(float* range)
{
  range[0] = this->ProgressRange[0];
  range[1] = this->ProgressRange[1];
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetProgressRange(const float range[2], int curStep, int numSteps)
{
  float stepSize = (range[1] - range[0]) / numSteps;
  this->ProgressRange[0] = range[0] + stepSize * curStep;
  this->ProgressRange[1] = range[0] + stepSize * (curStep + 1);
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetProgressRange(const float range[2], int curStep, const float* fractions)
{
  float width = range[1] - range[0];
  this->ProgressRange[0] = range[0] + fractions[curStep] * width;
  this->ProgressRange[1] = range[0] + fractions[curStep + 1] * width;
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void svtkXMLReader::UpdateProgressDiscrete(float progress)
{
  if (!this->AbortExecute)
  {
    // Round progress to nearest 100th.
    float rounded = static_cast<float>(int((progress * 100) + 0.5f)) / 100.f;
    if (this->GetProgress() != rounded)
    {
      this->UpdateProgress(rounded);
    }
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXMLReader::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->CurrentOutputInformation = outputVector->GetInformationObject(0);
  // FIXME This piece of code should be rewritten to handle at the same
  // time Pieces and TimeSteps. The REQUEST_DATA_NOT_GENERATED should
  // ideally be changed during execution, so that allocation still
  // happen when needed but can be skipped in demand (when doing
  // timesteps)
  if (this->NumberOfTimeSteps &&
    request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_NOT_GENERATED()))
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(svtkDemandDrivenPipeline::DATA_NOT_GENERATED(), 1);
    this->CurrentOutputInformation = nullptr;
    return 1;
  }
  // END FIXME

  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestData(request, inputVector, outputVector);
    this->CurrentOutputInformation = nullptr;
    return retVal;
  }

  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    int retVal = this->RequestDataObject(request, inputVector, outputVector);
    this->CurrentOutputInformation = nullptr;
    return retVal;
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    int retVal = this->RequestInformation(request, inputVector, outputVector);
    this->CurrentOutputInformation = nullptr;
    return retVal;
  }

  int retVal = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  this->CurrentOutputInformation = nullptr;
  return retVal;
}

//----------------------------------------------------------------------------
void svtkXMLReader::SetNumberOfTimeSteps(int num)
{
  if (num && (this->NumberOfTimeSteps != num))
  {
    this->NumberOfTimeSteps = num;
    delete[] this->TimeSteps;
    // Reallocate a buffer large enough
    this->TimeSteps = new int[num];
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkXMLReader::IsTimeStepInArray(int timestep, int* timesteps, int length)
{
  for (int i = 0; i < length; i++)
  {
    if (timesteps[i] == timestep)
    {
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkXMLReader::GetCurrentOutput()
{
  return this->CurrentOutput;
}

//----------------------------------------------------------------------------
svtkInformation* svtkXMLReader::GetCurrentOutputInformation()
{
  return this->CurrentOutputInformation;
}
