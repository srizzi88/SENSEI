/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkBivariateLinearTableThreshold.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkBivariateLinearTableThreshold.h"

#include "svtkDataArrayCollection.h"
#include "svtkDataObject.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"

#include <map>

svtkStandardNewMacro(svtkBivariateLinearTableThreshold);

class svtkBivariateLinearTableThreshold::Internals
{
public:
  std::vector<svtkIdType> ColumnsToThreshold;
  std::vector<svtkIdType> ColumnComponentsToThreshold;
};

svtkBivariateLinearTableThreshold::svtkBivariateLinearTableThreshold()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(2);

  this->Implementation = new Internals;

  this->Initialize();
}

svtkBivariateLinearTableThreshold::~svtkBivariateLinearTableThreshold()
{
  delete this->Implementation;
}

void svtkBivariateLinearTableThreshold::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "ColumnRanges: " << this->ColumnRanges[0] << " " << this->ColumnRanges[1] << endl;
  os << "UseNormalizedDistance: " << this->UseNormalizedDistance << endl;
  os << "Inclusive: " << this->Inclusive << endl;
  os << "DistanceThreshold: " << this->DistanceThreshold << endl;
  os << "LinearThresholdType: " << this->LinearThresholdType << endl;
}

void svtkBivariateLinearTableThreshold::Initialize()
{
  this->Inclusive = 0;
  this->Implementation->ColumnsToThreshold.clear();
  this->Implementation->ColumnComponentsToThreshold.clear();

  this->DistanceThreshold = 1.0;
  this->ColumnRanges[0] = 1.0;
  this->ColumnRanges[1] = 1.0;
  this->UseNormalizedDistance = 0;
  this->NumberOfLineEquations = 0;
  this->LinearThresholdType = BLT_NEAR;

  this->LineEquations = svtkSmartPointer<svtkDoubleArray>::New();
  this->LineEquations->SetNumberOfComponents(3);
  this->Modified();
}

int svtkBivariateLinearTableThreshold::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* inTable = svtkTable::GetData(inputVector[0], 0);
  svtkTable* outRowIdsTable = svtkTable::GetData(outputVector, OUTPUT_ROW_IDS);
  svtkTable* outRowDataTable = svtkTable::GetData(outputVector, OUTPUT_ROW_DATA);

  if (!inTable || this->GetNumberOfColumnsToThreshold() != 2)
  {
    return 1;
  }

  if (!outRowIdsTable)
  {
    svtkErrorMacro(<< "No output table, for some reason.");
    return 0;
  }

  svtkSmartPointer<svtkIdTypeArray> outIds = svtkSmartPointer<svtkIdTypeArray>::New();
  if (!ApplyThreshold(inTable, outIds))
  {
    svtkErrorMacro(<< "Error during threshold application.");
    return 0;
  }

  outRowIdsTable->Initialize();
  outRowIdsTable->AddColumn(outIds);

  outRowDataTable->Initialize();
  svtkIdType numColumns = inTable->GetNumberOfColumns();
  for (svtkIdType i = 0; i < numColumns; i++)
  {
    svtkDataArray* a = svtkDataArray::CreateDataArray(inTable->GetColumn(i)->GetDataType());
    a->SetNumberOfComponents(inTable->GetColumn(i)->GetNumberOfComponents());
    a->SetName(inTable->GetColumn(i)->GetName());
    outRowDataTable->AddColumn(a);
    a->Delete();
  }

  for (svtkIdType i = 0; i < outIds->GetNumberOfTuples(); i++)
  {
    outRowDataTable->InsertNextRow(inTable->GetRow(outIds->GetValue(i)));
  }

  return 1;
}

int svtkBivariateLinearTableThreshold::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }

  return 0;
}

int svtkBivariateLinearTableThreshold::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == OUTPUT_ROW_IDS || port == OUTPUT_ROW_DATA)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
    return 1;
  }

  return 0;
}

