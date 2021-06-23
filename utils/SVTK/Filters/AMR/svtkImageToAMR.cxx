/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToAMR.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageToAMR.h"

#include "svtkAMRBox.h"
#include "svtkAMRUtilities.h"
#include "svtkCellData.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkTuple.h"
#include "svtkUniformGrid.h"

#include "svtkAMRInformation.h"
#include <algorithm>
#include <vector>
namespace
{
// Split one box to eight
int SplitXYZ(svtkAMRBox inBox, int refinementRatio, std::vector<svtkAMRBox>& out)
{
  inBox.Refine(refinementRatio);
  const int* lo = inBox.GetLoCorner();
  const int* hi = inBox.GetHiCorner();

  // the cartesian product A[0][0..n[0]] X A[1][0..n[1] X A[2][0..n[2]] is the refined grid
  int A[3][3], n[3];
  for (int d = 0; d < 3; d++)
  {
    A[d][0] = lo[d] - 1;
    A[d][2] = hi[d];
    if (inBox.EmptyDimension(d))
    {
      n[d] = 1;
      A[d][1] = hi[d];
    }
    else
    {
      n[d] = 2;
      A[d][1] = (lo[d] + hi[d]) / 2;
    }
  }

  // create the refined boxes and push them to the output stack
  int numOut(0);
  for (int i = 0; i < n[0]; i++)
  {
    for (int j = 0; j < n[1]; j++)
    {
      for (int k = 0; k < n[2]; k++)
      {
        svtkAMRBox box;
        box.SetDimensions(
          A[0][i] + 1, A[1][j] + 1, A[2][k] + 1, A[0][i + 1], A[1][j + 1], A[2][k + 1]);
        out.push_back(box);
        numOut++;
      }
    }
  }
  return numOut;
}

int ComputeTreeHeight(int maxNumNodes, int degree)
{
  if (maxNumNodes <= 0)
  {
    return 0;
  }
  // could have used a formula, but this is more clear
  int height = 1;
  int numNodes = 1;
  while (numNodes <= maxNumNodes)
  {
    numNodes = numNodes + degree * numNodes;
    height++;
  }
  height--;
  return height;
}

// split the blocks into a tree that starts out as a single stem
// than turn a full tree. This shape is designed so that numLevels and maxNumBlocks
// constraint can be satisfied
void Split(const svtkAMRBox& rootBox, int numLevels, int refinementRatio, int maxNumBlocks,
  std::vector<std::vector<svtkAMRBox> >& out)
{
  out.clear();
  out.resize(1);
  out.back().push_back(rootBox);
  maxNumBlocks--;

  int treeDegree = rootBox.ComputeDimension() * 2;
  int numTreeLevels =
    std::min(numLevels, ComputeTreeHeight(maxNumBlocks - (numLevels - 1), treeDegree)) -
    1; // minus one because root already has one
  int level = 1;
  for (; level < numLevels - numTreeLevels; level++)
  {
    out.push_back(std::vector<svtkAMRBox>());
    const std::vector<svtkAMRBox>& parentBoxes = out[level - 1];
    std::vector<svtkAMRBox>& childBoxes = out[level];
    svtkAMRBox child = parentBoxes.back();
    child.Refine(refinementRatio);
    childBoxes.push_back(child);
  }

  for (; level < numLevels; level++)
  {
    out.push_back(std::vector<svtkAMRBox>());
    const std::vector<svtkAMRBox>& parentBoxes = out[level - 1];
    std::vector<svtkAMRBox>& childBoxes = out[level];
    for (size_t i = 0; i < parentBoxes.size(); i++)
    {
      const svtkAMRBox& parent = parentBoxes[i];
      SplitXYZ(parent, refinementRatio, childBoxes);
    }
  }
};

// create a grid by sampling from input using the indices in box
svtkUniformGrid* ConstructGrid(
  svtkImageData* input, const svtkAMRBox& box, int coarsenRatio, double* origin, double* spacing)
{
  int numPoints[3];
  box.GetNumberOfNodes(numPoints);

  svtkUniformGrid* grid = svtkUniformGrid::New();
  grid->Initialize();
  grid->SetDimensions(numPoints);
  grid->SetSpacing(spacing);
  grid->SetOrigin(origin);

  svtkPointData *inPD = input->GetPointData(), *outPD = grid->GetPointData();
  svtkCellData *inCD = input->GetCellData(), *outCD = grid->GetCellData();

  outPD->CopyAllocate(inPD, grid->GetNumberOfPoints());
  outCD->CopyAllocate(inCD, grid->GetNumberOfCells());

  svtkAMRBox box0(box);
  box0.Refine(coarsenRatio); // refine it to the image data level

  int extents[6];
  input->GetExtent(extents);
  int imLo[3] = { extents[0], extents[2], extents[4] };
  const int* lo = box.GetLoCorner();

  for (int iz = 0; iz < numPoints[2]; iz++)
  {
    for (int iy = 0; iy < numPoints[1]; iy++)
    {
      for (int ix = 0; ix < numPoints[0]; ix++)
      {
        int ijkDst[3] = { ix, iy, iz };
        svtkIdType idDst = grid->ComputePointId(ijkDst);

        int ijkSrc[3] = { (lo[0] + ix) * coarsenRatio + imLo[0],
          (lo[1] + iy) * coarsenRatio + imLo[1], (lo[2] + iz) * coarsenRatio + imLo[2] };

        svtkIdType idSrc = input->ComputePointId(ijkSrc);

        outPD->CopyData(inPD, idSrc, idDst);
      }
    }
  }

  int numCells[3];
  for (int d = 0; d < 3; d++)
  {
    numCells[d] = std::max(numPoints[d] - 1, 1);
  }

  for (int iz = 0; iz < numCells[2]; iz++)
  {
    for (int iy = 0; iy < numCells[1]; iy++)
    {
      for (int ix = 0; ix < numCells[0]; ix++)
      {
        int ijkDst[3] = { ix, iy, iz };
        svtkIdType idDst = grid->ComputeCellId(ijkDst);

        int ijkSrc[3] = { (lo[0] + ix) * coarsenRatio + imLo[0],
          (lo[1] + iy) * coarsenRatio + imLo[1], (lo[2] + iz) * coarsenRatio + imLo[2] };

        svtkIdType idSrc = input->ComputeCellId(ijkSrc);

        outCD->CopyData(inCD, idSrc, idDst);
      }
    }
  }

  return grid;
}

};

