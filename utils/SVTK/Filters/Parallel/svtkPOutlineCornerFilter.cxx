/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOutlineCornerFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPOutlineCornerFilter.h"

#include "svtkAMRInformation.h"
#include "svtkAppendPolyData.h"
#include "svtkBoundingBox.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTree.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationDataObjectKey.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineCornerSource.h"
#include "svtkOutlineSource.h"
#include "svtkOverlappingAMR.h"
#include "svtkPOutlineFilterInternals.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUniformGrid.h"
#include <vector>

svtkStandardNewMacro(svtkPOutlineCornerFilter);
svtkCxxSetObjectMacro(svtkPOutlineCornerFilter, Controller, svtkMultiProcessController);

svtkPOutlineCornerFilter::svtkPOutlineCornerFilter()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
  this->CornerFactor = 0.2;
  this->Internals = new svtkPOutlineFilterInternals;
  this->Internals->SetController(svtkMultiProcessController::GetGlobalController());
}

svtkPOutlineCornerFilter::~svtkPOutlineCornerFilter()
{
  this->SetController(nullptr);
  this->Internals->SetController(nullptr);
  delete this->Internals;
}

// ----------------------------------------------------------------------------
void svtkPOutlineCornerFilter::SetCornerFactor(double cornerFactor)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting "
                << "CornerFactor to " << CornerFactor);
  double tempCornerFactor =
    (cornerFactor < 0.001 ? 0.001 : (cornerFactor > 0.5 ? 0.5 : cornerFactor));

  if (this->CornerFactor != tempCornerFactor)
  {
    std::cerr << "CornerFactor: " << tempCornerFactor << std::endl;
    this->CornerFactor = tempCornerFactor;
    this->Internals->SetCornerFactor(tempCornerFactor);
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
int svtkPOutlineCornerFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Internals->SetIsCornerSource(true);
  return this->Internals->RequestData(request, inputVector, outputVector);
}

int svtkPOutlineCornerFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

void svtkPOutlineCornerFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CornerFactor: " << this->CornerFactor << "\n";
  os << indent << "Controller: " << this->Controller << endl;
}
