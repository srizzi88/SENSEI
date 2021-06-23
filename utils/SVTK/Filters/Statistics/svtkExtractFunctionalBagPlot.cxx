/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractFunctionalBagPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractFunctionalBagPlot.h"

#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <vector>

svtkStandardNewMacro(svtkExtractFunctionalBagPlot);

//-----------------------------------------------------------------------------
svtkExtractFunctionalBagPlot::svtkExtractFunctionalBagPlot()
{
  this->SetNumberOfInputPorts(2);
  this->DensityForP50 = 0;
  this->DensityForPUser = 0.;
  this->PUser = 95;
}

//-----------------------------------------------------------------------------
svtkExtractFunctionalBagPlot::~svtkExtractFunctionalBagPlot() = default;

//-----------------------------------------------------------------------------
void svtkExtractFunctionalBagPlot::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
class DensityVal
{
public:
  DensityVal(double d, svtkAbstractArray* arr)
    : Density(d)
    , Array(arr)
  {
  }
  bool operator<(const DensityVal& b) const { return this->Density > b.Density; }
  double Density;
  svtkAbstractArray* Array;
};

//-----------------------------------------------------------------------------
int svtkExtractFunctionalBagPlot::RequestData(svtkInformation* /*request*/,
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  svtkTable* inTable = svtkTable::GetData(inputVector[0]);
  svtkTable* inTableDensity = svtkTable::GetData(inputVector[1]);
  svtkTable* outTable = svtkTable::GetData(outputVector, 0);

  svtkIdType inNbColumns = inTable->GetNumberOfColumns();

  if (!inTable)
  {
    svtkDebugMacro(<< "Update event called with no input table.");
    return false;
  }

  if (!inTableDensity)
  {
    svtkDebugMacro(<< "Update event called with no density input table.");
    return false;
  }

  svtkDoubleArray* density =
    svtkArrayDownCast<svtkDoubleArray>(this->GetInputAbstractArrayToProcess(0, inTableDensity));
  if (!density)
  {
    svtkDebugMacro(<< "Update event called with non double density array.");
    return false;
  }

  svtkStringArray* varName =
    svtkArrayDownCast<svtkStringArray>(this->GetInputAbstractArrayToProcess(1, inTableDensity));
  if (!varName)
  {
    svtkDebugMacro(<< "Update event called with no variable name array.");
    return false;
  }

  svtkIdType nbPoints = varName->GetNumberOfValues();

  std::vector<svtkAbstractArray*> medianLines;
  std::vector<svtkAbstractArray*> q3Lines;
  std::set<svtkIdType> outliersSeries;

  for (svtkIdType i = 0; i < nbPoints; i++)
  {
    double d = density->GetValue(i);
    svtkAbstractArray* c = inTable->GetColumnByName(varName->GetValue(i));
    if (d < this->DensityForPUser)
    {
      outliersSeries.insert(i);
    }
    else
    {
      if (d > this->DensityForP50)
      {
        medianLines.push_back(c);
      }
      else
      {
        q3Lines.push_back(c);
      }
    }
  }

  svtkIdType nbRows = inTable->GetNumberOfRows();
  svtkIdType nbCols = inTable->GetNumberOfColumns();

  // Generate the median curve with median values for every sample.
  svtkNew<svtkDoubleArray> qMedPoints;
  qMedPoints->SetName("QMedianLine");
  qMedPoints->SetNumberOfComponents(1);
  qMedPoints->SetNumberOfTuples(nbRows);

  std::vector<double> vals;
  vals.resize(nbCols);
  for (svtkIdType i = 0; i < nbRows; i++)
  {
    for (svtkIdType j = 0; j < nbCols; j++)
    {
      vals[j] = inTable->GetValue(i, j).ToDouble();
    }
    std::sort(vals.begin(), vals.end());
    qMedPoints->SetTuple1(i, vals[nbCols / 2]);
  }

  // Generate the quad strip arrays
  std::ostringstream ss;
  ss << "Q3Points" << this->PUser;
  svtkNew<svtkDoubleArray> q3Points;
  q3Points->SetName(ss.str().c_str());
  q3Points->SetNumberOfComponents(2);
  q3Points->SetNumberOfTuples(nbRows);

  svtkNew<svtkDoubleArray> q2Points;
  q2Points->SetName("QMedPoints");
  q2Points->SetNumberOfComponents(2);
  q2Points->SetNumberOfTuples(nbRows);

  size_t medianCount = medianLines.size();
  size_t q3Count = q3Lines.size();
  for (svtkIdType i = 0; i < nbRows; i++)
  {
    double vMin = SVTK_DOUBLE_MAX;
    double vMax = SVTK_DOUBLE_MIN;
    for (size_t j = 0; j < medianCount; j++)
    {
      double v = medianLines[j]->GetVariantValue(i).ToDouble();
      if (v < vMin)
      {
        vMin = v;
      }
      if (v > vMax)
      {
        vMax = v;
      }
    }
    q2Points->SetTuple2(i, vMin, vMax);

    vMin = SVTK_DOUBLE_MAX;
    vMax = SVTK_DOUBLE_MIN;
    for (size_t j = 0; j < q3Count; j++)
    {
      double v = q3Lines[j]->GetVariantValue(i).ToDouble();
      if (v < vMin)
      {
        vMin = v;
      }
      if (v > vMax)
      {
        vMax = v;
      }
    }
    q3Points->SetTuple2(i, vMin, vMax);
  }

  // Append the input columns
  for (svtkIdType i = 0; i < inNbColumns; i++)
  {
    svtkAbstractArray* arr = inTable->GetColumn(i);
    if (outliersSeries.find(i) != outliersSeries.end())
    {
      svtkAbstractArray* arrCopy = arr->NewInstance();
      arrCopy->DeepCopy(arr);
      std::string name = std::string(arr->GetName()) + "_outlier";
      arrCopy->SetName(name.c_str());
      outTable->AddColumn(arrCopy);
      arrCopy->Delete();
    }
    else
    {
      outTable->AddColumn(arr);
    }
  }

  // Then add the 2 "bag" columns into the output table
  if (!q3Lines.empty())
  {
    outTable->AddColumn(q3Points);
  }
  if (!medianLines.empty())
  {
    outTable->AddColumn(q2Points);
  }
  outTable->AddColumn(qMedPoints);

  return 1;
}