void svtkBivariateLinearTableThreshold::AddColumnToThreshold(svtkIdType column, svtkIdType component)
{
  this->Implementation->ColumnsToThreshold.push_back(column);
  this->Implementation->ColumnComponentsToThreshold.push_back(component);
  this->Modified();
}

int svtkBivariateLinearTableThreshold::GetNumberOfColumnsToThreshold()
{
  return (int)this->Implementation->ColumnsToThreshold.size();
}

void svtkBivariateLinearTableThreshold::GetColumnToThreshold(
  svtkIdType idx, svtkIdType& column, svtkIdType& component)
{
  if (idx < 0 || idx >= (int)this->Implementation->ColumnsToThreshold.size())
  {
    column = -1;
    component = -1;
  }
  else
  {
    column = this->Implementation->ColumnsToThreshold[idx];
    component = this->Implementation->ColumnComponentsToThreshold[idx];
  }
}

void svtkBivariateLinearTableThreshold::ClearColumnsToThreshold()
{
  this->Implementation->ColumnsToThreshold.clear();
  this->Implementation->ColumnComponentsToThreshold.clear();
}

svtkIdTypeArray* svtkBivariateLinearTableThreshold::GetSelectedRowIds(int selection)
{
  svtkTable* table = svtkTable::SafeDownCast(this->GetOutput());

  if (!table)
    return nullptr;

  return svtkArrayDownCast<svtkIdTypeArray>(table->GetColumn(selection));
}

int svtkBivariateLinearTableThreshold::ApplyThreshold(
  svtkTable* tableToThreshold, svtkIdTypeArray* acceptedIds)
{
  // grab the first two arrays (and their components) to threshold
  svtkIdType column1, column2, component1, component2;

  if (this->GetNumberOfColumnsToThreshold() != 2)
  {
    svtkErrorMacro(<< "This threshold only works on two columns at a time.  Received: "
                  << this->GetNumberOfColumnsToThreshold());
    return 0;
  }

  this->GetColumnToThreshold(0, column1, component1);
  this->GetColumnToThreshold(1, column2, component2);

  svtkDataArray* a1 = svtkArrayDownCast<svtkDataArray>(tableToThreshold->GetColumn(column1));
  svtkDataArray* a2 = svtkArrayDownCast<svtkDataArray>(tableToThreshold->GetColumn(column2));

  if (!a1 || !a2)
  {
    svtkErrorMacro(<< "Wrong number of arrays received.");
    return 0;
  }

  if (a1->GetNumberOfTuples() != a2->GetNumberOfTuples())
  {
    svtkErrorMacro(<< "Two arrays to threshold must have the same number of tuples.");
    return 0;
  }

  int (svtkBivariateLinearTableThreshold::*thresholdFunc)(double, double) = nullptr;
  switch (this->LinearThresholdType)
  {
    case svtkBivariateLinearTableThreshold::BLT_ABOVE:
      thresholdFunc = &svtkBivariateLinearTableThreshold::ThresholdAbove;
      break;
    case svtkBivariateLinearTableThreshold::BLT_BELOW:
      thresholdFunc = &svtkBivariateLinearTableThreshold::ThresholdBelow;
      break;
    case svtkBivariateLinearTableThreshold::BLT_NEAR:
      thresholdFunc = &svtkBivariateLinearTableThreshold::ThresholdNear;
      break;
    case svtkBivariateLinearTableThreshold::BLT_BETWEEN:
      thresholdFunc = &svtkBivariateLinearTableThreshold::ThresholdBetween;
      break;
    default:
      svtkErrorMacro(<< "Threshold type not defined: " << this->LinearThresholdType);
      return 0;
  }

  acceptedIds->Initialize();
  svtkIdType numTuples = a1->GetNumberOfTuples();
  double v1, v2;
  for (int i = 0; i < numTuples; i++)
  {
    v1 = a1->GetComponent(i, component1);
    v2 = a2->GetComponent(i, component2);

    if ((this->*thresholdFunc)(v1, v2))
    {
      acceptedIds->InsertNextValue(i);
    }
  }

  return 1;
}

