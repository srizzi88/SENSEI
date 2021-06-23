/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSubdivisionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSubdivisionFilter.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkEdgeTable.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

#include <map>
#include <sstream>

// Construct object with number of subdivisions set to 1, check for
// triangles set to 1
svtkSubdivisionFilter::svtkSubdivisionFilter()
{
  this->NumberOfSubdivisions = 1;
  this->CheckForTriangles = 1;
}

int svtkSubdivisionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // validate the input
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // get the input
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  svtkIdType numCells, numPts;
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
  {
    svtkErrorMacro(<< "No data to subdivide");
    return 0;
  }

  if (this->CheckForTriangles)
  {
    std::map<int, int> badCellTypes;
    bool hasOnlyTris = true;
    svtkCellIterator* it = input->NewCellIterator();
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
    {
      if (it->GetCellType() != SVTK_TRIANGLE)
      {
        hasOnlyTris = false;
        badCellTypes[it->GetCellType()] += 1;
        continue;
      }
    }
    it->Delete();

    if (!hasOnlyTris)
    {
      std::ostringstream msg;
      std::map<int, int>::iterator cit;
      for (cit = badCellTypes.begin(); cit != badCellTypes.end(); ++cit)
      {
        msg << "Cell type: " << cit->first << " Count: " << cit->second << "\n";
      }
      svtkErrorMacro(<< this->GetClassName()
                    << " only operates on triangles, but "
                       "this data set has other cell types present.\n"
                    << msg.str());
      return 0;
    }
  }
  return 1;
}
void svtkSubdivisionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of subdivisions: " << this->GetNumberOfSubdivisions() << endl;
  os << indent << "Check for triangles: " << this->GetCheckForTriangles() << endl;
}
