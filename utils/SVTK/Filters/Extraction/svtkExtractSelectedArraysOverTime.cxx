/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedArraysOverTime.h"

#include "svtkDataSetAttributes.h"
#include "svtkExtractDataArraysOverTime.h"
#include "svtkExtractSelection.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cassert>

//****************************************************************************
svtkStandardNewMacro(svtkExtractSelectedArraysOverTime);
//----------------------------------------------------------------------------
svtkExtractSelectedArraysOverTime::svtkExtractSelectedArraysOverTime()
  : NumberOfTimeSteps(0)
  , FieldType(svtkSelectionNode::CELL)
  , ContentType(-1)
  , ReportStatisticsOnly(false)
  , Error(svtkExtractSelectedArraysOverTime::NoError)
  , SelectionExtractor(nullptr)
  , IsExecuting(false)
{
  this->SetNumberOfInputPorts(2);
  this->ArraysExtractor = svtkSmartPointer<svtkExtractDataArraysOverTime>::New();
  this->SelectionExtractor = svtkSmartPointer<svtkExtractSelection>::New();
}

//----------------------------------------------------------------------------
svtkExtractSelectedArraysOverTime::~svtkExtractSelectedArraysOverTime()
{
  this->SetSelectionExtractor(nullptr);
}

//----------------------------------------------------------------------------
void svtkExtractSelectedArraysOverTime::SetSelectionExtractor(svtkExtractSelection* extractor)
{
  if (this->SelectionExtractor != extractor)
  {
    this->SelectionExtractor = extractor;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkExtractSelection* svtkExtractSelectedArraysOverTime::GetSelectionExtractor()
{
  return this->SelectionExtractor;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedArraysOverTime::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
  os << indent << "SelectionExtractor: " << this->SelectionExtractor.Get() << endl;
  os << indent << "ReportStatisticsOnly: " << (this->ReportStatisticsOnly ? "ON" : "OFF") << endl;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedArraysOverTime::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    // We can handle composite datasets.
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedArraysOverTime::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  assert(this->ArraysExtractor != nullptr);
  return this->ArraysExtractor->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkExtractSelectedArraysOverTime::RequestUpdateExtent(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  assert(this->ArraysExtractor != nullptr);
  return this->ArraysExtractor->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkExtractSelectedArraysOverTime::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->ArraysExtractor->GetNumberOfTimeSteps() <= 0)
  {
    svtkErrorMacro("No time steps in input data!");
    return 0;
  }

  // get the output data object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // is this the first request
  if (!this->IsExecuting)
  {
    svtkSelection* selection = svtkSelection::GetData(inputVector[1], 0);
    if (!selection)
    {
      return 1;
    }

    if (!this->DetermineSelectionType(selection))
    {
      return 0;
    }

    bool reportStats = this->ReportStatisticsOnly;
    switch (this->ContentType)
    {
      // for the following types were the number of elements selected may
      // change over time, we can only track summaries.
      case svtkSelectionNode::QUERY:
        reportStats = true;
        break;
      default:
        break;
    }

    this->ArraysExtractor->SetReportStatisticsOnly(reportStats);
    const int association = svtkSelectionNode::ConvertSelectionFieldToAttributeType(this->FieldType);
    this->ArraysExtractor->SetFieldAssociation(association);
    switch (association)
    {
      case svtkDataObject::POINT:
        this->ArraysExtractor->SetInputArrayToProcess(0, 0, 0, association, "svtkOriginalPointIds");
        break;
      case svtkDataObject::CELL:
        this->ArraysExtractor->SetInputArrayToProcess(0, 0, 0, association, "svtkOriginalCellIds");
        break;
      case svtkDataObject::ROW:
        this->ArraysExtractor->SetInputArrayToProcess(0, 0, 0, association, "svtkOriginalRowIds");
        break;
      default:
        this->ArraysExtractor->SetInputArrayToProcess(
          0, 0, 0, association, svtkDataSetAttributes::GLOBALIDS);
        break;
    }
    this->IsExecuting = true;
  }

  auto extractedData = this->Extract(inputVector, outInfo);

  svtkSmartPointer<svtkDataObject> oldData = svtkDataObject::GetData(inputVector[0], 0);
  inputVector[0]->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), extractedData);
  const int status = this->ArraysExtractor->ProcessRequest(request, inputVector, outputVector);
  inputVector[0]->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), oldData);

  if (!status)
  {
    this->IsExecuting = false;
    return 0;
  }

  if (this->IsExecuting &&
    (!request->Has(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING()) ||
      request->Get(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING()) == 0))
  {
    this->PostExecute(request, inputVector, outputVector);
    this->IsExecuting = false;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedArraysOverTime::PostExecute(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  // nothing to do.
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkDataObject> svtkExtractSelectedArraysOverTime::Extract(
  svtkInformationVector** inputVector, svtkInformation* outInfo)
{
  svtkDataObject* input = svtkDataObject::GetData(inputVector[0], 0);
  svtkSelection* selInput = svtkSelection::GetData(inputVector[1], 0);
  svtkSmartPointer<svtkExtractSelection> filter = this->SelectionExtractor;
  if (filter == nullptr)
  {
    return input;
  }
  filter->SetPreserveTopology(0);
  filter->SetInputData(0, input);
  filter->SetInputData(1, selInput);

  svtkDebugMacro(<< "Preparing subfilter to extract from dataset");
  // pass all required information to the helper filter
  int piece = 0;
  int npieces = 1;
  int* uExtent = nullptr;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }

  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    uExtent = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  }
  filter->UpdatePiece(piece, npieces, 0, uExtent);

  svtkSmartPointer<svtkDataObject> extractedData;
  extractedData.TakeReference(filter->GetOutputDataObject(0)->NewInstance());
  extractedData->ShallowCopy(filter->GetOutputDataObject(0));

  double dtime = input->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
  extractedData->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), dtime);
  return extractedData;
}

//----------------------------------------------------------------------------
int svtkExtractSelectedArraysOverTime::DetermineSelectionType(svtkSelection* sel)
{
  int contentType = -1;
  int fieldType = -1;
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
  {
    svtkSelectionNode* node = sel->GetNode(cc);
    if (node)
    {
      int nodeFieldType = node->GetFieldType();
      int nodeContentType = node->GetContentType();
      if ((fieldType != -1 && fieldType != nodeFieldType) ||
        (contentType != -1 && contentType != nodeContentType))
      {
        svtkErrorMacro("All svtkSelectionNode instances within a svtkSelection"
                      " must have the same ContentType and FieldType.");
        return 0;
      }
      fieldType = nodeFieldType;
      contentType = nodeContentType;
    }
  }
  this->ContentType = contentType;
  this->FieldType = fieldType;
  switch (this->ContentType)
  {
    case svtkSelectionNode::BLOCKS:
      // if selection blocks, assume we're extract cells.
      this->FieldType = svtkSelectionNode::CELL;
      break;
    default:
      break;
  }
  return 1;
}
