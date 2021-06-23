/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkPPairwiseExtractHistogram2D.cxx

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
#include "svtkPPairwiseExtractHistogram2D.h"

#include "svtkArrayData.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkCollection.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageMedian3D.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPExtractHistogram2D.h"
#include "svtkPointData.h"
#include "svtkStatisticsAlgorithmPrivate.h"
#include "svtkStdString.h"
#include "svtkTable.h"
#include "svtkTimerLog.h"
#include "svtkUnsignedIntArray.h"

#include <set>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkPPairwiseExtractHistogram2D);
svtkCxxSetObjectMacro(svtkPPairwiseExtractHistogram2D, Controller, svtkMultiProcessController);

svtkPPairwiseExtractHistogram2D::svtkPPairwiseExtractHistogram2D()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

svtkPPairwiseExtractHistogram2D::~svtkPPairwiseExtractHistogram2D()
{
  this->SetController(nullptr);
}

void svtkPPairwiseExtractHistogram2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

svtkExtractHistogram2D* svtkPPairwiseExtractHistogram2D::NewHistogramFilter()
{
  svtkPExtractHistogram2D* ph = svtkPExtractHistogram2D::New();
  ph->SetController(this->Controller);
  return ph;
}
