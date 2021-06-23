/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOTUtilities.h"

#include "svtkDataArrayCollection.h"
#include "svtkDoubleArray.h"
#include "svtkOTIncludes.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"

using namespace OT;

//-----------------------------------------------------------------------------
Sample* svtkOTUtilities::SingleDimArraysToSample(svtkDataArrayCollection* arrays)
{
  if (arrays == nullptr)
  {
    return nullptr;
  }

  int numComp = arrays->GetNumberOfItems();
  if (numComp == 0)
  {
    svtkWarningWithObjectMacro(arrays, "Collection is empty");
    return nullptr;
  }
  int numTuples = arrays->GetItem(0)->GetNumberOfTuples();
  Sample* ns = new Sample(numTuples, numComp);

  int j = 0;
  arrays->InitTraversal();
  svtkDataArray* array = arrays->GetNextItem();
  while (array != nullptr)
  {
    if (numTuples != array->GetNumberOfTuples())
    {
      // TODO nullptr Object
      svtkErrorWithObjectMacro(arrays,
        "An array has not the expected number of tuples. Expecting: "
          << numTuples << " , got: " << array->GetNumberOfTuples() << " , dropping it");
      continue;
    }
    for (int i = 0; i < numTuples; ++i)
    {
      ns->at(i, j) = array->GetComponent(i, 0);
    }
    array = arrays->GetNextItem();
    j++;
  }
  return ns;
}

//-----------------------------------------------------------------------------
Sample* svtkOTUtilities::ArrayToSample(svtkDataArray* arr)
{
  if (arr == nullptr)
  {
    return nullptr;
  }

  svtkIdType numTuples = arr->GetNumberOfTuples();
  int numComp = arr->GetNumberOfComponents();
  Sample* ns = new Sample(numTuples, numComp);

  for (svtkIdType i = 0; i < numTuples; ++i)
  {
    for (unsigned int j = 0; j < ns->getDimension(); j++)
    {
      ns->at(i, j) = arr->GetComponent(i, j);
    }
  }
  return ns;
}

//-----------------------------------------------------------------------------
svtkDataArray* svtkOTUtilities::SampleToArray(Sample* ns)
{
  if (ns == nullptr)
  {
    return nullptr;
  }

  int numTuples = ns->getSize();
  int numComp = ns->getDimension();

  svtkDoubleArray* arr = svtkDoubleArray::New();
  arr->SetNumberOfTuples(numTuples);
  arr->SetNumberOfComponents(numComp);

  for (int i = 0; i < numTuples; i++)
  {
    for (int j = 0; j < numComp; j++)
    {
      arr->SetComponent(i, j, ns->at(i, j));
    }
  }
  return arr;
}
