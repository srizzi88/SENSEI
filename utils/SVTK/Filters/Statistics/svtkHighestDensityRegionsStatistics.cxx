/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHighestDensityRegionsStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkHighestDensityRegionsStatistics.h"

#include "svtkDataArrayCollection.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStatisticsAlgorithmPrivate.h"
#include "svtkTable.h"

#include <algorithm>
#include <set>
#include <sstream>

svtkStandardNewMacro(svtkHighestDensityRegionsStatistics);

// ----------------------------------------------------------------------
svtkHighestDensityRegionsStatistics::svtkHighestDensityRegionsStatistics()
{
  this->SmoothHC1[0] = 0.;
  // Initialize H smooth matrix to Identity.
  this->SetSigma(1.0);

  //  At the construction, no columns pair are requested yet
  this->NumberOfRequestedColumnsPair = 0;
}

// ----------------------------------------------------------------------
svtkHighestDensityRegionsStatistics::~svtkHighestDensityRegionsStatistics() = default;

// ----------------------------------------------------------------------
void svtkHighestDensityRegionsStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sigma matrix: " << this->SmoothHC1[0] << ", " << this->SmoothHC1[1] << ", "
     << this->SmoothHC2[0] << ", " << this->SmoothHC2[1] << "\n";
}

// ----------------------------------------------------------------------
void svtkHighestDensityRegionsStatistics::SetSigmaMatrix(
  double s11, double s12, double s21, double s22)
{
  if (this->SmoothHC1[0] == s11 && this->SmoothHC1[1] == s12 && this->SmoothHC2[0] == s21 &&
    this->SmoothHC2[1] == s22)
  {
    return;
  }

  this->SmoothHC1[0] = s11;
  this->SmoothHC1[1] = s12;
  this->SmoothHC2[0] = s21;
  this->SmoothHC2[1] = s22;

  this->Determinant = svtkMath::Determinant2x2(this->SmoothHC1, this->SmoothHC2);
  double invDet = 0.;
  if (this->Determinant != 0.)
  {
    invDet = 1.0 / this->Determinant;
  }

  // We compute and store the inverse of the smoothing matrix
  this->InvSigmaC1[0] = +invDet * this->SmoothHC2[1];
  this->InvSigmaC1[1] = -invDet * this->SmoothHC1[1];
  this->InvSigmaC2[0] = -invDet * this->SmoothHC2[0];
  this->InvSigmaC2[1] = +invDet * this->SmoothHC1[0];

  this->Modified();
}

// ----------------------------------------------------------------------
void svtkHighestDensityRegionsStatistics::SetSigma(double sigma)
{
  this->SetSigmaMatrix(sigma * sigma, 0, 0, sigma * sigma);
}

