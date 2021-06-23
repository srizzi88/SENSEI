/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimplePointsReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimplePointsReader.h"

#include "svtkCellArray.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtksys/FStream.hxx"

svtkStandardNewMacro(svtkSimplePointsReader);

//----------------------------------------------------------------------------
svtkSimplePointsReader::svtkSimplePointsReader()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkSimplePointsReader::~svtkSimplePointsReader()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void svtkSimplePointsReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
int svtkSimplePointsReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  // Make sure we have a file to read.
  if (!this->FileName)
  {
    svtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  // Open the input file.
  svtksys::ifstream fin(this->FileName);
  if (!fin)
  {
    svtkErrorMacro("Error opening file " << this->FileName);
    return 0;
  }

  // Allocate objects to hold points and vertex cells.
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  svtkSmartPointer<svtkCellArray> verts = svtkSmartPointer<svtkCellArray>::New();

  // Read points from the file.
  svtkDebugMacro("Reading points from file " << this->FileName);
  double x[3];
  while (fin >> x[0] >> x[1] >> x[2])
  {
    svtkIdType id = points->InsertNextPoint(x);
    verts->InsertNextCell(1, &id);
  }
  svtkDebugMacro("Read " << points->GetNumberOfPoints() << " points.");

  // Store the points and cells in the output data object.
  svtkPolyData* output = svtkPolyData::GetData(outputVector);
  output->SetPoints(points);
  output->SetVerts(verts);

  return 1;
}
