/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCommunicator.h"

#include "svtkBoundingBox.h"
#include "svtkCharArray.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTypes.h"
#include "svtkDataSetAttributes.h"
#include "svtkDataSetReader.h"
#include "svtkDataSetWriter.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkGenericDataObjectReader.h"
#include "svtkGenericDataObjectWriter.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkIntArray.h"
#include "svtkMultiProcessController.h"
#include "svtkMultiProcessStream.h"
#include "svtkNew.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkTable.h"
#include "svtkTypeTraits.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedLongArray.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include <algorithm>
#include <vector>

#define EXTENT_HEADER_SIZE 128

//=============================================================================
// Functions and classes that perform the default reduction operations.
#define STANDARD_OPERATION_DEFINITION(name, op)                                                    \
  template <class T>                                                                               \
  void svtkCommunicator##name##Func(const T* A, T* B, svtkIdType length)                             \
  {                                                                                                \
    for (svtkIdType i = 0; i < length; i++)                                                         \
      B[i] = (op);                                                                                 \
  }                                                                                                \
  class svtkCommunicator##name##Class : public svtkCommunicator::Operation                           \
  {                                                                                                \
  public:                                                                                          \
    void Function(const void* A, void* B, svtkIdType length, int datatype) override                 \
    {                                                                                              \
      switch (datatype)                                                                            \
      {                                                                                            \
        svtkTemplateMacro(svtkCommunicator##name##Func(                                              \
          reinterpret_cast<const SVTK_TT*>(A), reinterpret_cast<SVTK_TT*>(B), length));              \
      }                                                                                            \
    }                                                                                              \
    int Commutative() override { return 1; }                                                       \
  };

