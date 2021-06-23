/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyPointSource.h"

#include "svtkCellArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPolyPointSource);

svtkCxxSetObjectMacro(svtkPolyPointSource, Points, svtkPoints);

//----------------------------------------------------------------------------
svtkPolyPointSource::svtkPolyPointSource()
{
  this->Points = nullptr;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkPolyPointSource::~svtkPolyPointSource()
{
  if (this->Points)
  {
    this->Points->Delete();
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkPolyPointSource::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Points != nullptr)
  {
    time = this->Points->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkPolyPointSource::SetNumberOfPoints(svtkIdType numPoints)
{
  if (!this->Points)
  {
    svtkPoints* pts = svtkPoints::New(SVTK_DOUBLE);
    this->SetPoints(pts);
    this->Points = pts;
    pts->Delete();
  }

  if (numPoints != this->GetNumberOfPoints())
  {
    this->Points->SetNumberOfPoints(numPoints);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkPolyPointSource::GetNumberOfPoints()
{
  if (this->Points)
  {
    return this->Points->GetNumberOfPoints();
  }

  return 0;
}

//----------------------------------------------------------------------------
void svtkPolyPointSource::Resize(svtkIdType numPoints)
{
  if (!this->Points)
  {
    this->SetNumberOfPoints(numPoints);
  }

  if (numPoints != this->GetNumberOfPoints())
  {
    this->Points->Resize(numPoints);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkPolyPointSource::SetPoint(svtkIdType id, double x, double y, double z)
{
  if (!this->Points)
  {
    return;
  }

  if (id >= this->Points->GetNumberOfPoints())
  {
    svtkErrorMacro(<< "point id " << id << " is larger than the number of points");
    return;
  }

  this->Points->SetPoint(id, x, y, z);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkPolyPointSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPoints = this->GetNumberOfPoints();
  svtkSmartPointer<svtkIdList> pointIds = svtkSmartPointer<svtkIdList>::New();
  pointIds->SetNumberOfIds(numPoints);
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    pointIds->SetId(i, i);
  }

  svtkSmartPointer<svtkCellArray> PolyPoint = svtkSmartPointer<svtkCellArray>::New();
  PolyPoint->InsertNextCell(pointIds);

  output->SetPoints(this->Points);
  output->SetVerts(PolyPoint);

  return 1;
}

//----------------------------------------------------------------------------
void svtkPolyPointSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Points: " << this->Points << "\n";
}
