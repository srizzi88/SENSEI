/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAlgorithm.h"

#include "svtkAlgorithmOutput.h"
#include "svtkCellData.h"
#include "svtkCollection.h"
#include "svtkCollectionIterator.h"
#include "svtkCommand.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkErrorCode.h"
#include "svtkFieldData.h"
#include "svtkGarbageCollector.h"
#include "svtkGraph.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationExecutivePortKey.h"
#include "svtkInformationExecutivePortVectorKey.h"
#include "svtkInformationInformationVectorKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationStringVectorKey.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkProgressObserver.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkTrivialProducer.h"

#include <set>
#include <vector>
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkAlgorithm);

svtkCxxSetObjectMacro(svtkAlgorithm, Information, svtkInformation);

svtkInformationKeyMacro(svtkAlgorithm, INPUT_REQUIRED_DATA_TYPE, StringVector);
svtkInformationKeyMacro(svtkAlgorithm, INPUT_IS_OPTIONAL, Integer);
svtkInformationKeyMacro(svtkAlgorithm, INPUT_IS_REPEATABLE, Integer);
svtkInformationKeyMacro(svtkAlgorithm, INPUT_REQUIRED_FIELDS, InformationVector);
svtkInformationKeyMacro(svtkAlgorithm, PORT_REQUIREMENTS_FILLED, Integer);
svtkInformationKeyMacro(svtkAlgorithm, INPUT_PORT, Integer);
svtkInformationKeyMacro(svtkAlgorithm, INPUT_CONNECTION, Integer);
svtkInformationKeyMacro(svtkAlgorithm, INPUT_ARRAYS_TO_PROCESS, InformationVector);
svtkInformationKeyMacro(svtkAlgorithm, CAN_PRODUCE_SUB_EXTENT, Integer);
svtkInformationKeyMacro(svtkAlgorithm, CAN_HANDLE_PIECE_REQUEST, Integer);

svtkExecutive* svtkAlgorithm::DefaultExecutivePrototype = nullptr;

//----------------------------------------------------------------------------
class svtkAlgorithmInternals
{
public:
  // Proxy object instances for use in establishing connections from
  // the output ports to other algorithms.
  std::vector<svtkSmartPointer<svtkAlgorithmOutput> > Outputs;
};

//----------------------------------------------------------------------------
class svtkAlgorithmToExecutiveFriendship
{
public:
  static void SetAlgorithm(svtkExecutive* executive, svtkAlgorithm* algorithm)
  {
    executive->SetAlgorithm(algorithm);
  }
};

//----------------------------------------------------------------------------
svtkAlgorithm::svtkAlgorithm()
{
  this->AbortExecute = 0;
  this->ErrorCode = 0;
  this->Progress = 0.0;
  this->ProgressText = nullptr;
  this->Executive = nullptr;
  this->ProgressObserver = nullptr;
  this->InputPortInformation = svtkInformationVector::New();
  this->OutputPortInformation = svtkInformationVector::New();
  this->AlgorithmInternal = new svtkAlgorithmInternals;
  this->Information = svtkInformation::New();
  this->Information->Register(this);
  this->Information->Delete();
  this->ProgressShift = 0.0;
  this->ProgressScale = 1.0;
}

