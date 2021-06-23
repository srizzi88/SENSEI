/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkFieldDataSerializer.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkFieldDataSerializer.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkMultiProcessStream.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkStructuredData.h"
#include "svtkStructuredExtent.h"

#include <cassert> // For assert()
#include <cstring> // For memcpy

svtkStandardNewMacro(svtkFieldDataSerializer);

//------------------------------------------------------------------------------
svtkFieldDataSerializer::svtkFieldDataSerializer() = default;

//------------------------------------------------------------------------------
svtkFieldDataSerializer::~svtkFieldDataSerializer() = default;

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::SerializeMetaData(
  svtkFieldData* fieldData, svtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    svtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  // STEP 1: Loop through each array and write the metadata
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    svtkDataArray* dataArray = fieldData->GetArray(array);
    assert("pre: data array should not be nullptr!" && (dataArray != nullptr));

    int dataType = dataArray->GetDataType();
    int numComp = dataArray->GetNumberOfComponents();
    int numTuples = dataArray->GetNumberOfTuples();

    // serialize array information
    bytestream << dataType << numTuples << numComp;
    bytestream << std::string(dataArray->GetName());
  } // END for all arrays
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::DeserializeMetaData(svtkMultiProcessStream& bytestream,
  svtkStringArray* names, svtkIntArray* datatypes, svtkIntArray* dimensions)
{
  if (bytestream.Empty())
  {
    svtkGenericWarningMacro("ByteStream is empty");
    return;
  }

  if ((names == nullptr) || (datatypes == nullptr) || (dimensions == nullptr))
  {
    svtkGenericWarningMacro("ERROR: caller must pre-allocation names/datatypes/dimensions!");
    return;
  }

  // STEP 0: Extract the number of arrays
  int NumberOfArrays;
  bytestream >> NumberOfArrays;
  if (NumberOfArrays == 0)
  {
    return;
  }

  // STEP 1: Allocate output data-structures
  names->SetNumberOfValues(NumberOfArrays);
  datatypes->SetNumberOfValues(NumberOfArrays);
  dimensions->SetNumberOfComponents(2);
  dimensions->SetNumberOfTuples(NumberOfArrays);

  std::string* namesPtr = names->GetPointer(0);
  int* datatypesPtr = datatypes->GetPointer(0);
  int* dimensionsPtr = dimensions->GetPointer(0);

  // STEP 2: Extract metadata for each array in corresponding output arrays
  for (int arrayIdx = 0; arrayIdx < NumberOfArrays; ++arrayIdx)
  {
    bytestream >> datatypesPtr[arrayIdx] >> dimensionsPtr[arrayIdx * 2] >>
      dimensionsPtr[arrayIdx * 2 + 1] >> namesPtr[arrayIdx];
  } // END for all arrays
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::Serialize(svtkFieldData* fieldData, svtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    svtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if (fieldData->GetNumberOfArrays() == 0)
  {
    return;
  }

  // STEP 1: Loop through each array and serialize its metadata
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    svtkDataArray* dataArray = fieldData->GetArray(array);
    svtkFieldDataSerializer::SerializeDataArray(dataArray, bytestream);
  } // END for all arrays
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::SerializeTuples(
  svtkIdList* tupleIds, svtkFieldData* fieldData, svtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    svtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if (fieldData->GetNumberOfArrays() == 0)
  {
    return;
  }

  // STEP 1: Loop through each array, extract the data on the selected tuples
  // and serialize it
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    svtkDataArray* dataArray = fieldData->GetArray(array);

    // STEP 2: For each array extract only the selected tuples, i.e., a subset
    svtkDataArray* subSet = svtkFieldDataSerializer::ExtractSelectedTuples(tupleIds, dataArray);
    assert("pre: subset array is nullptr!" && (subSet != nullptr));

    // STEP 3: Serialize only a subset of the data
    svtkFieldDataSerializer::SerializeDataArray(subSet, bytestream);
    subSet->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::SerializeSubExtent(
  int subext[6], int gridExtent[6], svtkFieldData* fieldData, svtkMultiProcessStream& bytestream)
{
  if (fieldData == nullptr)
  {
    svtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  // STEP 0: Write the number of arrays
  bytestream << fieldData->GetNumberOfArrays();

  if (fieldData->GetNumberOfArrays() == 0)
  {
    return;
  }

  // STEP 1: Loop through each array, extract the data within the subext
  // and serialize it
  for (int array = 0; array < fieldData->GetNumberOfArrays(); ++array)
  {
    svtkDataArray* dataArray = fieldData->GetArray(array);

    // STEP 2: Extract the data within the requested sub-extent
    svtkDataArray* subSet =
      svtkFieldDataSerializer::ExtractSubExtentData(subext, gridExtent, dataArray);
    assert("pre: subset array is nullptr!" && (subSet != nullptr));

    // STEP 3: Serialize only a subset of the data
    svtkFieldDataSerializer::SerializeDataArray(subSet, bytestream);
    subSet->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::DeSerializeToSubExtent(
  int subext[6], int gridExtent[6], svtkFieldData* fieldData, svtkMultiProcessStream& bytestream)
{
  assert("pre: sub-extent outside grid-extent" && svtkStructuredExtent::Smaller(subext, gridExtent));

  if (fieldData == nullptr)
  {
    svtkGenericWarningMacro("Field data is nullptr!");
    return;
  }

  int numArrays = 0;
  bytestream >> numArrays;
  assert("post: numArrays mismatch!" && (numArrays == fieldData->GetNumberOfArrays()));

  int ijk[3];
  for (int array = 0; array < numArrays; ++array)
  {
    svtkDataArray* dataArray = nullptr;
    svtkFieldDataSerializer::DeserializeDataArray(bytestream, dataArray);
    assert("post: dataArray is nullptr!" && (dataArray != nullptr));
    assert("post: fieldData does not have array!" && fieldData->HasArray(dataArray->GetName()));

    svtkDataArray* targetArray = fieldData->GetArray(dataArray->GetName());
    assert("post: ncomp mismatch!" &&
      (dataArray->GetNumberOfComponents() == targetArray->GetNumberOfComponents()));

    for (ijk[0] = subext[0]; ijk[0] <= subext[1]; ++ijk[0])
    {
      for (ijk[1] = subext[2]; ijk[1] <= subext[3]; ++ijk[1])
      {
        for (ijk[2] = subext[4]; ijk[2] <= subext[5]; ++ijk[2])
        {
          svtkIdType sourceIdx = svtkStructuredData::ComputePointIdForExtent(subext, ijk);
          assert("post: sourceIdx out-of-bounds!" && (sourceIdx >= 0) &&
            (sourceIdx < dataArray->GetNumberOfTuples()));

          svtkIdType targetIdx = svtkStructuredData::ComputePointIdForExtent(gridExtent, ijk);
          assert("post: targetIdx out-of-bounds!" && (targetIdx >= 0) &&
            (targetIdx < targetArray->GetNumberOfTuples()));

          targetArray->SetTuple(targetIdx, sourceIdx, dataArray);
        } // END for all k
      }   // END for all j
    }     // END for all i

    dataArray->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
svtkDataArray* svtkFieldDataSerializer::ExtractSubExtentData(
  int subext[6], int gridExtent[6], svtkDataArray* inputDataArray)
{
  if (inputDataArray == nullptr)
  {
    svtkGenericWarningMacro("input data array is nullptr!");
    return nullptr;
  }

  // STEP 0: Acquire structured data description, i.e, XY_PLANE, XYZ_GRID etc.
  int description = svtkStructuredData::GetDataDescriptionFromExtent(gridExtent);

  // STEP 1: Allocate subset array
  svtkDataArray* subSetArray = svtkDataArray::CreateDataArray(inputDataArray->GetDataType());
  subSetArray->SetName(inputDataArray->GetName());
  subSetArray->SetNumberOfComponents(inputDataArray->GetNumberOfComponents());
  subSetArray->SetNumberOfTuples(svtkStructuredData::GetNumberOfPoints(subext, description));

  int ijk[3];
  for (ijk[0] = subext[0]; ijk[0] <= subext[1]; ++ijk[0])
  {
    for (ijk[1] = subext[2]; ijk[1] <= subext[3]; ++ijk[1])
    {
      for (ijk[2] = subext[4]; ijk[2] <= subext[5]; ++ijk[2])
      {
        // Compute the source index from the grid extent. Note, this could be
        // a cell index if the incoming gridExtent and subext are cell extents.
        svtkIdType sourceIdx =
          svtkStructuredData::ComputePointIdForExtent(gridExtent, ijk, description);
        assert("pre: source index is out-of-bounds" && (sourceIdx >= 0) &&
          (sourceIdx < inputDataArray->GetNumberOfTuples()));

        // Compute the target index in the subset array. Likewise, this could be
        // either a cell index or a node index depending on what gridExtent or
        // subext represent.
        svtkIdType targetIdx = svtkStructuredData::ComputePointIdForExtent(subext, ijk, description);
        assert("pre: target index is out-of-bounds" && (targetIdx >= 0) &&
          (targetIdx < subSetArray->GetNumberOfTuples()));

        subSetArray->SetTuple(targetIdx, sourceIdx, inputDataArray);
      } // END for all k
    }   // END for all j
  }     // END for all i

  return (subSetArray);
}

//------------------------------------------------------------------------------
svtkDataArray* svtkFieldDataSerializer::ExtractSelectedTuples(
  svtkIdList* tupleIds, svtkDataArray* inputDataArray)
{
  svtkDataArray* subSetArray = svtkDataArray::CreateDataArray(inputDataArray->GetDataType());
  subSetArray->SetName(inputDataArray->GetName());
  subSetArray->SetNumberOfComponents(inputDataArray->GetNumberOfComponents());
  subSetArray->SetNumberOfTuples(tupleIds->GetNumberOfIds());

  svtkIdType idx = 0;
  for (; idx < tupleIds->GetNumberOfIds(); ++idx)
  {
    svtkIdType tupleIdx = tupleIds->GetId(idx);
    assert("pre: tuple ID is out-of bounds" && (tupleIdx >= 0) &&
      (tupleIdx < inputDataArray->GetNumberOfTuples()));

    subSetArray->SetTuple(idx, tupleIdx, inputDataArray);
  } // END for all tuples to extract
  return (subSetArray);
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::SerializeDataArray(
  svtkDataArray* dataArray, svtkMultiProcessStream& bytestream)
{
  if (dataArray == nullptr)
  {
    svtkGenericWarningMacro("data array is nullptr!");
    return;
  }

  // STEP 0: Serialize array information
  int dataType = dataArray->GetDataType();
  int numComp = dataArray->GetNumberOfComponents();
  int numTuples = dataArray->GetNumberOfTuples();

  // serialize array information
  bytestream << dataType << numTuples << numComp;
  bytestream << std::string(dataArray->GetName());

  // STEP 1: Push the raw data into the bytestream
  // TODO: Add more cases for more datatypes here (?)
  unsigned int size = numComp * numTuples;
  if (dataArray->IsA("svtkFloatArray"))
  {
    bytestream.Push(static_cast<svtkFloatArray*>(dataArray)->GetPointer(0), size);
  }
  else if (dataArray->IsA("svtkDoubleArray"))
  {
    bytestream.Push(static_cast<svtkDoubleArray*>(dataArray)->GetPointer(0), size);
  }
  else if (dataArray->IsA("svtkIntArray"))
  {
    bytestream.Push(static_cast<svtkIntArray*>(dataArray)->GetPointer(0), size);
  }
  else if (dataArray->IsA("svtkIdTypeArray"))
  {
    bytestream.Push(static_cast<svtkIdTypeArray*>(dataArray)->GetPointer(0), size);
  }
  else
  {
    assert("ERROR: cannot serialize data of given type" && false);
    cerr << "Cannot serialize data of type=" << dataArray->GetDataType() << endl;
  }
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::Deserialize(svtkMultiProcessStream& bytestream, svtkFieldData* fieldData)
{
  if (fieldData == nullptr)
  {
    svtkGenericWarningMacro("FieldData is nullptr!");
    return;
  }

  if (bytestream.Empty())
  {
    svtkGenericWarningMacro("Bytestream is empty!");
    return;
  }

  // STEP 0: Get the number of arrays
  int numberOfArrays = 0;
  bytestream >> numberOfArrays;

  if (numberOfArrays == 0)
  {
    return;
  }

  // STEP 1: Loop and deserialize each array
  for (int array = 0; array < numberOfArrays; ++array)
  {
    svtkDataArray* dataArray = nullptr;
    svtkFieldDataSerializer::DeserializeDataArray(bytestream, dataArray);
    assert("post: deserialized data array should not be nullptr!" && (dataArray != nullptr));
    fieldData->AddArray(dataArray);
    dataArray->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
void svtkFieldDataSerializer::DeserializeDataArray(
  svtkMultiProcessStream& bytestream, svtkDataArray*& dataArray)
{
  if (bytestream.Empty())
  {
    svtkGenericWarningMacro("Bytestream is empty!");
    return;
  }

  // STEP 0: Deserialize array information
  int dataType, numTuples, numComp;
  std::string name;

  bytestream >> dataType >> numTuples >> numComp >> name;
  assert("pre: numComp >= 1" && (numComp >= 1));

  // STEP 1: Construct svtkDataArray object
  dataArray = svtkDataArray::CreateDataArray(dataType);
  dataArray->SetNumberOfComponents(numComp);
  dataArray->SetNumberOfTuples(numTuples);
  dataArray->SetName(name.c_str());

  // STEP 2: Extract raw data to svtkDataArray
  // TODO: Add more cases for more datatypes here (?)
  unsigned int size = numTuples * numComp;
  void* rawPtr = dataArray->GetVoidPointer(0);
  assert("pre: raw pointer is nullptr!" && (rawPtr != nullptr));
  switch (dataType)
  {
    case SVTK_FLOAT:
    {
      float* data = static_cast<float*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    case SVTK_DOUBLE:
    {
      double* data = static_cast<double*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    case SVTK_INT:
    {
      int* data = static_cast<int*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    case SVTK_ID_TYPE:
    {
      svtkIdType* data = static_cast<svtkIdType*>(rawPtr);
      bytestream.Pop(data, size);
    }
    break;
    default:
      assert("ERROR: cannot serialize data of given type" && false);
      cerr << "Cannot serialize data of type=" << dataArray->GetDataType() << endl;
  }
}
