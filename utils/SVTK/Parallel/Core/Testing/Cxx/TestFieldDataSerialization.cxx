/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFieldDataSerialization.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestFieldDataSerialization.cxx -- Test for svtkFieldDataSerializer
//
// .SECTION Description
//  Simple tests for serialization/de-serialization of field data.

#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFieldDataSerializer.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkMathUtilities.h"
#include "svtkMultiProcessStream.h"
#include "svtkPointData.h"
#include "svtkStringArray.h"

#include <cassert>
#include <sstream>

//------------------------------------------------------------------------------
svtkPointData* GetEmptyField()
{
  svtkPointData* field = svtkPointData::New();
  return (field);
}

//------------------------------------------------------------------------------
svtkIntArray* GetSampleIntArray(const int numTuples, const int numComp)
{
  assert("pre: numTuples > 0" && (numTuples > 0));
  assert("pre: numComp > 0" && (numComp > 0));

  svtkIntArray* array = svtkIntArray::New();
  array->SetNumberOfComponents(numComp);
  array->SetNumberOfTuples(numTuples);

  std::ostringstream oss;
  oss << "SampleIntArray-" << numComp;
  array->SetName(oss.str().c_str());

  int* ptr = static_cast<int*>(array->GetVoidPointer(0));
  int idx = 0;
  for (int i = 0; i < numTuples; ++i)
  {
    for (int j = 0; j < numComp; ++j)
    {
      ptr[i * numComp + j] = idx;
      ++idx;
    } // END for all components
  }   // END for all tuples

  return (array);
}

//------------------------------------------------------------------------------
svtkDoubleArray* GetSampleDoubleArray(const int numTuples, const int numComp)
{
  assert("pre: numTuples > 0" && (numTuples > 0));
  assert("pre: numComp > 0" && (numComp > 0));

  svtkDoubleArray* array = svtkDoubleArray::New();
  array->SetNumberOfComponents(numComp);
  array->SetNumberOfTuples(numTuples);

  std::ostringstream oss;
  oss << "SampleDoubleArray-" << numComp;
  array->SetName(oss.str().c_str());

  double* ptr = static_cast<double*>(array->GetVoidPointer(0));
  double val = 0.5;
  for (int i = 0; i < numTuples; ++i)
  {
    for (int j = 0; j < numComp; ++j)
    {
      ptr[i * numComp + j] = val;
      ++val;
    } // END for all components
  }   // END for all tuples

  return (array);
}

//------------------------------------------------------------------------------
svtkFloatArray* GetSampleFloatArray(const int numTuples, const int numComp)
{
  assert("pre: numTuples > 0" && (numTuples > 0));
  assert("pre: numComp > 0" && (numComp > 0));

  svtkFloatArray* array = svtkFloatArray::New();
  array->SetNumberOfComponents(numComp);
  array->SetNumberOfTuples(numTuples);

  std::ostringstream oss;
  oss << "SampleFloatArray-" << numComp;
  array->SetName(oss.str().c_str());

  float* ptr = static_cast<float*>(array->GetVoidPointer(0));
  float val = 0.5;
  for (int i = 0; i < numTuples; ++i)
  {
    for (int j = 0; j < numComp; ++j)
    {
      ptr[i * numComp + j] = val;
      ++val;
    } // END for all components
  }   // END for all tuples

  return (array);
}

//------------------------------------------------------------------------------
svtkPointData* GetSamplePointData(const int numTuples)
{
  assert("pre: numTuples > 0" && (numTuples > 0));

  // STEP 0: Get int field
  svtkPointData* field = svtkPointData::New();
  svtkIntArray* intDataArray = GetSampleIntArray(numTuples, 1);
  field->AddArray(intDataArray);
  intDataArray->Delete();

  // STEP 1: Get double field
  svtkDoubleArray* doubleDataArray = GetSampleDoubleArray(numTuples, 3);
  field->AddArray(doubleDataArray);
  doubleDataArray->Delete();

  // STEP 2: Get float field
  svtkFloatArray* floatDataArray = GetSampleFloatArray(numTuples, 2);
  field->AddArray(floatDataArray);
  floatDataArray->Delete();

  return (field);
}

