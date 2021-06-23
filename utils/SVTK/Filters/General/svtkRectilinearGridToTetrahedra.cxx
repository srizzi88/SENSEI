/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridToTetrahedra.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearGridToTetrahedra.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkSignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVoxel.h"

svtkStandardNewMacro(svtkRectilinearGridToTetrahedra);

// ways to convert to a voxel to tetrahedra.
// Note that the values 0 and 1 and -1 and 2 are important in
// DetermineGridDivisionTypes()
#define SVTK_TETRAHEDRALIZE_5 0
#define SVTK_TETRAHEDRALIZE_5_FLIP 1
#define SVTK_TETRAHEDRALIZE_6 6
#define SVTK_TETRAHEDRALIZE_12_CONFORM (-1)
#define SVTK_TETRAHEDRALIZE_12_CONFORM_FLIP 2
#define SVTK_TETRAHEDRALIZE_12 10

//-------------------------------------------------------------------------

svtkRectilinearGridToTetrahedra::svtkRectilinearGridToTetrahedra()
{
  this->TetraPerCell = SVTK_VOXEL_TO_5_TET;
  this->RememberVoxelId = 0;
}

//----------------------------------------------------------------------------

void svtkRectilinearGridToTetrahedra::SetInput(const double ExtentX, const double ExtentY,
  const double ExtentZ, const double SpacingX, const double SpacingY, const double SpacingZ,
  const double tol)
{
  double Extent[3];
  double Spacing[3];
  Extent[0] = ExtentX;
  Extent[1] = ExtentY;
  Extent[2] = ExtentZ;
  Spacing[0] = SpacingX;
  Spacing[1] = SpacingY;
  Spacing[2] = SpacingZ;
  this->SetInput(Extent, Spacing, tol);
}

//----------------------------------------------------------------------------

// Create an input for the filter
void svtkRectilinearGridToTetrahedra::SetInput(
  const double Extent[3], const double Spacing[3], const double tol)
{
  //
  // Determine the number of points in each direction, and the positions
  // The last voxel may have a different spacing to fit inside
  // the selected region
  //

  int i, j;
  int NumPointsInDir[3];
  svtkFloatArray* Coord[3];
  for (i = 0; i < 3; i++)
  {
    double NumRegion = Extent[i] / Spacing[i];

    // If we are really close to an integer number of elements, use the
    // integer number
    if (fabs(NumRegion - floor(NumRegion + 0.5)) < tol * Spacing[i])
      NumPointsInDir[i] = ((int)floor(NumRegion + 0.5)) + 1;
    else
      NumPointsInDir[i] = (int)ceil(Extent[i] / Spacing[i]) + 1;
    Coord[i] = svtkFloatArray::New();
    Coord[i]->SetNumberOfValues(NumPointsInDir[i] + 1);

    // The last data point inserted is exactly the Extent
    // Thus avoiding a bit of numerical error.
    for (j = 0; j < NumPointsInDir[i] - 1; j++)
    {
      Coord[i]->SetValue(j, Spacing[i] * j);
    }
    Coord[i]->SetValue(NumPointsInDir[i] - 1, Extent[i]);
  }

  //
  // Form the grid
  //

  svtkRectilinearGrid* RectGrid = svtkRectilinearGrid::New();
  RectGrid->SetDimensions(NumPointsInDir);
  RectGrid->SetXCoordinates(Coord[0]);
  RectGrid->SetYCoordinates(Coord[1]);
  RectGrid->SetZCoordinates(Coord[2]);

  Coord[0]->Delete();
  Coord[1]->Delete();
  Coord[2]->Delete();

  // Get the reference counting right.
  this->Superclass::SetInputData(RectGrid);
  RectGrid->Delete();
}

//----------------------------------------------------------------------------

