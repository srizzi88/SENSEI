/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearCellExtrusionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLinearCellExtrusionFilter
 * @brief   extrude polygonal data to create 3D cells from 2D cells
 *
 * svtkLinearCellExtrusionFilter is a modeling filter. It takes polygonal data as
 * input and generates an unstructured grid data on output. The input dataset is swept
 * according to the input cell data array value along the cell normal and creates
 * new 3D primitives.
 * Triangles will become Wedges, Quads will become Hexahedrons,
 * and Polygons will become Polyhedrons.
 * This filter currently takes into account only polys and discard vertices, lines and strips.
 * Unlike the svtkLinearExtrusionFilter, this filter is designed to extrude each cell independently
 * using its normal and its scalar value.
 *
 * @sa
 * svtkLinearExtrusionFilter
 */

#ifndef svtkLinearCellExtrusionFilter_h
#define svtkLinearCellExtrusionFilter_h

#include "svtkFiltersModelingModule.h"   // For export macro
#include "svtkIncrementalPointLocator.h" // For svtkIncrementalPointLocator
#include "svtkPolyDataAlgorithm.h"
#include "svtkSmartPointer.h" // For smart pointer

class SVTKFILTERSMODELING_EXPORT svtkLinearCellExtrusionFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkLinearCellExtrusionFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkLinearCellExtrusionFilter* New();

  //@{
  /**
   * Specify the scale factor applied on the cell value during extrusion.
   * Default is 1.0
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Specify if the algorithm should use the specified vector instead of cell normals.
   * Default is false
   */
  svtkSetMacro(UseUserVector, bool);
  svtkGetMacro(UseUserVector, bool);
  svtkBooleanMacro(UseUserVector, bool);
  //@}

  //@{
  /**
   * Specify the scale factor applied on the cell value during extrusion.
   */
  svtkSetVector3Macro(UserVector, double);
  svtkGetVector3Macro(UserVector, double);
  //@}

  //@{
  /**
   * Specify if the algorithm should merge duplicate points.
   * Default is false
   */
  svtkSetMacro(MergeDuplicatePoints, bool);
  svtkGetMacro(MergeDuplicatePoints, bool);
  svtkBooleanMacro(MergeDuplicatePoints, bool);
  //@}

  //@{
  /**
   * Specify a spatial locator for merging points.
   * By default, an instance of svtkMergePoints is used.
   */
  svtkGetSmartPointerMacro(Locator, svtkIncrementalPointLocator);
  svtkSetSmartPointerMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

protected:
  svtkLinearCellExtrusionFilter();
  ~svtkLinearCellExtrusionFilter() override = default;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  double ScaleFactor = 1.0;
  double UserVector[3] = { 0.0, 0.0, 1.0 };
  bool UseUserVector = false;
  bool MergeDuplicatePoints = false;
  svtkSmartPointer<svtkIncrementalPointLocator> Locator;

private:
  svtkLinearCellExtrusionFilter(const svtkLinearCellExtrusionFilter&) = delete;
  void operator=(const svtkLinearCellExtrusionFilter&) = delete;
};

#endif
