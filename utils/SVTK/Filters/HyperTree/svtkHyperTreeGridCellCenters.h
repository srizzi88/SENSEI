/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridCellCenters.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridCellCenters
 * @brief   generate points at center of hyper
 * tree grid leaf cell centers.
 *
 *
 * svtkHyperTreeGridCellCenters is a filter that takes as input an hyper
 * tree grid and generates on output points at the center of the leaf
 * cells in the hyper tree grid.
 * These points can be used for placing glyphs (svtkGlyph3D) or labeling
 * (svtkLabeledDataMapper).
 * The cell attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @sa
 * svtkCellCenters svtkHyperTreeGrid svtkGlyph3D
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridCellCenters_h
#define svtkHyperTreeGridCellCenters_h

#include "svtkCellCenters.h"
#include "svtkFiltersHyperTreeModule.h" // For export macro

class svtkBitArray;
class svtkDataSetAttributes;
class svtkHyperTreeGrid;
class svtkPolyData;
class svtkHyperTreeGridNonOrientedGeometryCursor;

class SVTKFILTERSHYPERTREE_EXPORT svtkHyperTreeGridCellCenters : public svtkCellCenters
{
public:
  static svtkHyperTreeGridCellCenters* New();
  svtkTypeMacro(svtkHyperTreeGridCellCenters, svtkCellCenters);
  void PrintSelf(ostream&, svtkIndent) override;

protected:
  svtkHyperTreeGridCellCenters();
  ~svtkHyperTreeGridCellCenters() override;

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int FillInputPortInformation(int, svtkInformation*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Main routine to process individual trees in the grid
   */
  virtual void ProcessTrees();

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(svtkHyperTreeGridNonOrientedGeometryCursor*);

  svtkHyperTreeGrid* Input;
  svtkPolyData* Output;

  svtkDataSetAttributes* InData;
  svtkDataSetAttributes* OutData;

  svtkPoints* Points;

  svtkPointData* InPointData;
  svtkPointData* OutPointData;

  svtkBitArray* InMask;

private:
  svtkHyperTreeGridCellCenters(const svtkHyperTreeGridCellCenters&) = delete;
  void operator=(const svtkHyperTreeGridCellCenters&) = delete;
};

#endif // svtkHyperTreeGridCellCenters_h