// Determine how to Divide each voxel in the svtkRectilinearGrid
void svtkRectilinearGridToTetrahedra::DetermineGridDivisionTypes(
  svtkRectilinearGrid* RectGrid, svtkSignedCharArray* VoxelSubdivisionType, const int& tetraPerCell)
{
  int numRec = RectGrid->GetNumberOfCells();
  int NumPointsInDir[3];
  RectGrid->GetDimensions(NumPointsInDir);

  // How to break into Tetrahedra.
  // For division into 5's, we need to flip from one orientation to
  // the next

  int Rec[3];
  int flip;
  int i;

  switch (tetraPerCell)
  {
    case (SVTK_VOXEL_TO_12_TET):
      for (i = 0; i < numRec; i++)
      {
        VoxelSubdivisionType->SetValue(i, SVTK_TETRAHEDRALIZE_12);
      }
      break;
    case (SVTK_VOXEL_TO_6_TET):
      for (i = 0; i < numRec; i++)
      {
        VoxelSubdivisionType->SetValue(i, SVTK_TETRAHEDRALIZE_6);
      }
      break;
    case (SVTK_VOXEL_TO_5_TET):
      for (Rec[0] = 0; Rec[0] < NumPointsInDir[0] - 1; Rec[0]++)
      {
        for (Rec[1] = 0; Rec[1] < NumPointsInDir[1] - 1; Rec[1]++)
        {
          flip = (Rec[1] + Rec[0]) % 2;
          for (Rec[2] = 0; Rec[2] < NumPointsInDir[2] - 1; Rec[2]++)
          {
            VoxelSubdivisionType->SetValue(RectGrid->ComputeCellId(Rec), flip);
            flip = 1 - flip;
          }
        }
      }
      break;
    case (SVTK_VOXEL_TO_5_AND_12_TET):
      for (Rec[0] = 0; Rec[0] < NumPointsInDir[0] - 1; Rec[0]++)
      {
        for (Rec[1] = 0; Rec[1] < NumPointsInDir[1] - 1; Rec[1]++)
        {
          flip = (Rec[1] + Rec[0]) % 2;
          for (Rec[2] = 0; Rec[2] < NumPointsInDir[2] - 1; Rec[2]++)
          {
            int CellId = RectGrid->ComputeCellId(Rec);
            if (VoxelSubdivisionType->GetValue(CellId) == 12)
              VoxelSubdivisionType->SetValue(CellId, 3 * flip - 1);
            else
              VoxelSubdivisionType->SetValue(CellId, flip);
            flip = 1 - flip;
          }
        }
      }
      break;
  }
}

//----------------------------------------------------------------------------

