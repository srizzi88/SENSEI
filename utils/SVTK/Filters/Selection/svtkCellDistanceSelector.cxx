/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellDistanceSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellDistanceSelector.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCellLinks.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"

#include <map>
#include <vector>

svtkStandardNewMacro(svtkCellDistanceSelector);

// ----------------------------------------------------------------------
svtkCellDistanceSelector::svtkCellDistanceSelector()
{
  this->Distance = 1;
  this->IncludeSeed = 1;
  this->AddIntermediate = 1;
  this->SetNumberOfInputPorts(2);
}

// ----------------------------------------------------------------------
svtkCellDistanceSelector::~svtkCellDistanceSelector() = default;

// ----------------------------------------------------------------------
void svtkCellDistanceSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------
int svtkCellDistanceSelector::FillInputPortInformation(int port, svtkInformation* info)
{
  switch (port)
  {
    case INPUT_MESH:
      info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
      break;
    case INPUT_SELECTION:
      info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
      break;
  }
  return 1;
}

// ----------------------------------------------------------------------
void svtkCellDistanceSelector::AddSelectionNode(
  svtkSelection* output, svtkSmartPointer<svtkDataArray> outIndices, int composite_index, int d)
{
  svtkSmartPointer<svtkSelectionNode> outSelNode = svtkSmartPointer<svtkSelectionNode>::New();
  outSelNode->SetContentType(svtkSelectionNode::INDICES);
  outSelNode->SetFieldType(svtkSelectionNode::CELL);
  outSelNode->GetProperties()->Set(svtkSelectionNode::COMPOSITE_INDEX(), composite_index);
  // NB: Use HIERARCHICAL_LEVEL key to store distance to original cells
  outSelNode->GetProperties()->Set(svtkSelectionNode::HIERARCHICAL_LEVEL(), d);
  outSelNode->SetSelectionList(outIndices);
  output->AddNode(outSelNode);
}

