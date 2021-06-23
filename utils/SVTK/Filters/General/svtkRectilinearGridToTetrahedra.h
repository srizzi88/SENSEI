/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridToTetrahedra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearGridToTetrahedra
 * @brief   create a Tetrahedral mesh from a RectilinearGrid
 *
 * svtkRectilinearGridToTetrahedra forms a mesh of Tetrahedra from a
 * svtkRectilinearGrid.  The tetrahedra can be 5 per cell, 6 per cell,
 * or a mixture of 5 or 12 per cell. The resulting mesh is consistent,
 * meaning that there are no edge crossings and that each tetrahedron
 * face is shared by two tetrahedra, except those tetrahedra on the
 * boundary. All tetrahedra are right handed.
 *
 * Note that 12 tetrahedra per cell means adding a point in the
 * center of the cell.
 *
 * In order to subdivide some cells into 5 and some cells into 12 tetrahedra:
 * SetTetraPerCellTo5And12();
 * Set the Scalars of the Input RectilinearGrid to be 5 or 12
 * depending on what you want per cell of the RectilinearGrid.
 *
 * If you set RememberVoxelId, the scalars of the tetrahedron
 * will be set to the Id of the Cell in the RectilinearGrid from which
 * the tetrahedron came.
 *
 * @par Thanks:
 *    This class was developed by Samson J. Timoner of the
 *    MIT Artificial Intelligence Laboratory
 *
 * @sa
 *    svtkDelaunay3D
 */

#ifndef svtkRectilinearGridToTetrahedra_h
#define svtkRectilinearGridToTetrahedra_h

// ways to create the mesh from voxels
#define SVTK_VOXEL_TO_12_TET 12
#define SVTK_VOXEL_TO_5_TET 5
#define SVTK_VOXEL_TO_6_TET 6
#define SVTK_VOXEL_TO_5_AND_12_TET -1

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"
class svtkRectilinearGrid;
class svtkSignedCharArray;
class svtkIdList;
class svtkCellArray;
class svtkPoints;

class SVTKFILTERSGENERAL_EXPORT svtkRectilinearGridToTetrahedra : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkRectilinearGridToTetrahedra, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Form 5 Tetrahedra per cube. Do not RememberVoxelId.
   */
  static svtkRectilinearGridToTetrahedra* New();

  //@{
  /**
   * Set the method to divide each cell (voxel) in the RectilinearGrid
   * into tetrahedra.
   */
  void SetTetraPerCellTo5() { SetTetraPerCell(SVTK_VOXEL_TO_5_TET); }
  void SetTetraPerCellTo6() { SetTetraPerCell(SVTK_VOXEL_TO_6_TET); }
  void SetTetraPerCellTo12() { SetTetraPerCell(SVTK_VOXEL_TO_12_TET); }
  void SetTetraPerCellTo5And12() { SetTetraPerCell(SVTK_VOXEL_TO_5_AND_12_TET); }
  svtkSetMacro(TetraPerCell, int);
  svtkGetMacro(TetraPerCell, int);
  //@}

  //@{
  /**
   * Should the tetrahedra have scalar data
   * indicating which Voxel they came from in the svtkRectilinearGrid?
   */
  svtkSetMacro(RememberVoxelId, svtkTypeBool);
  svtkGetMacro(RememberVoxelId, svtkTypeBool);
  svtkBooleanMacro(RememberVoxelId, svtkTypeBool);
  //@}

  /**
   * This function for convenience for creating a Rectilinear Grid
   * If Spacing does not fit evenly into extent, the last cell will
   * have a different width (or height or depth).
   * If Extent[i]/Spacing[i] is within tol of an integer, then
   * assume the programmer meant an integer for direction i.
   */
  void SetInput(const double Extent[3], const double Spacing[3], const double tol = 0.001);
  /**
   * This version of the function for the wrappers
   */
  void SetInput(const double ExtentX, const double ExtentY, const double ExtentZ,
    const double SpacingX, const double SpacingY, const double SpacingZ, const double tol = 0.001);

protected:
  svtkRectilinearGridToTetrahedra();
  ~svtkRectilinearGridToTetrahedra() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool RememberVoxelId;
  int TetraPerCell;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkRectilinearGridToTetrahedra(const svtkRectilinearGridToTetrahedra&) = delete;

  void operator=(const svtkRectilinearGridToTetrahedra&) = delete;

  /**
   * Determine how to Divide each cell (voxel) in the RectilinearGrid
   * Overwrites VoxelSubdivisionType with flipping information for forming the mesh
   */
  static void DetermineGridDivisionTypes(svtkRectilinearGrid* RectGrid,
    svtkSignedCharArray* VoxelSubdivisionType, const int& TetraPerCell);

  /**
   * Take the grid and make it into a tetrahedral mesh.
   */
  static void GridToTetMesh(svtkRectilinearGrid* RectGrid, svtkSignedCharArray* VoxelSubdivisionType,
    const int& TetraPerCell, const int& RememberVoxelId, svtkUnstructuredGrid* TetMesh);

  /**
   * Take a voxel and make tetrahedra out of it.
   * Add the resulting tetrahedra to TetraMesh. Also, should new
   * points need to be created, add them to NodeList.
   * Note that svtkIdList may be changed during this process (a point added).
   */
  static int TetrahedralizeVoxel(
    svtkIdList* VoxelCorners, const int& DivisionType, svtkPoints* NodeList, svtkCellArray* TetList);

  /**
   * Helper Function for TetrahedraizeVoxel
   * Adds a center point in the middle of the voxel
   */
  static inline void TetrahedralizeAddCenterPoint(svtkIdList* VoxelCorners, svtkPoints* NodeList);
};

#endif /* svtkRectilinearGridToTetrahedra_h */
