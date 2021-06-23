/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStatisticsAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkToolkits.h"

#include "svtkStatisticsAlgorithm.h"

#include "svtkDataObjectCollection.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStatisticsAlgorithmPrivate.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <sstream>
#include <vector>

svtkCxxSetObjectMacro(svtkStatisticsAlgorithm, AssessNames, svtkStringArray);

// ----------------------------------------------------------------------
svtkStatisticsAlgorithm::svtkStatisticsAlgorithm()
{
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(3);

  // If not told otherwise, only run Learn option
  this->LearnOption = true;
  this->DeriveOption = true;
  this->AssessOption = false;
  this->TestOption = false;
  // Most engines have only 1 primary table.
  this->NumberOfPrimaryTables = 1;
  this->AssessNames = svtkStringArray::New();
  this->Internals = new svtkStatisticsAlgorithmPrivate;
}

// ----------------------------------------------------------------------
svtkStatisticsAlgorithm::~svtkStatisticsAlgorithm()
{
  this->SetAssessNames(nullptr);
  delete this->Internals;
}

// ----------------------------------------------------------------------
void svtkStatisticsAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Learn: " << this->LearnOption << endl;
  os << indent << "Derive: " << this->DeriveOption << endl;
  os << indent << "Assess: " << this->AssessOption << endl;
  os << indent << "Test: " << this->TestOption << endl;
  os << indent << "NumberOfPrimaryTables: " << this->NumberOfPrimaryTables << endl;
  if (this->AssessNames)
  {
    this->AssessNames->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Internals: " << this->Internals << endl;
}

// ----------------------------------------------------------------------
int svtkStatisticsAlgorithm::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == INPUT_DATA)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }
  else if (port == INPUT_MODEL)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
    return 1;
  }
  else if (port == LEARN_PARAMETERS)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------
int svtkStatisticsAlgorithm::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == OUTPUT_DATA)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
    return 1;
  }
  else if (port == OUTPUT_MODEL)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
    return 1;
  }
  else if (port == OUTPUT_TEST)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
void svtkStatisticsAlgorithm::SetColumnStatus(const char* namCol, int status)
{
  this->Internals->SetBufferColumnStatus(namCol, status);
}

//---------------------------------------------------------------------------
void svtkStatisticsAlgorithm::ResetAllColumnStates()
{
  this->Internals->ResetBuffer();
}

//---------------------------------------------------------------------------
int svtkStatisticsAlgorithm::RequestSelectedColumns()
{
  return this->Internals->AddBufferToRequests();
}

//---------------------------------------------------------------------------
void svtkStatisticsAlgorithm::ResetRequests()
{
  this->Internals->ResetRequests();
}

//---------------------------------------------------------------------------
svtkIdType svtkStatisticsAlgorithm::GetNumberOfRequests()
{
  return this->Internals->GetNumberOfRequests();
}

//---------------------------------------------------------------------------
svtkIdType svtkStatisticsAlgorithm::GetNumberOfColumnsForRequest(svtkIdType request)
{
  return this->Internals->GetNumberOfColumnsForRequest(request);
}

//---------------------------------------------------------------------------
const char* svtkStatisticsAlgorithm::GetColumnForRequest(svtkIdType r, svtkIdType c)
{
  static svtkStdString columnName;
  if (this->Internals->GetColumnForRequest(r, c, columnName))
  {
    return columnName.c_str();
  }
  return nullptr;
}

//---------------------------------------------------------------------------
int svtkStatisticsAlgorithm::GetColumnForRequest(svtkIdType r, svtkIdType c, svtkStdString& columnName)
{
  return this->Internals->GetColumnForRequest(r, c, columnName) ? 1 : 0;
}

// ----------------------------------------------------------------------
void svtkStatisticsAlgorithm::AddColumn(const char* namCol)
{
  if (this->Internals->AddColumnToRequests(namCol))
  {
    this->Modified();
  }
}

// ----------------------------------------------------------------------
void svtkStatisticsAlgorithm::AddColumnPair(const char* namColX, const char* namColY)
{
  if (this->Internals->AddColumnPairToRequests(namColX, namColY))
  {
    this->Modified();
  }
}

// ----------------------------------------------------------------------
bool svtkStatisticsAlgorithm::SetParameter(
  const char* svtkNotUsed(parameter), int svtkNotUsed(index), svtkVariant svtkNotUsed(value))
{
  return false;
}