//----------------------------------------------------------------------------
svtkAlgorithm::~svtkAlgorithm()
{
  this->SetInformation(nullptr);
  if (this->Executive)
  {
    this->Executive->UnRegister(this);
    this->Executive = nullptr;
  }
  if (this->ProgressObserver)
  {
    this->ProgressObserver->UnRegister(this);
    this->ProgressObserver = nullptr;
  }
  this->InputPortInformation->Delete();
  this->OutputPortInformation->Delete();
  delete this->AlgorithmInternal;
  delete[] this->ProgressText;
  this->ProgressText = nullptr;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetProgressObserver(svtkProgressObserver* po)
{
  // This intentionally does not modify the algorithm as it
  // is usually done by executives during execution and we don't
  // want the filter to change its mtime during execution.
  if (po != this->ProgressObserver)
  {
    if (this->ProgressObserver)
    {
      this->ProgressObserver->UnRegister(this);
    }
    this->ProgressObserver = po;
    if (po)
    {
      po->Register(this);
    }
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetProgressShiftScale(double shift, double scale)
{
  this->ProgressShift = shift;
  this->ProgressScale = scale;
}

//----------------------------------------------------------------------------
// Update the progress of the process object. If a ProgressMethod exists,
// executes it. Then set the Progress ivar to amount. The parameter amount
// should range between (0,1).
void svtkAlgorithm::UpdateProgress(double amount)
{
  amount = this->GetProgressShift() + this->GetProgressScale() * amount;

  // clamp to [0, 1].
  amount = svtkMath::Min(amount, 1.0);
  amount = svtkMath::Max(amount, 0.0);

  if (this->ProgressObserver)
  {
    this->ProgressObserver->UpdateProgress(amount);
  }
  else
  {
    this->Progress = amount;
    this->InvokeEvent(svtkCommand::ProgressEvent, static_cast<void*>(&amount));
  }
}

//----------------------------------------------------------------------------
svtkInformation* svtkAlgorithm ::GetInputArrayFieldInformation(
  int idx, svtkInformationVector** inputVector)
{
  // first get out association
  svtkInformation* info = this->GetInputArrayInformation(idx);

  // then get the actual info object from the pinfo
  int port = info->Get(INPUT_PORT());
  int connection = info->Get(INPUT_CONNECTION());
  int fieldAssoc = info->Get(svtkDataObject::FIELD_ASSOCIATION());
  svtkInformation* inInfo = inputVector[port]->GetInformationObject(connection);

  if (info->Has(svtkDataObject::FIELD_NAME()))
  {
    const char* name = info->Get(svtkDataObject::FIELD_NAME());
    return svtkDataObject::GetNamedFieldInformation(inInfo, fieldAssoc, name);
  }
  int fType = info->Get(svtkDataObject::FIELD_ATTRIBUTE_TYPE());
  return svtkDataObject::GetActiveFieldInformation(inInfo, fieldAssoc, fType);
}

//----------------------------------------------------------------------------
svtkInformation* svtkAlgorithm::GetInputArrayInformation(int idx)
{
  // add this info into the algorithms info object
  svtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    inArrayVec = svtkInformationVector::New();
    this->Information->Set(INPUT_ARRAYS_TO_PROCESS(), inArrayVec);
    inArrayVec->Delete();
  }
  svtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    inArrayInfo = svtkInformation::New();
    inArrayVec->SetInformationObject(idx, inArrayInfo);
    inArrayInfo->Delete();
  }
  return inArrayInfo;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetInputArrayToProcess(int idx, svtkInformation* inInfo)
{
  svtkInformation* info = this->GetInputArrayInformation(idx);
  info->Copy(inInfo, 1);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, int attributeType)
{
  svtkInformation* info = this->GetInputArrayInformation(idx);

  info->Set(INPUT_PORT(), port);
  info->Set(INPUT_CONNECTION(), connection);
  info->Set(svtkDataObject::FIELD_ASSOCIATION(), fieldAssociation);
  info->Set(svtkDataObject::FIELD_ATTRIBUTE_TYPE(), attributeType);

  // remove name if there is one
  info->Remove(svtkDataObject::FIELD_NAME());

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetInputArrayToProcess(int idx, int port, int connection,
  const char* fieldAssociation, const char* fieldAttributeTypeOrName)
{
  if (!fieldAssociation)
  {
    svtkErrorMacro("Association is required");
    return;
  }
  if (!fieldAttributeTypeOrName)
  {
    svtkErrorMacro("Attribute type or array name is required");
    return;
  }

  int i;

  // Try to convert the string argument to an enum value
  int association = -1;
  for (i = 0; i < svtkDataObject::NUMBER_OF_ASSOCIATIONS; i++)
  {
    if (strcmp(fieldAssociation, svtkDataObject::GetAssociationTypeAsString(i)) == 0)
    {
      association = i;
      break;
    }
  }
  if (association == -1)
  {
    svtkErrorMacro("Unrecognized association type: " << fieldAssociation);
    return;
  }

  int attributeType = -1;
  for (i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    if (strcmp(fieldAttributeTypeOrName, svtkDataSetAttributes::GetLongAttributeTypeAsString(i)) ==
      0)
    {
      attributeType = i;
      break;
    }
  }
  if (attributeType == -1)
  {
    // Set by association and array name
    this->SetInputArrayToProcess(idx, port, connection, association, fieldAttributeTypeOrName);
    return;
  }

  // Set by association and attribute type
  this->SetInputArrayToProcess(idx, port, connection, association, attributeType);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  // ignore nullptr string
  if (!name)
  {
    return;
  }

  svtkInformation* info = this->GetInputArrayInformation(idx);

  // remove fieldAttr if there is one
  info->Remove(svtkDataObject::FIELD_ATTRIBUTE_TYPE());

  // Check to see whether the current input array matches -
  // if so we're done.
  if (info->Has(svtkDataObject::FIELD_NAME()) && info->Get(INPUT_PORT()) == port &&
    info->Get(INPUT_CONNECTION()) == connection &&
    info->Get(svtkDataObject::FIELD_ASSOCIATION()) == fieldAssociation &&
    info->Get(svtkDataObject::FIELD_NAME()) &&
    strcmp(info->Get(svtkDataObject::FIELD_NAME()), name) == 0)
  {
    return;
  }

  info->Set(INPUT_PORT(), port);
  info->Set(INPUT_CONNECTION(), connection);
  info->Set(svtkDataObject::FIELD_ASSOCIATION(), fieldAssociation);
  info->Set(svtkDataObject::FIELD_NAME(), name);

  this->Modified();
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetInputArrayAssociation(int idx, svtkInformationVector** inputVector)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  this->GetInputArrayToProcess(idx, inputVector, association);
  return association;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetInputArrayAssociation(
  int idx, int connection, svtkInformationVector** inputVector)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  this->GetInputArrayToProcess(idx, connection, inputVector, association);
  return association;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetInputArrayAssociation(int idx, svtkDataObject* input)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  this->GetInputArrayToProcess(idx, input, association);
  return association;
}

//----------------------------------------------------------------------------
svtkDataArray* svtkAlgorithm::GetInputArrayToProcess(int idx, svtkInformationVector** inputVector)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputArrayToProcess(idx, inputVector, association);
}

//----------------------------------------------------------------------------
svtkDataArray* svtkAlgorithm::GetInputArrayToProcess(
  int idx, svtkInformationVector** inputVector, int& association)
{
  return svtkArrayDownCast<svtkDataArray>(
    this->GetInputAbstractArrayToProcess(idx, inputVector, association));
}

//----------------------------------------------------------------------------
svtkDataArray* svtkAlgorithm::GetInputArrayToProcess(
  int idx, int connection, svtkInformationVector** inputVector)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputArrayToProcess(idx, connection, inputVector, association);
}

//----------------------------------------------------------------------------
svtkDataArray* svtkAlgorithm::GetInputArrayToProcess(
  int idx, int connection, svtkInformationVector** inputVector, int& association)
{
  return svtkArrayDownCast<svtkDataArray>(
    this->GetInputAbstractArrayToProcess(idx, connection, inputVector, association));
}

//----------------------------------------------------------------------------
svtkDataArray* svtkAlgorithm::GetInputArrayToProcess(int idx, svtkDataObject* input)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputArrayToProcess(idx, input, association);
}

