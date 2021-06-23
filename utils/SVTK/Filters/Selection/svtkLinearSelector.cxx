#include "svtkLinearSelector.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataSetRange.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"

#include <vector>

svtkStandardNewMacro(svtkLinearSelector);
svtkCxxSetObjectMacro(svtkLinearSelector, Points, svtkPoints);

// ----------------------------------------------------------------------
svtkLinearSelector::svtkLinearSelector()
{

  this->StartPoint[0] = this->StartPoint[1] = this->StartPoint[2] = 0.0;
  this->EndPoint[0] = this->EndPoint[1] = this->EndPoint[2] = 1.0;
  this->Tolerance = 0.;
  this->IncludeVertices = true;
  this->VertexEliminationTolerance = 1.e-6;
  this->Points = nullptr;
}

// ----------------------------------------------------------------------
svtkLinearSelector::~svtkLinearSelector()
{
  this->SetPoints(nullptr);
}

// ----------------------------------------------------------------------
void svtkLinearSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Point 1: (" << this->StartPoint[0] << ", " << this->StartPoint[1] << ", "
     << this->StartPoint[2] << ")\n";

  os << indent << "Point 2: (" << this->EndPoint[0] << ", " << this->EndPoint[1] << ", "
     << this->EndPoint[2] << ")\n";

  os << indent << "Points: ";
  if (this->Points)
  {
    this->Points->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Include Vertices: " << (this->IncludeVertices ? "Yes" : "No") << "\n";

  os << indent << "VertexEliminationTolerance: " << this->VertexEliminationTolerance << "\n";
}

// ----------------------------------------------------------------------
int svtkLinearSelector::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");

  return 1;
}

// ----------------------------------------------------------------------
int svtkLinearSelector::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get information objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get input and output
  svtkCompositeDataSet* compositeInput =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkSelection* output = svtkSelection::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // preparation de l'output
  if (!output)
  {
    svtkErrorMacro(<< "svtkLinearSelector: filter does not have any output.");
    return 0;
  } // if ( ! output )

  if (!compositeInput)
  {
    svtkErrorMacro(<< "svtkLinearSelector: filter does not have any input.");
    return 0;
  } // if ( ! compositeInput )

  // Now traverse the input
  using Opts = svtk::CompositeDataSetOptions;
  for (auto node : svtk::Range(compositeInput, Opts::SkipEmptyNodes))
  {
    // Retrieve indices of current object
    svtkDataSet* input = svtkDataSet::SafeDownCast(node.GetDataObject());
    svtkIdTypeArray* indices = svtkIdTypeArray::New();
    this->SeekIntersectingCells(input, indices);

    // Create and add selection node
    svtkSelectionNode* n = svtkSelectionNode::New();
    n->SetContentType(svtkSelectionNode::INDICES);
    n->SetFieldType(svtkSelectionNode::CELL);
    n->GetProperties()->Set(
      svtkSelectionNode::COMPOSITE_INDEX(), static_cast<int>(node.GetFlatIndex()));
    n->SetSelectionList(indices);
    output->AddNode(n);

    // Clean up
    n->Delete();
    indices->Delete();
  }

  return 1;
}

// ----------------------------------------------------------------------
void svtkLinearSelector::SeekIntersectingCells(svtkDataSet* input, svtkIdTypeArray* outIndices)
{
  svtkIdType nSegments = this->Points ? this->Points->GetNumberOfPoints() - 1 : 1;

  // Reject meaningless parameterizations
  if (nSegments < 1)
  {
    svtkWarningMacro(<< "Cannot intersect: not enough points to define a broken line.");
    return;
  }

  // Prepare lists of start and end points
  svtkIdType nCoords = 3 * nSegments;
  double* startPoints = new double[nCoords];
  double* endPoints = new double[nCoords];

  if (this->Points)
  {
    // Prepare and store segment vertices
    if (this->IncludeVertices)
    {
      // Vertices are included, use full segment extent
      for (svtkIdType i = 0; i < nSegments; ++i)
      {
        svtkIdType offset = 3 * i;
        this->Points->GetPoint(i, startPoints + offset);
        this->Points->GetPoint(i + 1, endPoints + offset);
        cerr << i - 1 << ": " << startPoints[offset] << " " << startPoints[offset + 1] << " "
             << startPoints[offset + 2] << endl;
      }
    } // if ( this->IncludeVertices )
    else
    {
      // Vertices are excluded, reduce segment by given ratio
      for (svtkIdType i = 0; i < nSegments; ++i)
      {
        svtkIdType offset = 3 * i;
        this->Points->GetPoint(i, startPoints + offset);
        this->Points->GetPoint(i + 1, endPoints + offset);
        for (int j = 0; j < 3; ++j, ++offset)
        {
          double delta =
            this->VertexEliminationTolerance * (endPoints[offset] - startPoints[offset]);
          startPoints[offset] += delta;
          endPoints[offset] -= delta;
        } // for ( j )
      }   // for ( i )
    }     // else
  }       // if ( this->Points )
  else    // if ( this->Points )
  {
    // Prepare and store segment vertices
    if (this->IncludeVertices)
    {
      // Vertices are included, use full segment extent
      for (int i = 0; i < 3; ++i)
      {
        startPoints[i] = this->StartPoint[i];
        endPoints[i] = this->EndPoint[i];
      }
    } // if ( this->IncludeVertices )
    else
    {
      // Vertices are excluded, reduce segment by given ratio
      for (int i = 0; i < 3; ++i)
      {
        double delta = this->VertexEliminationTolerance * (this->EndPoint[i] - this->StartPoint[i]);
        startPoints[i] = this->StartPoint[i] + delta;
        endPoints[i] = this->EndPoint[i] - delta;
      }
    } // else
  }   // else // if ( this->Points )

  // Iterate over cells
  const svtkIdType nCells = input->GetNumberOfCells();
  for (svtkIdType id = 0; id < nCells; ++id)
  {
    svtkCell* cell = input->GetCell(id);
    if (cell)
    {
      // Storage for coordinates of intersection with the line
      double coords[3];
      double pcoords[3];
      double t = 0;
      int subId = 0;

      // Seek intersection of cell with each segment
      for (svtkIdType i = 0; i < nSegments; ++i)
      {
        // Intersection with a line segment
        svtkIdType offset = 3 * i;
        if (cell->IntersectWithLine(
              startPoints + offset, endPoints + offset, this->Tolerance, t, coords, pcoords, subId))
        {
          outIndices->InsertNextValue(id);
        }
      } // for ( i )
    }   // if ( cell )
  }     // for ( svtkIdType id = 0; id < nCells; ++ id )

  // Clean up
  delete[] startPoints;
  delete[] endPoints;
}