// Take the grid and make it into a tetrahedral mesh.
void svtkRectilinearGridToTetrahedra::GridToTetMesh(svtkRectilinearGrid* RectGrid,
  svtkSignedCharArray* VoxelSubdivisionType, const int& tetraPerCell, const int& rememberVoxelId,
  svtkUnstructuredGrid* TetMesh)
{
  int i, j;
  int numPts = RectGrid->GetNumberOfPoints();
  int numRec = RectGrid->GetNumberOfCells();

  // We need a point list and a cell list
  svtkPoints* NodePoints = svtkPoints::New();
  svtkCellArray* TetList = svtkCellArray::New();

  // Guess number of points and cells!!
  // For mixture of 5 and 12 tet per cell,
  // it is easier to way overguess to avoid re-allocation
  // slowness and range checking during insertion.

  switch (tetraPerCell)
  {
    case (SVTK_VOXEL_TO_5_TET):
      NodePoints->Allocate(numPts);
      TetList->AllocateEstimate(numPts * 5, 4);
      break;
    case (SVTK_VOXEL_TO_5_AND_12_TET):
    case (SVTK_VOXEL_TO_12_TET):
      NodePoints->Allocate(numPts * 2);
      TetList->AllocateEstimate(numPts * 12, 4);
      break;
  }

  // Start by copying over the points
  for (i = 0; i < numPts; i++)
  {
    NodePoints->InsertNextPoint(RectGrid->GetPoint(i));
  }

  // If they want, we can add Scalar Data
  // to the Tets indicating the Voxel Id the tet
  // came from.
  svtkIntArray* TetOriginalVoxel = nullptr;
  if (rememberVoxelId)
  {
    TetOriginalVoxel = svtkIntArray::New();
    TetOriginalVoxel->Allocate(12 * numRec);
  }

  // 9 ids, 8 corners and a possible center to be added later
  //        during the tet creation
  svtkIdList* VoxelCorners = svtkIdList::New();
  VoxelCorners->SetNumberOfIds(9);

  int NumTetFromVoxel;
  for (i = 0; i < numRec; i++)
  {
    RectGrid->GetCellPoints(i, VoxelCorners);
    NumTetFromVoxel = TetrahedralizeVoxel(
      VoxelCorners, (int)VoxelSubdivisionType->GetValue(i), NodePoints, TetList);
    if (rememberVoxelId)
    {
      for (j = 0; j < NumTetFromVoxel; j++)
      {
        TetOriginalVoxel->InsertNextValue(i);
      }
    }
  }

  //
  // It may be there are extra points at the end of the PointList.
  //

  NodePoints->Squeeze();

  //
  // Form the Mesh
  //

  // Need to tell the tet mesh that every cell is a Tetrahedron
  int numTet = TetList->GetNumberOfCells();
  int* CellTypes = new int[numTet];
  for (i = 0; i < numTet; i++)
  {
    CellTypes[i] = SVTK_TETRA;
  }

  TetMesh->SetPoints(NodePoints);
  TetMesh->SetCells(CellTypes, TetList);

  //
  // Add Scalar Types if wanted
  //

  if (rememberVoxelId)
  {
    TetOriginalVoxel->Squeeze();
    int idx = TetMesh->GetCellData()->AddArray(TetOriginalVoxel);
    TetMesh->GetCellData()->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    TetOriginalVoxel->Delete();
  }

  //
  // Clean Up
  //
  delete[] CellTypes;
  NodePoints->Delete();
  TetList->Delete();
  VoxelCorners->Delete();

  TetMesh->Squeeze();
}

//----------------------------------------------------------------------------
// Helper Function for Tetrahedralize Voxel
inline void svtkRectilinearGridToTetrahedra::TetrahedralizeAddCenterPoint(
  svtkIdList* VoxelCorners, svtkPoints* NodeList)
{
  // Need to add a center point
  double c1[3], c2[3];
  NodeList->GetPoint(VoxelCorners->GetId(0), c2);
  NodeList->GetPoint(VoxelCorners->GetId(7), c1);
  double center[3];
  center[0] = (c1[0] + c2[0]) / 2.0;
  center[1] = (c1[1] + c2[1]) / 2.0;
  center[2] = (c1[2] + c2[2]) / 2.0;

  VoxelCorners->InsertId(8, NodeList->InsertNextPoint(center));
}

//----------------------------------------------------------------------------

// Split A Cube into Tetrahedrons
// According to the DivisionType
// There had better be 0..8 voxel corners, though only 0..7 maybe needed.
// Why? This function may add id 8 to VoxelCorners.
// If a point needs to be inserted into the nodelist, insert
// it at NextPointId. Assume there is space in the nodelist.
// Return the number of Tets Added.

