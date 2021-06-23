/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSpatialRepresentationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSpatialRepresentationFilter.h"

#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkLocator.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"

#include <set>

class svtkSpatialRepresentationFilterInternal
{
public:
  std::set<int> Levels;
};

svtkStandardNewMacro(svtkSpatialRepresentationFilter);
svtkCxxSetObjectMacro(svtkSpatialRepresentationFilter, SpatialRepresentation, svtkLocator);

svtkSpatialRepresentationFilter::svtkSpatialRepresentationFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SpatialRepresentation = nullptr;
  this->MaximumLevel = 0;
  this->GenerateLeaves = false;
  this->Internal = new svtkSpatialRepresentationFilterInternal;
}

svtkSpatialRepresentationFilter::~svtkSpatialRepresentationFilter()
{
  if (this->SpatialRepresentation)
  {
    this->SpatialRepresentation->UnRegister(this);
    this->SpatialRepresentation = nullptr;
  }
  delete this->Internal;
}

void svtkSpatialRepresentationFilter::AddLevel(int level)
{
  this->Internal->Levels.insert(level);
}

void svtkSpatialRepresentationFilter::ResetLevels()
{
  this->Internal->Levels.clear();
}

int svtkSpatialRepresentationFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0]);
  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector);

  if (this->SpatialRepresentation == nullptr)
  {
    svtkErrorMacro(<< "SpatialRepresentation is nullptr.");
    return 0;
  }

  this->SpatialRepresentation->SetDataSet(input);
  this->SpatialRepresentation->Update();
  this->MaximumLevel = this->SpatialRepresentation->GetLevel();

  //
  // Loop over all requested levels generating new levels as necessary
  //
  std::set<int>::iterator it;
  for (it = this->Internal->Levels.begin(); it != this->Internal->Levels.end(); ++it)
  {
    if (*it <= this->MaximumLevel)
    {
      svtkNew<svtkPolyData> level_representation;
      output->SetBlock(*it, level_representation);
      this->SpatialRepresentation->GenerateRepresentation(*it, level_representation);
    }
  }
  if (this->GenerateLeaves)
  {
    svtkNew<svtkPolyData> leaf_representation;
    output->SetBlock(this->MaximumLevel + 1, leaf_representation);
    this->SpatialRepresentation->GenerateRepresentation(-1, leaf_representation);
  }

  return 1;
}

void svtkSpatialRepresentationFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Maximum Level: " << this->MaximumLevel << "\n";
  os << indent << "GenerateLeaves: " << this->GenerateLeaves << "\n";

  if (this->SpatialRepresentation)
  {
    os << indent << "Spatial Representation: " << this->SpatialRepresentation << "\n";
  }
  else
  {
    os << indent << "Spatial Representation: (none)\n";
  }
}

//----------------------------------------------------------------------------
void svtkSpatialRepresentationFilter::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // This filter shares our input and is therefore involved in a
  // reference loop.
  svtkGarbageCollectorReport(collector, this->SpatialRepresentation, "SpatialRepresentation");
}

//----------------------------------------------------------------------------
int svtkSpatialRepresentationFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}
