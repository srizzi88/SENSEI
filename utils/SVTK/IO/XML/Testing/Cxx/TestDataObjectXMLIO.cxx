#include <svtkBitArray.h>
#include <svtkCellData.h>
#include <svtkCubeSource.h>
#include <svtkDataObjectWriter.h>
#include <svtkDelaunay3D.h>
#include <svtkDirectedGraph.h>
#include <svtkEdgeListIterator.h>
#include <svtkFieldData.h>
#include <svtkFloatArray.h>
#include <svtkGraph.h>
#include <svtkImageData.h>
#include <svtkImageNoiseSource.h>
#include <svtkInformation.h>
#include <svtkIntArray.h>
#include <svtkMutableDirectedGraph.h>
#include <svtkNew.h>
#include <svtkPermuteOptions.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkRandomGraphSource.h>
#include <svtkRectilinearGrid.h>
#include <svtkSmartPointer.h>
#include <svtkStructuredGrid.h>
#include <svtkTable.h>
#include <svtkTesting.h>
#include <svtkTree.h>
#include <svtkUndirectedGraph.h>
#include <svtkUniformGrid.h>
#include <svtkUnstructuredGrid.h>
#include <svtkVariant.h>
#include <svtkXMLDataSetWriter.h>
#include <svtkXMLGenericDataObjectReader.h>

#include <sstream>
#include <string>

// Serializable keys to test:
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationStringVectorKey.h"
#include "svtkInformationUnsignedLongKey.h"