int svtkRectilinearGridToTetrahedra::TetrahedralizeVoxel(
  svtkIdList* VoxelCorners, const int& DivisionType, svtkPoints* NodeList, svtkCellArray* TetList)
{

  // See svtkVoxel::Triangulate
  /*  Looking at the rect: Corner labeling

       0  1
       2  3

     Directly behind them:
      4   5
      6   7

  and 8 is in the middle of the cube if used

  Want right handed Tetrahedra...
  */

  // Split voxel in 2 along diagonal, 3 tets on either side
  static const int tet6[6][4] = {
    { 1, 6, 2, 3 },
    { 1, 6, 7, 5 },
    { 1, 6, 3, 7 },
    { 1, 6, 0, 2 },
    { 1, 6, 5, 4 },
    { 1, 6, 4, 0 },
  };

  static const int tet5[5][4] = {
    { 0, 1, 4, 2 },
    { 1, 4, 7, 5 },
    { 1, 4, 2, 7 },
    { 1, 2, 3, 7 },
    { 2, 7, 4, 6 },
  };
  static const int tet5flip[5][4] = {
    { 3, 1, 0, 5 },
    { 0, 3, 6, 2 },
    { 3, 5, 6, 7 },
    { 0, 6, 5, 4 },
    { 0, 3, 5, 6 },
  };

  // 12 tet to confirm to tet5
  static const int tet12_conform[12][4] = {
    /* Left side */
    { 8, 2, 4, 0 },
    { 8, 4, 2, 6 },
    /* Back side */
    { 8, 7, 4, 6 },
    { 8, 4, 7, 5 },
    /* Bottom side */
    { 8, 7, 2, 3 },
    { 8, 2, 7, 6 },
    /* Right side */
    { 8, 7, 1, 5 },
    { 8, 1, 7, 3 },
    /* Front side */
    { 8, 1, 2, 0 },
    { 8, 2, 1, 3 },
    /* Top side */
    { 8, 4, 1, 0 },
    { 8, 1, 4, 5 },
  };

  // 12 tet to confirm to tet5flip
  static int tet12_conform_flip[12][4] = {
    /* Left side */
    { 8, 0, 6, 4 },
    { 8, 6, 0, 2 },
    /* Back side */
    { 8, 5, 6, 7 },
    { 8, 6, 5, 4 },
    /* Bottom side */
    { 8, 3, 6, 2 },
    { 8, 6, 3, 7 },
    /* Right side */
    { 8, 3, 5, 7 },
    { 8, 5, 3, 1 },
    /* Front side */
    { 8, 3, 0, 1 },
    { 8, 0, 3, 2 },
    /* Top side */
    { 8, 5, 0, 4 },
    { 8, 0, 5, 1 },
  };

  // 12 tet chosen to have the least number of edges per node
  static int tet12[12][4] = {
    /* Left side */
    { 8, 2, 4, 0 },
    { 8, 4, 2, 6 },
    /* Back side */
    { 8, 7, 4, 6 },
    { 8, 4, 7, 5 },
    /* Right side */
    { 8, 3, 5, 7 },
    { 8, 5, 3, 1 },
    /* Front side */
    { 8, 3, 0, 1 },
    { 8, 0, 3, 2 },
    /* Top side */
    { 8, 5, 0, 4 },
    { 8, 0, 5, 1 },
    /* Bottom side */
    { 8, 7, 2, 3 },
    { 8, 2, 7, 6 },
  };

  int i, j;
  // Get the point Ids
  int numTet = 0; // =0 removes warning messages
  svtkIdType TetPts[4];

  switch (DivisionType)
  {
    case (SVTK_TETRAHEDRALIZE_6):
      numTet = 6;
      for (i = 0; i < numTet; i++)
      {
        for (j = 0; j < 4; j++)
        {
          TetPts[j] = VoxelCorners->GetId(tet6[i][j]);
        }
        TetList->InsertNextCell((svtkIdType)4, TetPts);
      }
      break;
    case (SVTK_TETRAHEDRALIZE_5):
      numTet = 5;
      for (i = 0; i < numTet; i++)
      {
        for (j = 0; j < 4; j++)
        {
          TetPts[j] = VoxelCorners->GetId(tet5[i][j]);
        }
        TetList->InsertNextCell((svtkIdType)4, TetPts);
      }
      break;
    case (SVTK_TETRAHEDRALIZE_5_FLIP):
      numTet = 5;
      for (i = 0; i < numTet; i++)
      {
        for (j = 0; j < 4; j++)
        {
          TetPts[j] = VoxelCorners->GetId(tet5flip[i][j]);
        }
        TetList->InsertNextCell((svtkIdType)4, TetPts);
      }
      break;
    case (SVTK_TETRAHEDRALIZE_12):
      numTet = 12;
      TetrahedralizeAddCenterPoint(VoxelCorners, NodeList);
      for (i = 0; i < numTet; i++)
      {
        for (j = 0; j < 4; j++)
        {
          TetPts[j] = VoxelCorners->GetId(tet12[i][j]);
        }
        TetList->InsertNextCell((svtkIdType)4, TetPts);
      }
      break;
    case (SVTK_TETRAHEDRALIZE_12_CONFORM):
      numTet = 12;
      TetrahedralizeAddCenterPoint(VoxelCorners, NodeList);
      for (i = 0; i < numTet; i++)
      {
        for (j = 0; j < 4; j++)
        {
          TetPts[j] = VoxelCorners->GetId(tet12_conform[i][j]);
        }
        TetList->InsertNextCell((svtkIdType)4, TetPts);
      }
      break;
    case (SVTK_TETRAHEDRALIZE_12_CONFORM_FLIP):
      numTet = 12;
      TetrahedralizeAddCenterPoint(VoxelCorners, NodeList);
      for (i = 0; i < numTet; i++)
      {
        for (j = 0; j < 4; j++)
        {
          TetPts[j] = VoxelCorners->GetId(tet12_conform_flip[i][j]);
        }
        TetList->InsertNextCell((svtkIdType)4, TetPts);
      }
      break;
  }
  return numTet;
}

