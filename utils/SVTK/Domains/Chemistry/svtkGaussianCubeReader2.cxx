/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianCubeReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkGaussianCubeReader2.h"

#include "svtkDataObject.h"
#include "svtkExecutive.h"
#include "svtkFieldData.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPeriodicTable.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSimpleBondPerceiver.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"
#include "svtksys/FStream.hxx"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkGaussianCubeReader2);

//----------------------------------------------------------------------------
svtkGaussianCubeReader2::svtkGaussianCubeReader2()
  : FileName(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(2);

  // Add the second output for the grid data

  svtkImageData* grid;
  grid = svtkImageData::New();
  grid->ReleaseData();
  this->GetExecutive()->SetOutputData(1, grid);
  grid->Delete();
}

//----------------------------------------------------------------------------
svtkGaussianCubeReader2::~svtkGaussianCubeReader2()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
svtkMolecule* svtkGaussianCubeReader2::GetOutput()
{
  return svtkMolecule::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void svtkGaussianCubeReader2::SetOutput(svtkMolecule* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
svtkImageData* svtkGaussianCubeReader2::GetGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(1));
}

int svtkGaussianCubeReader2::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // the set the information for the imagedata output
  svtkInformation* gridInfo = this->GetExecutive()->GetOutputInformation(1);

  char title[256];

  if (!this->FileName)
  {
    return 0;
  }

  svtksys::ifstream file_in(this->FileName);

  if (!file_in.is_open())
  {
    svtkErrorMacro("GaussianCubeReader2 error opening file: " << this->FileName);
    return 0;
  }

  file_in.getline(title, 256); // first title line
  file_in.getline(title, 256); // second title line with name of scalar field?

  // Read in number of atoms, x-origin, y-origin z-origin
  double tmpd;
  int n1, n2, n3;
  if (!(file_in >> n1 >> tmpd >> tmpd >> tmpd))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    file_in.close();
    return 0;
  }

  if (!(file_in >> n2 >> tmpd >> tmpd >> tmpd))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    file_in.close();
    return 0;
  }
  if (!(file_in >> n3 >> tmpd >> tmpd >> tmpd))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    file_in.close();
    return 0;
  }

  svtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);
  gridInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, n1 - 1, 0, n2 - 1, 0, n3 - 1);
  gridInfo->Set(svtkDataObject::ORIGIN(), 0, 0, 0);
  gridInfo->Set(svtkDataObject::SPACING(), 1, 1, 1);

  file_in.close();

  svtkDataObject::SetPointDataActiveScalarInfo(gridInfo, SVTK_FLOAT, -1);
  return 1;
}

int svtkGaussianCubeReader2::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  char title[256];
  double elements[16];
  int NumberOfAtoms;
  int n1, n2, n3; // grid resolution
  float tmp, *cubedata;
  bool orbitalCubeFile = false;
  int numberOfOrbitals;

  svtkMolecule* output = svtkMolecule::SafeDownCast(svtkDataObject::GetData(outputVector));

  if (!output)
  {
    svtkErrorMacro(<< "svtkGaussianCubeReader2 does not have a svtkMolecule "
                     "as output.");
    return 1;
  }
  // Output 0 (the default is the svtkMolecule)
  // Output 1 will be the gridded Image data

  if (!this->FileName)
  {
    return 0;
  }

  svtksys::ifstream file_in(this->FileName);

  if (!file_in.is_open())
  {
    svtkErrorMacro("GaussianCubeReader2 error opening file: " << this->FileName);
    return 0;
  }

  file_in.getline(title, 256); // first title line
  file_in.getline(title, 256); // second title line with name of scalar field?

  // Read in number of atoms, x-origin, y-origin z-origin
  //
  if (!(file_in >> NumberOfAtoms >> elements[3] >> elements[7] >> elements[11]))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading atoms, x-origin y-origin z-origin.");
    file_in.close();
    return 0;
  }
  if (NumberOfAtoms < 0)
  {
    NumberOfAtoms = -NumberOfAtoms;
    orbitalCubeFile = true;
  }
  if (!(file_in >> n1 >> elements[0] >> elements[4] >> elements[8]))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    file_in.close();
    return 0;
  }
  if (!(file_in >> n2 >> elements[1] >> elements[5] >> elements[9]))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    file_in.close();
    return 0;
  }
  if (!(file_in >> n3 >> elements[2] >> elements[6] >> elements[10]))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    file_in.close();
    return 0;
  }
  elements[12] = 0;
  elements[13] = 0;
  elements[14] = 0;
  elements[15] = 1;

  svtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);

  svtkTransform* Transform = svtkTransform::New();

  Transform->SetMatrix(elements);
  Transform->Inverse();
  // construct svtkMolecule
  int atomType;
  float xyz[3];
  float dummy;

  for (int i = 0; i < NumberOfAtoms; i++)
  {
    if (!(file_in >> atomType >> dummy >> xyz[0] >> xyz[1] >> xyz[2]))
    {
      svtkErrorMacro("GaussianCubeReader error reading file: "
        << this->FileName << " Premature EOF while reading molecule.");
      file_in.close();
      return 0;
    }
    Transform->TransformPoint(xyz, xyz);
    output->AppendAtom(atomType, xyz[0], xyz[1], xyz[2]);
  }
  Transform->Delete();
  // construct grid data

  svtkImageData* grid = this->GetGridOutput();
  if (orbitalCubeFile)
  {
    if (!(file_in >> numberOfOrbitals))
    {
      svtkErrorMacro("GaussianCubeReader error reading file: "
        << this->FileName << " Premature EOF while reading number of orbitals.");
      file_in.close();
      return 0;
    }
    for (int k = 0; k < numberOfOrbitals; k++)
    {
      if (!(file_in >> tmp))
      {
        svtkErrorMacro("GaussianCubeReader error reading file: "
          << this->FileName << " Premature EOF while reading orbitals.");
        file_in.close();
        return 0;
      }
    }
  }

  svtkInformation* gridInfo = this->GetExecutive()->GetOutputInformation(1);
  gridInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, n1 - 1, 0, n2 - 1, 0, n3 - 1);
  gridInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    gridInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  grid->SetExtent(gridInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  grid->SetOrigin(0, 0, 0);
  grid->SetSpacing(1, 1, 1);
  grid->AllocateScalars(SVTK_FLOAT, 1);

  grid->GetPointData()->GetScalars()->SetName(title);

  cubedata = (float*)grid->GetPointData()->GetScalars()->GetVoidPointer(0);
  int N1N2 = n1 * n2;

  for (int i = 0; i < n1; i++)
  {
    int JN1 = 0;
    for (int j = 0; j < n2; j++)
    {
      for (int k = 0; k < n3; k++)
      {
        if (!(file_in >> tmp))
        {
          svtkErrorMacro("GaussianCubeReader error reading file: "
            << this->FileName << " Premature EOF while reading scalars.");
          file_in.close();
          return 0;
        }
        cubedata[k * N1N2 + JN1 + i] = tmp;
      }
      JN1 += n1;
    }
  }
  file_in.close();

  return 1;
}

int svtkGaussianCubeReader2::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    return this->Superclass::FillOutputPortInformation(port, info);
  }
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}

void svtkGaussianCubeReader2::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
