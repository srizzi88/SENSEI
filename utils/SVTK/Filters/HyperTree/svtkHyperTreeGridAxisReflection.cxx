/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridAxisReflection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridAxisReflection.h"

#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridScales.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUniformHyperTreeGrid.h"

svtkStandardNewMacro(svtkHyperTreeGridAxisReflection);

//-----------------------------------------------------------------------------
svtkHyperTreeGridAxisReflection::svtkHyperTreeGridAxisReflection()
{
  // Default reflection plane is lower X bounding plane
  this->Plane = USE_X_MIN;

  // Default plane position is at origin
  this->Center = 0.;

  // JB Pour sortir un maillage de meme type que celui en entree
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
svtkHyperTreeGridAxisReflection::~svtkHyperTreeGridAxisReflection() = default;

//----------------------------------------------------------------------------
void svtkHyperTreeGridAxisReflection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Plane: " << this->Plane << endl;
  os << indent << "Center: " << this->Center << endl;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridAxisReflection::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkHyperTreeGridAxisReflection::ProcessTrees(svtkHyperTreeGrid* input, svtkDataObject* outputDO)
{
  // Skip empty inputs
  if (input->GetNumberOfLeaves() == 0)
  {
    return 1;
  }

  // Downcast output data object to hyper tree grid
  svtkHyperTreeGrid* output = svtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    svtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Shallow copy structure of input into output
  output->CopyStructure(input);

  // Shallow copy data of input into output
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->PassData(this->InData);

  // Retrieve reflection direction and coordinates to be reflected
  unsigned int direction = 0;
  double offset = 0.;
  svtkUniformHyperTreeGrid* inputUHTG = svtkUniformHyperTreeGrid::SafeDownCast(input);
  svtkUniformHyperTreeGrid* outputUHTG = svtkUniformHyperTreeGrid::SafeDownCast(outputDO);
  if (inputUHTG)
  {
    assert(outputUHTG);
    double origin[3];
    inputUHTG->GetOrigin(origin);
    double scale[3];
    inputUHTG->GetGridScale(scale);
    unsigned int pmod3 = this->Plane % 3;
    if (!pmod3)
    {
      direction = 0;
    }
    else if (pmod3 == 1)
    {
      direction = 1;
    }
    else
    {
      direction = 2;
    }

    // Retrieve size of reflected coordinates array
    unsigned int size = inputUHTG->GetCellDims()[direction];

    // Compute offset
    if (this->Plane < 3)
    {
      double u = origin[direction];
      double v = origin[direction] + size * scale[0];
      offset = u < v ? 2. * u : 2. * v;
    }
    else if (this->Plane < 6)
    {
      double u = origin[direction];
      double v = origin[direction] + size * scale[0];
      offset = u > v ? 2. * u : 2. * v;
    }
    else
    {
      offset = 2 * this->Center;
    }

    // Create array for reflected coordinates
    // Reflect point coordinate
    // Assign new coordinates to appropriate axis
    origin[direction] = offset - origin[direction];
    scale[direction] = -scale[direction];

    outputUHTG->SetOrigin(origin);
    outputUHTG->SetGridScale(scale);
  }
  else
  {
    svtkDataArray* inCoords = nullptr;
    unsigned int pmod3 = this->Plane % 3;
    if (!pmod3)
    {
      direction = 0;
      inCoords = input->GetXCoordinates();
    }
    else if (pmod3 == 1)
    {
      direction = 1;
      inCoords = input->GetYCoordinates();
    }
    else
    {
      direction = 2;
      inCoords = input->GetZCoordinates();
    }

    // Retrieve size of reflected coordinates array
    unsigned int size = input->GetCellDims()[direction];

    // Compute offset
    if (this->Plane < 3)
    {
      double u = inCoords->GetTuple1(0);
      double v = inCoords->GetTuple1(size);
      offset = u < v ? 2. * u : 2. * v;
    }
    else if (this->Plane < 6)
    {
      double u = inCoords->GetTuple1(0);
      double v = inCoords->GetTuple1(size);
      offset = u > v ? 2. * u : 2. * v;
    }
    else
    {
      offset = 2 * this->Center;
    }

    // Create array for reflected coordinates
    ++size;
    svtkDoubleArray* outCoords = svtkDoubleArray::New();
    outCoords->SetNumberOfTuples(size);

    // Reflect point coordinate
    double coord;
    for (unsigned int i = 0; i < size; ++i)
    {
      coord = inCoords->GetTuple1(i);
      outCoords->SetTuple1(i, offset - coord);
    } // i

    // Assign new coordinates to appropriate axis
    switch (direction)
    {
      case 0:
        output->SetXCoordinates(outCoords);
        break;
      case 1:
        output->SetYCoordinates(outCoords);
        break;
      case 2:
        output->SetZCoordinates(outCoords);
    } // switch ( direction )

    // Clean up
    outCoords->Delete();
  }

  // Retrieve interface arrays if available
  svtkDataArray* inNormals = nullptr;
  svtkDataArray* inIntercepts = nullptr;
  bool hasInterface = input->GetHasInterface() ? true : false;
  if (hasInterface)
  {
    inNormals = this->OutData->GetArray(output->GetInterfaceNormalsName());
    inIntercepts = this->OutData->GetArray(output->GetInterfaceInterceptsName());

    if (!inNormals || !inIntercepts)
    {
      svtkWarningMacro(<< "Incomplete material interface data; ignoring it.");
      hasInterface = false;
    }
  }

  // Create arrays for reflected interface if present
  svtkDoubleArray* outNormals = nullptr;
  svtkDoubleArray* outIntercepts = nullptr;
  if (hasInterface)
  {
    svtkIdType nTuples = inNormals->GetNumberOfTuples();
    outNormals = svtkDoubleArray::New();
    outNormals->SetNumberOfComponents(3);
    outNormals->SetNumberOfTuples(nTuples);
    outIntercepts = svtkDoubleArray::New();
    outIntercepts->SetNumberOfComponents(3);
    outIntercepts->SetNumberOfTuples(nTuples);

    // Reflect interface normals if present
    // Iterate over all cells
    for (svtkIdType i = 0; i < nTuples; ++i)
    {
      // Compute and stored reflected normal
      double norm[3];
      memcpy(norm, inNormals->GetTuple3(i), 3 * sizeof(double));
      norm[direction] = -norm[direction];
      outNormals->SetTuple3(i, norm[0], norm[1], norm[2]);

      // Compute and store reflected intercept
      double* inter = inIntercepts->GetTuple3(i);
      inter[0] -= 2. * offset * norm[direction];
      outIntercepts->SetTuple3(i, inter[0], inter[1], inter[2]);
    } // i

    // Assign new interface arrays if available
    this->OutData->SetVectors(outNormals);
    this->OutData->AddArray(outIntercepts);
  } // if ( hasInterface )

  // Clean up
  if (hasInterface)
  {
    outNormals->Delete();
    outIntercepts->Delete();
  }

  // Mise a jour du Scales des HTs
  svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
  output->InitializeTreeIterator(it);
  svtkHyperTree* tree = nullptr;
  svtkIdType index;
  while ((tree = it.GetNextTree(index)))
  {
    assert(tree->GetTreeIndex() == index);
    double origin[3];
    double scale[3];
    output->GetLevelZeroOriginAndSizeFromIndex(index, origin, scale);
    // JB Quid du Uniform ?
    tree->SetScales(std::make_shared<svtkHyperTreeGridScales>(output->GetBranchFactor(), scale));
  }
  //

  return 1;
}