void svtkBivariateLinearTableThreshold::AddLineEquation(double* p1, double* p2)
{
  double a = p1[1] - p2[1];
  double b = p2[0] - p1[0];
  double c = p1[0] * p2[1] - p2[0] * p1[1];

  this->AddLineEquation(a, b, c);
}

void svtkBivariateLinearTableThreshold::AddLineEquation(double* p, double slope)
{
  double p2[2] = { p[0] + 1.0, p[1] + slope };
  this->AddLineEquation(p, p2);
}

void svtkBivariateLinearTableThreshold::AddLineEquation(double a, double b, double c)
{
  double norm = sqrt(a * a + b * b);
  a /= norm;
  b /= norm;
  c /= norm;

  this->LineEquations->InsertNextTuple3(a, b, c);
  this->NumberOfLineEquations++;
}

void svtkBivariateLinearTableThreshold::ClearLineEquations()
{
  this->LineEquations->Initialize();
  this->NumberOfLineEquations = 0;
}

void svtkBivariateLinearTableThreshold::ComputeImplicitLineFunction(
  double* p1, double* p2, double* abc)
{
  abc[0] = p1[1] - p2[1];
  abc[1] = p2[0] - p1[0];
  abc[2] = p1[0] * p2[1] - p2[0] * p1[1];
}

void svtkBivariateLinearTableThreshold::ComputeImplicitLineFunction(
  double* p, double slope, double* abc)
{
  double p2[2] = { p[0] + 1.0, p[1] + slope };
  svtkBivariateLinearTableThreshold::ComputeImplicitLineFunction(p, p2, abc);
}

int svtkBivariateLinearTableThreshold::ThresholdAbove(double x, double y)
{
  double *c, v;
  for (int i = 0; i < this->NumberOfLineEquations; i++)
  {
    c = this->LineEquations->GetTuple3(i);
    v = c[0] * x + c[1] * y + c[2];

    if ((this->GetInclusive() && v >= 0) || (!this->GetInclusive() && v > 0))
    {
      return 1;
    }
  }
  return 0;
}

int svtkBivariateLinearTableThreshold::ThresholdBelow(double x, double y)
{
  double *c, v;
  for (int i = 0; i < this->NumberOfLineEquations; i++)
  {
    c = this->LineEquations->GetTuple3(i);
    v = c[0] * x + c[1] * y + c[2];

    if ((this->GetInclusive() && v <= 0) || (!this->GetInclusive() && v < 0))
    {
      return 1;
    }
  }
  return 0;
}

int svtkBivariateLinearTableThreshold::ThresholdNear(double x, double y)
{
  double *c, v;
  for (int i = 0; i < this->NumberOfLineEquations; i++)
  {
    c = this->LineEquations->GetTuple3(i);

    if (this->UseNormalizedDistance)
    {
      double dx = fabs(x - (-c[1] * y - c[2]) / c[0]);
      double dy = fabs(y - (-c[0] * x - c[2]) / c[1]);

      double dxn = dx / this->ColumnRanges[0];
      double dyn = dy / this->ColumnRanges[1];

      v = sqrt(dxn * dxn + dyn * dyn);
    }
    else
    {
      v = fabs(c[0] * x + c[1] * y + c[2]);
    }

    if ((this->GetInclusive() && v <= this->DistanceThreshold) ||
      (!this->GetInclusive() && v < this->DistanceThreshold))
    {
      return 1;
    }
  }

  return 0;
}

int svtkBivariateLinearTableThreshold::ThresholdBetween(double x, double y)
{
  return (this->ThresholdAbove(x, y) && this->ThresholdBelow(x, y));
}
