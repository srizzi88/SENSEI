/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianCubeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGaussianCubeReader.h"

#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTransform.h"
#include "svtkUnsignedCharArray.h"
#include <svtksys/SystemTools.hxx>

#include <svtksys/SystemTools.hxx>

#include <cctype>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkGaussianCubeReader);

//----------------------------------------------------------------------------
// Construct object with merging set to true.
svtkGaussianCubeReader::svtkGaussianCubeReader()
{
  this->Transform = svtkTransform::New();
  // Add the second output for the grid data

  this->SetNumberOfOutputPorts(2);
  svtkImageData* grid;
  grid = svtkImageData::New();
  grid->ReleaseData();
  this->GetExecutive()->SetOutputData(1, grid);
  grid->Delete();
}

//----------------------------------------------------------------------------
svtkGaussianCubeReader::~svtkGaussianCubeReader()
{

  this->Transform->Delete();
  // must delete the second output added
}

//----------------------------------------------------------------------------
int svtkGaussianCubeReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  FILE* fp;
  char title[256];
  char data_name[256];
  double elements[16];
  int JN1, N1N2, n1, n2, n3, i, j, k;
  float tmp, *cubedata;
  bool orbitalCubeFile = false;
  int numberOfOrbitals;

  // Output 0 (the default is the polydata)
  // Output 1 will be the gridded Image data

  svtkImageData* grid = this->GetGridOutput();

  if (!this->FileName)
  {
    return 0;
  }

  if ((fp = svtksys::SystemTools::Fopen(this->FileName, "r")) == nullptr)
  {
    svtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  if (!fgets(title, 256, fp))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }

  // TODO: SystemTools::Split should be replaced by a SystemTools::SplitN call
  // which only splits up to N times as soon as it exists
  std::vector<std::string> tokens;
  svtksys::SystemTools::Split(title, tokens, ':');
  if (tokens.size() > 2)
  {
    for (std::size_t token = 3; token < tokens.size(); ++token)
    {
      tokens[2] += ":" + tokens[token];
    }
    strcpy(data_name, tokens[2].c_str());
    fprintf(stderr, "label = %s\n", data_name);
  }

  if (!fgets(title, 256, fp))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }

  // Read in number of atoms, x-origin, y-origin z-origin
  //
  if (fscanf(fp, "%d %lf %lf %lf", &(this->NumberOfAtoms), &elements[3], &elements[7],
        &elements[11]) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading atoms, x-origin y-origin z-origin.");
    fclose(fp);
    return 0;
  }
  if (this->NumberOfAtoms < 0)
  {
    this->NumberOfAtoms = -this->NumberOfAtoms;
    orbitalCubeFile = true;
  }

  if (fscanf(fp, "%d %lf %lf %lf", &n1, &elements[0], &elements[4], &elements[8]) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    fclose(fp);
    return 0;
  }
  if (fscanf(fp, "%d %lf %lf %lf", &n2, &elements[1], &elements[5], &elements[9]) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    fclose(fp);
    return 0;
  }
  if (fscanf(fp, "%d %lf %lf %lf", &n3, &elements[2], &elements[6], &elements[10]) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading elements.");
    fclose(fp);
    return 0;
  }
  elements[12] = 0;
  elements[13] = 0;
  elements[14] = 0;
  elements[15] = 1;

  svtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);

  Transform->SetMatrix(elements);
  Transform->Inverse();

  this->ReadMolecule(fp, output);

  if (orbitalCubeFile)
  {
    if (fscanf(fp, "%d", &numberOfOrbitals) != 1)
    {
      svtkErrorMacro("GaussianCubeReader error reading file: "
        << this->FileName << " Premature EOF while reading number of orbitals.");
      fclose(fp);
      return 0;
    }
    for (k = 0; k < numberOfOrbitals; k++)
    {
      if (fscanf(fp, "%f", &tmp) != 1)
      {
        svtkErrorMacro("GaussianCubeReader error reading file: "
          << this->FileName << " Premature EOF while reading orbitals.");
        fclose(fp);
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
  N1N2 = n1 * n2;

  for (i = 0; i < n1; i++)
  {
    JN1 = 0;
    for (j = 0; j < n2; j++)
    {
      for (k = 0; k < n3; k++)
      {
        if (fscanf(fp, "%f", &tmp) != 1)
        {
          svtkErrorMacro("GaussianCubeReader error reading file: "
            << this->FileName << " Premature EOF while reading scalars.");
          fclose(fp);
          return 0;
        }
        cubedata[k * N1N2 + JN1 + i] = tmp;
      }
      JN1 += n1;
    }
  }
  fclose(fp);

  return 1;
}

//----------------------------------------------------------------------------
void svtkGaussianCubeReader::ReadSpecificMolecule(FILE* fp)
{
  int i, j;
  float x[3];
  float dummy;

  for (i = 0; i < this->NumberOfAtoms; i++)
  {
    if (fscanf(fp, "%d %f %f %f %f", &j, &dummy, x, x + 1, x + 2) != 5)
    {
      svtkErrorMacro("GaussianCubeReader error reading file: "
        << this->FileName << " Premature EOF while reading molecule.");
      fclose(fp);
      return;
    }
    this->Transform->TransformPoint(x, x);
    this->Points->InsertNextPoint(x);
    this->AtomType->InsertNextValue(j - 1);
    this->AtomTypeStrings->InsertNextValue("Xx");
    this->Residue->InsertNextValue(-1);
    this->Chain->InsertNextValue(0);
    this->SecondaryStructures->InsertNextValue(0);
    this->SecondaryStructuresBegin->InsertNextValue(0);
    this->SecondaryStructuresEnd->InsertNextValue(0);
    this->IsHetatm->InsertNextValue(0);
  }
}

//----------------------------------------------------------------------------
svtkImageData* svtkGaussianCubeReader::GetGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
void svtkGaussianCubeReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "Filename: " << (this->FileName ? this->FileName : "(none)") << "\n";

  os << "Transform: ";
  if (this->Transform)
  {
    os << endl;
    this->Transform->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}

//----------------------------------------------------------------------------
// Default implementation - copy information from first input to all outputs
int svtkGaussianCubeReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // the set the information for the imagedat output
  svtkInformation* gridInfo = this->GetExecutive()->GetOutputInformation(1);

  FILE* fp;
  char title[256];

  if (!this->FileName)
  {
    return 0;
  }

  if ((fp = svtksys::SystemTools::Fopen(this->FileName, "r")) == nullptr)
  {
    svtkErrorMacro(<< "File " << this->FileName << " not found");
    return 0;
  }

  if (!fgets(title, 256, fp))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }
  if (!fgets(title, 256, fp))
  {
    svtkErrorMacro("GaussianCubeReader error reading file: "
      << this->FileName << " Premature EOF while reading title.");
    fclose(fp);
    return 0;
  }

  // Read in number of atoms, x-origin, y-origin z-origin
  double tmpd;
  int n1, n2, n3;
  if (fscanf(fp, "%d %lf %lf %lf", &n1, &tmpd, &tmpd, &tmpd) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }

  if (fscanf(fp, "%d %lf %lf %lf", &n1, &tmpd, &tmpd, &tmpd) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }
  if (fscanf(fp, "%d %lf %lf %lf", &n2, &tmpd, &tmpd, &tmpd) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }
  if (fscanf(fp, "%d %lf %lf %lf", &n3, &tmpd, &tmpd, &tmpd) != 4)
  {
    svtkErrorMacro("GaussianCubeReader error reading file: " << this->FileName
                                                            << " Premature EOF while grid size.");
    fclose(fp);
    return 0;
  }

  svtkDebugMacro(<< "Grid Size " << n1 << " " << n2 << " " << n3);
  gridInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, n1 - 1, 0, n2 - 1, 0, n3 - 1);
  gridInfo->Set(svtkDataObject::ORIGIN(), 0, 0, 0);
  gridInfo->Set(svtkDataObject::SPACING(), 1, 1, 1);

  fclose(fp);

  svtkDataObject::SetPointDataActiveScalarInfo(gridInfo, SVTK_FLOAT, -1);
  return 1;
}

//----------------------------------------------------------------------------
int svtkGaussianCubeReader::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    return this->Superclass::FillOutputPortInformation(port, info);
  }
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}
