/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPAutoCorrelativeStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkToolkits.h"

#include "svtkPAutoCorrelativeStatistics.h"

#include "svtkCommunicator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"
#include "svtkVariant.h"

svtkStandardNewMacro(svtkPAutoCorrelativeStatistics);
svtkCxxSetObjectMacro(svtkPAutoCorrelativeStatistics, Controller, svtkMultiProcessController);
//-----------------------------------------------------------------------------
svtkPAutoCorrelativeStatistics::svtkPAutoCorrelativeStatistics()
{
  this->Controller = 0;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
svtkPAutoCorrelativeStatistics::~svtkPAutoCorrelativeStatistics()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void svtkPAutoCorrelativeStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
void svtkPAutoCorrelativeStatistics::Learn(
  svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta)
{
  if (!outMeta)
  {
    return;
  }

  // First calculate correlative statistics on local data set
  this->Superclass::Learn(inData, inParameters, outMeta);

  svtkTable* primaryTab = svtkTable::SafeDownCast(outMeta->GetBlock(0));
  if (!primaryTab)
  {
    return;
  }

  svtkIdType nRow = primaryTab->GetNumberOfRows();
  if (!nRow)
  {
    // No statistics were calculated.
    return;
  }

  // Make sure that parallel updates are needed, otherwise leave it at that.
  int np = this->Controller->GetNumberOfProcesses();
  if (np < 2)
  {
    return;
  }

  // Now get ready for parallel calculations
  svtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    svtkErrorMacro("No parallel communicator.");
  }

  // (All) gather all sample sizes
  int n_l = primaryTab->GetValueByName(0, "Cardinality").ToInt(); // Cardinality
  int* n_g = new int[np];
  com->AllGather(&n_l, n_g, 1);

  // Iterate over all parameter rows
  for (int r = 0; r < nRow; ++r)
  {
    // (All) gather all local M statistics
    double M_l[5];
    M_l[0] = primaryTab->GetValueByName(r, "Mean Xs").ToDouble();
    M_l[1] = primaryTab->GetValueByName(r, "Mean Xt").ToDouble();
    M_l[2] = primaryTab->GetValueByName(r, "M2 Xs").ToDouble();
    M_l[3] = primaryTab->GetValueByName(r, "M2 Xt").ToDouble();
    M_l[4] = primaryTab->GetValueByName(r, "M XsXt").ToDouble();
    double* M_g = new double[5 * np];
    com->AllGather(M_l, M_g, 5);

    // Aggregate all local quintuples of M statistics into global ones
    int ns = n_g[0];
    double meanXs = M_g[0];
    double meanXt = M_g[1];
    double mom2Xs = M_g[2];
    double mom2Xt = M_g[3];
    double momXsXt = M_g[4];

    for (int i = 1; i < np; ++i)
    {
      int ns_l = n_g[i];
      int N = ns + ns_l;

      int o = 5 * i;
      double meanXs_part = M_g[o];
      double meanXt_part = M_g[o + 1];
      double mom2Xs_part = M_g[o + 2];
      double mom2Xt_part = M_g[o + 3];
      double momXsXt_part = M_g[o + 4];

      double invN = 1. / static_cast<double>(N);

      double deltaXs = meanXs_part - meanXs;
      double deltaXs_sur_N = deltaXs * invN;

      double deltaXt = meanXt_part - meanXt;
      double deltaXt_sur_N = deltaXt * invN;

      int prod_ns = ns * ns_l;

      mom2Xs += mom2Xs_part + prod_ns * deltaXs * deltaXs_sur_N;

      mom2Xt += mom2Xt_part + prod_ns * deltaXt * deltaXt_sur_N;

      momXsXt += momXsXt_part + prod_ns * deltaXs * deltaXt_sur_N;

      meanXs += ns_l * deltaXs_sur_N;

      meanXt += ns_l * deltaXt_sur_N;

      ns = N;
    }

    primaryTab->SetValueByName(r, "Mean Xs", meanXs);
    primaryTab->SetValueByName(r, "Mean Xt", meanXt);
    primaryTab->SetValueByName(r, "M2 Xs", mom2Xs);
    primaryTab->SetValueByName(r, "M2 Xt", mom2Xt);
    primaryTab->SetValueByName(r, "M XsXt", momXsXt);

    // Set global statistics
    primaryTab->SetValueByName(r, "Cardinality", ns);

    // Clean-up
    delete[] M_g;
  }
  delete[] n_g;
}

// ----------------------------------------------------------------------
void svtkPAutoCorrelativeStatistics::Test(
  svtkTable* inData, svtkMultiBlockDataSet* inMeta, svtkTable* outMeta)
{
  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    svtkWarningMacro("Parallel correlative statistics: Hypothesis testing not implemented for more "
                    "than 1 process.");
    return;
  }

  this->Superclass::Test(inData, inMeta, outMeta);
}
