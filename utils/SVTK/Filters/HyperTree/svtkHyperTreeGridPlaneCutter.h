/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridPlaneCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridPlaneCutter
 * @brief   cut an hyper tree grid volume with
 * a plane and generate a polygonal cut surface.
 *
 *
 * svtkHyperTreeGridPlaneCutter is a filter that takes as input an hyper tree
 * grid and a single plane and generates the polygonal data intersection surface.
 * This cut is computed at the leaf cells of the hyper tree.
 * It is left as an option to decide whether the cut should be computed over
 * the original AMR mesh or over its dual; in the latter case, perfect
 * connectivity (i.e., mesh conformity in the FE sense) is achieved at the
 * cost of interpolation to the dual of the input AMR mesh, and therefore
 * of missing intersection plane pieces near the primal boundary.
 *
 * @sa
 * svtkHyperTreeGrid svtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay on a idea of Guenole Harel and Jacques-Bernard Lekien,
 * 2016 This class was modified by Rogeli Grima Torres, 2016 This class was modified by
 * Jacques-Bernard Lekien, 2018 This work was supported by Commissariat a l'Energie Atomique CEA,
 * DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridPlaneCutter_h
#define svtkHyperTreeGridPlaneCutter_h

#include "svtkFiltersHyperTreeModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkCellArray;
class svtkCutter;
class svtkIdList;
class svtkPoints;
class svtkHyperTreeGridNonOrientedGeometryCursor;
class svtkHyperTreeGridNonOrientedMooreSuperCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridPlaneCutter : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridPlaneCutter* New();
  svtkTypeMacro(svtkHyperTreeGridPlaneCutter, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  //@{
  /**
   * Specify the plane with its [a,b,c,d] Cartesian coefficients:
   * a*x + b*y + c*z = d
   */
  void SetPlane(double a, double b, double c, double d);
  svtkGetVector4Macro(Plane, double);
  //@}

  //@{
  /**
   * Returns 0 if plane's normal is aligned with X axis, 1 if it is aligned with Y axis, 2 if it
   * is aligned with Z axis. Returns -1 if not aligned with any principal axis.
   */
  svtkGetMacro(AxisAlignment, int);
  //@}

  //@{
  /**
   * Returns true if plane's normal is aligned with the corresponding axis, false elsewise.
   */
  bool IsPlaneOrthogonalToXAxis() { return this->AxisAlignment == 0; }
  bool IsPlaneOrthogonalToYAxis() { return this->AxisAlignment == 1; }
  bool IsPlaneOrthogonalToZAxis() { return this->AxisAlignment == 2; }
  //}@

  //@{
  /**
   * Set/Get whether output mesh should be computed on dual grid
   */
  svtkSetMacro(Dual, int);
  svtkGetMacro(Dual, int);
  svtkBooleanMacro(Dual, int);
  //@}

protected:
  svtkHyperTreeGridPlaneCutter();
  ~svtkHyperTreeGridPlaneCutter() override;

  /**
   * Resets every attributes to a minimal state needed for the algorithm to execute
   */
  virtual void Reset();

  /**
   * For this algorithm the output is a svtkPolyData instance
   */
  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * Top-level routine to generate plane cut
   */
  int ProcessTrees(svtkHyperTreeGrid*, svtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves, cutting primal cells
   */
  void RecursivelyProcessTreePrimal(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Recursively decide whether cell is intersected by plane
   */
  bool RecursivelyPreProcessTree(svtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Recursively descend into tree down to leaves, cutting dual cells
   */
  void RecursivelyProcessTreeDual(svtkHyperTreeGridNonOrientedMooreSuperCursor*);

  /**
   * Check if a cursor is intersected by a plane
   */
  bool CheckIntersection(double[8][3], double[8]);

  // Check if a cursor is intersected by a plane.
  // Don't return function evaluations
  bool CheckIntersection(double[8][3]);

  /**
   * Compute the intersection between an edge and a plane
   */
  void PlaneCut(int, int, double[8][3], int&, double[][3]);

  /**
   * Reorder cut points following the perimeter of the cut.
   */
  void ReorderCutPoints(int, double[][3]);

  /**
   * Storage for the plane cutter parameters
   */
  double Plane[4];

  /**
   * Decide whether output mesh should be a computed on dual grid
   */
  int Dual;

  /**
   * Storage for pre-selected cells to be processed in dual mode
   */
  svtkBitArray* SelectedCells;

  /**
   * Storage for points of output unstructured mesh
   */
  svtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  svtkCellArray* Cells;

  /**
   * Storage for dual vertex indices
   */
  svtkIdList* Leaves;

  /**
   * Storage for dual vertices at center of primal cells
   */
  svtkPoints* Centers;

  /**
   * Cutter to be used on dual cells
   */
  svtkCutter* Cutter;

  /**
   * material Mask
   */
  svtkBitArray* InMask;

  /**
   * Flag computed at plane creation to know wether it is aligned with x, y or z axis
   */
  int AxisAlignment;

private:
  svtkHyperTreeGridPlaneCutter(const svtkHyperTreeGridPlaneCutter&) = delete;
  void operator=(const svtkHyperTreeGridPlaneCutter&) = delete;
};

#endif /* svtkHyperTreeGridPlaneCutter_h */
