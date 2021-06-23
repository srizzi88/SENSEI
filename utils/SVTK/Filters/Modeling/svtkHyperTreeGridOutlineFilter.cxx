/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridOutlineFilter.h"

#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineSource.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkHyperTreeGridOutlineFilter);

//----------------------------------------------------------------------------
svtkHyperTreeGridOutlineFilter::svtkHyperTreeGridOutlineFilter()
{
  this->OutlineSource = svtkOutlineSource::New();

  this->GenerateFaces = 0;
}

//----------------------------------------------------------------------------
svtkHyperTreeGridOutlineFilter::~svtkHyperTreeGridOutlineFilter()
{
  if (this->OutlineSource != nullptr)
  {
    this->OutlineSource->Delete();
    this->OutlineSource = nullptr;
  }
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridOutlineFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkHyperTreeGrid* input =
    svtkHyperTreeGrid::SafeDownCast(inInfo->Get(svtkHyperTreeGrid::DATA_OBJECT()));
  if (!input)
  {
    svtkErrorMacro(
      "Incorrect type of input: " << inInfo->Get(svtkHyperTreeGrid::DATA_OBJECT())->GetClassName());
    return 0;
  }

  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    svtkErrorMacro(
      "Incorrect type of output: " << outInfo->Get(svtkDataObject::DATA_OBJECT())->GetClassName());
    return 0;
  }

  svtkDebugMacro(<< "Creating dataset outline");

  //
  // Let OutlineSource do all the work
  //

  this->OutlineSource->SetBounds(input->GetBounds());
  this->OutlineSource->SetGenerateFaces(this->GenerateFaces);
  this->OutlineSource->Update();

  output->CopyStructure(this->OutlineSource->GetOutput());

  return 1;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridOutlineFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridOutlineFilter::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
// JBL Pour moi, c'est un defaut de design de svtkHyperTreeGridAlgorithm
int svtkHyperTreeGridOutlineFilter::ProcessTrees(
  svtkHyperTreeGrid* svtkNotUsed(input), svtkDataObject* svtkNotUsed(outputDO))
{
  return 1;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridOutlineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Faces: " << (this->GenerateFaces ? "On\n" : "Off\n");
}
