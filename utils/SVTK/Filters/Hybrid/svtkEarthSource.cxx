/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEarthSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEarthSource.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkEarthSource);

// Description:
// Construct an Earth with radius = 1.0 and OnRatio set at 10. The outlines are drawn
// in wireframe as default.
svtkEarthSource::svtkEarthSource()
{
  this->Radius = 1.0;
  this->OnRatio = 10;
  this->Outline = 1;

  this->SetNumberOfInputPorts(0);
}

void svtkEarthSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "OnRatio: " << this->OnRatio << "\n";
  os << indent << "Outline: " << (this->Outline ? "On\n" : "Off\n");
}

#include "svtkEarthSourceData.cxx"

int svtkEarthSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int i;
  int maxPts;
  int maxPolys;
  svtkPoints* newPoints;
  svtkFloatArray* newNormals;
  svtkCellArray* newPolys;
  double x[3], base[3];
  svtkIdType Pts[4000];
  int npts, land, offset;
  int actualpts, actualpolys;
  double scale = 1.0 / 30000.0;

  //
  // Set things up; allocate memory
  //
  maxPts = 12000 / this->OnRatio;
  maxPolys = 16;
  actualpts = actualpolys = 0;

  newPoints = svtkPoints::New();
  newPoints->Allocate(maxPts);
  newNormals = svtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3 * maxPts);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(maxPolys, 4000 / this->OnRatio);

  //
  // Create points
  //
  offset = 0;
  while (1)
  {
    // read a polygon
    npts = svtkEarthData[offset++];
    if ((npts == 0) || (actualpolys > maxPolys))
    {
      break;
    }

    land = svtkEarthData[offset++];

    base[0] = 0;
    base[1] = 0;
    base[2] = 0;

    for (i = 1; i <= npts; i++)
    {
      base[0] += svtkEarthData[offset++] * scale;
      base[1] += svtkEarthData[offset++] * scale;
      base[2] += svtkEarthData[offset++] * scale;

      x[0] = base[2] * this->Radius;
      x[1] = base[0] * this->Radius;
      x[2] = base[1] * this->Radius;

      if ((land == 1) && (npts > this->OnRatio * 3))
      {
        // use only every OnRatioth point in the polygon
        if ((i % this->OnRatio) == 0)
        {
          newPoints->InsertNextPoint(x);
          svtkMath::Normalize(x);
          newNormals->InsertNextTuple(x);
          actualpts++;
        }
      }
    }

    if ((land == 1) && (npts > this->OnRatio * 3))
    {
      //
      // Generate mesh connectivity for this polygon
      //

      for (i = 0; i < (npts / this->OnRatio); i++)
      {
        Pts[i] = (actualpts - npts / this->OnRatio) + i;
      }

      if (this->Outline) // close the loop in the line
      {
        Pts[i] = (actualpts - npts / this->OnRatio);
        newPolys->InsertNextCell(i + 1, Pts);
      }
      else
      {
        newPolys->InsertNextCell(i, Pts);
      }

      actualpolys++;
    }
  }

  //
  // Update ourselves and release memory
  //
  output->SetPoints(newPoints);
  newPoints->Delete();

  output->GetPointData()->SetNormals(newNormals);
  newNormals->Delete();

  if (this->Outline) // lines or polygons
  {
    output->SetLines(newPolys);
  }
  else
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  output->Squeeze();

  return 1;
}
