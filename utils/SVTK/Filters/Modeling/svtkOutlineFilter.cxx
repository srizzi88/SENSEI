/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOutlineFilter.h"

#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkPolyData.h"

#include <set>

class svtkOutlineFilter::svtkIndexSet : public std::set<unsigned int>
{
};

svtkStandardNewMacro(svtkOutlineFilter);

//----------------------------------------------------------------------------
svtkOutlineFilter::svtkOutlineFilter()
{
  this->GenerateFaces = 0;
  this->CompositeStyle = svtkOutlineFilter::ROOT_AND_LEAFS;
  this->OutputPointsPrecision = SINGLE_PRECISION;
  this->Indices = new svtkOutlineFilter::svtkIndexSet();
}

//----------------------------------------------------------------------------
svtkOutlineFilter::~svtkOutlineFilter()
{
  delete this->Indices;
}

//----------------------------------------------------------------------------
void svtkOutlineFilter::AddIndex(unsigned int index)
{
  if (this->Indices->find(index) == this->Indices->end())
  {
    this->Indices->insert(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkOutlineFilter::RemoveIndex(unsigned int index)
{
  if (this->Indices->find(index) != this->Indices->end())
  {
    this->Indices->erase(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkOutlineFilter::RemoveAllIndices()
{
  if (!this->Indices->empty())
  {
    this->Indices->clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkOutlineFilter::AppendOutline(
  svtkPoints* points, svtkCellArray* lines, svtkCellArray* faces, double bounds[6])
{
  // Since points have likely been inserted before, keep track of the point
  // offsets
  svtkIdType offset[8];

  // Insert points
  double x[3];
  x[0] = bounds[0];
  x[1] = bounds[2];
  x[2] = bounds[4];
  offset[0] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[2];
  x[2] = bounds[4];
  offset[1] = points->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[3];
  x[2] = bounds[4];
  offset[2] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[3];
  x[2] = bounds[4];
  offset[3] = points->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[2];
  x[2] = bounds[5];
  offset[4] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[2];
  x[2] = bounds[5];
  offset[5] = points->InsertNextPoint(x);
  x[0] = bounds[0];
  x[1] = bounds[3];
  x[2] = bounds[5];
  offset[6] = points->InsertNextPoint(x);
  x[0] = bounds[1];
  x[1] = bounds[3];
  x[2] = bounds[5];
  offset[7] = points->InsertNextPoint(x);

  // Produce topology, either lines or quad faces
  svtkIdType pts[4];

  // Always generate wire edges. I think this is dumb but historical....
  pts[0] = offset[0];
  pts[1] = offset[1];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[2];
  pts[1] = offset[3];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[4];
  pts[1] = offset[5];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[6];
  pts[1] = offset[7];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[0];
  pts[1] = offset[2];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[1];
  pts[1] = offset[3];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[4];
  pts[1] = offset[6];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[5];
  pts[1] = offset[7];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[0];
  pts[1] = offset[4];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[1];
  pts[1] = offset[5];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[2];
  pts[1] = offset[6];
  lines->InsertNextCell(2, pts);
  pts[0] = offset[3];
  pts[1] = offset[7];
  lines->InsertNextCell(2, pts);

  if (this->GenerateFaces)
  {
    pts[0] = offset[1];
    pts[1] = offset[0];
    pts[2] = offset[2];
    pts[3] = offset[3];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[0];
    pts[1] = offset[1];
    pts[2] = offset[5];
    pts[3] = offset[4];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[2];
    pts[1] = offset[0];
    pts[2] = offset[4];
    pts[3] = offset[6];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[3];
    pts[1] = offset[2];
    pts[2] = offset[6];
    pts[3] = offset[7];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[1];
    pts[1] = offset[3];
    pts[2] = offset[7];
    pts[3] = offset[5];
    faces->InsertNextCell(4, pts);
    pts[0] = offset[7];
    pts[1] = offset[6];
    pts[2] = offset[4];
    pts[3] = offset[5];
    faces->InsertNextCell(4, pts);
  }
}

//----------------------------------------------------------------------------
int svtkOutlineFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. Differentiate between composite and typical datasets.
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkCompositeDataSet* compInput =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkCompositeDataSet::DATA_OBJECT()));
  if (input == nullptr && compInput == nullptr)
  {
    svtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Creating outline");

  // Each outline is passed down to the core generation function
  svtkNew<svtkPoints> pts;
  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    pts->SetDataType(SVTK_DOUBLE);
  }
  else
  {
    pts->SetDataType(SVTK_FLOAT);
  }

  svtkNew<svtkCellArray> lines;
  svtkNew<svtkCellArray> faces;

  // If svtkDataSet input just create one bounding box. Composites may require multiple.
  double bds[6];
  if (input)
  {
    // dataset bounding box
    input->GetBounds(bds);
    this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
  }
  else
  {
    // Root bounding box
    if (this->CompositeStyle == ROOT_LEVEL || this->CompositeStyle == ROOT_AND_LEAFS)
    {
      compInput->GetBounds(bds);
      this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
    }

    // Leafs
    if (this->CompositeStyle == LEAF_DATASETS || this->CompositeStyle == ROOT_AND_LEAFS)
    {
      svtkCompositeDataIterator* iter = compInput->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (ds)
        {
          ds->GetBounds(bds);
          this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
        }
      }
      iter->Delete();
    }

    // Specified flat indices
    if (this->CompositeStyle == SPECIFIED_INDEX)
    {
      svtkCompositeDataIterator* iter = compInput->NewIterator();
      svtkDataObjectTreeIterator* treeIter = svtkDataObjectTreeIterator::SafeDownCast(iter);
      if (treeIter)
      {
        treeIter->VisitOnlyLeavesOff();
      }
      unsigned int index;
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (ds)
        {
          index = iter->GetCurrentFlatIndex();
          if (this->Indices->find(index) != this->Indices->end())
          {
            ds->GetBounds(bds);
            this->AppendOutline(pts.GetPointer(), lines.GetPointer(), faces.GetPointer(), bds);
          }
        } // if a dataset
      }
      iter->Delete();
    }
  }

  // Specify output
  output->SetPoints(pts.GetPointer());
  output->SetLines(lines.GetPointer());

  if (this->GenerateFaces)
  {
    output->SetPolys(faces.GetPointer());
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkOutlineFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkOutlineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Faces: " << (this->GenerateFaces ? "On\n" : "Off\n");
  os << indent << "Composite Style: " << this->CompositeStyle << endl;
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
  os << indent
     << "Composite indices: " << (!this->Indices->empty() ? "(Specified)\n" : "(Not specified)\n");
}
