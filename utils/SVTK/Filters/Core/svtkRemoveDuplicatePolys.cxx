/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveDuplicatePolys.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRemoveDuplicatePolys.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCollection.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

svtkStandardNewMacro(svtkRemoveDuplicatePolys);

//----------------------------------------------------------------------------
svtkRemoveDuplicatePolys::svtkRemoveDuplicatePolys() {}

//----------------------------------------------------------------------------
svtkRemoveDuplicatePolys::~svtkRemoveDuplicatePolys() {}

//----------------------------------------------------------------------------
void svtkRemoveDuplicatePolys::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkRemoveDuplicatePolys::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPolys() == 0)
  {
    // set up a polyData with same data arrays as input, but
    // no points, polys or data.
    output->ShallowCopy(input);
    return 1;
  }

  // Copy over the original points. Assume there are no degenerate points.
  output->SetPoints(input->GetPoints());

  // Remove duplicate polys.
  std::map<std::set<int>, svtkIdType> polySet;
  std::map<std::set<int>, svtkIdType>::iterator polyIter;

  // Now copy the polys.
  svtkIdList* polyPoints = svtkIdList::New();
  const svtkIdType numberOfPolys = input->GetNumberOfPolys();
  svtkIdType progressStep = numberOfPolys / 100;
  if (progressStep == 0)
  {
    progressStep = 1;
  }

  output->AllocateCopy(input);
  int ndup = 0;

  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyAllocate(input->GetCellData(), numberOfPolys);

  for (int id = 0; id < numberOfPolys; ++id)
  {
    if (id % progressStep == 0)
    {
      this->UpdateProgress(0.8 + 0.2 * (static_cast<float>(id) / numberOfPolys));
    }

    // duplicate points do not make poly vertices or triangles
    // strips degenerate so don't remove them
    int polyType = input->GetCellType(id);
    if (polyType == SVTK_POLY_VERTEX || polyType == SVTK_TRIANGLE_STRIP)
    {
      input->GetCellPoints(id, polyPoints);
      svtkIdType newId = output->InsertNextCell(polyType, polyPoints);
      output->GetCellData()->CopyData(input->GetCellData(), id, newId);
      continue;
    }

    input->GetCellPoints(id, polyPoints);
    std::set<int> nn;
    std::vector<int> ptIds;
    for (int i = 0; i < polyPoints->GetNumberOfIds(); ++i)
    {
      int polyPtId = polyPoints->GetId(i);
      nn.insert(polyPtId);
      ptIds.push_back(polyPtId);
    }

    // this conditional may generate non-referenced nodes
    polyIter = polySet.find(nn);

    // only copy a cell to the output if it is neither degenerate nor duplicate
    if (nn.size() == static_cast<unsigned int>(polyPoints->GetNumberOfIds()) &&
      polyIter == polySet.end())
    {
      svtkIdType newId = output->InsertNextCell(input->GetCellType(id), polyPoints);
      output->GetCellData()->CopyData(input->GetCellData(), id, newId);
      polySet[nn] = newId;
    }
    else if (polyIter != polySet.end())
    {
      ++ndup; // cell has duplicate(s)
    }
  }

  if (ndup)
  {
    svtkDebugMacro(<< "svtkRemoveDuplicatePolys : " << ndup
                  << " duplicate polys (multiple instances of a polygon) have been"
                  << " removed." << endl);

    polyPoints->Delete();
    output->Squeeze();
  }

  return 1;
}
