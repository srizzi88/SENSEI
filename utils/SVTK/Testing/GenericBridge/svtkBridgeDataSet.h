/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeDataSet
 * @brief   Implementation of svtkGenericDataSet.
 *
 * It is just an example that show how to implement the Generic. It is also
 * used for testing and evaluating the Generic.
 */

#ifndef svtkBridgeDataSet_h
#define svtkBridgeDataSet_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericDataSet.h"

class svtkDataSet;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeDataSet : public svtkGenericDataSet
{
public:
  static svtkBridgeDataSet* New();
  svtkTypeMacro(svtkBridgeDataSet, svtkGenericDataSet);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the dataset that will be manipulated through the adaptor interface.
   */
  svtkDataSet* GetDataSet();

  /**
   * Set the dataset that will be manipulated through the adaptor interface.
   * \pre ds_exists: ds!=0
   */
  void SetDataSet(svtkDataSet* ds);

  /**
   * Number of points composing the dataset. See NewPointIterator for more
   * details.
   * \post positive_result: result>=0
   */
  svtkIdType GetNumberOfPoints() override;

  /**
   * Number of cells that explicitly define the dataset. See NewCellIterator
   * for more details.
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   * \post positive_result: result>=0
   */
  svtkIdType GetNumberOfCells(int dim = -1) override;

  /**
   * Return -1 if the dataset is explicitly defined by cells of several
   * dimensions or if there is no cell. If the dataset is explicitly defined by
   * cells of a unique
   * dimension, return this dimension.
   * \post valid_range: (result>=-1) && (result<=3)
   */
  int GetCellDimension() override;

  /**
   * Get a list of types of cells in a dataset. The list consists of an array
   * of types (not necessarily in any order), with a single entry per type.
   * For example a dataset 5 triangles, 3 lines, and 100 hexahedra would
   * result a list of three entries, corresponding to the types SVTK_TRIANGLE,
   * SVTK_LINE, and SVTK_HEXAHEDRON.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   * \pre types_exist: types!=0
   */
  void GetCellTypes(svtkCellTypes* types) override;

  /**
   * Cells of dimension `dim' (or all dimensions if -1) that explicitly define
   * the dataset. For instance, it will return only tetrahedra if the mesh is
   * defined by tetrahedra. If the mesh is composed of two parts, one with
   * tetrahedra and another part with triangles, it will return both, but will
   * not return edges and vertices.
   * \pre valid_dim_range: (dim>=-1) && (dim<=3)
   * \post result_exists: result!=0
   */
  svtkGenericCellIterator* NewCellIterator(int dim = -1) override;

  /**
   * Boundaries of dimension `dim' (or all dimensions if -1) of the dataset.
   * If `exteriorOnly' is true, only the exterior boundaries of the dataset
   * will be returned, otherwise it will return exterior and interior
   * boundaries.
   * \pre valid_dim_range: (dim>=-1) && (dim<=2)
   * \post result_exists: result!=0
   */
  svtkGenericCellIterator* NewBoundaryIterator(int dim = -1, int exteriorOnly = 0) override;

  /**
   * Points composing the dataset; they can be on a vertex or isolated.
   * \post result_exists: result!=0
   */
  svtkGenericPointIterator* NewPointIterator() override;

  /**
   * Estimated size needed after tessellation (or special operation)
   */
  svtkIdType GetEstimatedSize() override;

  /**
   * Locate closest cell to position `x' (global coordinates) with respect to
   * a tolerance squared `tol2' and an initial guess `cell' (if valid). The
   * result consists in the `cell', the `subId' of the sub-cell (0 if primary
   * cell), the parametric coordinates `pcoord' of the position. It returns
   * whether the position is inside the cell or not. Tolerance is used to
   * control how close the point is to be considered "in" the cell.
   * THIS METHOD IS NOT THREAD SAFE.
   * \pre not_empty: GetNumberOfCells()>0
   * \pre cell_exists: cell!=0
   * \pre positive_tolerance: tol2>0
   */
  int FindCell(double x[3], svtkGenericCellIterator*& cell, double tol2, int& subId,
    double pcoords[3]) override;

  /**
   * Locate closest point `p' to position `x' (global coordinates)
   * \pre not_empty: GetNumberOfPoints()>0
   * \pre p_exists: p!=0
   */
  void FindPoint(double x[3], svtkGenericPointIterator* p) override;

  /**
   * Datasets are composite objects and need to check each part for MTime.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Compute the geometry bounding box.
   */
  void ComputeBounds() override;

protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  svtkBridgeDataSet();
  ~svtkBridgeDataSet() override;

  friend class svtkBridgeCell;
  friend class svtkBridgeCellIterator;
  friend class svtkBridgeCellIteratorOnDataSet;
  friend class svtkBridgeCellIteratorOne;
  friend class svtkBridgePointIterator;
  friend class svtkBridgePointIteratorOnCell;
  friend class svtkBridgePointIteratorOnDataSet;
  friend class svtkBridgePointIteratorOne;

  /**
   * Compute the number of cells for each dimension and the list of types of
   * cells.
   */
  void ComputeNumberOfCellsAndTypes();

  svtkDataSet* Implementation;
  svtkIdType NumberOf0DCells;
  svtkIdType NumberOf1DCells;
  svtkIdType NumberOf2DCells;
  svtkIdType NumberOf3DCells;
  svtkCellTypes* Types;
  svtkTimeStamp ComputeNumberOfCellsTime; // for number of cells and cell types

private:
  svtkBridgeDataSet(const svtkBridgeDataSet&) = delete;
  void operator=(const svtkBridgeDataSet&) = delete;
};

#endif