//----------------------------------------------------------------------------
svtkDataArray* svtkAlgorithm::GetInputArrayToProcess(int idx, svtkDataObject* input, int& association)
{
  return svtkArrayDownCast<svtkDataArray>(
    this->GetInputAbstractArrayToProcess(idx, input, association));
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkAlgorithm::GetInputAbstractArrayToProcess(
  int idx, svtkInformationVector** inputVector)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputAbstractArrayToProcess(idx, inputVector, association);
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkAlgorithm::GetInputAbstractArrayToProcess(
  int idx, svtkInformationVector** inputVector, int& association)
{
  svtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    svtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }
  svtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    svtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }

  int connection = inArrayInfo->Get(INPUT_CONNECTION());
  return this->GetInputAbstractArrayToProcess(idx, connection, inputVector, association);
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkAlgorithm::GetInputAbstractArrayToProcess(
  int idx, int connection, svtkInformationVector** inputVector)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputAbstractArrayToProcess(idx, connection, inputVector, association);
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkAlgorithm::GetInputAbstractArrayToProcess(
  int idx, int connection, svtkInformationVector** inputVector, int& association)
{
  svtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    svtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }
  svtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    svtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }

  int port = inArrayInfo->Get(INPUT_PORT());
  svtkInformation* inInfo = inputVector[port]->GetInformationObject(connection);
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  return this->GetInputAbstractArrayToProcess(idx, input, association);
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkAlgorithm::GetInputAbstractArrayToProcess(int idx, svtkDataObject* input)
{
  int association = svtkDataObject::FIELD_ASSOCIATION_NONE;
  return this->GetInputAbstractArrayToProcess(idx, input, association);
}

//----------------------------------------------------------------------------
svtkAbstractArray* svtkAlgorithm::GetInputAbstractArrayToProcess(
  int idx, svtkDataObject* input, int& association)
{
  if (!input)
  {
    return nullptr;
  }

  svtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (!inArrayVec)
  {
    svtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }
  svtkInformation* inArrayInfo = inArrayVec->GetInformationObject(idx);
  if (!inArrayInfo)
  {
    svtkErrorMacro("Attempt to get an input array for an index that has not been specified");
    return nullptr;
  }

  int fieldAssoc = inArrayInfo->Get(svtkDataObject::FIELD_ASSOCIATION());
  association = fieldAssoc;

  if (inArrayInfo->Has(svtkDataObject::FIELD_NAME()))
  {
    const char* name = inArrayInfo->Get(svtkDataObject::FIELD_NAME());

    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_NONE)
    {
      svtkFieldData* fd = input->GetFieldData();
      return fd->GetAbstractArray(name);
    }

    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
      svtkTable* inputT = svtkTable::SafeDownCast(input);
      if (!inputT)
      {
        svtkErrorMacro("Attempt to get row data from a non-table");
        return nullptr;
      }
      svtkFieldData* fd = inputT->GetRowData();
      return fd->GetAbstractArray(name);
    }

    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_VERTICES ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_EDGES)
    {
      svtkGraph* inputG = svtkGraph::SafeDownCast(input);
      if (!inputG)
      {
        svtkErrorMacro("Attempt to get vertex or edge data from a non-graph");
        return nullptr;
      }
      svtkFieldData* fd = nullptr;
      if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_VERTICES)
      {
        association = svtkDataObject::FIELD_ASSOCIATION_VERTICES;
        fd = inputG->GetVertexData();
      }
      else
      {
        association = svtkDataObject::FIELD_ASSOCIATION_EDGES;
        fd = inputG->GetEdgeData();
      }
      return fd->GetAbstractArray(name);
    }

    if (svtkGraph::SafeDownCast(input) && fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      return svtkGraph::SafeDownCast(input)->GetVertexData()->GetAbstractArray(name);
    }

    if (svtkHyperTreeGrid::SafeDownCast(input))
    {
      return svtkHyperTreeGrid::SafeDownCast(input)->GetPointData()->GetAbstractArray(name);
    }

    svtkDataSet* inputDS = svtkDataSet::SafeDownCast(input);
    if (!inputDS)
    {
      svtkErrorMacro("Attempt to get point or cell data from a data object");
      return nullptr;
    }

    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      return inputDS->GetPointData()->GetAbstractArray(name);
    }
    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS &&
      inputDS->GetPointData()->GetAbstractArray(name))
    {
      association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
      return inputDS->GetPointData()->GetAbstractArray(name);
    }

    association = svtkDataObject::FIELD_ASSOCIATION_CELLS;
    return inputDS->GetCellData()->GetAbstractArray(name);
  }
  else if (inArrayInfo->Has(svtkDataObject::FIELD_ATTRIBUTE_TYPE()))
  {
    svtkDataSet* inputDS = svtkDataSet::SafeDownCast(input);
    if (!inputDS)
    {
      if (svtkHyperTreeGrid::SafeDownCast(input))
      {
        int fType = inArrayInfo->Get(svtkDataObject::FIELD_ATTRIBUTE_TYPE());
        return svtkHyperTreeGrid::SafeDownCast(input)->GetPointData()->GetAbstractAttribute(fType);
      }
      svtkErrorMacro("Attempt to get point or cell data from a data object");
      return nullptr;
    }
    int fType = inArrayInfo->Get(svtkDataObject::FIELD_ATTRIBUTE_TYPE());
    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      return inputDS->GetPointData()->GetAbstractAttribute(fType);
    }
    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS &&
      inputDS->GetPointData()->GetAbstractAttribute(fType))
    {
      association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
      return inputDS->GetPointData()->GetAbstractAttribute(fType);
    }

    association = svtkDataObject::FIELD_ASSOCIATION_CELLS;
    return inputDS->GetCellData()->GetAbstractAttribute(fType);
  }
  else
  {
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->HasExecutive())
  {
    os << indent << "Executive: " << this->Executive << "\n";
  }
  else
  {
    os << indent << "Executive: (none)\n";
  }

  os << indent << "ErrorCode: " << svtkErrorCode::GetStringFromErrorCode(this->ErrorCode) << endl;

  if (this->Information)
  {
    os << indent << "Information: " << this->Information << "\n";
  }
  else
  {
    os << indent << "Information: (none)\n";
  }

  os << indent << "AbortExecute: " << (this->AbortExecute ? "On\n" : "Off\n");
  os << indent << "Progress: " << this->Progress << "\n";
  if (this->ProgressText)
  {
    os << indent << "Progress Text: " << this->ProgressText << "\n";
  }
  else
  {
    os << indent << "Progress Text: (None)\n";
  }
}

