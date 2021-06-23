/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyLineSource.h"

#include "svtkCellArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPolyLineSource);

//----------------------------------------------------------------------------
svtkPolyLineSource::svtkPolyLineSource()
{
  this->Closed = 0;
}

//----------------------------------------------------------------------------
svtkPolyLineSource::~svtkPolyLineSource() {}

//----------------------------------------------------------------------------
int svtkPolyLineSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPoints = this->GetNumberOfPoints();
  svtkSmartPointer<svtkIdList> pointIds = svtkSmartPointer<svtkIdList>::New();
  pointIds->SetNumberOfIds(this->Closed ? numPoints + 1 : numPoints);
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    pointIds->SetId(i, i);
  }
  if (this->Closed)
  {
    pointIds->SetId(numPoints, 0);
  }

  svtkSmartPointer<svtkCellArray> polyLine = svtkSmartPointer<svtkCellArray>::New();
  polyLine->InsertNextCell(pointIds);

  output->SetPoints(this->Points);
  output->SetLines(polyLine);

  return 1;
}

//----------------------------------------------------------------------------
void svtkPolyLineSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Closed: " << this->Closed << "\n";
}