//----------------------------------------------------------------------------

int svtkRectilinearGridToTetrahedra::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkRectilinearGrid* RectGrid =
    svtkRectilinearGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Create internal version of VoxelSubdivisionType
  // VoxelSubdivisionType indicates how to subdivide each cell
  svtkSignedCharArray* VoxelSubdivisionType;
  VoxelSubdivisionType = svtkSignedCharArray::New();

  // If we have a mixture of 5 and 12 Tet, we need to get the information from
  // the scalars of the Input. Note that we will modify the array internally
  // so we need to copy it.
  if (this->TetraPerCell == SVTK_VOXEL_TO_5_AND_12_TET)
  {
    svtkDataArray* TempVoxelSubdivisionType = RectGrid->GetCellData()->GetScalars();
    if (TempVoxelSubdivisionType == nullptr)
    {
      svtkErrorMacro(<< "Scalars to input Should be set!");
      return 1;
    }
    VoxelSubdivisionType->SetNumberOfValues(RectGrid->GetNumberOfCells());
    VoxelSubdivisionType->svtkSignedCharArray::DeepCopy(TempVoxelSubdivisionType);
  }
  else
  { // Otherwise, just create the GridDivisionTypes
    VoxelSubdivisionType->SetNumberOfValues(RectGrid->GetNumberOfCells());
  }

  svtkDebugMacro(<< "Number of points: " << RectGrid->GetNumberOfPoints());
  svtkDebugMacro(<< "Number of voxels in input: " << RectGrid->GetNumberOfCells());

  // Determine how each Cell should be subdivided
  DetermineGridDivisionTypes(RectGrid, VoxelSubdivisionType, this->TetraPerCell);

  // Subdivide each cell to a tetrahedron, forming the TetMesh
  GridToTetMesh(RectGrid, VoxelSubdivisionType, this->TetraPerCell, this->RememberVoxelId, output);

  svtkDebugMacro(<< "Number of output points: " << output->GetNumberOfPoints());
  svtkDebugMacro(<< "Number of output tetrahedra: " << output->GetNumberOfCells());

  // Clean Up
  VoxelSubdivisionType->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkRectilinearGridToTetrahedra ::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------

void svtkRectilinearGridToTetrahedra::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Mesh Type: " << this->TetraPerCell << "\n";
  os << indent << "RememberVoxel Id: " << this->RememberVoxelId << "\n";
}