//----------------------------------------------------------------------------
int svtkAlgorithm::HasExecutive()
{
  return this->Executive ? 1 : 0;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkAlgorithm::GetExecutive()
{
  // Create the default executive if we do not have one already.
  if (!this->HasExecutive())
  {
    svtkExecutive* e = this->CreateDefaultExecutive();
    this->SetExecutive(e);
    e->Delete();
  }
  return this->Executive;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetExecutive(svtkExecutive* newExecutive)
{
  svtkExecutive* oldExecutive = this->Executive;
  if (newExecutive != oldExecutive)
  {
    if (newExecutive)
    {
      newExecutive->Register(this);
      svtkAlgorithmToExecutiveFriendship::SetAlgorithm(newExecutive, this);
    }
    this->Executive = newExecutive;
    if (oldExecutive)
    {
      svtkAlgorithmToExecutiveFriendship::SetAlgorithm(oldExecutive, nullptr);
      oldExecutive->UnRegister(this);
    }
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkAlgorithm::ProcessRequest(
  svtkInformation* request, svtkCollection* inInfo, svtkInformationVector* outInfo)
{
  svtkSmartPointer<svtkCollectionIterator> iter;
  iter.TakeReference(inInfo->NewIterator());

  std::vector<svtkInformationVector*> ivectors;
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkInformationVector* iv = svtkInformationVector::SafeDownCast(iter->GetCurrentObject());
    if (!iv)
    {
      return 0;
    }
    ivectors.push_back(iv);
  }
  if (ivectors.empty())
  {
    return this->ProcessRequest(request, (svtkInformationVector**)nullptr, outInfo);
  }
  else
  {
    return this->ProcessRequest(request, &ivectors[0], outInfo);
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkAlgorithm::ProcessRequest(
  svtkInformation* /* request */, svtkInformationVector**, svtkInformationVector*)
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::ComputePipelineMTime(svtkInformation* /* request */, svtkInformationVector**,
  svtkInformationVector*, int /* requestFromOutputPort */, svtkMTimeType* mtime)
{
  // By default algorithms contribute only their own modified time.
  *mtime = this->GetMTime();
  return 1;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::ModifyRequest(svtkInformation* /*request*/, int /*when*/)
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetNumberOfInputPorts()
{
  return this->InputPortInformation->GetNumberOfInformationObjects();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetNumberOfInputPorts(int n)
{
  // Sanity check.
  if (n < 0)
  {
    svtkErrorMacro("Attempt to set number of input ports to " << n);
    n = 0;
  }

  // We must remove all connections from ports that are removed.
  for (int i = n; i < this->GetNumberOfInputPorts(); ++i)
  {
    this->SetNumberOfInputConnections(i, 0);
  }

  // Set the number of input port information objects.
  this->InputPortInformation->SetNumberOfInformationObjects(n);
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetNumberOfOutputPorts()
{
  return this->OutputPortInformation->GetNumberOfInformationObjects();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetNumberOfOutputPorts(int n)
{
  // Sanity check.
  if (n < 0)
  {
    svtkErrorMacro("Attempt to set number of output ports to " << n);
    n = 0;
  }

  // We must remove all connections from ports that are removed.
  for (int i = n; i < this->GetNumberOfOutputPorts(); ++i)
  {
    // Get the producer and its output information for this port.
    svtkExecutive* producer = this->GetExecutive();
    svtkInformation* info = producer->GetOutputInformation(i);

    // Remove all consumers' references to this producer on this port.
    svtkExecutive** consumers = svtkExecutive::CONSUMERS()->GetExecutives(info);
    int* consumerPorts = svtkExecutive::CONSUMERS()->GetPorts(info);
    int consumerCount = svtkExecutive::CONSUMERS()->Length(info);
    for (int j = 0; j < consumerCount; ++j)
    {
      svtkInformationVector* inputs = consumers[j]->GetInputInformation(consumerPorts[j]);
      inputs->Remove(info);
    }

    // Remove this producer's references to all consumers on this port.
    svtkExecutive::CONSUMERS()->Remove(info);
  }

  // Set the number of output port information objects.
  this->OutputPortInformation->SetNumberOfInformationObjects(n);

  // Set the number of connection proxy objects.
  this->AlgorithmInternal->Outputs.resize(n);
}

//----------------------------------------------------------------------------
svtkInformation* svtkAlgorithm::GetInputPortInformation(int port)
{
  if (!this->InputPortIndexInRange(port, "get information object for"))
  {
    return nullptr;
  }

  // Get the input port information object.
  svtkInformation* info = this->InputPortInformation->GetInformationObject(port);

  // Fill it if it has not yet been filled.
  if (!info->Has(PORT_REQUIREMENTS_FILLED()))
  {
    if (this->FillInputPortInformation(port, info))
    {
      info->Set(PORT_REQUIREMENTS_FILLED(), 1);
    }
    else
    {
      info->Clear();
    }
  }

  // Return the information object.
  return info;
}

//----------------------------------------------------------------------------
svtkInformation* svtkAlgorithm::GetOutputPortInformation(int port)
{
  if (!this->OutputPortIndexInRange(port, "get information object for"))
  {
    return nullptr;
  }

  // Get the output port information object.
  svtkInformation* info = this->OutputPortInformation->GetInformationObject(port);

  // Fill it if it has not yet been filled.
  if (!info->Has(PORT_REQUIREMENTS_FILLED()))
  {
    if (this->FillOutputPortInformation(port, info))
    {
      info->Set(PORT_REQUIREMENTS_FILLED(), 1);
    }
    else
    {
      info->Clear();
    }
  }

  // Return the information object.
  return info;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::FillInputPortInformation(int, svtkInformation*)
{
  svtkErrorMacro("FillInputPortInformation is not implemented.");
  return 0;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::FillOutputPortInformation(int, svtkInformation*)
{
  svtkErrorMacro("FillOutputPortInformation is not implemented.");
  return 0;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::InputPortIndexInRange(int index, const char* action)
{
  // Make sure the index of the input port is in range.
  if (index < 0 || index >= this->GetNumberOfInputPorts())
  {
    svtkErrorMacro("Attempt to " << (action ? action : "access") << " input port index " << index
                                << " for an algorithm with " << this->GetNumberOfInputPorts()
                                << " input ports.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::OutputPortIndexInRange(int index, const char* action)
{
  // Make sure the index of the output port is in range.
  if (index < 0 || index >= this->GetNumberOfOutputPorts())
  {
    svtkErrorMacro("Attempt to " << (action ? action : "access") << " output port index " << index
                                << " for an algorithm with " << this->GetNumberOfOutputPorts()
                                << " output ports.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetDefaultExecutivePrototype(svtkExecutive* proto)
{
  if (svtkAlgorithm::DefaultExecutivePrototype == proto)
  {
    return;
  }
  if (svtkAlgorithm::DefaultExecutivePrototype)
  {
    svtkAlgorithm::DefaultExecutivePrototype->UnRegister(nullptr);
    svtkAlgorithm::DefaultExecutivePrototype = nullptr;
  }
  if (proto)
  {
    proto->Register(nullptr);
  }
  svtkAlgorithm::DefaultExecutivePrototype = proto;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkAlgorithm::CreateDefaultExecutive()
{
  if (svtkAlgorithm::DefaultExecutivePrototype)
  {
    return svtkAlgorithm::DefaultExecutivePrototype->NewInstance();
  }
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::Register(svtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::UnRegister(svtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->Executive, "Executive");
}

/// These are convenience methods to forward to the executive

//----------------------------------------------------------------------------
svtkDataObject* svtkAlgorithm::GetOutputDataObject(int port)
{
  return this->GetExecutive()->GetOutputData(port);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkAlgorithm::GetInputDataObject(int port, int connection)
{
  return this->GetExecutive()->GetInputData(port, connection);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::RemoveAllInputs()
{
  this->SetInputConnection(0, nullptr);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::RemoveAllInputConnections(int port)
{
  this->SetInputConnection(port, nullptr);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetInputConnection(svtkAlgorithmOutput* input)
{
  this->SetInputConnection(0, input);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetInputConnection(int port, svtkAlgorithmOutput* input)
{
  if (!this->InputPortIndexInRange(port, "connect"))
  {
    return;
  }

  // Get the producer/consumer pair for the connection.
  svtkExecutive* producer =
    (input && input->GetProducer()) ? input->GetProducer()->GetExecutive() : nullptr;
  int producerPort = producer ? input->GetIndex() : 0;
  svtkExecutive* consumer = this->GetExecutive();
  int consumerPort = port;

  // Get the vector of connected input information objects.
  svtkInformationVector* inputs = consumer->GetInputInformation(consumerPort);

  // Get the information object from the producer of the new input.
  svtkInformation* newInfo = producer ? producer->GetOutputInformation(producerPort) : nullptr;

  // Check if the connection is already present.
  if (!newInfo && inputs->GetNumberOfInformationObjects() == 0)
  {
    return;
  }
  else if (newInfo == inputs->GetInformationObject(0) &&
    inputs->GetNumberOfInformationObjects() == 1)
  {
    return;
  }

  // The connection is not present.
  svtkDebugMacro("Setting connection to input port index "
    << consumerPort << " from output port index " << producerPort << " on algorithm "
    << (producer ? producer->GetAlgorithm()->GetClassName() : "") << "("
    << (producer ? producer->GetAlgorithm() : nullptr) << ").");

  // Add this consumer to the new input's list of consumers.
  if (newInfo)
  {
    svtkExecutive::CONSUMERS()->Append(newInfo, consumer, consumerPort);
  }

  // Remove this consumer from all old inputs' lists of consumers.
  for (int i = 0; i < inputs->GetNumberOfInformationObjects(); ++i)
  {
    if (svtkInformation* oldInfo = inputs->GetInformationObject(i))
    {
      svtkExecutive::CONSUMERS()->Remove(oldInfo, consumer, consumerPort);
    }
  }

  // Make the new input the only connection.
  if (newInfo)
  {
    inputs->SetInformationObject(0, newInfo);
    inputs->SetNumberOfInformationObjects(1);
  }
  else
  {
    inputs->SetNumberOfInformationObjects(0);
  }

  // This algorithm has been modified.
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::AddInputConnection(svtkAlgorithmOutput* input)
{
  this->AddInputConnection(0, input);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::AddInputConnection(int port, svtkAlgorithmOutput* input)
{
  if (!this->InputPortIndexInRange(port, "connect"))
  {
    return;
  }

  // If there is no input do nothing.
  if (!input || !input->GetProducer())
  {
    return;
  }

  // Get the producer/consumer pair for the connection.
  svtkExecutive* producer = input->GetProducer()->GetExecutive();
  int producerPort = input->GetIndex();
  svtkExecutive* consumer = this->GetExecutive();
  int consumerPort = port;

  // Get the vector of connected input information objects.
  svtkInformationVector* inputs = consumer->GetInputInformation(consumerPort);

  // Add the new connection.
  svtkDebugMacro("Adding connection to input port index "
    << consumerPort << " from output port index " << producerPort << " on algorithm "
    << producer->GetAlgorithm()->GetClassName() << "(" << producer->GetAlgorithm() << ").");

  // Get the information object from the producer of the new input.
  svtkInformation* newInfo = producer->GetOutputInformation(producerPort);

  // Add this consumer to the input's list of consumers.
  svtkExecutive::CONSUMERS()->Append(newInfo, consumer, consumerPort);

  // Add the information object to the list of inputs.
  inputs->Append(newInfo);

  // This algorithm has been modified.
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::RemoveInputConnection(int port, int idx)
{
  if (!this->InputPortIndexInRange(port, "disconnect"))
  {
    return;
  }

  svtkAlgorithmOutput* input = this->GetInputConnection(port, idx);
  if (input)
  {
    // We need to check if this connection exists multiple times.
    // If it does, we can't remove this from the consumers list.
    int numConnections = 0;
    int numInputConnections = this->GetNumberOfInputConnections(0);
    for (int i = 0; i < numInputConnections; i++)
    {
      if (input == this->GetInputConnection(port, i))
      {
        numConnections++;
      }
    }

    svtkExecutive* consumer = this->GetExecutive();
    int consumerPort = port;

    // Get the vector of connected input information objects.
    svtkInformationVector* inputs = consumer->GetInputInformation(consumerPort);

    // Get the producer/consumer pair for the connection.
    svtkExecutive* producer = input->GetProducer()->GetExecutive();
    int producerPort = input->GetIndex();

    // Get the information object from the producer of the old input.
    svtkInformation* oldInfo = producer->GetOutputInformation(producerPort);

    // Only connected once, remove this from inputs consumer list.
    if (numConnections == 1)
    {
      // Remove this consumer from the old input's list of consumers.
      svtkExecutive::CONSUMERS()->Remove(oldInfo, consumer, consumerPort);
    }

    // Remove the information object from the list of inputs.
    inputs->Remove(idx);

    // This algorithm has been modified.
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::RemoveInputConnection(int port, svtkAlgorithmOutput* input)
{
  if (!this->InputPortIndexInRange(port, "disconnect"))
  {
    return;
  }

  // If there is no input do nothing.
  if (!input || !input->GetProducer())
  {
    return;
  }

  // Get the producer/consumer pair for the connection.
  svtkExecutive* producer = input->GetProducer()->GetExecutive();
  int producerPort = input->GetIndex();
  svtkExecutive* consumer = this->GetExecutive();
  int consumerPort = port;

  // Get the vector of connected input information objects.
  svtkInformationVector* inputs = consumer->GetInputInformation(consumerPort);

  // Remove the connection.
  svtkDebugMacro("Removing connection to input port index "
    << consumerPort << " from output port index " << producerPort << " on algorithm "
    << producer->GetAlgorithm()->GetClassName() << "(" << producer->GetAlgorithm() << ").");

  // Get the information object from the producer of the old input.
  svtkInformation* oldInfo = producer->GetOutputInformation(producerPort);

  // Remove this consumer from the old input's list of consumers.
  svtkExecutive::CONSUMERS()->Remove(oldInfo, consumer, consumerPort);

  // Remove the information object from the list of inputs.
  inputs->Remove(oldInfo);

  // This algorithm has been modified.
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetNthInputConnection(int port, int index, svtkAlgorithmOutput* input)
{
  if (!this->InputPortIndexInRange(port, "replace connection"))
  {
    return;
  }

  // Get the producer/consumer pair for the connection.
  svtkExecutive* producer =
    (input && input->GetProducer()) ? input->GetProducer()->GetExecutive() : nullptr;
  int producerPort = producer ? input->GetIndex() : 0;
  svtkExecutive* consumer = this->GetExecutive();
  int consumerPort = port;

  // Get the vector of connected input information objects.
  svtkInformationVector* inputs = consumer->GetInputInformation(consumerPort);

  // Check for any existing connection with this index.
  svtkInformation* oldInfo = inputs->GetInformationObject(index);

  // Get the information object from the producer of the input.
  svtkInformation* newInfo = producer ? producer->GetOutputInformation(producerPort) : nullptr;

  // If the connection has not changed, do nothing.
  if (newInfo == oldInfo)
  {
    return;
  }

  // Set the connection.
  svtkDebugMacro("Setting connection index "
    << index << " to input port index " << consumerPort << " from output port index "
    << producerPort << " on algorithm "
    << (producer ? producer->GetAlgorithm()->GetClassName() : "") << "("
    << (producer ? producer->GetAlgorithm() : nullptr) << ").");

  // Add the consumer to the new input's list of consumers.
  if (newInfo)
  {
    svtkExecutive::CONSUMERS()->Append(newInfo, consumer, consumerPort);
  }

  // Remove the consumer from the old input's list of consumers.
  if (oldInfo)
  {
    svtkExecutive::CONSUMERS()->Remove(oldInfo, consumer, consumerPort);
  }

  // Store the information object in the vector of input connections.
  inputs->SetInformationObject(index, newInfo);

  // This algorithm has been modified.
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetNumberOfInputConnections(int port, int n)
{
  // Get the consumer executive and port number.
  svtkExecutive* consumer = this->GetExecutive();
  int consumerPort = port;

  // Get the vector of connected input information objects.
  svtkInformationVector* inputs = consumer->GetInputInformation(consumerPort);

  // If the number of connections has not changed, do nothing.
  if (n == inputs->GetNumberOfInformationObjects())
  {
    return;
  }

  // Remove connections beyond the new number.
  for (int i = n; i < inputs->GetNumberOfInformationObjects(); ++i)
  {
    // Remove each input's reference to this consumer.
    if (svtkInformation* oldInfo = inputs->GetInformationObject(i))
    {
      svtkExecutive::CONSUMERS()->Remove(oldInfo, consumer, consumerPort);
    }
  }

  // Set the number of connected inputs.  Non-existing inputs will be
  // empty information objects.
  inputs->SetNumberOfInformationObjects(n);

  // This algorithm has been modified.
  this->Modified();
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkAlgorithm::GetOutputPort(int port)
{
  if (!this->OutputPortIndexInRange(port, "get"))
  {
    return nullptr;
  }

  // Create the svtkAlgorithmOutput proxy object if there is not one.
  if (!this->AlgorithmInternal->Outputs[port])
  {
    this->AlgorithmInternal->Outputs[port] = svtkSmartPointer<svtkAlgorithmOutput>::New();
    this->AlgorithmInternal->Outputs[port]->SetProducer(this);
    this->AlgorithmInternal->Outputs[port]->SetIndex(port);
  }

  // Return the proxy object instance.
  return this->AlgorithmInternal->Outputs[port];
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetNumberOfInputConnections(int port)
{
  if (this->Executive)
  {
    return this->Executive->GetNumberOfInputConnections(port);
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetTotalNumberOfInputConnections()
{
  int i;
  int total = 0;
  for (i = 0; i < this->GetNumberOfInputPorts(); ++i)
  {
    total += this->GetNumberOfInputConnections(i);
  }
  return total;
}

//----------------------------------------------------------------------------
svtkInformation* svtkAlgorithm::GetOutputInformation(int port)
{
  return this->GetExecutive()->GetOutputInformation(port);
}

//----------------------------------------------------------------------------
svtkInformation* svtkAlgorithm::GetInputInformation(int port, int index)
{
  if (index < 0 || index >= this->GetNumberOfInputConnections(port))
  {
    svtkErrorMacro("Attempt to get connection index "
      << index << " for input port " << port << ", which has "
      << this->GetNumberOfInputConnections(port) << " connections.");
    return nullptr;
  }
  return this->GetExecutive()->GetInputInformation(port, index);
}

//----------------------------------------------------------------------------
svtkAlgorithm* svtkAlgorithm::GetInputAlgorithm(int port, int index)
{
  int dummy;
  return this->GetInputAlgorithm(port, index, dummy);
}

//----------------------------------------------------------------------------
svtkAlgorithm* svtkAlgorithm::GetInputAlgorithm(int port, int index, int& algPort)
{
  svtkAlgorithmOutput* aoutput = this->GetInputConnection(port, index);
  if (!aoutput)
  {
    return nullptr;
  }
  algPort = aoutput->GetIndex();
  return aoutput->GetProducer();
}

//----------------------------------------------------------------------------
svtkExecutive* svtkAlgorithm::GetInputExecutive(int port, int index)
{
  if (index < 0 || index >= this->GetNumberOfInputConnections(port))
  {
    svtkErrorMacro("Attempt to get connection index "
      << index << " for input port " << port << ", which has "
      << this->GetNumberOfInputConnections(port) << " connections.");
    return nullptr;
  }
  if (svtkInformation* info = this->GetExecutive()->GetInputInformation(port, index))
  {
    // Get the executive producing this input.  If there is none, then
    // it is a nullptr input.
    svtkExecutive* producer;
    int producerPort;
    svtkExecutive::PRODUCER()->Get(info, producer, producerPort);
    return producer;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkAlgorithm::GetInputConnection(int port, int index)
{
  if (port < 0 || port >= this->GetNumberOfInputPorts())
  {
    svtkErrorMacro("Attempt to get connection index " << index << " for input port " << port
                                                     << ", for an algorithm with "
                                                     << this->GetNumberOfInputPorts() << " ports.");
    return nullptr;
  }
  if (index < 0 || index >= this->GetNumberOfInputConnections(port))
  {
    return nullptr;
  }
  if (svtkInformation* info = this->GetExecutive()->GetInputInformation(port, index))
  {
    // Get the executive producing this input.  If there is none, then
    // it is a nullptr input.
    svtkExecutive* producer;
    int producerPort;
    svtkExecutive::PRODUCER()->Get(info, producer, producerPort);
    if (producer)
    {
      return producer->GetAlgorithm()->GetOutputPort(producerPort);
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::Update()
{
  int port = -1;
  if (this->GetNumberOfOutputPorts())
  {
    port = 0;
  }
  this->Update(port);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::Update(int port)
{
  this->GetExecutive()->Update(port);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkAlgorithm::Update(int port, svtkInformationVector* requests)
{
  svtkStreamingDemandDrivenPipeline* sddp =
    svtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (sddp)
  {
    return sddp->Update(port, requests);
  }
  else
  {
    return this->GetExecutive()->Update(port);
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkAlgorithm::Update(svtkInformation* requests)
{
  svtkNew<svtkInformationVector> reqs;
  reqs->SetInformationObject(0, requests);
  return this->Update(0, reqs);
}

//----------------------------------------------------------------------------
int svtkAlgorithm::UpdatePiece(int piece, int numPieces, int ghostLevels, const int extents[6])
{
  typedef svtkStreamingDemandDrivenPipeline svtkSDDP;

  svtkNew<svtkInformation> reqs;
  reqs->Set(svtkSDDP::UPDATE_PIECE_NUMBER(), piece);
  reqs->Set(svtkSDDP::UPDATE_NUMBER_OF_PIECES(), numPieces);
  reqs->Set(svtkSDDP::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  if (extents)
  {
    reqs->Set(svtkSDDP::UPDATE_EXTENT(), extents, 6);
  }
  return this->Update(reqs);
}

//----------------------------------------------------------------------------
int svtkAlgorithm::UpdateExtent(const int extents[6])
{
  typedef svtkStreamingDemandDrivenPipeline svtkSDDP;

  svtkNew<svtkInformation> reqs;
  reqs->Set(svtkSDDP::UPDATE_EXTENT(), extents, 6);
  return this->Update(reqs);
}

//----------------------------------------------------------------------------
int svtkAlgorithm::UpdateTimeStep(
  double time, int piece, int numPieces, int ghostLevels, const int extents[6])
{
  typedef svtkStreamingDemandDrivenPipeline svtkSDDP;

  svtkNew<svtkInformation> reqs;
  reqs->Set(svtkSDDP::UPDATE_TIME_STEP(), time);
  if (piece >= 0)
  {
    reqs->Set(svtkSDDP::UPDATE_PIECE_NUMBER(), piece);
    reqs->Set(svtkSDDP::UPDATE_NUMBER_OF_PIECES(), numPieces);
    reqs->Set(svtkSDDP::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  }
  if (extents)
  {
    reqs->Set(svtkSDDP::UPDATE_EXTENT(), extents, 6);
  }
  return this->Update(reqs);
}

//----------------------------------------------------------------------------
void svtkAlgorithm::PropagateUpdateExtent()
{
  this->UpdateInformation();

  svtkStreamingDemandDrivenPipeline* sddp =
    svtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (sddp)
  {
    sddp->PropagateUpdateExtent(-1);
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::UpdateInformation()
{
  svtkDemandDrivenPipeline* ddp = svtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (ddp)
  {
    ddp->UpdateInformation();
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::UpdateDataObject()
{
  svtkDemandDrivenPipeline* ddp = svtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (ddp)
  {
    ddp->UpdateDataObject();
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::UpdateWholeExtent()
{
  svtkStreamingDemandDrivenPipeline* sddp =
    svtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (sddp)
  {
    sddp->UpdateWholeExtent();
  }
  else
  {
    this->Update();
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::ConvertTotalInputToPortConnection(int ind, int& port, int& conn)
{
  port = 0;
  conn = 0;
  while (ind && port < this->GetNumberOfInputPorts())
  {
    int pNumCon;
    pNumCon = this->GetNumberOfInputConnections(port);
    if (ind >= pNumCon)
    {
      port++;
      ind -= pNumCon;
    }
    else
    {
      return;
    }
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::ReleaseDataFlagOn()
{
  if (svtkDemandDrivenPipeline* ddp = svtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
  {
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      ddp->SetReleaseDataFlag(i, 1);
    }
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::ReleaseDataFlagOff()
{
  if (svtkDemandDrivenPipeline* ddp = svtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
  {
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      ddp->SetReleaseDataFlag(i, 0);
    }
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetReleaseDataFlag(int val)
{
  if (svtkDemandDrivenPipeline* ddp = svtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
  {
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      ddp->SetReleaseDataFlag(i, val);
    }
  }
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetReleaseDataFlag()
{
  if (svtkDemandDrivenPipeline* ddp = svtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive()))
  {
    return ddp->GetReleaseDataFlag(0);
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::UpdateExtentIsEmpty(svtkInformation* pinfo, svtkDataObject* output)
{
  if (output == nullptr)
  {
    return 1;
  }

  // get the pinfo object then call the info signature
  return this->UpdateExtentIsEmpty(
    pinfo, output->GetInformation()->Get(svtkDataObject::DATA_EXTENT_TYPE()));
}
//----------------------------------------------------------------------------
int svtkAlgorithm::UpdateExtentIsEmpty(svtkInformation* info, int extentType)
{
  if (!info)
  {
    return 1;
  }

  int* ext;

  switch (extentType)
  {
    case SVTK_PIECES_EXTENT:
      // Special way of asking for no input.
      if (info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()) == 0)
      {
        return 1;
      }
      break;

    case SVTK_3D_EXTENT:
      ext = info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
      // Special way of asking for no input. (zero volume)
      if (!ext || ext[0] == (ext[1] + 1) || ext[2] == (ext[3] + 1) || ext[4] == (ext[5] + 1))
      {
        return 1;
      }
      break;

      // We should never have this case occur
    default:
      svtkErrorMacro(<< "Internal error - invalid extent type!");
      break;
  }

  return 0;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetProgressText(const char* ptext)
{
  if (!this->ProgressText && !ptext)
  {
    return;
  }
  if (this->ProgressText && ptext && (strcmp(this->ProgressText, ptext)) == 0)
  {
    return;
  }
  delete[] this->ProgressText;
  this->ProgressText = svtksys::SystemTools::DuplicateString(ptext);
}

// This is here to shut off warnings about deprecated functions
// calling deprecated functions.
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

//----------------------------------------------------------------------------
int* svtkAlgorithm::GetUpdateExtent(int port)
{
  if (this->GetOutputInformation(port))
  {
    return svtkStreamingDemandDrivenPipeline::GetUpdateExtent(this->GetOutputInformation(port));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::GetUpdateExtent(int port, int& x0, int& x1, int& y0, int& y1, int& z0, int& z1)
{
  if (this->GetOutputInformation(port))
  {
    int extent[6];
    svtkStreamingDemandDrivenPipeline::GetUpdateExtent(this->GetOutputInformation(port), extent);
    x0 = extent[0];
    x1 = extent[1];
    y0 = extent[2];
    y1 = extent[3];
    z0 = extent[4];
    z1 = extent[5];
  }
}

//----------------------------------------------------------------------------
void svtkAlgorithm::GetUpdateExtent(int port, int extent[6])
{
  if (this->GetOutputInformation(port))
  {
    svtkStreamingDemandDrivenPipeline::GetUpdateExtent(this->GetOutputInformation(port), extent);
  }
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetUpdatePiece(int port)
{
  if (this->GetOutputInformation(port))
  {
    return svtkStreamingDemandDrivenPipeline::GetUpdatePiece(this->GetOutputInformation(port));
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetUpdateNumberOfPieces(int port)
{
  if (this->GetOutputInformation(port))
  {
    return svtkStreamingDemandDrivenPipeline::GetUpdateNumberOfPieces(
      this->GetOutputInformation(port));
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkAlgorithm::GetUpdateGhostLevel(int port)
{
  if (this->GetOutputInformation(port))
  {
    return svtkStreamingDemandDrivenPipeline::GetUpdateGhostLevel(this->GetOutputInformation(port));
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkAlgorithm::SetInputDataObject(int port, svtkDataObject* input)
{
  if (input == nullptr)
  {
    // Setting a nullptr input removes the connection.
    this->SetInputConnection(port, nullptr);
    return;
  }

  // We need to setup a trivial producer connection. However, we need to ensure
  // that the input is indeed different from what's currently setup otherwise
  // the algorithm will be modified unnecessarily. This will make it possible
  // for users to call SetInputData(..) with the same data-output and not have
  // the filter re-execute unless the data really changed.

  if (!this->InputPortIndexInRange(port, "connect"))
  {
    return;
  }

  if (this->GetNumberOfInputConnections(port) == 1)
  {
    svtkAlgorithmOutput* current = this->GetInputConnection(port, 0);
    svtkAlgorithm* producer = current ? current->GetProducer() : nullptr;
    if (svtkTrivialProducer::SafeDownCast(producer) && producer->GetOutputDataObject(0) == input)
    {
      // the data object is unchanged. Nothing to do here.
      return;
    }
  }

  svtkTrivialProducer* tp = svtkTrivialProducer::New();
  tp->SetOutput(input);
  this->SetInputConnection(port, tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
void svtkAlgorithm::AddInputDataObject(int port, svtkDataObject* input)
{
  if (input)
  {
    svtkTrivialProducer* tp = svtkTrivialProducer::New();
    tp->SetOutput(input);
    this->AddInputConnection(port, tp->GetOutputPort());
    tp->Delete();
  }
}

//----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
void svtkAlgorithm::SetProgress(double val)
{
  SVTK_LEGACY_REPLACED_BODY(svtkAlgorithm::SetProgress, "SVTK 9.0", svtkAlgorithm::UpdateProgress);
  this->UpdateProgress(val);
}
#endif