// ----------------------------------------------------------------------
int svtkStatisticsAlgorithm::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Extract inputs
  svtkTable* inData = svtkTable::GetData(inputVector[INPUT_DATA], 0);
  svtkMultiBlockDataSet* inModel = svtkMultiBlockDataSet::GetData(inputVector[INPUT_MODEL], 0);
  svtkTable* inParameters = svtkTable::GetData(inputVector[LEARN_PARAMETERS], 0);

  // Extract outputs
  svtkTable* outData = svtkTable::GetData(outputVector, OUTPUT_DATA);
  svtkMultiBlockDataSet* outModel = svtkMultiBlockDataSet::GetData(outputVector, OUTPUT_MODEL);
  svtkTable* outTest = svtkTable::GetData(outputVector, OUTPUT_TEST);

  // If input data table is not null then shallow copy it to output
  if (inData)
  {
    outData->ShallowCopy(inData);
  }

  // If there are any columns selected in the buffer which have not been
  // turned into a request by RequestSelectedColumns(), add them now.
  // There should be no effect if svtkStatisticsAlgorithmPrivate::Buffer is empty.
  // This is here to accommodate the simpler user interfaces in OverView for
  // univariate and bivariate algorithms which will not call RequestSelectedColumns()
  // on their own.
  this->RequestSelectedColumns();

  // Calculate primary statistics if requested
  if (this->LearnOption)
  {
    // First, learn primary statistics from data; otherwise, only use input model as output model
    this->Learn(inData, inParameters, outModel);

    // Second, aggregate learned models with input model if one is present
    if (inModel)
    {
      svtkDataObjectCollection* models = svtkDataObjectCollection::New();
      models->AddItem(inModel);
      models->AddItem(outModel);
      this->Aggregate(models, outModel);
      models->Delete();
    }
  }
  else
  {
    // No input data and no input model result in an error condition
    if (!inModel)
    {
      svtkErrorMacro("No model available AND no Learn phase requested. Cannot proceed with "
                    "statistics algorithm.");
      return 1;
    }

    // Since no learn phase was requested, the output model is equal to the input one
    outModel->ShallowCopy(inModel);
  }

  // Calculate derived statistics if requested
  if (this->DeriveOption)
  {
    this->Derive(outModel);
  }

  // Assess data with respect to statistical model if requested
  if (this->AssessOption)
  {
    this->Assess(inData, outModel, outData);
  }

  // Calculate test statistics if requested
  if (this->TestOption)
  {
    this->Test(inData, outModel, outTest);
  }

  return 1;
}

// ----------------------------------------------------------------------
void svtkStatisticsAlgorithm::Assess(
  svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outData, int numVariables)
{
  if (!inData)
  {
    return;
  }

  if (!inMeta)
  {
    return;
  }

  // Loop over requests
  for (std::set<std::set<svtkStdString> >::const_iterator rit = this->Internals->Requests.begin();
       rit != this->Internals->Requests.end(); ++rit)
  {
    // Storage for variable names of the request (smart pointer because of several exit points)
    svtkSmartPointer<svtkStringArray> varNames = svtkSmartPointer<svtkStringArray>::New();
    varNames->SetNumberOfValues(numVariables);

    // Each request must contain numVariables columns of interest (additional columns are ignored)
    bool invalidRequest = false;
    int v = 0;
    for (std::set<svtkStdString>::const_iterator it = rit->begin();
         v < numVariables && it != rit->end(); ++v, ++it)
    {
      // Try to retrieve column with corresponding name in input data
      svtkStdString varName = *it;

      // If requested column does not exist in input, ignore request
      if (!inData->GetColumnByName(varName))
      {
        svtkWarningMacro("InData table does not have a column "
          << varName.c_str() << ". Ignoring request containing it.");

        invalidRequest = true;
        break;
      }

      // If column with corresponding name was found, store name
      varNames->SetValue(v, varName);
    }
    if (invalidRequest)
    {
      continue;
    }

    // If request is too short, it must also be ignored
    if (v < numVariables)
    {
      svtkWarningMacro("Only " << v << " variables in the request while " << numVariables
                              << "are needed. Ignoring request.");

      continue;
    }

    // Store names to be able to use SetValueByName, and create the outData columns
    svtkIdType nAssessments = this->AssessNames->GetNumberOfValues();
    std::vector<svtkStdString> names(nAssessments);
    svtkIdType nRowData = inData->GetNumberOfRows();
    for (svtkIdType a = 0; a < nAssessments; ++a)
    {
      // Prepare string for numVariables-tuple of variable names
      std::ostringstream assessColName;
      assessColName << this->AssessNames->GetValue(a) << "(";
      for (int i = 0; i < numVariables; ++i)
      {
        // Insert comma before each variable name, save the first one
        if (i > 0)
        {
          assessColName << ",";
        }
        assessColName << varNames->GetValue(i);
      }
      assessColName << ")";

      names[a] = assessColName.str().c_str();

      // Create assessment columns with names <AssessmentName>(var1,...,varN)
      svtkDoubleArray* assessColumn = svtkDoubleArray::New();
      assessColumn->SetName(names[a]);
      assessColumn->SetNumberOfTuples(nRowData);
      outData->AddColumn(assessColumn);
      assessColumn->Delete();
    }

    // Select assess functor
    AssessFunctor* dfunc;
    this->SelectAssessFunctor(outData, inMeta, varNames, dfunc);

    if (!dfunc)
    {
      // Functor selection did not work. Do nothing.
      svtkWarningMacro("AssessFunctors could not be allocated. Ignoring request.");
    }
    else
    {
      // Assess each entry of the column
      svtkDoubleArray* assessResult = svtkDoubleArray::New();
      for (svtkIdType r = 0; r < nRowData; ++r)
      {
        // Apply functor
        (*dfunc)(assessResult, r);
        for (svtkIdType a = 0; a < nAssessments; ++a)
        {
          // Store each assessment value in corresponding assessment column
          outData->SetValueByName(r, names[a], assessResult->GetValue(a));
        }
      }

      assessResult->Delete();
    }

    delete dfunc;
  }
}