namespace
{

static svtkNew<svtkTesting> TestingData; // For temporary path

static const char* BIT_ARRAY_NAME = "BitArray";
static const char* IDTYPE_ARRAY_NAME = "IdTypeArray";

static svtkInformationDoubleKey* TestDoubleKey =
  svtkInformationDoubleKey::MakeKey("Double", "XMLTestKey");
// Test RequiredLength keys. DoubleVector must have Length() == 3
static svtkInformationDoubleVectorKey* TestDoubleVectorKey =
  svtkInformationDoubleVectorKey::MakeKey("DoubleVector", "XMLTestKey", 3);
static svtkInformationIdTypeKey* TestIdTypeKey =
  svtkInformationIdTypeKey::MakeKey("IdType", "XMLTestKey");
static svtkInformationIntegerKey* TestIntegerKey =
  svtkInformationIntegerKey::MakeKey("Integer", "XMLTestKey");
static svtkInformationIntegerVectorKey* TestIntegerVectorKey =
  svtkInformationIntegerVectorKey::MakeKey("IntegerVector", "XMLTestKey");
static svtkInformationStringKey* TestStringKey =
  svtkInformationStringKey::MakeKey("String", "XMLTestKey");
static svtkInformationStringVectorKey* TestStringVectorKey =
  svtkInformationStringVectorKey::MakeKey("StringVector", "XMLTestKey");
static svtkInformationUnsignedLongKey* TestUnsignedLongKey =
  svtkInformationUnsignedLongKey::MakeKey("UnsignedLong", "XMLTestKey");

bool stringEqual(const std::string& expect, const std::string& actual)
{
  if (expect != actual)
  {
    std::cerr << "Strings do not match! Expected: '" << expect << "', got: '" << actual << "'.\n";
    return false;
  }
  return true;
}

bool stringEqual(const std::string& expect, const char* actual)
{
  return stringEqual(expect, std::string(actual ? actual : ""));
}

template <typename T>
bool compareValues(const std::string& desc, T expect, T actual)
{
  if (expect != actual)
  {
    std::cerr << "Failed comparison for '" << desc << "'. Expected '" << expect << "', got '"
              << actual << "'.\n";
    return false;
  }
  return true;
}

// Generate a somewhat interesting bit pattern for the test bit arrays:
int bitArrayFunc(svtkIdType i)
{
  return (i + (i / 2) + (i / 3) + (i / 5) + (i / 7) + (i / 11)) % 2;
}

void fillBitArray(svtkBitArray* bits)
{
  bits->SetName(BIT_ARRAY_NAME);
  bits->SetNumberOfComponents(4);
  bits->SetNumberOfTuples(100);
  svtkIdType numValues = bits->GetNumberOfValues();
  for (svtkIdType i = 0; i < numValues; ++i)
  {
    bits->SetValue(i, bitArrayFunc(i));
  }
}

bool validateBitArray(svtkAbstractArray* abits)
{
  if (!abits)
  {
    std::cerr << "Bit array not found.\n";
    return false;
  }

  svtkBitArray* bits = svtkBitArray::SafeDownCast(abits);
  if (!bits)
  {
    std::cerr << "Bit Array is incorrect type: " << abits->GetClassName() << ".\n";
    return false;
  }

  svtkIdType numValues = bits->GetNumberOfValues();
  if (numValues != 400)
  {
    std::cerr << "Expected 400 values in bit array, got: " << numValues << "\n";
    return false;
  }

  for (svtkIdType i = 0; i < numValues; ++i)
  {
    int expected = bitArrayFunc(i);
    int actual = bits->GetValue(i);
    if (actual != expected)
    {
      std::cerr << "Bit array invalid - expected " << expected << " , got " << actual
                << " for valueIdx " << i << ".\n";
      return false;
    }
  }

  return true;
}

void fillIdTypeArray(svtkIdTypeArray* ids)
{
  ids->SetName(IDTYPE_ARRAY_NAME);
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(100);
  for (svtkIdType i = 0; i < 100; ++i)
  {
    ids->SetValue(i, i);
  }
}

bool validateIdTypeArray(svtkAbstractArray* aids)
{
  if (!aids)
  {
    std::cerr << "IdType array not found.\n";
    return false;
  }

  // Ignore the case when the aids is of smaller type than svtkIdType size
  // As this is a possible case when saving data as 32bit with 64bit ids.
  if (aids->GetDataTypeSize() < SVTK_SIZEOF_ID_TYPE)
  {
    return true;
  }

  svtkIdTypeArray* ids = svtkIdTypeArray::SafeDownCast(aids);
  if (!ids)
  {
    std::cerr << "idType Array is of incorrect type: " << aids->GetClassName() << ".\n";
    return false;
  }

  svtkIdType numValues = ids->GetNumberOfValues();
  if (numValues != 100)
  {
    std::cerr << "Expected 100 values in id array, got: " << numValues << "\n";
    return false;
  }

  for (svtkIdType i = 0; i < numValues; ++i)
  {
    if (ids->GetValue(i) != i)
    {
      std::cerr << "id array invalid - expected " << i << " , got " << ids->GetValue(i)
                << " for valueIdx " << i << ".\n";
      return false;
    }
  }

  return true;
}

void InitializeDataCommon(svtkDataObject* data)
{
  svtkFieldData* fd = data->GetFieldData();
  if (!fd)
  {
    fd = svtkFieldData::New();
    data->SetFieldData(fd);
    fd->FastDelete();
  }

  // Add a dummy array to test component name and information key serialization.
  svtkNew<svtkFloatArray> array;
  array->SetName("Test Array");
  fd->AddArray(array);
  array->SetNumberOfComponents(3);
  array->SetComponentName(0, "Component 0 name");
  array->SetComponentName(1, "Component 1 name");
  array->SetComponentName(2, "Component 2 name");

  // Test information keys that can be serialized
  svtkInformation* info = array->GetInformation();
  info->Set(TestDoubleKey, 1.0);
  // Setting from an array, since keys with RequiredLength cannot use Append.
  double doubleVecData[3] = { 1., 90., 260. };
  info->Set(TestDoubleVectorKey, doubleVecData, 3);
  info->Set(TestIdTypeKey, 5);
  info->Set(TestIntegerKey, 408);
  info->Append(TestIntegerVectorKey, 1);
  info->Append(TestIntegerVectorKey, 5);
  info->Append(TestIntegerVectorKey, 45);
  info->Set(TestStringKey, "Test String!\nLine2");
  info->Append(TestStringVectorKey, "First");
  info->Append(TestStringVectorKey, "Second (with whitespace!)");
  info->Append(TestStringVectorKey, "Third (with\nnewline!)");
  info->Set(TestUnsignedLongKey, 9);

  // Ensure that bit arrays are handled properly (#17197)
  svtkNew<svtkBitArray> bits;
  fillBitArray(bits);
  fd->AddArray(bits);

  // Ensure that idType arrays are handled properly (#17421)
  svtkNew<svtkIdTypeArray> ids;
  fillIdTypeArray(ids);
  fd->AddArray(ids);
}

bool CompareDataCommon(svtkDataObject* data)
{
  svtkFieldData* fd = data->GetFieldData();
  if (!fd)
  {
    std::cerr << "Field data object missing.\n";
    return false;
  }

  svtkDataArray* array = fd->GetArray("Test Array");
  if (!array)
  {
    std::cerr << "Missing testing array from field data.\n";
    return false;
  }

  if (array->GetNumberOfComponents() != 3)
  {
    std::cerr << "Test array expected to have 3 components, has " << array->GetNumberOfComponents()
              << std::endl;
    return false;
  }

  if (!array->GetComponentName(0) ||
    (strcmp("Component 0 name", array->GetComponentName(0)) != 0) || !array->GetComponentName(1) ||
    (strcmp("Component 1 name", array->GetComponentName(1)) != 0) || !array->GetComponentName(2) ||
    (strcmp("Component 2 name", array->GetComponentName(2)) != 0))
  {
    std::cerr << "Incorrect component names on test array.\n";
    return false;
  }

  svtkInformation* info = array->GetInformation();
  if (!info)
  {
    std::cerr << "Missing array information.\n";
    return false;
  }

  if (!compareValues("double key", 1., info->Get(TestDoubleKey)) ||
    !compareValues("double vector key length", 3, info->Length(TestDoubleVectorKey)) ||
    !compareValues("double vector key @0", 1., info->Get(TestDoubleVectorKey, 0)) ||
    !compareValues("double vector key @1", 90., info->Get(TestDoubleVectorKey, 1)) ||
    !compareValues("double vector key @2", 260., info->Get(TestDoubleVectorKey, 2)) ||
    !compareValues<svtkIdType>("idtype key", 5, info->Get(TestIdTypeKey)) ||
    !compareValues("integer key", 408, info->Get(TestIntegerKey)) ||
    !compareValues("integer vector key length", 3, info->Length(TestIntegerVectorKey)) ||
    !compareValues("integer vector key @0", 1, info->Get(TestIntegerVectorKey, 0)) ||
    !compareValues("integer vector key @1", 5, info->Get(TestIntegerVectorKey, 1)) ||
    !compareValues("integer vector key @2", 45, info->Get(TestIntegerVectorKey, 2)) ||
    !stringEqual("Test String!\nLine2", info->Get(TestStringKey)) ||
    !compareValues("string vector key length", 3, info->Length(TestStringVectorKey)) ||
    !stringEqual("First", info->Get(TestStringVectorKey, 0)) ||
    !stringEqual("Second (with whitespace!)", info->Get(TestStringVectorKey, 1)) ||
    !stringEqual("Third (with\nnewline!)", info->Get(TestStringVectorKey, 2)) ||
    !compareValues("unsigned long key", 9ul, info->Get(TestUnsignedLongKey)))
  {
    return false;
  }

  if (!validateBitArray(fd->GetAbstractArray(BIT_ARRAY_NAME)))
  {
    return false;
  }

  if (!validateIdTypeArray(fd->GetAbstractArray(IDTYPE_ARRAY_NAME)))
  {
    return false;
  }

  return true;
}

void InitializeData(svtkImageData* Data)
{
  svtkImageNoiseSource* const source = svtkImageNoiseSource::New();
  source->SetWholeExtent(0, 15, 0, 15, 0, 0);
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();

  InitializeDataCommon(Data);
}

bool CompareData(svtkImageData* Output, svtkImageData* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }

