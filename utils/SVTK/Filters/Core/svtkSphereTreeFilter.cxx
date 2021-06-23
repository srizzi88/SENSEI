/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereTreeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSphereTreeFilter.h"
#include "svtkAbstractArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSphereTree.h"
#include "svtkStructuredGrid.h"

svtkStandardNewMacro(svtkSphereTreeFilter);
svtkCxxSetObjectMacro(svtkSphereTreeFilter, SphereTree, svtkSphereTree);

// Construct object.
//----------------------------------------------------------------------------
svtkSphereTreeFilter::svtkSphereTreeFilter()
{
  this->SphereTree = nullptr;
  this->TreeHierarchy = true;

  this->ExtractionMode = SVTK_SPHERE_TREE_LEVELS;
  this->Level = (-1);
  this->Point[0] = this->Point[1] = this->Point[2] = 0.0;
  this->Ray[0] = 1.0;
  this->Ray[1] = this->Ray[2] = 0.0;
  this->Normal[0] = this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
}

//----------------------------------------------------------------------------
svtkSphereTreeFilter::~svtkSphereTreeFilter()
{
  this->SetSphereTree(nullptr);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the sphere tree is modified,
// then this object is modified as well.
svtkMTimeType svtkSphereTreeFilter::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();
  unsigned long time;

  if (this->SphereTree)
  {
    time = this->SphereTree->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
// Produce the sphere tree as requested
int svtkSphereTreeFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDebugMacro(<< "Generating spheres");

  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = nullptr;
  if (inInfo != nullptr)
  {
    input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Make sure there is data
  svtkIdType numCells;
  int numLevels = 0;
  if (this->SphereTree) // first choice
  {
    // Get number of spheres/cells from root level
    numLevels = this->SphereTree->GetNumberOfLevels();
    this->SphereTree->GetTreeSpheres(numLevels - 1, numCells);
  }
  else if (input) // next choice
  {
    numCells = input->GetNumberOfCells();
  }
  else // oh oh no input
  {
    svtkWarningMacro(<< "No input!");
    return 1;
  }

  // If no sphere tree, create one from the input
  if (!this->SphereTree)
  {
    this->SphereTree = svtkSphereTree::New();
    this->SphereTree->SetBuildHierarchy(this->TreeHierarchy);
    this->SphereTree->Build(input);
    numLevels = this->SphereTree->GetNumberOfLevels();
    this->SphereTree->GetTreeSpheres(numLevels - 1, numCells);
  }

  // See if hierarchy was created
  int builtHierarchy = this->SphereTree->GetBuildHierarchy() && this->TreeHierarchy;

  // Allocate: points (center of spheres), radii, level in tree
  svtkPoints* newPts = svtkPoints::New();
  newPts->SetDataTypeToDouble();

  svtkDoubleArray* radii = svtkDoubleArray::New();
  radii->Allocate(numCells);

  svtkIntArray* levels = nullptr; // in case they are needed

  const double* cellSpheres = this->SphereTree->GetCellSpheres();
  if (this->ExtractionMode == SVTK_SPHERE_TREE_LEVELS)
  {
    levels = svtkIntArray::New();
    levels->Allocate(numCells);

    // Create a point per cell. Create a scalar per cell (the radius).
    if (this->Level < 0 || this->Level == (numLevels - 1))
    {
      svtkIdType cellId;
      const double* sphere = cellSpheres;
      for (cellId = 0; cellId < numCells; cellId++, sphere += 4)
      {
        newPts->InsertPoint(cellId, sphere);
        radii->InsertValue(cellId, sphere[3]);
        levels->InsertValue(cellId, numLevels);
      }
    }

    // If the hierarchy is requested, generate these points too.
    if (builtHierarchy)
    {
      int i, level;
      svtkIdType numSpheres;
      const double* lSphere;
      for (level = 0; level < numLevels; ++level)
      {
        if (this->Level < 0 || this->Level == level)
        {
          lSphere = this->SphereTree->GetTreeSpheres(level, numSpheres);
          for (i = 0; i < numSpheres; ++i, lSphere += 4)
          {
            newPts->InsertNextPoint(lSphere);
            radii->InsertNextValue(lSphere[3]);
            levels->InsertNextValue(level);
          }
        }
      }
    }
  } // extract levels

  else // perform geometric query
  {
    // Use the slower API because it tests the code better
    svtkIdList* cellIds = svtkIdList::New();
    svtkIdType cellId, numSelectedCells;
    const double* sphere = cellSpheres;

    if (this->ExtractionMode == SVTK_SPHERE_TREE_POINT)
    {
      this->SphereTree->SelectPoint(this->Point, cellIds);
    }
    else if (this->ExtractionMode == SVTK_SPHERE_TREE_LINE)
    {
      this->SphereTree->SelectLine(this->Point, this->Ray, cellIds);
    }
    else if (this->ExtractionMode == SVTK_SPHERE_TREE_PLANE)
    {
      this->SphereTree->SelectPlane(this->Point, this->Normal, cellIds);
    }

    numSelectedCells = cellIds->GetNumberOfIds();
    for (svtkIdType i = 0; i < numSelectedCells; ++i)
    {
      cellId = cellIds->GetId(i);
      sphere = cellSpheres + 4 * cellId;
      newPts->InsertPoint(i, sphere);
      radii->InsertValue(i, sphere[3]);
    }
    cellIds->Delete();
  } // geometric queries

  // Produce output
  output->SetPoints(newPts);
  newPts->Delete();

  radii->SetName("SphereTree");
  output->GetPointData()->SetScalars(radii);
  radii->Delete();

  if (levels != nullptr)
  {
    levels->SetName("SphereLevels");
    output->GetPointData()->AddArray(levels);
    levels->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkSphereTreeFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
const char* svtkSphereTreeFilter::GetExtractionModeAsString()
{
  if (this->ExtractionMode == SVTK_SPHERE_TREE_LEVELS)
  {
    return "Levels";
  }
  else if (this->ExtractionMode == SVTK_SPHERE_TREE_POINT)
  {
    return "Point";
  }
  else if (this->ExtractionMode == SVTK_SPHERE_TREE_LINE)
  {
    return "Line";
  }
  else
  {
    return "Plane";
  }
}

//----------------------------------------------------------------------------
void svtkSphereTreeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sphere Tree: " << this->SphereTree << "\n";
  os << indent << "Build Tree Hierarchy: " << (this->TreeHierarchy ? "On\n" : "Off\n");

  os << indent << "Extraction Mode: " << this->GetExtractionModeAsString() << endl;

  os << indent << "Level: " << this->Level << "\n";

  os << indent << "Point: (" << this->Point[0] << ", " << this->Point[1] << ", " << this->Point[2]
     << ")\n";

  os << indent << "Ray: (" << this->Ray[0] << ", " << this->Ray[1] << ", " << this->Ray[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")\n";
}
