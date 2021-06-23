/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExerciseMultiProcessController.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ExerciseMultiProcessController.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageGaussianSource.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkProcessGroup.h"
#include "svtkSphereSource.h"
#include "svtkTypeTraits.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedLongArray.h"

#include <string.h>
#include <time.h>
#include <vector>

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

// Update progress only on root node.
#define COUT(msg)                                                                                  \
  if (controller->GetLocalProcessId() == 0)                                                        \
    cout << "" msg << endl;

//=============================================================================
// A simple structure for passing data in and out of the parallel function.
struct ExerciseMultiProcessControllerArgs
{
  int retval;
};

//-----------------------------------------------------------------------------
// A class to throw in the case of an error.
class ExerciseMultiProcessControllerError
{
};

//=============================================================================
// Establish a custom reduction operation that multiplies 2x2 matrices.
template <class T>
void MatrixMultArray(const T* A, T* B, svtkIdType length)
{
  for (svtkIdType i = 0; i < length / 4; i++)
  {
    T newVal[4];
    newVal[0] = A[0] * B[0] + A[1] * B[2];
    newVal[1] = A[0] * B[1] + A[1] * B[3];
    newVal[2] = A[2] * B[0] + A[3] * B[2];
    newVal[3] = A[2] * B[1] + A[3] * B[3];
    std::copy(newVal, newVal + 4, B);
    A += 4;
    B += 4;
  }
}

// Specialize for floats for greater precision.
template <>
void MatrixMultArray(const float* A, float* B, svtkIdType length)
{
  double* tmpA = new double[length];
  double* tmpB = new double[length];
  for (svtkIdType i = 0; i < length; i++)
  {
    tmpA[i] = static_cast<double>(A[i]);
    tmpB[i] = static_cast<double>(B[i]);
  }
  MatrixMultArray(tmpA, tmpB, length);
  for (svtkIdType i = 0; i < length; i++)
  {
    B[i] = static_cast<float>(tmpB[i]);
  }
  delete[] tmpA;
  delete[] tmpB;
}

class MatrixMultOperation : public svtkCommunicator::Operation
{
public:
  void Function(const void* A, void* B, svtkIdType length, int type)
  {
    switch (type)
    {
      svtkTemplateMacro(MatrixMultArray((const SVTK_TT*)A, (SVTK_TT*)B, length));
    }
  }
  int Commutative() { return 0; }
};

//=============================================================================
// Compare if things are equal (or as close as we can expect).
template <class T>
inline int AreEqual(T a, T b)
{
  return a == b;
}

template <class T>
inline T myAbs(T x)
{
  return (x < 0) ? -x : x;
}

template <>
inline int AreEqual(float a, float b)
{
  float tolerance = myAbs(0.01f * a);
  return (myAbs(a - b) <= tolerance);
}

template <>
inline int AreEqual(double a, double b)
{
  double tolerance = myAbs(0.000001f * a);
  return (myAbs(a - b) <= tolerance);
}

//=============================================================================
// Check to see if any of the processes failed.
static void CheckSuccess(svtkMultiProcessController* controller, int success)
{
  int allSuccess;
  controller->Reduce(&success, &allSuccess, 1, svtkCommunicator::LOGICAL_AND_OP, 0);
  controller->Broadcast(&allSuccess, 1, 0);

  if (!allSuccess || !success)
  {
    COUT("**** Detected an ERROR ****");
    throw ExerciseMultiProcessControllerError();
  }
}

//-----------------------------------------------------------------------------
template <class T>
int CompareArrays(const T* A, const T* B, svtkIdType length)
{
  for (svtkIdType i = 0; i < length; i++)
  {
    if (A[i] != B[i])
    {
      svtkGenericWarningMacro("Encountered mismatched arrays.");
      return 0;
    }
  }
  return 1;
}