// ----------------------------------------------------------------------
int svtkCellDistanceSelector::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  // Retrieve input mesh as composite object
  svtkInformation* inDataObjectInfo = inputVector[INPUT_MESH]->GetInformationObject(0);
  svtkCompositeDataSet* compositeInput =
    svtkCompositeDataSet::SafeDownCast(inDataObjectInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Retrieve input selection
  svtkInformation* inSelectionInfo = inputVector[INPUT_SELECTION]->GetInformationObject(0);
  svtkSelection* inputSelection =
    svtkSelection::SafeDownCast(inSelectionInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Retrieve output selection
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkSelection* output = svtkSelection::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!compositeInput)
  {
    svtkErrorMacro(<< "Missing input data object");
    return 0;
  }

  if (!inputSelection)
  {
    svtkErrorMacro(<< "Missing input selection");
    return 0;
  }

  std::map<int, std::vector<svtkSelectionNode*> > partSelections;
  int nSelNodes = inputSelection->GetNumberOfNodes();
  for (int i = 0; i < nSelNodes; ++i)
  {
    svtkSelectionNode* sn = inputSelection->GetNode(i);
    int composite_index = sn->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX());
    partSelections[composite_index].push_back(sn);
  }

  svtkCompositeDataIterator* inputIterator = compositeInput->NewIterator();
  inputIterator->SkipEmptyNodesOn();
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();
  while (!inputIterator->IsDoneWithTraversal())
  {
    svtkDataSet* input = svtkDataSet::SafeDownCast(inputIterator->GetCurrentDataObject());
    // NB: composite indices start at 1
    int composite_index = inputIterator->GetCurrentFlatIndex();
    inputIterator->GoToNextItem();

    std::vector<svtkSelectionNode*>::iterator selNodeIt = partSelections[composite_index].begin();
    while (selNodeIt != partSelections[composite_index].end())
    {
      svtkSelectionNode* selectionNode = *selNodeIt;
      ++selNodeIt;

      svtkDataArray* selectionList =
        svtkArrayDownCast<svtkDataArray>(selectionNode->GetSelectionList());
      svtkIdType numSeeds = selectionList->GetNumberOfTuples();
      if (numSeeds > 0 && selectionNode->GetContentType() == svtkSelectionNode::INDICES &&
        selectionNode->GetFieldType() == svtkSelectionNode::CELL && input->GetNumberOfCells() > 0)
      {
        svtkIdType numCells = input->GetNumberOfCells();

        svtkUnstructuredGrid* ug_input = svtkUnstructuredGrid::SafeDownCast(input);
        svtkStructuredGrid* sg_input = svtkStructuredGrid::SafeDownCast(input);
        svtkPolyData* pd_input = svtkPolyData::SafeDownCast(input);

        if (ug_input)
        {
          if (!ug_input->GetCellLinks())
          {
            ug_input->BuildLinks();
          }
        }

        std::vector<int> flags(numCells, 0);

        svtkSmartPointer<svtkIdTypeArray> outIndices = svtkSmartPointer<svtkIdTypeArray>::New();
        outIndices->SetNumberOfTuples(numSeeds);

        int seedCount = 0;
        for (int i = 0; i < numSeeds; ++i)
        {
          svtkIdType cellIndex = static_cast<svtkIdType>(selectionList->GetTuple1(i));
          if (cellIndex >= 0 && cellIndex < numCells)
          {
            flags[cellIndex] = true;
            outIndices->SetTuple1(seedCount++, cellIndex);
          }
          else
          {
            svtkWarningMacro(<< "Cell index out of bounds in selection (" << cellIndex << "/"
                            << numCells << ")\n");
          }
        }
        outIndices->SetNumberOfTuples(seedCount);

        svtkSmartPointer<svtkIdTypeArray> finalIndices = svtkSmartPointer<svtkIdTypeArray>::New();
        svtkSmartPointer<svtkIntArray> cellDistance = svtkSmartPointer<svtkIntArray>::New();
        cellDistance->SetName("Cell Distance");

        // Iterate over increasing topological distance until desired distance is met
        for (int d = 0; d < this->Distance; ++d)
        {
          svtkSmartPointer<svtkIdTypeArray> nextIndices = svtkSmartPointer<svtkIdTypeArray>::New();

          if (ug_input)
          {
            int nIndices = outIndices->GetNumberOfTuples();
            for (int i = 0; i < nIndices; ++i)
            {
              svtkIdType cellIndex = static_cast<svtkIdType>(outIndices->GetTuple1(i));
              const svtkIdType* points;
              svtkIdType n;
              ug_input->GetCellPoints(cellIndex, n, points);
              for (int k = 0; k < n; ++k)
              {
                svtkIdType pid = points[k];
                svtkIdType np, *cells;
                ug_input->GetPointCells(pid, np, cells);
                for (int j = 0; j < np; ++j)
                {
                  svtkIdType cid = cells[j];
                  if (cid >= 0 && cid < numCells)
                  {
                    if (!flags[cid])
                    {
                      flags[cid] = true;
                      nextIndices->InsertNextValue(cid);
                    }
                  }
                  else
                  {
                    svtkWarningMacro(<< "Selection's cell index out of bounds (" << cid << "/"
                                    << numCells << ")\n");
                  }
                }
              }
            }
          } // if ( ug_input )
          else if (pd_input)
          {
            pd_input->BuildLinks();
            int nIndices = outIndices->GetNumberOfTuples();
            for (int i = 0; i < nIndices; ++i)
            {
              svtkIdType cellIndex = static_cast<svtkIdType>(outIndices->GetTuple1(i));
              const svtkIdType* points;
              svtkIdType n;
              pd_input->GetCellPoints(cellIndex, n, points);
              for (int k = 0; k < n; ++k)
              {
                svtkIdType pid = points[k];
                svtkIdType np;
                svtkIdType* cells;
                pd_input->GetPointCells(pid, np, cells);
                for (int j = 0; j < np; j++)
                {
                  svtkIdType cid = cells[j];
                  if (cid >= 0 && cid < numCells)
                  {
                    if (!flags[cid])
                    {
                      flags[cid] = true;
                      nextIndices->InsertNextValue(cid);
                    }
                  }
                  else
                  {
                    svtkWarningMacro(<< "Selection's cell index out of bounds (" << cid << "/"
                                    << numCells << ")\n");
                  }
                }
              }
            }
          } // else if ( ug_input )
          else if (sg_input)
          {
            int dim[3];
            sg_input->GetDimensions(dim);
            --dim[0];
            --dim[1];
            --dim[2];

            int nIndices = outIndices->GetNumberOfTuples();
            for (int idx = 0; idx < nIndices; ++idx)
            {
              svtkIdType cellIndex = static_cast<svtkIdType>(outIndices->GetTuple1(idx));
              svtkIdType cellId = cellIndex;
              svtkIdType ijk[3];
              for (int c = 0; c < 3; ++c)
              {
                if (dim[c] <= 1)
                {
                  ijk[c] = 0;
                }
                else
                {
                  ijk[c] = cellId % dim[c];
                  cellId /= dim[c];
                }
              } // c
              for (int k = -1; k <= 1; ++k)
              {
                for (int j = -1; j <= 1; ++j)
                {
                  for (int i = -1; i <= 1; ++i)
                  {
                    int I = ijk[0] + i;
                    int J = ijk[1] + j;
                    int K = ijk[2] + k;
                    if (I >= 0 && I < dim[0] && J >= 0 && J < dim[1] && K >= 0 && K < dim[2])
                    {
                      cellId = I + J * dim[0] + K * dim[0] * dim[1];
                      if (cellId >= 0 && cellId < numCells)
                      {
                        if (!flags[cellId])
                        {
                          flags[cellId] = true;
                          nextIndices->InsertNextValue(cellId);
                        }
                      }
                      else
                      {
                        svtkWarningMacro(<< "Selection's cell index out of bounds (" << cellId << "/"
                                        << numCells << ")\n");
                      }
                    }
                  } // i
                }   // j
              }     // k
            }       // idx
          }         // else if ( sg_input )
          else
          {
            svtkErrorMacro(<< "Unsupported data type : " << input->GetClassName() << "\n");
          }

          if ((!d && this->IncludeSeed) || (d > 0 && this->AddIntermediate))
          {
            int ni = outIndices->GetNumberOfTuples();
            for (int i = 0; i < ni; ++i)
            {
              cellDistance->InsertNextTuple1(d);
              finalIndices->InsertNextTuple1(outIndices->GetTuple1(i));
            } // i
          }   // if ( ( ! d && this->IncludeSeed ) || ( d > 0 && this->AddIntermediate ) )

          outIndices = nextIndices;
        } // for ( int d = 0; d < this->Distance; ++ d )

        //        if( ( ! this->Distance && this->IncludeSeed ) || ( this->Distance > 0 &&
        //        this->AddIntermediate ) )
        if ((!this->Distance && this->IncludeSeed) || this->Distance > 0)
        {
          int ni = outIndices->GetNumberOfTuples();
          cerr << "There are " << ni << " tuples\n";
          for (int i = 0; i < ni; ++i)
          {
            cellDistance->InsertNextTuple1(this->Distance);
            finalIndices->InsertNextTuple1(outIndices->GetTuple1(i));
          } // i
        }

        // Store selected cells for given seed cell
        if (finalIndices->GetNumberOfTuples() > 0)
        {
          svtkSmartPointer<svtkSelectionNode> outSelNode = svtkSmartPointer<svtkSelectionNode>::New();
          outSelNode->SetContentType(svtkSelectionNode::INDICES);
          outSelNode->SetFieldType(svtkSelectionNode::CELL);
          outSelNode->GetProperties()->Set(svtkSelectionNode::COMPOSITE_INDEX(), composite_index);
          outSelNode->SetSelectionList(finalIndices);
          outSelNode->GetSelectionData()->AddArray(cellDistance);
          output->AddNode(outSelNode);
        }
      } // if numSeeds > 0 etc.
    }   // while selNodeIt
  }     // while inputIterator

  // Clean up
  inputIterator->Delete();

  return 1;
}