  if (memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  const int point_count =
    Input->GetDimensions()[0] * Input->GetDimensions()[1] * Input->GetDimensions()[2];
  for (int point = 0; point != point_count; ++point)
  {
    if (memcmp(Input->GetPoint(point), Output->GetPoint(point), 3 * sizeof(double)))
      return false;
  }

  return true;
}

void InitializeData(svtkPolyData* Data)
{
  svtkCubeSource* const source = svtkCubeSource::New();
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();

  InitializeDataCommon(Data);
}

bool CompareData(svtkPolyData* Output, svtkPolyData* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }
  if (Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if (Input->GetNumberOfPolys() != Output->GetNumberOfPolys())
    return false;

  return true;
}

void InitializeData(svtkRectilinearGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
  InitializeDataCommon(Data);
}

bool CompareData(svtkRectilinearGrid* Output, svtkRectilinearGrid* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }
  if (memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  return true;
}

void InitializeData(svtkUniformGrid* Data)
{
  InitializeData(static_cast<svtkImageData*>(Data));
  InitializeDataCommon(Data);
}

void InitializeData(svtkUnstructuredGrid* Data)
{
  svtkCubeSource* const source = svtkCubeSource::New();
  svtkDelaunay3D* const delaunay = svtkDelaunay3D::New();
  delaunay->AddInputConnection(source->GetOutputPort());
  delaunay->Update();

  Data->ShallowCopy(delaunay->GetOutput());

  delaunay->Delete();
  source->Delete();

  InitializeDataCommon(Data);
}

bool CompareData(svtkUnstructuredGrid* Output, svtkUnstructuredGrid* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }
  if (Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if (Input->GetNumberOfCells() != Output->GetNumberOfCells())
    return false;

  return true;
}

//------------------------------------------------------------------------------
// Determine the data object read as member Type for a given WriterDataObjectT.
template <typename WriterDataObjectT>
struct GetReaderDataObjectType
{
  using Type = WriterDataObjectT;
};

// Specialize for svtkUniformGrid --> svtkImageData
template <>
struct GetReaderDataObjectType<svtkUniformGrid>
{
  using Type = svtkImageData;
};

class WriterConfig : public svtkPermuteOptions<svtkXMLDataSetWriter>
{
public:
  WriterConfig()
  {
    this->AddOptionValues("ByteOrder", &svtkXMLDataObjectWriter::SetByteOrder, "BigEndian",
      svtkXMLWriter::BigEndian, "LittleEndian", svtkXMLWriter::LittleEndian);
    this->AddOptionValues("HeaderType", &svtkXMLDataObjectWriter::SetHeaderType, "32Bit",
      svtkXMLWriter::UInt32, "64Bit", svtkXMLWriter::UInt64);
    this->AddOptionValues("CompressorType", &svtkXMLDataObjectWriter::SetCompressorType, "NONE",
      svtkXMLWriter::NONE, "ZLIB", svtkXMLWriter::ZLIB, "LZ4", svtkXMLWriter::LZ4);
    this->AddOptionValues("DataMode", &svtkXMLDataObjectWriter::SetDataMode, "Ascii",
      svtkXMLWriter::Ascii, "Binary", svtkXMLWriter::Binary, "Appended", svtkXMLWriter::Appended);

    // Calling svtkXMLWriter::SetIdType throws an Error while requesting 64 bit
    // ids if this option isn't set:
    this->AddOptionValue(
      "IdType", &svtkXMLDataObjectWriter::SetIdType, "32Bit", svtkXMLWriter::Int32);
#ifdef SVTK_USE_64BIT_IDS
    this->AddOptionValue(
      "IdType", &svtkXMLDataObjectWriter::SetIdType, "64Bit", svtkXMLWriter::Int64);
#endif
  }
};

//------------------------------------------------------------------------------
// Main test function for a given data type and writer configuration.
template <typename WriterDataObjectT>
bool TestDataObjectXMLSerialization(const WriterConfig& writerConfig)
{
  using ReaderDataObjectT = typename GetReaderDataObjectType<WriterDataObjectT>::Type;

  WriterDataObjectT* const output_data = WriterDataObjectT::New();
  InitializeData(output_data);

  std::ostringstream filename;
  filename << TestingData->GetTempDirectory() << "/" << output_data->GetClassName() << "-"
           << writerConfig.GetCurrentPermutationName();

  svtkXMLDataSetWriter* const writer = svtkXMLDataSetWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename.str().c_str());
  writerConfig.ApplyCurrentPermutation(writer);
  writer->Write();
  writer->Delete();

