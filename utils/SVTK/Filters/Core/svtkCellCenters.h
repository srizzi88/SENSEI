/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellCenters.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellCenters
 * @brief   generate points at center of cells
 *
 * svtkCellCenters is a filter that takes as input any dataset and
 * generates on output points at the center of the cells in the dataset.
 * These points can be used for placing glyphs (svtkGlyph3D) or labeling
 * (svtkLabeledDataMapper). (The center is the parametric center of the
 * cell, not necessarily the geometric or bounding box center.) The cell
 * attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @note
 * Empty cells will be ignored but will require a one by one cell to
 * point data copy that will make the processing slower.
 *
 * @sa
 * svtkGlyph3D svtkLabeledDataMapper
 */

#ifndef svtkCellCenters_h
#define svtkCellCenters_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkDoubleArray;

class SVTKFILTERSCORE_EXPORT svtkCellCenters : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkCellCenters, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with vertex cell generation turned off.
   */
  static svtkCellCenters* New();

  //@{
  /**
   * Enable/disable the generation of vertex cells. The default
   * is Off.
   */
  svtkSetMacro(VertexCells, bool);
  svtkGetMacro(VertexCells, bool);
  svtkBooleanMacro(VertexCells, bool);
  //@}

  //@{
  /**
   * Enable/disable whether input cell data arrays should be passed through (or
   * copied) as output point data arrays. Default is `true` i.e. the arrays will
   * be propagated.
   */
  svtkSetMacro(CopyArrays, bool);
  svtkGetMacro(CopyArrays, bool);
  svtkBooleanMacro(CopyArrays, bool);
  //@}

  /**
   * Compute centers of cells from a dataset, storing them in the centers array.
   */
  static void ComputeCellCenters(svtkDataSet* dataset, svtkDoubleArray* centers);

protected:
  svtkCellCenters() = default;
  ~svtkCellCenters() override = default;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  bool VertexCells = false;
  bool CopyArrays = true;

private:
  svtkCellCenters(const svtkCellCenters&) = delete;
  void operator=(const svtkCellCenters&) = delete;
};

#endif