// ----------------------------------------------------------------------
void svtkHighestDensityRegionsStatistics::Learn(
  svtkTable* inData, svtkTable* svtkNotUsed(inParameters), svtkMultiBlockDataSet* outMeta)
{
  if (!inData || !outMeta)
  {
    return;
  }

  svtkNew<svtkTable> outputColumns;

  std::set<std::set<svtkStdString> >::const_iterator reqIt;

  // Make sure the number of requested pairs of columns is 0
  // before the computation.
  this->NumberOfRequestedColumnsPair = 0;

  // Populate outputColumns with columns that are requested from
  // the input dataset
  for (reqIt = this->Internals->Requests.begin(); reqIt != this->Internals->Requests.end(); ++reqIt)
  {
    // Each request contains only one pair of columns of interest
    // (if there are others, they are ignored).
    std::set<svtkStdString>::const_iterator colIt = reqIt->begin();
    const svtkStdString& colY = *colIt;
    if (!inData->GetColumnByName(colY.c_str()))
    {
      svtkWarningMacro(
        "InData table does not have a column " << colY.c_str() << ". Ignoring this pair.");
      continue;
    }

    ++colIt;
    const svtkStdString& colX = *colIt;
    if (!inData->GetColumnByName(colX.c_str()))
    {
      svtkWarningMacro(
        "InData table does not have a column " << colX.c_str() << ". Ignoring this pair.");
      continue;
    }

    // Verify column types
    svtkDataArray* inputColX = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colX.c_str()));
    svtkDataArray* inputColY = svtkArrayDownCast<svtkDataArray>(inData->GetColumnByName(colY.c_str()));
    if (!inputColX || !inputColY)
    {
      svtkErrorMacro(<< "HDR cannot work with columns that are not of svtkDataArray type");
      return;
    }

    svtkDataArray* arrX = svtkDataArray::CreateDataArray(inputColX->GetDataType());
    arrX->DeepCopy(inputColX);
    arrX->SetName(inputColX->GetName());
    outputColumns->AddColumn(arrX);

    svtkDataArray* arrY = svtkDataArray::CreateDataArray(inputColY->GetDataType());
    arrY->DeepCopy(inputColY);
    arrY->SetName(inputColY->GetName());
    outputColumns->AddColumn(arrY);

    // Compute for the two columns and each observations the estimator of
    // density. Create a double Array that contains number of requested data
    // series components. Each tuple will contain the correspondent value
    // casted if necessary into a double.

    svtkNew<svtkDoubleArray> inObservations;
    inObservations->SetNumberOfComponents(2);
    inObservations->SetNumberOfTuples(outputColumns->GetNumberOfRows());

    inObservations->CopyComponent(0, inputColX, 0);
    inObservations->CopyComponent(1, inputColY, 0);

    // outObservations store the density vector
    svtkDataArray* outObservations = svtkDataArray::CreateDataArray(inObservations->GetDataType());
    outObservations->SetNumberOfComponents(1);
    outObservations->SetNumberOfTuples(inObservations->GetNumberOfTuples());

    this->ComputeHDR(inObservations, outObservations);
    std::stringstream ss;
    ss << "HDR (" << inputColX->GetName() << "," << inputColY->GetName() << ")";
    outObservations->SetName(ss.str().c_str());
    outputColumns->AddColumn(outObservations);

    arrX->Delete();
    arrY->Delete();
    outObservations->Delete();

    // One requested pair of columns has been added.
    this->NumberOfRequestedColumnsPair++;
  } // End requests iteration.

  outMeta->SetNumberOfBlocks(1);
  outMeta->SetBlock(0, outputColumns);
  svtkInformation* info = outMeta->GetMetaData(static_cast<unsigned int>(0));
  info->Set(svtkCompositeDataSet::NAME(), "Estimator of density Data");
}

// ----------------------------------------------------------------------
void svtkHighestDensityRegionsStatistics::Derive(svtkMultiBlockDataSet*) {}

// ----------------------------------------------------------------------
double svtkHighestDensityRegionsStatistics::ComputeHDR(svtkDataArray* inObs, svtkDataArray* outDensity)
{
  return ComputeHDR(inObs, inObs, outDensity);
}

// ----------------------------------------------------------------------
double svtkHighestDensityRegionsStatistics ::ComputeHDR(
  svtkDataArray* inObs, svtkDataArray* inPointsOfInterest, svtkDataArray* outDensity)
{
  svtkIdType nbObservations = inObs->GetNumberOfTuples();
  svtkIdType nbPoints = inPointsOfInterest->GetNumberOfTuples();

  if (nbObservations == 0)
  {
    svtkErrorMacro(<< "Empty observation array");
    return 0.0;
  }
  double sum = 0.0;

  double denom = 1.0 / static_cast<double>(nbObservations);

  // Let's compute the HDR for each points of interest
  for (svtkIdType i = 0; i < nbPoints; i++)
  {
    double currentXi[2];
    double currentXj[2];
    double hdr = 0.0;

    // We are working in a bivariate model.
    inPointsOfInterest->GetTuple(i, currentXi);
    // Sum all gaussian kernel
    for (svtkIdType j = 0; j < nbObservations; j++)
    {
      inObs->GetTuple(j, currentXj);

      const double deltaX = currentXi[0] - currentXj[0];
      const double deltaY = currentXi[1] - currentXj[1];
      hdr += this->ComputeSmoothGaussianKernel(inObs->GetNumberOfComponents(), deltaX, deltaY);
    }
    double d = denom * hdr;
    outDensity->SetTuple1(i, d);
    sum += d;
  }

  return sum;
}

// ----------------------------------------------------------------------
double svtkHighestDensityRegionsStatistics::ComputeSmoothGaussianKernel(
  int svtkNotUsed(dimension), double khx, double khy)
{
  // Call the standard gaussian kernel with the new random vector.
  double d = khx * (this->InvSigmaC1[0] * khx + this->InvSigmaC2[0] * khy) +
    khy * (this->InvSigmaC1[1] * khx + this->InvSigmaC2[1] * khy);

  return (exp(-d * 0.5)) / (2.0 * svtkMath::Pi() * this->Determinant);
}