//------------------------------------------------------------------------------
bool AreArraysEqual(svtkDataArray* A1, svtkDataArray* A2)
{
  assert("pre: array 1 is nullptr!" && (A1 != nullptr));
  assert("pre: array 2 is nullptr!" && (A2 != nullptr));

  if (A1->GetDataType() != A2->GetDataType())
  {
    cerr << "ERROR: array datatype mismatch!\n";
    return false;
  }

  if (strcmp(A1->GetName(), A2->GetName()) != 0)
  {
    cerr << "ERROR: array name mismatch!\n";
    cerr << "A1: " << A1->GetName() << endl;
    cerr << "A2: " << A2->GetName() << endl;
    return false;
  }

  if (A1->GetNumberOfTuples() != A2->GetNumberOfTuples())
  {
    cerr << "ERROR: number of tuples mismatch for array ";
    cerr << A1->GetName() << endl;
    return false;
  }

  if (A1->GetNumberOfComponents() != A2->GetNumberOfComponents())
  {
    cerr << "ERROR: number of tuples mismatch for array ";
    cerr << A1->GetName() << endl;
    return false;
  }

  int M = A1->GetNumberOfTuples();
  int N = A1->GetNumberOfComponents();

  switch (A1->GetDataType())
  {
    case SVTK_FLOAT:
    {
      float* a1 = static_cast<float*>(A1->GetVoidPointer(0));
      float* a2 = static_cast<float*>(A2->GetVoidPointer(0));
      for (int i = 0; i < M; ++i)
      {
        for (int j = 0; j < N; ++j)
        {
          int idx = i * N + j;
          if (!svtkMathUtilities::FuzzyCompare(a1[idx], a2[idx]))
          {
            cerr << "INFO:" << a1[idx] << " != " << a2[idx] << endl;
            cerr << "ERROR: float array mismatch!\n";
            return false;
          } // END if not equal
        }   // END for all N
      }     // END for all M
    }
    break;
    case SVTK_DOUBLE:
    {
      double* a1 = static_cast<double*>(A1->GetVoidPointer(0));
      double* a2 = static_cast<double*>(A2->GetVoidPointer(0));
      for (int i = 0; i < M; ++i)
      {
        for (int j = 0; j < N; ++j)
        {
          int idx = i * N + j;
          if (!svtkMathUtilities::FuzzyCompare(a1[idx], a2[idx]))
          {
            cerr << "INFO:" << a1[idx] << " != " << a2[idx] << endl;
            cerr << "ERROR: float array mismatch!\n";
            return false;
          } // END if not equal
        }   // END for all N
      }     // END for all M
    }
    break;
    case SVTK_INT:
    {
      int* a1 = static_cast<int*>(A1->GetVoidPointer(0));
      int* a2 = static_cast<int*>(A2->GetVoidPointer(0));
      for (int i = 0; i < M; ++i)
      {
        for (int j = 0; j < N; ++j)
        {
          int idx = i * N + j;
          if (a1[idx] != a2[idx])
          {
            cerr << "INFO:" << a1[idx] << " != " << a2[idx] << endl;
            cerr << "ERROR: int array mismatch!\n";
            return false;
          }
        } // END for N
      }   // END for all M
    }
    break;
    default:
      cerr << "ERROR: unhandled case! Code should not reach here!\n";
      return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool AreFieldsEqual(svtkFieldData* F1, svtkFieldData* F2)
{
  assert("pre: field 1 is nullptr!" && (F1 != nullptr));
  assert("pre: field 2 is nullptr!" && (F2 != nullptr));

  if (F1->GetNumberOfArrays() != F2->GetNumberOfArrays())
  {
    cerr << "ERROR: number of arrays mismatch between fields!\n";
    return false;
  }

  bool status = true;
  for (int array = 0; array < F1->GetNumberOfArrays(); ++array)
  {
    svtkDataArray* a1 = F1->GetArray(array);
    svtkDataArray* a2 = F2->GetArray(array);
    if (!AreArraysEqual(a1, a2))
    {
      cerr << "ERROR: array " << a1->GetName() << " and " << a2->GetName();
      cerr << " do not match!\n";
      status = false;
    }
    else
    {
      cout << "INFO: " << a1->GetName() << " fields are equal!\n";
      cout.flush();
    }
  } // END for all arrays
  return (status);
}

//------------------------------------------------------------------------------
int TestFieldDataMetaData()
{
  int rc = 0;

  // STEP 0: Construct the field data
  svtkPointData* field = GetSamplePointData(5);
  assert("pre: field is nullptr!" && (field != nullptr));

  // STEP 1: Serialize the field data in a bytestream
  svtkMultiProcessStream bytestream;
  svtkFieldDataSerializer::SerializeMetaData(field, bytestream);

  // STEP 2: De-serialize the metadata
  svtkStringArray* namesArray = svtkStringArray::New();
  svtkIntArray* datatypesArray = svtkIntArray::New();
  svtkIntArray* dimensionsArray = svtkIntArray::New();

  svtkFieldDataSerializer::DeserializeMetaData(
    bytestream, namesArray, datatypesArray, dimensionsArray);

  svtkIdType NumberOfArrays = namesArray->GetNumberOfValues();
  std::string* names = static_cast<std::string*>(namesArray->GetVoidPointer(0));
  int* datatypes = static_cast<int*>(datatypesArray->GetVoidPointer(0));
  int* dimensions = static_cast<int*>(dimensionsArray->GetVoidPointer(0));

  // STEP 3: Check deserialized data with expected values
  if (NumberOfArrays != field->GetNumberOfArrays())
  {
    ++rc;
    cerr << "ERROR: NumberOfArrays=" << NumberOfArrays
         << " expected val=" << field->GetNumberOfArrays() << "\n";
  }
  assert("pre: names arrays is nullptr" && (names != nullptr));
  assert("pre: datatypes is nullptr" && (datatypes != nullptr));
  assert("pre: dimensions is nullptr" && (dimensions != nullptr));

  for (int i = 0; i < NumberOfArrays; ++i)
  {
    svtkDataArray* dataArray = field->GetArray(i);
    if (strcmp(dataArray->GetName(), names[i].c_str()) != 0)
    {
      rc++;
      cerr << "ERROR: Array name mismatch!\n";
    }
    if (dataArray->GetDataType() != datatypes[i])
    {
      rc++;
      cerr << "ERROR: Array data type mismatch!\n";
    }
    if (dataArray->GetNumberOfTuples() != dimensions[i * 2])
    {
      rc++;
      cerr << "ERROR: Array number of tuples mismatch!\n";
    }
    if (dataArray->GetNumberOfComponents() != dimensions[i * 2 + 1])
    {
      rc++;
      cerr << "ERROR: Array number of components mismatch!\n";
    }
  } // END for all arrays

  // STEP 4: Clean up memory
  namesArray->Delete();
  datatypesArray->Delete();
  dimensionsArray->Delete();
  field->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
int TestFieldData()
{
  int rc = 0;

  svtkPointData* field = GetSamplePointData(5);
  assert("pre: field is nullptr!" && (field != nullptr));

  svtkMultiProcessStream bytestream;
  svtkFieldDataSerializer::Serialize(field, bytestream);
  if (bytestream.Empty())
  {
    cerr << "ERROR: failed to serialize field data, bytestream is empty!\n";
    rc++;
    return (rc);
  }

  svtkPointData* field2 = svtkPointData::New();
  svtkFieldDataSerializer::Deserialize(bytestream, field2);
  if (!AreFieldsEqual(field, field2))
  {
    cerr << "ERROR: fields are not equal!\n";
    rc++;
    return (rc);
  }
  else
  {
    cout << "Fields are equal!\n";
    cout.flush();
  }

  field->Delete();
  field2->Delete();
  return (rc);
}

//------------------------------------------------------------------------------
int TestFieldDataSerialization(int argc, char* argv[])
{
  // Resolve compiler warnings about unused vars
  static_cast<void>(argc);
  static_cast<void>(argv);

  int rc = 0;
  rc += TestFieldData();

  cout << "Testing metadata serialization...";
  rc += TestFieldDataMetaData();
  cout << "[DONE]\n";
  return (rc);
}