svtkStandardNewMacro(svtkImageToAMR);
//----------------------------------------------------------------------------
svtkImageToAMR::svtkImageToAMR()
{
  this->NumberOfLevels = 2;
  this->RefinementRatio = 2;
  this->MaximumNumberOfBlocks = 100;
}

//----------------------------------------------------------------------------
svtkImageToAMR::~svtkImageToAMR() = default;

//----------------------------------------------------------------------------
int svtkImageToAMR::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageToAMR::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  svtkImageData* input = svtkImageData::GetData(inputVector[0], 0);
  svtkOverlappingAMR* amr = svtkOverlappingAMR::GetData(outputVector);

  if (input->GetDataDimension() < 2)
  {
    svtkErrorMacro("Image dimension must be at least two.");
    return 0;
  }

  int whole_extent[6];
  inInfo->Get(svtkCompositeDataPipeline::WHOLE_EXTENT(), whole_extent);

  int dims[3] = { whole_extent[1] - whole_extent[0] + 1, whole_extent[3] - whole_extent[2] + 1,
    whole_extent[5] - whole_extent[4] + 1 };

  double inputBounds[6];
  input->GetBounds(inputBounds);

  double inputOrigin[3] = { inputBounds[0], inputBounds[2], inputBounds[4] };

  double inputSpacing[3];
  input->GetSpacing(inputSpacing);

  int gridDescription = svtkStructuredData::GetDataDescription(dims);

  // check whether the parameters are valid
  // and compute the base image resolution
  int dims0[3];
  double spacing0[3];
  for (int d = 0; d < 3; d++)
  {
    if (dims[d] <= 1)
    {
      if (dims[d] == 0)
      {
        svtkWarningMacro("Zero dimension? Really?");
      }
      dims0[d] = 1;
      spacing0[d] = 1.0;
    }
    else
    {
      int r = (int)(pow(static_cast<double>(this->RefinementRatio), this->NumberOfLevels - 1));
      if ((dims[d] - 1) % r != 0)
      {
        svtkErrorMacro("Image cannot be refined");
        return 0;
      }
      dims0[d] = 1 + (dims[d] - 1) / r;
      spacing0[d] = r * inputSpacing[d];
    }
  }

  svtkAMRBox rootBox(inputOrigin, dims0, spacing0, inputOrigin, gridDescription);

  std::vector<std::vector<svtkAMRBox> > amrBoxes;
  Split(
    rootBox, this->NumberOfLevels, this->RefinementRatio, this->MaximumNumberOfBlocks, amrBoxes);

  std::vector<int> blocksPerLevel;
  for (size_t i = 0; i < amrBoxes.size(); i++)
  {
    blocksPerLevel.push_back(static_cast<int>(amrBoxes[i].size()));
  }

  unsigned int numLevels = static_cast<unsigned int>(blocksPerLevel.size());

  amr->Initialize(static_cast<int>(numLevels), &blocksPerLevel[0]);
  amr->SetOrigin(inputOrigin);
  amr->SetGridDescription(gridDescription);

  double spacingi[3] = { spacing0[0], spacing0[1], spacing0[2] };
  for (unsigned int i = 0; i < numLevels; i++)
  {
    amr->SetSpacing(i, spacingi);
    for (int d = 0; d < 3; d++)
    {
      spacingi[d] /= this->RefinementRatio;
    }
  }

  for (unsigned int level = 0; level < numLevels; level++)
  {
    const std::vector<svtkAMRBox>& boxes = amrBoxes[level];
    for (size_t i = 0; i < boxes.size(); i++)
    {
      amr->SetAMRBox(level, static_cast<unsigned int>(i), boxes[i]);
    }
  }

  for (unsigned int level = 0; level < numLevels; level++)
  {
    double spacing[3];
    amr->GetSpacing(level, spacing);
    int coarsenRatio = (int)pow(static_cast<double>(this->RefinementRatio),
      static_cast<int>(numLevels - 1 - level)); // against the finest level
    for (size_t i = 0; i < amr->GetNumberOfDataSets(level); i++)
    {
      const svtkAMRBox& box = amr->GetAMRBox(level, static_cast<unsigned int>(i));
      double origin[3];
      svtkAMRBox::GetBoxOrigin(box, inputOrigin, spacing, origin);
      svtkUniformGrid* grid = ConstructGrid(input, box, coarsenRatio, origin, spacing);
      amr->SetDataSet(level, static_cast<unsigned int>(i), grid);
      grid->Delete();
    }
  }

  svtkAMRUtilities::BlankCells(amr);
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageToAMR::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfLevels: " << this->NumberOfLevels << endl;
  os << indent << "RefinementRatio: " << this->RefinementRatio << endl;
}