  svtkXMLGenericDataObjectReader* const reader = svtkXMLGenericDataObjectReader::New();
  reader->SetFileName(filename.str().c_str());
  reader->Update();

  svtkDataObject* obj = reader->GetOutput();
  ReaderDataObjectT* input_data = ReaderDataObjectT::SafeDownCast(obj);
  if (!input_data)
  {
    reader->Delete();
    output_data->Delete();
    return false;
  }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  if (!result)
  {
    std::cerr << "Comparison failed. Filename: " << filename.str() << "\n";
  }

  return result;
}

//------------------------------------------------------------------------------
// Test all permutations of the writer configuration with a given data type.
template <typename WriterDataObjectT>
bool TestWriterPermutations()
{
  bool result = true;
  WriterConfig config;

  config.InitPermutations();
  while (!config.IsDoneWithPermutations())
  {
    { // Some progress/debugging output:
      std::string testName;
      svtkNew<WriterDataObjectT> dummy;
      std::ostringstream tmp;
      tmp << dummy->GetClassName() << " [" << config.GetCurrentPermutationName() << "]";
      testName = tmp.str();
      std::cerr << "Testing: " << testName << "..." << std::endl;
    }

    if (!TestDataObjectXMLSerialization<WriterDataObjectT>(config))
    {
      std::cerr << "Failed.\n\n";
      result = false;
    }

    config.GoToNextPermutation();
  }

  return result;
}

} // end anon namespace

int TestDataObjectXMLIO(int argc, char* argv[])
{
  TestingData->AddArguments(argc, argv);

  int result = 0;

  if (!TestWriterPermutations<svtkImageData>())
  {
    result = 1;
  }
  if (!TestWriterPermutations<svtkUniformGrid>())
  {
    // note that the current output from serializing a svtkUniformGrid
    // is a svtkImageData. this is the same as writing out a
    // svtkUniformGrid using svtkXMLImageDataWriter.
    result = 1;
  }
  if (!TestWriterPermutations<svtkPolyData>())
  {
    result = 1;
  }
  if (!TestWriterPermutations<svtkRectilinearGrid>())
  {
    result = 1;
  }
  //  if(!TestWriterPermutations<svtkStructuredGrid>())
  //    {
  //    result = 1;
  //    }
  if (!TestWriterPermutations<svtkUnstructuredGrid>())
  {
    result = 1;
  }

  return result;
}