#define STANDARD_OPERATION_FLOAT_OVERRIDE(name)                                                    \
  static void svtkCommunicator##name##Func(const double*, double*, svtkIdType)                       \
  {                                                                                                \
    svtkGenericWarningMacro(<< #name << " not supported for floating point numbers");               \
  }                                                                                                \
  static void svtkCommunicator##name##Func(const float*, float*, svtkIdType)                         \
  {                                                                                                \
    svtkGenericWarningMacro(<< #name << " not supported for floating point numbers");               \
  }

STANDARD_OPERATION_DEFINITION(Max, (A[i] < B[i] ? B[i] : A[i]));
STANDARD_OPERATION_DEFINITION(Min, (A[i] < B[i] ? A[i] : B[i]));
STANDARD_OPERATION_DEFINITION(Sum, A[i] + B[i]);
STANDARD_OPERATION_DEFINITION(Product, A[i] * B[i]);
STANDARD_OPERATION_FLOAT_OVERRIDE(LogicalAnd);
STANDARD_OPERATION_DEFINITION(LogicalAnd, A[i] && B[i]);
STANDARD_OPERATION_FLOAT_OVERRIDE(BitwiseAnd);
STANDARD_OPERATION_DEFINITION(BitwiseAnd, A[i] & B[i]);
STANDARD_OPERATION_FLOAT_OVERRIDE(LogicalOr);
STANDARD_OPERATION_DEFINITION(LogicalOr, A[i] || B[i]);
STANDARD_OPERATION_FLOAT_OVERRIDE(BitwiseOr);
STANDARD_OPERATION_DEFINITION(BitwiseOr, A[i] | B[i]);
STANDARD_OPERATION_FLOAT_OVERRIDE(LogicalXor);
STANDARD_OPERATION_DEFINITION(LogicalXor, (!A[i] && B[i]) || (A[i] && !B[i]));
STANDARD_OPERATION_FLOAT_OVERRIDE(BitwiseXor);
STANDARD_OPERATION_DEFINITION(BitwiseXor, A[i] ^ B[i]);

//=============================================================================
svtkCommunicator::svtkCommunicator()
{
  this->LocalProcessId = 0;
  this->NumberOfProcesses = 1;
  this->MaximumNumberOfProcesses = svtkTypeTraits<int>::Max();
  this->Count = 0;
}

//----------------------------------------------------------------------------
svtkCommunicator::~svtkCommunicator() = default;

//----------------------------------------------------------------------------
int svtkCommunicator::UseCopy = 0;
void svtkCommunicator::SetUseCopy(int useCopy)
{
  svtkCommunicator::UseCopy = useCopy;
}

//----------------------------------------------------------------------------
void svtkCommunicator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaximumNumberOfProcesses: " << this->MaximumNumberOfProcesses << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "LocalProcessId: " << this->LocalProcessId << endl;
  os << indent << "Count: " << this->Count << endl;
}

//----------------------------------------------------------------------------
void svtkCommunicator::SetNumberOfProcesses(int num)
{
  if (num == this->NumberOfProcesses)
  {
    return;
  }

  if (num < 1 || num > this->MaximumNumberOfProcesses)
  {
    svtkErrorMacro(<< num << " is an invalid number of processes try a number from 1 to "
                  << this->NumberOfProcesses);
    return;
  }

  this->NumberOfProcesses = num;
  this->Modified();
}

//----------------------------------------------------------------------------
// Need to add better error checking
int svtkCommunicator::Send(svtkDataObject* data, int remoteHandle, int tag)
{
  // If the receiving end is using with ANY_SOURCE, we have a problem because
  // some versions of MPI might deliver the multiple data objects require out of
  // order.  To get around this, on the first message we send the actual source
  // and a mangled tag.  The remote process then receives the rest of the
  // messages with the specific source and mangled tag, which are guaranteed to
  // be received in the correct order.
  static int tagMangler = 1000;
  int mangledTag = tag + tagMangler++;
  int header[2];
  header[0] = this->LocalProcessId;
  header[1] = mangledTag;
  this->Send(header, 2, remoteHandle, tag);
  tag = mangledTag;

  int data_type = data ? data->GetDataObjectType() : -1;
  this->Send(&data_type, 1, remoteHandle, tag);

  switch (data_type)
  {
    case -1:
      // nullptr data.
      return 1;

    // error on types we can't send
    case SVTK_DATA_OBJECT:
    case SVTK_DATA_SET:
    case SVTK_PIECEWISE_FUNCTION:
    case SVTK_POINT_SET:
    case SVTK_UNIFORM_GRID:
    case SVTK_GENERIC_DATA_SET:
    case SVTK_HYPER_OCTREE:
    case SVTK_COMPOSITE_DATA_SET:
    case SVTK_HIERARCHICAL_BOX_DATA_SET: // since we cannot send svtkUniformGrid anyways.
    case SVTK_MULTIGROUP_DATA_SET:       // obsolete
    case SVTK_HIERARCHICAL_DATA_SET:     // obsolete
    default:
      svtkWarningMacro(<< "Cannot send " << data->GetClassName());
      return 0;

    // send elemental data objects
    case SVTK_DIRECTED_GRAPH:
    case SVTK_UNDIRECTED_GRAPH:
    case SVTK_IMAGE_DATA:
    case SVTK_POLY_DATA:
    case SVTK_RECTILINEAR_GRID:
    case SVTK_STRUCTURED_GRID:
    case SVTK_STRUCTURED_POINTS:
    case SVTK_TABLE:
    case SVTK_TREE:
    case SVTK_UNSTRUCTURED_GRID:
    case SVTK_MULTIBLOCK_DATA_SET:
    case SVTK_UNIFORM_GRID_AMR:
    case SVTK_OVERLAPPING_AMR:
      return this->SendElementalDataObject(data, remoteHandle, tag);
  }
}

//----------------------------------------------------------------------------
int svtkCommunicator::SendElementalDataObject(svtkDataObject* data, int remoteHandle, int tag)
{
  SVTK_CREATE(svtkCharArray, buffer);
  if (svtkCommunicator::MarshalDataObject(data, buffer))
  {
    return this->Send(buffer, remoteHandle, tag);
  }

  // could not marshal data
  return 0;
}

//----------------------------------------------------------------------------
int svtkCommunicator::Send(svtkDataArray* data, int remoteHandle, int tag)
{
  // If the receiving end is using with ANY_SOURCE, we have a problem because
  // some versions of MPI might deliver the multiple data objects require out of
  // order.  To get around this, on the first message we send the actual source
  // and a mangled tag.  The remote process then receives the rest of the
  // messages with the specific source and mangled tag, which are guaranteed to
  // be received in the correct order.
  static int tagMangler = 1000;
  int mangledTag = tag + tagMangler++;
  int header[2];
  header[0] = this->LocalProcessId;
  header[1] = mangledTag;
  this->Send(header, 2, remoteHandle, tag);
  tag = mangledTag;

  int type = -1;
  if (data == nullptr)
  {
    this->Send(&type, 1, remoteHandle, tag);
    return 1;
  }

  // send array type
  type = data->GetDataType();
  this->Send(&type, 1, remoteHandle, tag);

  // send array tuples
  svtkIdType numTuples = data->GetNumberOfTuples();
  this->Send(&numTuples, 1, remoteHandle, tag);

  // send number of components in array
  int numComponents = data->GetNumberOfComponents();
  this->Send(&numComponents, 1, remoteHandle, tag);

  svtkIdType size = numTuples * numComponents;

  const char* name = data->GetName();
  int len = 0;
  if (name)
  {
    len = static_cast<int>(strlen(name)) + 1;
  }

  // send length of name
  this->Send(&len, 1, remoteHandle, tag);

  if (len > 0)
  {
    // send name
    this->Send(const_cast<char*>(name), len, remoteHandle, tag);
  }

  // do nothing if size is zero.
  if (size == 0)
  {
    return 1;
  }

  // now send the raw array
  this->SendVoidArray(data->GetVoidPointer(0), size, type, remoteHandle, tag);
  return 1;
}

//----------------------------------------------------------------------------
int svtkCommunicator::Receive(svtkDataObject* data, int remoteHandle, int tag)
{
  // fill in the data object we are given
  return this->ReceiveDataObject(data, remoteHandle, tag, -1);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkCommunicator::ReceiveDataObject(int remoteHandle, int tag)
{
  // If we are receiving with ANY_SOURCE, we have a problem because some
  // versions of MPI might deliver the multiple data objects require out of
  // order.  To get around this, on the first message we receive the actual
  // source and a mangled tag.  We then receive the rest of the messages with
  // the specific source and mangled tag, which we are guaranteed to receive in
  // the correct order.
  int header[2];
  this->Receive(header, 2, remoteHandle, tag);
  // Use the specific source and tag.
  if (remoteHandle == svtkMultiProcessController::ANY_SOURCE)
  {
    remoteHandle = header[0];
  }
  tag = header[1];

  int data_type = 0;
  this->Receive(&data_type, 1, remoteHandle, tag);
  if (data_type < 0)
  {
    // nullptr data object.
    return nullptr;
  }
  // manufacture a data object of the proper type to fill
  svtkDataObject* dObj = svtkDataObjectTypes::NewDataObject(data_type);
  if (dObj != nullptr)
  {
    if (this->ReceiveDataObject(dObj, remoteHandle, tag, data_type) == 1)
    {
      return dObj;
    }
  }
  if (dObj)
  {
    dObj->Delete();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkCommunicator::ReceiveDataObject(svtkDataObject* data, int remoteHandle, int tag, int dataType)
{
  // If we have not yet received the data type, get the header and data type.
  int data_type = dataType;
  if (data_type == -1)
  {
    // If we are receiving with ANY_SOURCE, we have a problem because some
    // versions of MPI might deliver the multiple data objects require out of
    // order.  To get around this, on the first message we receive the actual
    // source and a mangled tag.  We then receive the rest of the messages with
    // the specific source and mangled tag, which we are guaranteed to receive
    // in the correct order.
    int header[2];
    this->Receive(header, 2, remoteHandle, tag);
    // Use the specific source and tag.
    if (remoteHandle == svtkMultiProcessController::ANY_SOURCE)
    {
      remoteHandle = header[0];
    }
    tag = header[1];

    this->Receive(&data_type, 1, remoteHandle, tag);
    if (data->GetDataObjectType() != data_type)
    {
      svtkErrorMacro("Cannot receive object, type sent is different from destination.");
      return 0;
    }
  }

  switch (data_type)
  {
    // error on types we can't receive
    case SVTK_DATA_OBJECT:
    case SVTK_DATA_SET:
    case SVTK_PIECEWISE_FUNCTION:
    case SVTK_POINT_SET:
    case SVTK_UNIFORM_GRID:
    case SVTK_GENERIC_DATA_SET:
    case SVTK_HYPER_OCTREE:
    case SVTK_COMPOSITE_DATA_SET:
    case SVTK_HIERARCHICAL_BOX_DATA_SET: // since we cannot send svtkUniformGrid anyways.
    case SVTK_MULTIGROUP_DATA_SET:       // obsolete.
    case SVTK_HIERARCHICAL_DATA_SET:     // obsolete.
    default:
      svtkWarningMacro(<< "Cannot receive "
                      << svtkDataObjectTypes::GetClassNameFromTypeId(data_type));
      return 0;

    // receive elemental data objects
    case SVTK_DIRECTED_GRAPH:
    case SVTK_UNDIRECTED_GRAPH:
    case SVTK_IMAGE_DATA:
    case SVTK_POLY_DATA:
    case SVTK_RECTILINEAR_GRID:
    case SVTK_STRUCTURED_GRID:
    case SVTK_STRUCTURED_POINTS:
    case SVTK_TABLE:
    case SVTK_TREE:
    case SVTK_UNSTRUCTURED_GRID:
    case SVTK_MULTIBLOCK_DATA_SET:
    case SVTK_UNIFORM_GRID_AMR:
    case SVTK_OVERLAPPING_AMR:
      return this->ReceiveElementalDataObject(data, remoteHandle, tag);
  }
}

//----------------------------------------------------------------------------
int svtkCommunicator::ReceiveElementalDataObject(svtkDataObject* data, int remoteHandle, int tag)
{
  SVTK_CREATE(svtkCharArray, buffer);
  if (!this->Receive(buffer, remoteHandle, tag))
  {
    return 0;
  }

  return svtkCommunicator::UnMarshalDataObject(buffer, data);
}

int svtkCommunicator::Receive(svtkDataArray* data, int remoteHandle, int tag)
{
  // If we are receiving with ANY_SOURCE, we have a problem because some
  // versions of MPI might deliver the multiple data objects require out of
  // order.  To get around this, on the first message we receive the actual
  // source and a mangled tag.  We then receive the rest of the messages with
  // the specific source and mangled tag, which we are guaranteed to receive in
  // the correct order.
  int header[2];
  this->Receive(header, 2, remoteHandle, tag);
  // Use the specific source and tag.
  if (remoteHandle == svtkMultiProcessController::ANY_SOURCE)
  {
    remoteHandle = header[0];
  }
  tag = header[1];

  // First receive the data type.
  int type;
  if (!this->Receive(&type, 1, remoteHandle, tag))
  {
    svtkErrorMacro("Could not receive data!");
    return 0;
  }

  if (type == -1)
  { // This indicates a nullptr object was sent. Do nothing.
    return 1;
  }

  if (type != data->GetDataType())
  {
    svtkErrorMacro("Send/receive data types do not match!");
    return 0;
  }

  // Next receive the number of tuples.
  svtkIdType numTuples;
  if (!this->Receive(&numTuples, 1, remoteHandle, tag))
  {
    svtkErrorMacro("Could not receive data!");
    return 0;
  }

  // Next receive the number of components.
  int numComponents;
  this->Receive(&numComponents, 1, remoteHandle, tag);

  svtkIdType size = numTuples * numComponents;
  data->SetNumberOfComponents(numComponents);
  data->SetNumberOfTuples(numTuples);

  // Next receive the length of the name.
  int nameLength;
  this->Receive(&nameLength, 1, remoteHandle, tag);

  if (nameLength > 0)
  {
    char* str = new char[nameLength];

    // Receive the name
    this->Receive(str, nameLength, remoteHandle, tag);
    data->SetName(str);
  }
  else
  {
    data->SetName(nullptr);
  }

  if (size < 0)
  {
    svtkErrorMacro("Bad data length");
    return 0;
  }

  // Do nothing if size is zero.
  if (size == 0)
  {
    return 1;
  }

  // now receive the raw array.
  this->ReceiveVoidArray(data->GetVoidPointer(0), size, type, remoteHandle, tag);

  return 1;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::MarshalDataObject(svtkDataObject* object, svtkCharArray* buffer)
{
  buffer->Initialize();
  buffer->SetNumberOfComponents(1);

  if (object == nullptr)
  {
    buffer->SetNumberOfTuples(0);
    return 1;
  }

  SVTK_CREATE(svtkGenericDataObjectWriter, writer);

  svtkSmartPointer<svtkDataObject> copy;
  copy.TakeReference(object->NewInstance());
  copy->ShallowCopy(object);

  writer->SetFileTypeToBinary();
  // There is a problem with binary files with no data.
  if (svtkDataSet::SafeDownCast(copy) != nullptr)
  {
    svtkDataSet* ds = svtkDataSet::SafeDownCast(copy);
    if (ds->GetNumberOfCells() + ds->GetNumberOfPoints() == 0)
    {
      writer->SetFileTypeToASCII();
    }
  }
  writer->WriteToOutputStringOn();
  writer->SetInputData(copy);

  if (!writer->Write())
  {
    svtkGenericWarningMacro("Error detected while marshaling data object.");
    return 0;
  }
  const svtkIdType size = writer->GetOutputStringLength();
  if (object->GetExtentType() == SVTK_3D_EXTENT)
  {
    // You would think that the extent information would be properly saved, but
    // no, it is not.
    int extent[6] = { 0, 0, 0, 0, 0, 0 };
    svtkRectilinearGrid* rg = svtkRectilinearGrid::SafeDownCast(object);
    svtkStructuredGrid* sg = svtkStructuredGrid::SafeDownCast(object);
    svtkImageData* id = svtkImageData::SafeDownCast(object);
    if (rg)
    {
      rg->GetExtent(extent);
    }
    else if (sg)
    {
      sg->GetExtent(extent);
    }
    else if (id)
    {
      id->GetExtent(extent);
    }
    char extentHeader[EXTENT_HEADER_SIZE];
    snprintf(extentHeader, sizeof(extentHeader), "EXTENT %d %d %d %d %d %d", extent[0], extent[1],
      extent[2], extent[3], extent[4], extent[5]);

    buffer->SetNumberOfTuples(size + EXTENT_HEADER_SIZE);
    memcpy(buffer->GetPointer(0), extentHeader, EXTENT_HEADER_SIZE);
    memcpy(buffer->GetPointer(EXTENT_HEADER_SIZE), writer->GetOutputString(), size);
  }
  else
  {
    buffer->SetArray(
      writer->RegisterAndGetOutputString(), size, 0, svtkCharArray::SVTK_DATA_ARRAY_DELETE);
    buffer->SetNumberOfTuples(size);
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::UnMarshalDataObject(svtkCharArray* buffer, svtkDataObject* object)
{
  if (!object)
  {
    svtkGenericWarningMacro("Invalid 'object'!");
    return 0;
  }
  svtkSmartPointer<svtkDataObject> dobj = svtkCommunicator::UnMarshalDataObject(buffer);
  if (dobj)
  {
    if (!dobj->IsA(object->GetClassName()))
    {
      svtkGenericWarningMacro("Type mismatch while unmarshalling data.");
    }
    object->ShallowCopy(dobj);
  }
  else
  {
    object->Initialize();
  }
  return 1;
}

//-----------------------------------------------------------------------------
svtkSmartPointer<svtkDataObject> svtkCommunicator::UnMarshalDataObject(svtkCharArray* buffer)
{
  svtkIdType bufferSize = buffer ? buffer->GetNumberOfTuples() : 0;
  if (bufferSize <= 0)
  {
    return nullptr;
  }

  // You would think that the extent information would be properly saved, but
  // no, it is not.
  int extent[6] = { 0, 0, 0, 0, 0, 0 };
  char* bufferArray = buffer->GetPointer(0);
  if (strncmp(bufferArray, "EXTENT", 6) == 0)
  {
    sscanf(bufferArray, "EXTENT %d %d %d %d %d %d", &extent[0], &extent[1], &extent[2], &extent[3],
      &extent[4], &extent[5]);
    bufferArray += EXTENT_HEADER_SIZE;
    bufferSize -= EXTENT_HEADER_SIZE;
  }

  // Make a temporary array object holding the part of the buffer that can be
  // parsed by the reader.
  svtkNew<svtkCharArray> objectBuffer;
  objectBuffer->SetNumberOfComponents(1);
  objectBuffer->SetArray(bufferArray, bufferSize, 1);

  svtkNew<svtkGenericDataObjectReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputArray(objectBuffer);
  reader->Update();

  svtkSmartPointer<svtkDataObject> dobj = reader->GetOutputDataObject(0);
  if (dobj->GetExtentType() == SVTK_3D_EXTENT)
  {
    if (svtkRectilinearGrid* rg = svtkRectilinearGrid::SafeDownCast(dobj))
    {
      rg->SetExtent(extent);
    }
    else if (svtkStructuredGrid* sg = svtkStructuredGrid::SafeDownCast(dobj))
    {
      sg->SetExtent(extent);
    }
    else if (svtkImageData* id = svtkImageData::SafeDownCast(dobj))
    {
      // If we fix the extent, we need to fix the origin too.
      double origin[3];
      id->GetOrigin(origin);
      double spacing[3];
      id->GetSpacing(spacing);
      int readerExt[6];
      id->GetExtent(readerExt);
      for (int i = 0; i < 3; i++)
      {
        if (readerExt[2 * i] != extent[2 * i])
        {
          origin[i] = origin[i] - (extent[2 * i] - readerExt[2 * i]) * spacing[i];
        }
      }
      id->SetExtent(extent);
      id->SetOrigin(origin);
    }
  }
  return dobj;
}

// The processors are views as a heap tree. The root is the processor of
// id 0.
//-----------------------------------------------------------------------------
int svtkCommunicator::GetParentProcessor(int proc)
{
  int result;
  if (proc % 2 == 1)
  {
    result = proc >> 1; // /2
  }
  else
  {
    result = (proc - 1) >> 1; // /2
  }
  return result;
}

int svtkCommunicator::GetLeftChildProcessor(int proc)
{
  return (proc << 1) + 1; // *2+1
}

int svtkCommunicator::ComputeGlobalBounds(int processNumber, int numProcessors,
  svtkBoundingBox* bounds, int* rhb, int* lhb, int hasBoundsTag, int localBoundsTag,
  int globalBoundsTag)
{
  int parent = 0;
  int leftHasBounds = 0, rightHasBounds = 0;
  int left = this->GetLeftChildProcessor(processNumber);
  int right = left + 1;
  if (processNumber > 0) // not root (nothing to do if root)
  {
    parent = this->GetParentProcessor(processNumber);
  }

  double otherBounds[6];
  if (left < numProcessors)
  {
    this->Receive(&leftHasBounds, 1, left, hasBoundsTag);
    if (lhb)
    {
      *lhb = leftHasBounds;
    }

    if (leftHasBounds)
    {
      this->Receive(otherBounds, 6, left, localBoundsTag);
      bounds->AddBounds(otherBounds);
    }
  }
  if (right < numProcessors)
  {
    // Grab the bounds from right child
    this->Receive(&rightHasBounds, 1, right, hasBoundsTag);

    if (rhb)
    {
      *rhb = rightHasBounds;
    }

    if (rightHasBounds)
    {
      this->Receive(otherBounds, 6, right, localBoundsTag);
      bounds->AddBounds(otherBounds);
    }
  }

  // If there are bounds to send do so
  int boundsHaveBeenSet = bounds->IsValid();
  double b[6];
  // Send local to parent, Receive global from the parent.
  if (processNumber > 0) // not root (nothing to do if root)
  {
    this->Send(&boundsHaveBeenSet, 1, parent, hasBoundsTag);
    if (boundsHaveBeenSet)
    {
      // Copy the bounds to an array so we can send them

      bounds->GetBounds(b);
      this->Send(b, 6, parent, localBoundsTag);

      this->Receive(b, 6, parent, globalBoundsTag);
      bounds->AddBounds(b);
    }
  }

  if (!boundsHaveBeenSet) // empty, no bounds, nothing to do
  {
    return 1;
  }

  // Send it to children.
  bounds->GetBounds(b);
  if (left < numProcessors)
  {
    if (leftHasBounds)
    {
      this->Send(b, 6, left, globalBoundsTag);
    }
    if (right < numProcessors)
    {
      if (rightHasBounds)
      {
        this->Send(b, 6, right, globalBoundsTag);
      }
    }
  }
  return 1;
}

//=============================================================================
// Collective operations.
//
// The implementations of these are very simple and probably inefficient.
// Most of the time we expect to be using an MPI controller, which has its
// own special implementations in the underlying API, so there is no good
// reason to work on creating a really good implementation here.

//-----------------------------------------------------------------------------
void svtkCommunicator::Barrier()
{
  int junk = 0;
  if (this->LocalProcessId == 0)
  {
    for (int i = 1; i < this->NumberOfProcesses; i++)
    {
      this->Receive(&junk, 1, i, BARRIER_TAG);
    }
  }
  else
  {
    this->Send(&junk, 1, 0, BARRIER_TAG);
  }
  this->Broadcast(&junk, 1, 0);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::BroadcastVoidArray(void* data, svtkIdType length, int type, int srcProcessId)
{
  if (srcProcessId == this->LocalProcessId)
  {
    int result = 1;
    for (int i = 0; i < this->NumberOfProcesses; i++)
    {
      if (i != this->LocalProcessId)
      {
        result &= this->SendVoidArray(data, length, type, i, BROADCAST_TAG);
      }
    }
    return result;
  }
  else
  {
    return this->ReceiveVoidArray(data, length, type, srcProcessId, BROADCAST_TAG);
  }
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Broadcast(svtkDataObject* data, int srcProcessId)
{
  SVTK_CREATE(svtkCharArray, buffer);
  if (this->LocalProcessId == srcProcessId)
  {
    if (svtkCommunicator::MarshalDataObject(data, buffer))
    {
      return this->Broadcast(buffer, srcProcessId);
    }
    else
    {
      // Could not marshal data.
      return 0;
    }
  }
  else
  {
    if (!this->Broadcast(buffer, srcProcessId))
    {
      return 0;
    }
    return svtkCommunicator::UnMarshalDataObject(buffer, data);
  }
}

//-----------------------------------------------------------------------------
// We are more careful about duplicating all the metadata in the broadcast than
// the other collective operations, because it is more like a send/recv.
int svtkCommunicator::Broadcast(svtkDataArray* data, int srcProcessId)
{
  int type;
  svtkIdType numTuples;
  int numComponents;
  int nameLength = 0;
  char* name = nullptr;

  // On the source process, extract the metadata.
  if (this->LocalProcessId == srcProcessId)
  {
    type = data->GetDataType();
    numTuples = data->GetNumberOfTuples();
    numComponents = data->GetNumberOfComponents();
    nameLength = 0;
    name = data->GetName();
    if (name)
    {
      nameLength = static_cast<int>(strlen(name)) + 1;
    }
  }

  // Broadcast the metadata
  if (!this->Broadcast(&type, 1, srcProcessId))
    return 0;
  if (!this->Broadcast(&numTuples, 1, srcProcessId))
    return 0;
  if (!this->Broadcast(&numComponents, 1, srcProcessId))
    return 0;
  if (!this->Broadcast(&nameLength, 1, srcProcessId))
    return 0;

  // On the destinations, allocate buffers.
  if (this->LocalProcessId != srcProcessId)
  {
    if (data->GetDataType() != type)
    {
      svtkErrorMacro("Broadcast data types do not match!");
      return 0;
    }
    name = (nameLength > 0) ? new char[nameLength] : nullptr;
    data->SetNumberOfComponents(numComponents);
    data->SetNumberOfTuples(numTuples);
  }

  // Send the actual data.
  if (nameLength > 0)
  {
    if (!this->Broadcast(name, nameLength, srcProcessId))
      return 0;
  }
  if (!this->BroadcastVoidArray(
        data->GetVoidPointer(0), numTuples * numComponents, data->GetDataType(), srcProcessId))
    return 0;

  // Cleanup
  if (this->LocalProcessId != srcProcessId)
  {
    if (nameLength > 0)
    {
      data->SetName(name);
      delete[] name;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::GatherVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int destProcessId)
{
  if (this->LocalProcessId == destProcessId)
  {
    int result = 1;
    char* dest = reinterpret_cast<char*>(recvBuffer);
    int typeSize = 1;
    switch (type)
    {
      svtkTemplateMacro(typeSize = sizeof(SVTK_TT));
    }
    // Copy local data first in case buffers are the same.
    memmove(dest + this->LocalProcessId * length * typeSize, sendBuffer, length * typeSize);
    // Receive everything else.
    for (int i = 0; i < this->NumberOfProcesses; i++)
    {
      if (this->LocalProcessId != i)
      {
        result &= this->ReceiveVoidArray(dest + i * length * typeSize, length, type, i, GATHER_TAG);
      }
    }
    return result;
  }
  else
  {
    return this->SendVoidArray(sendBuffer, length, type, destProcessId, GATHER_TAG);
  }
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Gather(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int destProcessId)
{
  int type = sendBuffer->GetDataType();
  const void* sb = sendBuffer->GetVoidPointer(0);
  void* rb = nullptr;
  int numComponents = sendBuffer->GetNumberOfComponents();
  svtkIdType numTuples = sendBuffer->GetNumberOfTuples();
  if (this->LocalProcessId == destProcessId)
  {
    if (type != recvBuffer->GetDataType())
    {
      svtkErrorMacro(<< "Data type mismatch.");
      return 0;
    }
    recvBuffer->SetNumberOfComponents(numComponents);
    recvBuffer->SetNumberOfTuples(numTuples * this->NumberOfProcesses);
    rb = recvBuffer->GetVoidPointer(0);
  }
  return this->GatherVoidArray(sb, rb, numComponents * numTuples, type, destProcessId);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Gather(svtkDataObject* sendBuffer,
  std::vector<svtkSmartPointer<svtkDataObject> >& recvBuffer, int destProcessId)
{
  svtkNew<svtkCharArray> sendArray;
  if (svtkCommunicator::MarshalDataObject(sendBuffer, sendArray) == 0)
  {
    svtkErrorMacro("Marshalling failed! Cannot 'Gather' successfully!");
    sendArray->Initialize();
  }

  svtkNew<svtkCharArray> fullRecvArray;
  std::vector<svtkSmartPointer<svtkDataArray> > recvArrays(this->NumberOfProcesses);
  if (this->LocalProcessId == destProcessId)
  {
    recvBuffer.resize(this->NumberOfProcesses);
    for (int cc = 0; cc < this->NumberOfProcesses; ++cc)
    {
      recvArrays[cc] = svtkSmartPointer<svtkCharArray>::New();
    }
  }

  if (this->GatherV(sendArray, fullRecvArray, &recvArrays[0], destProcessId))
  {
    if (this->LocalProcessId == destProcessId)
    {
      for (int cc = 0; cc < this->NumberOfProcesses; ++cc)
      {
        svtkSmartPointer<svtkDataObject> dobj =
          svtkCommunicator::UnMarshalDataObject(svtkArrayDownCast<svtkCharArray>(recvArrays[cc]));
        recvBuffer[cc] = dobj;
      }
    }
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Gather(const svtkMultiProcessStream& sendBuffer,
  std::vector<svtkMultiProcessStream>& recvBuffer, int destProcessId)
{
  svtkNew<svtkUnsignedCharArray> sendArray;
  auto rawData = sendBuffer.GetRawData();
  sendArray->SetArray(&rawData[0], static_cast<svtkIdType>(rawData.size()), /*save*/ 1);

  svtkNew<svtkUnsignedCharArray> fullRecvArray;
  std::vector<svtkSmartPointer<svtkDataArray> > recvArrays(this->NumberOfProcesses);
  if (this->LocalProcessId == destProcessId)
  {
    recvBuffer.resize(this->NumberOfProcesses);
    for (int cc = 0; cc < this->NumberOfProcesses; ++cc)
    {
      recvArrays[cc] = svtkSmartPointer<svtkUnsignedCharArray>::New();
    }
  }

  if (this->GatherV(sendArray, fullRecvArray, &recvArrays[0], destProcessId))
  {
    if (this->LocalProcessId == destProcessId)
    {
      for (int cc = 0; cc < this->NumberOfProcesses; ++cc)
      {
        auto array = svtkArrayDownCast<svtkUnsignedCharArray>(recvArrays[cc]);
        recvBuffer[cc].SetRawData(
          array->GetPointer(0), static_cast<unsigned int>(array->GetNumberOfValues()));
      }
    }
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::GatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer,
  svtkSmartPointer<svtkDataArray>* recvBuffers, int destProcessId)
{
  svtkNew<svtkIdTypeArray> recvLengths;
  svtkNew<svtkIdTypeArray> offsets;
  int retValue = this->GatherV(sendBuffer, recvBuffer, recvLengths, offsets, destProcessId);
  if (destProcessId == this->LocalProcessId)
  {
    int numComponents = sendBuffer->GetNumberOfComponents();
    for (int i = 0; i < this->NumberOfProcesses; ++i)
    {
      recvBuffers[i]->SetNumberOfComponents(numComponents);
      recvBuffers[i]->SetVoidArray(static_cast<unsigned char*>(recvBuffer->GetVoidPointer(0)) +
          offsets->GetValue(i) * recvBuffer->GetElementComponentSize(),
        recvLengths->GetValue(i) * recvBuffer->GetElementComponentSize(), 1);
    }
  }
  return retValue;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::GatherVElementalDataObject(
  svtkDataObject* sendData, svtkSmartPointer<svtkDataObject>* receiveData, int destProcessId)
{
  svtkNew<svtkCharArray> sendBuffer;
  svtkNew<svtkCharArray> recvBuffer;
  std::vector<svtkSmartPointer<svtkDataArray> > recvBuffers(this->NumberOfProcesses);

  svtkCommunicator::MarshalDataObject(sendData, sendBuffer);
  if (this->LocalProcessId == destProcessId)
  {
    for (int i = 0; i < this->NumberOfProcesses; ++i)
    {
      recvBuffers[i] = svtkSmartPointer<svtkCharArray>::New();
    }
  }
  if (this->GatherV(sendBuffer, recvBuffer, &recvBuffers[0], destProcessId))
  {
    if (this->LocalProcessId == destProcessId)
    {
      for (int i = 0; i < this->NumberOfProcesses; ++i)
      {
        if (!svtkCommunicator::UnMarshalDataObject(
              svtkArrayDownCast<svtkCharArray>(recvBuffers[i]), receiveData[i]))
        {
          return 0;
        }
      }
    }
  }
  else
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkCommunicator::GatherV(
  svtkDataObject* sendData, svtkSmartPointer<svtkDataObject>* receiveData, int destProcessId)
{
  int sendType = sendData ? sendData->GetDataObjectType() : -1;
  switch (sendType)
  {
    // error on types we can't send
    case SVTK_DATA_OBJECT:
    case SVTK_DATA_SET:
    case SVTK_PIECEWISE_FUNCTION:
    case SVTK_POINT_SET:
    case SVTK_UNIFORM_GRID:
    case SVTK_GENERIC_DATA_SET:
    case SVTK_HYPER_OCTREE:
    case SVTK_COMPOSITE_DATA_SET:
    case SVTK_HIERARCHICAL_BOX_DATA_SET: // since we cannot send svtkUniformGrid anyways.
    case SVTK_MULTIGROUP_DATA_SET:       // obsolete
    case SVTK_HIERARCHICAL_DATA_SET:     // obsolete
    default:
      svtkErrorMacro(<< "Cannot gather " << sendData->GetClassName());
      return 0;

    // send elemental data objects
    case SVTK_DIRECTED_GRAPH:
    case SVTK_UNDIRECTED_GRAPH:
    case SVTK_IMAGE_DATA:
    case SVTK_POLY_DATA:
    case SVTK_RECTILINEAR_GRID:
    case SVTK_STRUCTURED_GRID:
    case SVTK_STRUCTURED_POINTS:
    case SVTK_TABLE:
    case SVTK_TREE:
    case SVTK_UNSTRUCTURED_GRID:
    case SVTK_MULTIBLOCK_DATA_SET:
    case SVTK_UNIFORM_GRID_AMR:
    case SVTK_OVERLAPPING_AMR:

    case -1:
      return this->GatherVElementalDataObject(sendData, receiveData, destProcessId);
  }
}

//-----------------------------------------------------------------------------
int svtkCommunicator::GatherVVoidArray(const void* sendBuffer, void* recvBuffer,
  svtkIdType sendLength, svtkIdType* recvLengths, svtkIdType* offsets, int type, int destProcessId)
{
  if (this->LocalProcessId == destProcessId)
  {
    int result = 1;
    char* dest = reinterpret_cast<char*>(recvBuffer);
    int typeSize = 1;
    switch (type)
    {
      svtkTemplateMacro(typeSize = sizeof(SVTK_TT));
    }
    // Copy local data first in case buffers are the same.
    memmove(dest + offsets[this->LocalProcessId] * typeSize, sendBuffer, sendLength * typeSize);
    // Receive everything else.
    for (int i = 0; i < this->NumberOfProcesses; i++)
    {
      if (this->LocalProcessId != i)
      {
        result &= this->ReceiveVoidArray(
          dest + offsets[i] * typeSize, recvLengths[i], type, i, GATHERV_TAG);
      }
    }
    return result;
  }
  else
  {
    return this->SendVoidArray(sendBuffer, sendLength, type, destProcessId, GATHERV_TAG);
  }
}

//-----------------------------------------------------------------------------
int svtkCommunicator::GatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int destProcessId)
{
  svtkNew<svtkIdTypeArray> recvLengths;
  svtkNew<svtkIdTypeArray> offsets;
  return this->GatherV(sendBuffer, recvBuffer, recvLengths, offsets, destProcessId);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::GatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer,
  svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
{
  int type = sendBuffer->GetDataType();
  if (recvBuffer && (type != recvBuffer->GetDataType()))
  {
    svtkErrorMacro("Send/receive buffers do not match!");
    return 0;
  }
  return this->GatherVVoidArray(sendBuffer->GetVoidPointer(0),
    (recvBuffer ? recvBuffer->GetVoidPointer(0) : nullptr),
    (sendBuffer->GetNumberOfComponents() * sendBuffer->GetNumberOfTuples()), recvLengths, offsets,
    type, destProcessId);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::GatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer,
  svtkIdTypeArray* recvLengthsArray, svtkIdTypeArray* offsetsArray, int destProcessId)
{
  svtkIdType* recvLengths = recvLengthsArray->WritePointer(0, this->GetNumberOfProcesses());
  svtkIdType* offsets = offsetsArray->WritePointer(0, this->GetNumberOfProcesses() + 1);
  int numComponents = sendBuffer->GetNumberOfComponents();
  svtkIdType numTuples = sendBuffer->GetNumberOfTuples();
  svtkIdType sendLength = numComponents * numTuples;
  if (!this->Gather(&sendLength, recvLengths, 1, destProcessId))
  {
    return 0;
  }
  if (destProcessId == this->LocalProcessId)
  {
    offsets[0] = 0;
    for (int i = 0; i < this->NumberOfProcesses; i++)
    {
      if ((recvLengths[i] % numComponents) != 0)
      {
        svtkWarningMacro(<< "Not all send buffers have same tuple size.");
      }
      offsets[i + 1] = offsets[i] + recvLengths[i];
    }
    recvBuffer->SetNumberOfComponents(numComponents);
    recvBuffer->SetNumberOfTuples(offsets[this->NumberOfProcesses] / numComponents);
  }
  return this->GatherV(sendBuffer, recvBuffer, recvLengths, offsets, destProcessId);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::ScatterVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int srcProcessId)
{
  if (this->LocalProcessId == srcProcessId)
  {
    int result = 1;
    const char* src = reinterpret_cast<const char*>(sendBuffer);
    int typeSize = 1;
    switch (type)
    {
      svtkTemplateMacro(typeSize = sizeof(SVTK_TT));
    }
    // Send to everywhere.
    for (int i = 0; i < this->NumberOfProcesses; i++)
    {
      if (this->LocalProcessId == i)
      {
        memmove(recvBuffer, src + this->LocalProcessId * length * typeSize, length * typeSize);
      }
      else
      {
        result &= this->SendVoidArray(src + i * length * typeSize, length, type, i, SCATTER_TAG);
      }
    }
    return result;
  }
  else
  {
    return this->ReceiveVoidArray(recvBuffer, length, type, srcProcessId, SCATTER_TAG);
  }
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Scatter(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int srcProcessId)
{
  int type = recvBuffer->GetDataType();
  const void* sb = nullptr;
  void* rb = recvBuffer->GetVoidPointer(0);
  int numComponents = recvBuffer->GetNumberOfComponents();
  svtkIdType numTuples = recvBuffer->GetNumberOfTuples();
  if (this->LocalProcessId == srcProcessId)
  {
    if (type != sendBuffer->GetDataType())
    {
      svtkErrorMacro(<< "Data type mismatch.");
      return 0;
    }
    if (sendBuffer->GetNumberOfComponents() * sendBuffer->GetNumberOfTuples() <
      numComponents * numTuples)
    {
      svtkErrorMacro(<< "Send buffer not large enough for requested data.");
      return 0;
    }
    sb = sendBuffer->GetVoidPointer(0);
  }
  return this->ScatterVoidArray(sb, rb, numComponents * numTuples, type, srcProcessId);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::ScatterVVoidArray(const void* sendBuffer, void* recvBuffer,
  svtkIdType* sendLengths, svtkIdType* offsets, svtkIdType recvLength, int type, int srcProcessId)
{
  if (this->LocalProcessId == srcProcessId)
  {
    int result = 1;
    const char* src = reinterpret_cast<const char*>(sendBuffer);
    int typeSize = 1;
    switch (type)
    {
      svtkTemplateMacro(typeSize = sizeof(SVTK_TT));
    }
    // Send to everywhere except myself.
    for (int i = 0; i < this->NumberOfProcesses; i++)
    {
      if (this->LocalProcessId != i)
      {
        result &=
          this->SendVoidArray(src + offsets[i] * typeSize, sendLengths[i], type, i, SCATTERV_TAG);
      }
    }
    // Send to myself last in case send and receive buffers are the same.
    memmove(recvBuffer, src + offsets[this->LocalProcessId] * typeSize, recvLength * typeSize);
    return result;
  }
  else
  {
    return this->ReceiveVoidArray(recvBuffer, recvLength, type, srcProcessId, SCATTERV_TAG);
  }
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllGatherVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type)
{
  int result = 1;
  result &= this->GatherVoidArray(sendBuffer, recvBuffer, length, type, 0);
  result &= this->BroadcastVoidArray(recvBuffer, length * this->NumberOfProcesses, type, 0);
  return result;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllGather(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer)
{
  int type = sendBuffer->GetDataType();
  if (type != recvBuffer->GetDataType())
  {
    svtkErrorMacro(<< "Send and receive types do not match.");
    return 0;
  }
  int numComponents = sendBuffer->GetNumberOfComponents();
  svtkIdType numTuples = sendBuffer->GetNumberOfTuples();
  recvBuffer->SetNumberOfComponents(numComponents);
  recvBuffer->SetNumberOfTuples(numTuples * this->NumberOfProcesses);
  return this->AllGatherVoidArray(
    sendBuffer->GetVoidPointer(0), recvBuffer->GetVoidPointer(0), numComponents * numTuples, type);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllGatherVVoidArray(const void* sendBuffer, void* recvBuffer,
  svtkIdType sendLength, svtkIdType* recvLengths, svtkIdType* offsets, int type)
{
  int result = 1;
  result &=
    this->GatherVVoidArray(sendBuffer, recvBuffer, sendLength, recvLengths, offsets, type, 0);
  // Find the maximum place in the array that contains data.
  svtkIdType maxIndex = 0;
  for (int i = 0; i < this->NumberOfProcesses; i++)
  {
    svtkIdType index = recvLengths[i] + offsets[i];
    maxIndex = (maxIndex < index) ? index : maxIndex;
  }
  result &= this->BroadcastVoidArray(recvBuffer, maxIndex, type, 0);
  return result;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllGatherV(
  svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, svtkIdType* recvLengths, svtkIdType* offsets)
{
  int type = sendBuffer->GetDataType();
  if (type != recvBuffer->GetDataType())
  {
    svtkErrorMacro("Send/receive buffers do not match!");
    return 0;
  }
  return this->AllGatherVVoidArray(sendBuffer->GetVoidPointer(0), recvBuffer->GetVoidPointer(0),
    (sendBuffer->GetNumberOfComponents() * sendBuffer->GetNumberOfTuples()), recvLengths, offsets,
    type);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllGatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer)
{
  std::vector<svtkIdType> recvLengths(this->NumberOfProcesses);
  std::vector<svtkIdType> offsets(this->NumberOfProcesses + 1);
  int numComponents = sendBuffer->GetNumberOfComponents();
  svtkIdType numTuples = sendBuffer->GetNumberOfTuples();
  svtkIdType sendLength = numComponents * numTuples;
  if (!this->AllGather(&sendLength, &recvLengths.at(0), 1))
  {
    return 0;
  }
  offsets[0] = 0;
  for (int i = 0; i < this->NumberOfProcesses; i++)
  {
    if ((recvLengths[i] % numComponents) != 0)
    {
      svtkWarningMacro(<< "Not all send buffers have same tuple size.");
    }
    offsets[i + 1] = offsets[i] + recvLengths[i];
  }
  recvBuffer->SetNumberOfComponents(numComponents);
  recvBuffer->SetNumberOfTuples(offsets[this->NumberOfProcesses] / numComponents);
  return this->AllGatherV(sendBuffer, recvBuffer, &recvLengths.at(0), &offsets.at(0));
}

//-----------------------------------------------------------------------------
int svtkCommunicator::ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length,
  int type, int operation, int destProcessId)
{
#define OP_CASE(id, opclass)                                                                       \
  case id:                                                                                         \
    opClass = new svtkCommunicator##opclass##Class;                                                 \
    break;

  svtkCommunicator::Operation* opClass = nullptr;

  switch (operation)
  {
    OP_CASE(MAX_OP, Max);
    OP_CASE(MIN_OP, Min);
    OP_CASE(SUM_OP, Sum);
    OP_CASE(PRODUCT_OP, Product);
    OP_CASE(LOGICAL_AND_OP, LogicalAnd);
    OP_CASE(BITWISE_AND_OP, BitwiseAnd);
    OP_CASE(LOGICAL_OR_OP, LogicalOr);
    OP_CASE(BITWISE_OR_OP, BitwiseOr);
    OP_CASE(LOGICAL_XOR_OP, LogicalXor);
    OP_CASE(BITWISE_XOR_OP, BitwiseXor);
    default:
      svtkWarningMacro(<< "Operation number " << operation << " not supported.");
      return 0;
  }

  int retVal = this->ReduceVoidArray(sendBuffer, recvBuffer, length, type, opClass, destProcessId);
  delete opClass;

  return retVal;

#undef OP_CASE
}

//-----------------------------------------------------------------------------
int svtkCommunicator::ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length,
  int type, Operation* operation, int destProcessId)
{
  if (this->LocalProcessId < this->NumberOfProcesses - 1)
  {
    this->ReceiveVoidArray(recvBuffer, length, type, this->LocalProcessId + 1, REDUCE_TAG);
    operation->Function(sendBuffer, recvBuffer, length, type);
    sendBuffer = recvBuffer;
  }

  if (this->LocalProcessId > 0)
  {
    this->SendVoidArray(sendBuffer, length, type, this->LocalProcessId - 1, REDUCE_TAG);
    if (this->LocalProcessId == destProcessId)
    {
      this->ReceiveVoidArray(recvBuffer, length, type, 0, REDUCE_TAG);
    }
  }
  else // this->LocalProcessId == 0
  {
    if (destProcessId != 0)
    {
      this->SendVoidArray(sendBuffer, length, type, destProcessId, REDUCE_TAG);
    }
    else if (this->NumberOfProcesses == 1)
    {
      // Special case: just one process.  Copy src to destination.
      switch (type)
      {
        svtkTemplateMacro(std::copy(reinterpret_cast<const SVTK_TT*>(sendBuffer),
          reinterpret_cast<const SVTK_TT*>(sendBuffer) + length,
          reinterpret_cast<SVTK_TT*>(recvBuffer)));
      }
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Reduce(
  svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int operation, int destProcessId)
{
  int type = sendBuffer->GetDataType();
  int components = sendBuffer->GetNumberOfComponents();
  svtkIdType tuples = sendBuffer->GetNumberOfTuples();

  if (type != recvBuffer->GetDataType())
  {
    svtkErrorMacro(<< "Send and receive types do not match.");
    return 0;
  }
  recvBuffer->SetNumberOfComponents(components);
  recvBuffer->SetNumberOfTuples(tuples);

  return this->ReduceVoidArray(sendBuffer->GetVoidPointer(0), recvBuffer->GetVoidPointer(0),
    components * tuples, type, operation, destProcessId);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Reduce(
  svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, Operation* operation, int destProcessId)
{
  int type = sendBuffer->GetDataType();
  int components = sendBuffer->GetNumberOfComponents();
  svtkIdType tuples = sendBuffer->GetNumberOfTuples();

  if (type != recvBuffer->GetDataType())
  {
    svtkErrorMacro(<< "Send and receive types do not match.");
    return 0;
  }
  recvBuffer->SetNumberOfComponents(components);
  recvBuffer->SetNumberOfTuples(tuples);

  return this->ReduceVoidArray(sendBuffer->GetVoidPointer(0), recvBuffer->GetVoidPointer(0),
    components * tuples, type, operation, destProcessId);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllReduceVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int operation)
{
  if (this->ReduceVoidArray(sendBuffer, recvBuffer, length, type, operation, 0))
  {
    return this->BroadcastVoidArray(recvBuffer, length, type, 0);
  }
  return 0;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllReduceVoidArray(
  const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, Operation* operation)
{
  if (this->ReduceVoidArray(sendBuffer, recvBuffer, length, type, operation, 0))
  {
    return this->BroadcastVoidArray(recvBuffer, length, type, 0);
  }
  return 0;
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllReduce(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int operation)
{
  int type = sendBuffer->GetDataType();
  int components = sendBuffer->GetNumberOfComponents();
  svtkIdType tuples = sendBuffer->GetNumberOfTuples();

  if (type != recvBuffer->GetDataType())
  {
    svtkErrorMacro(<< "Send and receive types do not match.");
    return 0;
  }
  recvBuffer->SetNumberOfComponents(components);
  recvBuffer->SetNumberOfTuples(tuples);

  return this->AllReduceVoidArray(sendBuffer->GetVoidPointer(0), recvBuffer->GetVoidPointer(0),
    components * tuples, type, operation);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::AllReduce(
  svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, Operation* operation)
{
  int type = sendBuffer->GetDataType();
  int components = sendBuffer->GetNumberOfComponents();
  svtkIdType tuples = sendBuffer->GetNumberOfTuples();

  if (type != recvBuffer->GetDataType())
  {
    svtkErrorMacro(<< "Send and receive types do not match.");
    return 0;
  }
  recvBuffer->SetNumberOfComponents(components);
  recvBuffer->SetNumberOfTuples(tuples);

  return this->AllReduceVoidArray(sendBuffer->GetVoidPointer(0), recvBuffer->GetVoidPointer(0),
    components * tuples, type, operation);
}

//-----------------------------------------------------------------------------
int svtkCommunicator::Broadcast(svtkMultiProcessStream& stream, int srcProcessId)
{
  if (this->GetLocalProcessId() == srcProcessId)
  {
    std::vector<unsigned char> data;
    stream.GetRawData(data);
    unsigned int length = static_cast<unsigned int>(data.size());
    if (!this->Broadcast(reinterpret_cast<int*>(&length), 1, srcProcessId))
    {
      return 0;
    }
    if (length > 0)
    {
      return this->Broadcast(&data[0], length, srcProcessId);
    }
    return 1;
  }
  else
  {
    stream.Reset();
    unsigned int length = 0;
    if (!this->Broadcast(reinterpret_cast<int*>(&length), 1, srcProcessId))
    {
      return 0;
    }
    if (length > 0)
    {
      std::vector<unsigned char> data;
      data.resize(length);
      if (!this->Broadcast(&data[0], length, srcProcessId))
      {
        return 0;
      }
      stream.SetRawData(data);
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
int svtkCommunicator::Send(const svtkMultiProcessStream& stream, int remoteId, int tag)
{
  std::vector<unsigned char> data;
  stream.GetRawData(data);
  unsigned int length = static_cast<unsigned int>(data.size());
  if (!this->Send(&length, 1, remoteId, tag))
  {
    return 0;
  }
  if (length > 0)
  {
    return this->Send(&data[0], length, remoteId, tag);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkCommunicator::Receive(svtkMultiProcessStream& stream, int remoteId, int tag)
{
  stream.Reset();

  unsigned int length;
  if (!this->Receive(&length, 1, remoteId, tag))
  {
    return 0;
  }

  if (length > 0)
  {
    std::vector<unsigned char> data;
    data.resize(length);
    if (!this->Receive(&data[0], length, remoteId, tag))
    {
      return 0;
    }
    stream.SetRawData(data);
  }
  return 1;
}