int CompareDataArrays(svtkDataArray* A, svtkDataArray* B)
{
  if (A == B)
    return 1;

  int type = A->GetDataType();
  int numComponents = A->GetNumberOfComponents();
  svtkIdType numTuples = A->GetNumberOfTuples();
  if (type != B->GetDataType())
  {
    svtkGenericWarningMacro("Arrays have different types.");
    return 0;
  }
  if (numComponents != B->GetNumberOfComponents())
  {
    svtkGenericWarningMacro("Arrays have different numbers of components.");
    return 0;
  }
  if (numTuples != B->GetNumberOfTuples())
  {
    svtkGenericWarningMacro("Arrays have different numbers of tuples.");
    return 0;
  }
  if (A->GetName() && (strcmp(A->GetName(), B->GetName()) != 0))
  {
    svtkGenericWarningMacro("Arrays have different names.");
    return 0;
  }
  switch (type)
  {
    svtkTemplateMacro(return CompareArrays(
      (SVTK_TT*)A->GetVoidPointer(0), (SVTK_TT*)B->GetVoidPointer(0), numComponents * numTuples));
    default:
      svtkGenericWarningMacro("Invalid type?");
  }
  return 0;
}

static int CompareFieldData(svtkFieldData* fd1, svtkFieldData* fd2)
{
  if (fd1->GetNumberOfArrays() != fd2->GetNumberOfArrays())
  {
    svtkGenericWarningMacro(<< "Different number of arrays in " << fd1->GetClassName());
    return 0;
  }
  for (int i = 0; i < fd1->GetNumberOfArrays(); i++)
  {
    svtkAbstractArray* array1 = fd1->GetAbstractArray(i);
    // If the array does not have a name, then there is no good way to get
    // the equivalent array on the other end since the arrays may not be in
    // the same order.
    if (!array1->GetName())
      continue;
    svtkAbstractArray* array2 = fd2->GetAbstractArray(array1->GetName());
    if (!CompareDataArrays(
          svtkArrayDownCast<svtkDataArray>(array1), svtkArrayDownCast<svtkDataArray>(array2)))
      return 0;
  }

  return 1;
}

static int CompareDataSetAttributes(svtkDataSetAttributes* dsa1, svtkDataSetAttributes* dsa2)
{
  if (!CompareDataArrays(dsa1->GetScalars(), dsa2->GetScalars()))
    return 0;

  return CompareFieldData(dsa1, dsa2);
}

