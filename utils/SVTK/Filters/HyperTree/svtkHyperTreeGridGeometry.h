/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridGeometry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridGeometry
 * @brief   Hyper tree grid outer surface
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien and Guenole Harel, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridGeometry_h
#define svtkHyperTreeGridGeometry_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkBitArray;
class svtkCellArray;
class svtkHyperTreeGrid;
class svtkPoints;
class svtkIncrementalPointLocator;
class svtkDoubleArray;
class svtkHyperTreeGridNonOrientedGeometryCursor;
class svtkHyperTreeGridNonOrientedVonNeumannSuperCursor;
class svtkIdList;
class svtkIdTypeArray;
class svtkUnsignedCharArray;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridGeometry : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridGeometry* New();
  svtkTypeMacro(svtkHyperTreeGridGeometry, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  //@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  svtkSetMacro(Merging, bool);
  svtkGetMacro(Merging, bool);
  //@}

protected:
  svtkHyperTreeGridGeometry();
  ~svtkHyperTreeGridGeometry() override;

  /**
   * For this algorithm the output is a svtkPolyData instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Main routine to generate external boundary
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTreeNot3D(svtkHyperTreeGridNonOrientedGeometryCursor*);
  void RecursivelyProcessTree3D(svtkHyperTreeGridNonOrientedVonNeumannSuperCursor*, unsigned char);

  /**
   * Process 1D leaves and issue corresponding edges (lines)
   */
  void ProcessLeaf1D(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 2D leaves and issue corresponding faces (quads)
   */
  void ProcessLeaf2D(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 3D leaves and issue corresponding cells (voxels)
   */
  void ProcessLeaf3D(svtkHyperTreeGridNonOrientedVonNeumannSuperCursor*);

  /**
   * Helper method to generate a face based on its normal and offset from cursor origin
   */
  void AddFace(svtkIdType useId, const double* origin, const double* size, unsigned int offset,
    unsigned int orientation, unsigned char hideEdge);

  void AddFace2(svtkIdType inId, svtkIdType useId, const double* origin, const double* size,
    unsigned int offset, unsigned int orientation, bool create = true);

  /**
   * material Mask
   */
  svtkBitArray* Mask;

  /**
   * Pure Material Mask
   */
  svtkBitArray* PureMask;

  /**
   * Dimension of input grid
   */
  unsigned int Dimension;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation;

  /**
   * Branch Factor
   */
  int BranchFactor;

  /**
   * Storage for points of output unstructured mesh
   */
  svtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  svtkCellArray* Cells;

  /**
   *JB Un locator est utilise afin de produire un maillage avec moins
   *JB de points. Le gain en 3D est de l'ordre d'un facteur 4 !
   */
  bool Merging;
  svtkIncrementalPointLocator* Locator;

  // JB A RECUPERER DANS LE .H SVTK9
  bool HasInterface;
  svtkDoubleArray* Normals;
  svtkDoubleArray* Intercepts;

  svtkIdList* FaceIDs;
  svtkPoints* FacePoints;

  svtkIdType EdgesA[12];
  svtkIdType EdgesB[12];

  svtkIdTypeArray* FacesA;
  svtkIdTypeArray* FacesB;

  svtkDoubleArray* FaceScalarsA;
  svtkDoubleArray* FaceScalarsB;

  /**
   * Array used to hide edges
   * left by masked cells.
   */
  svtkUnsignedCharArray* EdgeFlags;

private:
  svtkHyperTreeGridGeometry(const svtkHyperTreeGridGeometry&) = delete;
  void operator=(const svtkHyperTreeGridGeometry&) = delete;
};

#endif /* svtkHyperTreeGridGeometry_h */