// This is not a complete comparison.  There are plenty of things not actually
// checked.  It only checks svtkImageData and svtkPolyData in detail.
static int CompareDataObjects(svtkDataObject* obj1, svtkDataObject* obj2)
{
  if (obj1->GetDataObjectType() != obj2->GetDataObjectType())
  {
    svtkGenericWarningMacro("Data objects are not of the same tyep.");
    return 0;
  }

  if (!CompareFieldData(obj1->GetFieldData(), obj2->GetFieldData()))
    return 0;

  svtkDataSet* ds1 = svtkDataSet::SafeDownCast(obj1);
  svtkDataSet* ds2 = svtkDataSet::SafeDownCast(obj2);

  if (ds1->GetNumberOfPoints() != ds2->GetNumberOfPoints())
  {
    svtkGenericWarningMacro("Point counts do not agree.");
    return 0;
  }
  if (ds1->GetNumberOfCells() != ds2->GetNumberOfCells())
  {
    svtkGenericWarningMacro("Cell counts do not agree.");
    return 0;
  }

  if (!CompareDataSetAttributes(ds1->GetPointData(), ds2->GetPointData()))
  {
    return 0;
  }
  if (!CompareDataSetAttributes(ds1->GetCellData(), ds2->GetCellData()))
  {
    return 0;
  }

  svtkImageData* id1 = svtkImageData::SafeDownCast(ds1);
  svtkImageData* id2 = svtkImageData::SafeDownCast(ds1);
  if (id1 && id2)
  {
    if ((id1->GetDataDimension() != id2->GetDataDimension()) ||
      (id1->GetDimensions()[0] != id2->GetDimensions()[0]) ||
      (id1->GetDimensions()[1] != id2->GetDimensions()[1]) ||
      (id1->GetDimensions()[2] != id2->GetDimensions()[2]))
    {
      svtkGenericWarningMacro("Dimensions of image data do not agree.");
      return 0;
    }

    if (!CompareArrays(id1->GetExtent(), id2->GetExtent(), 6))
      return 0;
    if (!CompareArrays(id1->GetSpacing(), id2->GetSpacing(), 3))
      return 0;
    if (!CompareArrays(id1->GetOrigin(), id2->GetOrigin(), 3))
      return 0;
  }

  svtkPointSet* ps1 = svtkPointSet::SafeDownCast(ds1);
  svtkPointSet* ps2 = svtkPointSet::SafeDownCast(ds2);
  if (ps1 && ps2)
  {
    if (!CompareDataArrays(ps1->GetPoints()->GetData(), ps2->GetPoints()->GetData()))
      return 0;

    svtkPolyData* pd1 = svtkPolyData::SafeDownCast(ps1);
    svtkPolyData* pd2 = svtkPolyData::SafeDownCast(ps2);

    auto compareCellArrays = [](svtkCellArray* ca1, svtkCellArray* ca2) -> bool {
      return (CompareDataArrays(ca1->GetOffsetsArray(), ca2->GetOffsetsArray()) &&
        CompareDataArrays(ca1->GetConnectivityArray(), ca2->GetConnectivityArray()));
    };

    if (pd1 && pd2)
    {
      if (!compareCellArrays(pd1->GetVerts(), pd2->GetVerts()) ||
        !compareCellArrays(pd1->GetLines(), pd2->GetLines()) ||
        !compareCellArrays(pd1->GetPolys(), pd2->GetPolys()) ||
        !compareCellArrays(pd1->GetStrips(), pd2->GetStrips()))
      {
        return 0;
      }
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
template <class baseType, class arrayType>
void ExerciseType(svtkMultiProcessController* controller)
{
  COUT("---- Exercising " << svtkTypeTraits<baseType>::SizedName());

  typedef typename svtkTypeTraits<baseType>::PrintType printType;

  const int rank = controller->GetLocalProcessId();
  const int numProc = controller->GetNumberOfProcesses();
  int i;
  int result;
  int srcProcessId;
  int destProcessId;
  svtkIdType length;
  std::vector<svtkIdType> lengths;
  lengths.resize(numProc);
  std::vector<svtkIdType> offsets;
  offsets.resize(numProc);
  const int arraySize = (numProc < 8) ? 8 : numProc;

  // Fill up some random arrays.  Note that here and elsewhere we are careful to
  // have each process request the same random numbers.  The pseudorandomness
  // gives us the same values on all processes.
  std::vector<svtkSmartPointer<arrayType> > sourceArrays;
  sourceArrays.resize(numProc);
  for (i = 0; i < numProc; i++)
  {
    sourceArrays[i] = svtkSmartPointer<arrayType>::New();
    sourceArrays[i]->SetNumberOfComponents(1);
    sourceArrays[i]->SetNumberOfTuples(arraySize);
    char name[80];
    snprintf(name, sizeof(name), "%lf", svtkMath::Random());
    sourceArrays[i]->SetName(name);
    for (int j = 0; j < arraySize; j++)
    {
      sourceArrays[i]->SetValue(j, static_cast<baseType>(svtkMath::Random(-16.0, 16.0)));
    }
  }
  COUT("Source Arrays:");
  if (rank == 0)
  {
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < arraySize; j++)
      {
        cout << setw(9) << static_cast<printType>(sourceArrays[i]->GetValue(j));
      }
      cout << endl;
    }
  }

  SVTK_CREATE(arrayType, buffer);
  SVTK_CREATE(arrayType, tmpSource);

  COUT("Basic send and receive.");
  result = 1;
  buffer->Initialize();
  buffer->SetNumberOfComponents(1);
  buffer->SetNumberOfTuples(arraySize);
  for (i = 0; i < numProc; i++)
  {
    if (i < rank)
    {
      controller->Receive(buffer->GetPointer(0), arraySize, i, 9876);
      result &= CompareArrays(sourceArrays[i]->GetPointer(0), buffer->GetPointer(0), arraySize);
      controller->Send(sourceArrays[rank]->GetPointer(0), arraySize, i, 5432);
    }
    else if (i > rank)
    {
      controller->Send(sourceArrays[rank]->GetPointer(0), arraySize, i, 9876);
      controller->Receive(buffer->GetPointer(0), arraySize, i, 5432);
      result &= CompareArrays(sourceArrays[i]->GetPointer(0), buffer->GetPointer(0), arraySize);
    }
  }
  CheckSuccess(controller, result);

  COUT("Broadcast");
  srcProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  if (rank == srcProcessId)
  {
    buffer->DeepCopy(sourceArrays[srcProcessId]);
  }
  controller->Broadcast(buffer->GetPointer(0), arraySize, srcProcessId);
  result =
    CompareArrays(sourceArrays[srcProcessId]->GetPointer(0), buffer->GetPointer(0), arraySize);
  CheckSuccess(controller, result);

  COUT("Gather");
  destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.99));
  buffer->SetNumberOfTuples(numProc * arraySize);
  result = 1;
  if (rank == destProcessId)
  {
    controller->Gather(
      sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, destProcessId);
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < arraySize; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
        {
          svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
        }
      }
    }
  }
  else
  {
    controller->Gather(sourceArrays[rank]->GetPointer(0), nullptr, arraySize, destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("All Gather");
  result = 1;
  controller->AllGather(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize);
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < arraySize; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
      {
        svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector Gather");
  offsets[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  }
  destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  result = 1;
  if (rank == destProcessId)
  {
    controller->GatherV(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), lengths[rank],
      &lengths[0], &offsets[0], destProcessId);
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < lengths[i]; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
        {
          svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
        }
      }
    }
  }
  else
  {
    controller->GatherV(
      sourceArrays[rank]->GetPointer(0), nullptr, lengths[rank], nullptr, nullptr, destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("Vector All Gather");
  offsets[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  }
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  result = 1;
  controller->AllGatherV(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), lengths[rank],
    &lengths[0], &offsets[0]);
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < lengths[i]; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
      {
        svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Scatter");
  srcProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  length = arraySize / numProc;
  buffer->SetNumberOfTuples(length);
  if (rank == srcProcessId)
  {
    controller->Scatter(
      sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), length, srcProcessId);
  }
  else
  {
    controller->Scatter(nullptr, buffer->GetPointer(0), length, srcProcessId);
  }
  result = 1;
  for (i = 0; i < length; i++)
  {
    if (sourceArrays[srcProcessId]->GetValue(rank * length + i) != buffer->GetValue(i))
    {
      svtkGenericWarningMacro(<< "Scattered array from " << srcProcessId << " incorrect.");
      result = 0;
      break;
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector Scatter");
  srcProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  for (i = 0; i < numProc; i++)
  {
    offsets[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize - 0.01));
    lengths[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize - offsets[i] + 0.99));
  }
  buffer->SetNumberOfTuples(lengths[rank]);
  if (rank == srcProcessId)
  {
    controller->ScatterV(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), &lengths[0],
      &offsets[0], lengths[rank], srcProcessId);
  }
  else
  {
    controller->ScatterV(
      nullptr, buffer->GetPointer(0), &lengths[0], &offsets[0], lengths[rank], srcProcessId);
  }
  result = 1;
  for (i = 0; i < lengths[rank]; i++)
  {
    if (sourceArrays[srcProcessId]->GetValue(offsets[rank] + i) != buffer->GetValue(i))
    {
      svtkGenericWarningMacro(<< "Scattered array from " << srcProcessId << " incorrect.");
      result = 0;
      break;
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("Reduce");
    destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
    buffer->SetNumberOfTuples(arraySize);
    result = 1;
    controller->Reduce(sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize,
      svtkCommunicator::SUM_OP, destProcessId);
    if (rank == destProcessId)
    {
      for (i = 0; i < arraySize; i++)
      {
        baseType total = static_cast<baseType>(0);
        for (int j = 0; j < numProc; j++)
          total += sourceArrays[j]->GetValue(i);
        if (!AreEqual(total, buffer->GetValue(i)))
        {
          svtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                                 << buffer->GetValue(i));
          result = 0;
          break;
        }
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom Reduce");
  MatrixMultOperation operation;
  destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  buffer->SetNumberOfTuples(arraySize);
  result = 1;
  controller->Reduce(
    sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, &operation, destProcessId);
  SVTK_CREATE(arrayType, totalArray);
  totalArray->DeepCopy(sourceArrays[numProc - 1]);
  for (i = numProc - 2; i >= 0; i--)
  {
    MatrixMultArray(sourceArrays[i]->GetPointer(0), totalArray->GetPointer(0), arraySize);
  }
  if (rank == destProcessId)
  {
    for (i = 0; i < arraySize; i++)
    {
      if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
      {
        svtkGenericWarningMacro(<< "Unequal computation in reduce: " << totalArray->GetValue(i)
                               << " vs. " << buffer->GetValue(i));
        result = 0;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("All Reduce");
    buffer->SetNumberOfTuples(arraySize);
    result = 1;
    controller->AllReduce(
      sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, svtkCommunicator::SUM_OP);
    for (i = 0; i < arraySize; i++)
    {
      baseType total = static_cast<baseType>(0);
      for (int j = 0; j < numProc; j++)
        total += sourceArrays[j]->GetValue(i);
      if (!AreEqual(total, buffer->GetValue(i)))
      {
        svtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                               << buffer->GetValue(i));
        result = 0;
        break;
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom All Reduce");
  buffer->SetNumberOfTuples(arraySize);
  result = 1;
  controller->AllReduce(
    sourceArrays[rank]->GetPointer(0), buffer->GetPointer(0), arraySize, &operation);
  for (i = 0; i < arraySize; i++)
  {
    if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
    {
      svtkGenericWarningMacro(<< "Unequal computation in reduce: " << totalArray->GetValue(i)
                             << " vs. " << buffer->GetValue(i));
      result = 0;
      break;
    }
  }
  CheckSuccess(controller, result);

  //------------------------------------------------------------------
  // Repeat all the tests, but this time passing the svtkDataArray directly.
  COUT("Basic send and receive with svtkDataArray.");
  result = 1;
  buffer->Initialize();
  for (i = 0; i < numProc; i++)
  {
    if (i < rank)
    {
      controller->Receive(buffer, i, 9876);
      result &= CompareDataArrays(sourceArrays[i], buffer);
      controller->Send(sourceArrays[rank], i, 5432);
    }
    else if (i > rank)
    {
      controller->Send(sourceArrays[rank], i, 9876);
      controller->Receive(buffer, i, 5432);
      result &= CompareDataArrays(sourceArrays[i], buffer);
    }
  }
  CheckSuccess(controller, result);

  COUT("Send and receive svtkDataArray with ANY_SOURCE as source.");
  if (rank == 0)
  {
    for (i = 1; i < numProc; i++)
    {
      buffer->Initialize();
      controller->Receive(buffer, svtkMultiProcessController::ANY_SOURCE, 7127);
      result &= CompareDataArrays(sourceArrays[0], buffer);
    }
  }
  else
  {
    controller->Send(sourceArrays[0], 0, 7127);
  }
  CheckSuccess(controller, result);

  COUT("Broadcast with svtkDataArray");
  buffer->Initialize();
  srcProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  if (rank == srcProcessId)
  {
    buffer->DeepCopy(sourceArrays[srcProcessId]);
    buffer->SetName(sourceArrays[srcProcessId]->GetName());
  }
  controller->Broadcast(buffer, srcProcessId);
  result = CompareDataArrays(sourceArrays[srcProcessId], buffer);
  CheckSuccess(controller, result);

  COUT("Gather with svtkDataArray");
  destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.99));
  buffer->Initialize();
  result = 1;
  if (rank == destProcessId)
  {
    controller->Gather(sourceArrays[rank], buffer, destProcessId);
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < arraySize; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
        {
          svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
        }
      }
    }
  }
  else
  {
    controller->Gather(sourceArrays[rank], nullptr, destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("Vector Gather with svtkDataArray");
  offsets[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  }
  destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  result = 1;
  controller->GatherV(tmpSource, buffer, &lengths[0], &offsets[0], destProcessId);
  if (rank == destProcessId)
  {
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < lengths[i]; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
        {
          svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
        }
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector Gather with svtkDataArray (automatic receive sizes)");
  lengths[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    lengths[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  }
  destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->Initialize();
  result = 1;
  if (rank == destProcessId)
  {
    controller->GatherV(tmpSource, buffer, destProcessId);
    int k = 0;
    for (i = 0; i < numProc; i++)
    {
      for (int j = 0; j < lengths[i]; j++)
      {
        if (sourceArrays[i]->GetValue(j) != buffer->GetValue(k++))
        {
          svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
          result = 0;
          break;
        }
      }
    }
  }
  else
  {
    controller->GatherV(tmpSource, nullptr, destProcessId);
  }
  CheckSuccess(controller, result);

  COUT("All Gather with svtkDataArray");
  buffer->Initialize();
  result = 1;
  controller->AllGather(sourceArrays[rank], buffer);
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < arraySize; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(i * arraySize + j))
      {
        svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector All Gather with svtkDataArray");
  offsets[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99));
  lengths[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    offsets[i] =
      (offsets[i - 1] + lengths[i - 1] + static_cast<svtkIdType>(svtkMath::Random(0.0, 2.99)));
    lengths[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  }
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->SetNumberOfTuples(offsets[numProc - 1] + lengths[numProc - 1]);
  result = 1;
  controller->AllGatherV(tmpSource, buffer, &lengths[0], &offsets[0]);
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < lengths[i]; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(offsets[i] + j))
      {
        svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Vector All Gather with svtkDataArray (automatic receive sizes)");
  lengths[0] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  for (i = 1; i < numProc; i++)
  {
    lengths[i] = static_cast<svtkIdType>(svtkMath::Random(0.0, arraySize + 0.99));
  }
  tmpSource->DeepCopy(sourceArrays[rank]);
  tmpSource->SetNumberOfTuples(lengths[rank]);
  buffer->Initialize();
  result = 1;
  controller->AllGatherV(tmpSource, buffer);
  int k = 0;
  for (i = 0; i < numProc; i++)
  {
    for (int j = 0; j < lengths[i]; j++)
    {
      if (sourceArrays[i]->GetValue(j) != buffer->GetValue(k++))
      {
        svtkGenericWarningMacro("Gathered array from " << i << " incorrect.");
        result = 0;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  COUT("Scatter with svtkDataArray");
  srcProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  length = arraySize / numProc;
  buffer->SetNumberOfTuples(length);
  if (rank == srcProcessId)
  {
    controller->Scatter(sourceArrays[rank], buffer, srcProcessId);
  }
  else
  {
    controller->Scatter(nullptr, buffer, srcProcessId);
  }
  result = 1;
  for (i = 0; i < length; i++)
  {
    if (sourceArrays[srcProcessId]->GetValue(rank * length + i) != buffer->GetValue(i))
    {
      svtkGenericWarningMacro(<< "Scattered array from " << srcProcessId << " incorrect.");
      result = 0;
      break;
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("Reduce with svtkDataArray");
    destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
    buffer->Initialize();
    result = 1;
    controller->Reduce(sourceArrays[rank], buffer, svtkCommunicator::SUM_OP, destProcessId);
    if (rank == destProcessId)
    {
      for (i = 0; i < arraySize; i++)
      {
        baseType total = static_cast<baseType>(0);
        for (int j = 0; j < numProc; j++)
          total += sourceArrays[j]->GetValue(i);
        if (!AreEqual(total, buffer->GetValue(i)))
        {
          svtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                                 << buffer->GetValue(i));
          result = 0;
          break;
        }
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom Reduce with svtkDataArray");
  destProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  buffer->Initialize();
  result = 1;
  controller->Reduce(sourceArrays[rank], buffer, &operation, destProcessId);
  if (rank == destProcessId)
  {
    for (i = 0; i < arraySize; i++)
    {
      if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
      {
        svtkGenericWarningMacro(<< "Unequal computation in reduce: " << totalArray->GetValue(i)
                               << " vs. " << buffer->GetValue(i));
        result = 0;
        break;
      }
    }
  }
  CheckSuccess(controller, result);

  if (sizeof(baseType) > 1)
  {
    // Sum operation not defined for char/byte in some MPI implementations.
    COUT("All Reduce with svtkDataArray");
    buffer->Initialize();
    result = 1;
    controller->AllReduce(sourceArrays[rank], buffer, svtkCommunicator::SUM_OP);
    for (i = 0; i < arraySize; i++)
    {
      baseType total = static_cast<baseType>(0);
      for (int j = 0; j < numProc; j++)
        total += sourceArrays[j]->GetValue(i);
      if (!AreEqual(total, buffer->GetValue(i)))
      {
        svtkGenericWarningMacro(<< "Unequal computation in reduce: " << total << " vs. "
                               << buffer->GetValue(i));
        result = 0;
        break;
      }
    }
    CheckSuccess(controller, result);
  }

  COUT("Custom All Reduce with svtkDataArray");
  buffer->Initialize();
  result = 1;
  controller->AllReduce(sourceArrays[rank], buffer, &operation);
  for (i = 0; i < arraySize; i++)
  {
    if (!AreEqual(totalArray->GetValue(i), buffer->GetValue(i)))
    {
      svtkGenericWarningMacro(<< "Unequal computation in reduce: " << totalArray->GetValue(i)
                             << " vs. " << buffer->GetValue(i));
      result = 0;
      break;
    }
  }
  CheckSuccess(controller, result);
}

//-----------------------------------------------------------------------------
// Check the functions that transfer a data object.
static void ExerciseDataObject(
  svtkMultiProcessController* controller, svtkDataObject* source, svtkDataObject* buffer)
{
  COUT("---- Exercising " << source->GetClassName());

  int rank = controller->GetLocalProcessId();
  int numProc = controller->GetNumberOfProcesses();
  int result;
  int i;

  COUT("Basic send and receive with svtkDataObject.");
  result = 1;
  for (i = 0; i < numProc; i++)
  {
    if (i < rank)
    {
      buffer->Initialize();
      controller->Receive(buffer, i, 9876);
      result &= CompareDataObjects(source, buffer);
      controller->Send(source, i, 5432);
    }
    else if (i > rank)
    {
      controller->Send(source, i, 9876);
      buffer->Initialize();
      controller->Receive(buffer, i, 5432);
      result &= CompareDataObjects(source, buffer);
    }
  }
  CheckSuccess(controller, result);

  COUT("Send and receive svtkDataObject with ANY_SOURCE as source.");
  if (rank == 0)
  {
    for (i = 1; i < numProc; i++)
    {
      buffer->Initialize();
      controller->Receive(buffer, svtkMultiProcessController::ANY_SOURCE, 3462);
      result &= CompareDataObjects(source, buffer);
    }
  }
  else
  {
    controller->Send(source, 0, 3462);
  }
  CheckSuccess(controller, result);

  COUT("Broadcast with svtkDataObject");
  buffer->Initialize();
  int srcProcessId = static_cast<int>(svtkMath::Random(0.0, numProc - 0.01));
  if (rank == srcProcessId)
  {
    buffer->DeepCopy(source);
  }
  controller->Broadcast(buffer, srcProcessId);
  result = CompareDataObjects(source, buffer);
  CheckSuccess(controller, result);
}

//-----------------------------------------------------------------------------
static void Run(svtkMultiProcessController* controller, void* _args)
{
  ExerciseMultiProcessControllerArgs* args =
    reinterpret_cast<ExerciseMultiProcessControllerArgs*>(_args);
  args->retval = 0;

  COUT(<< endl
       << "Exercising " << controller->GetClassName() << ", " << controller->GetNumberOfProcesses()
       << " processes");

  try
  {
    ExerciseType<int, svtkIntArray>(controller);
    ExerciseType<unsigned long, svtkUnsignedLongArray>(controller);
    ExerciseType<char, svtkCharArray>(controller);
    ExerciseType<unsigned char, svtkUnsignedCharArray>(controller);
    ExerciseType<float, svtkFloatArray>(controller);
    ExerciseType<double, svtkDoubleArray>(controller);
    ExerciseType<svtkIdType, svtkIdTypeArray>(controller);

    SVTK_CREATE(svtkImageGaussianSource, imageSource);
    imageSource->SetWholeExtent(-10, 10, -10, 10, -10, 10);
    imageSource->Update();
    ExerciseDataObject(controller, imageSource->GetOutput(), svtkSmartPointer<svtkImageData>::New());

    SVTK_CREATE(svtkSphereSource, polySource);
    polySource->Update();
    ExerciseDataObject(controller, polySource->GetOutput(), svtkSmartPointer<svtkPolyData>::New());
  }
  catch (ExerciseMultiProcessControllerError)
  {
    args->retval = 1;
  }
}

//-----------------------------------------------------------------------------
int ExerciseMultiProcessController(svtkMultiProcessController* controller)
{
  controller->CreateOutputWindow();

  // First, let us create a random seed that everyone will have.
  int seed;
  seed = time(nullptr);
  controller->Broadcast(&seed, 1, 0);
  COUT("**** Random Seed = " << seed << " ****");
  svtkMath::RandomSeed(seed);

  ExerciseMultiProcessControllerArgs args;

  controller->SetSingleMethod(Run, &args);
  controller->SingleMethodExecute();

  if (args.retval)
    return args.retval;

  // Run the same tests, except this time on a subgroup of processes.
  // We make sure that each subgroup has at least one process in it.
  SVTK_CREATE(svtkProcessGroup, group1);
  SVTK_CREATE(svtkProcessGroup, group2);
  group1->Initialize(controller);
  group1->RemoveProcessId(controller->GetNumberOfProcesses() - 1);
  group2->Initialize(controller);
  group2->RemoveAllProcessIds();
  group2->AddProcessId(controller->GetNumberOfProcesses() - 1);
  for (int i = controller->GetNumberOfProcesses() - 2; i >= 1; i--)
  {
    if (svtkMath::Random() < 0.5)
    {
      group1->RemoveProcessId(i);
      group2->AddProcessId(i);
    }
  }
  svtkMultiProcessController *subcontroller1, *subcontroller2;
  subcontroller1 = controller->CreateSubController(group1);
  subcontroller2 = controller->CreateSubController(group2);
  if (subcontroller1 && subcontroller2)
  {
    cout << "**** ERROR: Process " << controller->GetLocalProcessId()
         << " belongs to both subgroups! ****" << endl;
    subcontroller1->Delete();
    subcontroller2->Delete();
    return 1;
  }
  else if (subcontroller1)
  {
    subcontroller1->SetSingleMethod(Run, &args);
    subcontroller1->SingleMethodExecute();
    subcontroller1->Delete();
  }
  else if (subcontroller2)
  {
    subcontroller2->SetSingleMethod(Run, &args);
    subcontroller2->SingleMethodExecute();
    subcontroller2->Delete();
  }
  else
  {
    cout << "**** Error: Process " << controller->GetLocalProcessId()
         << " does not belong to either subgroup! ****" << endl;
  }
  try
  {
    CheckSuccess(controller, !args.retval);
  }
  catch (ExerciseMultiProcessControllerError)
  {
    args.retval = 1;
  }

  int color = (group1->GetLocalProcessId() >= 0) ? 1 : 2;
  svtkMultiProcessController* subcontroller = controller->PartitionController(color, 0);
  subcontroller->SetSingleMethod(Run, &args);
  subcontroller->SingleMethodExecute();
  subcontroller->Delete();

  try
  {
    CheckSuccess(controller, !args.retval);
  }
  catch (ExerciseMultiProcessControllerError)
  {
    args.retval = 1;
  }

  return args.retval;
}
