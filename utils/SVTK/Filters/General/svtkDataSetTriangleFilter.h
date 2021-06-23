/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetTriangleFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataSetTriangleFilter
 * @brief   triangulate any type of dataset
 *
 * svtkDataSetTriangleFilter generates n-dimensional simplices from any input
 * dataset. That is, 3D cells are converted to tetrahedral meshes, 2D cells
 * to triangles, and so on. The triangulation is guaranteed to be compatible.
 *
 * This filter uses simple 1D and 2D triangulation techniques for cells
 * that are of topological dimension 2 or less. For 3D cells--due to the
 * issue of face compatibility across quadrilateral faces (which way to
 * orient the diagonal?)--a fancier ordered Delaunay triangulation is used.
 * This approach produces templates on the fly for triangulating the
 * cells. The templates are then used to do the actual triangulation.
 *
 * @sa
 * svtkOrderedTriangulator svtkTriangleFilter
 */

#ifndef svtkDataSetTriangleFilter_h
#define svtkDataSetTriangleFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkOrderedTriangulator;

class SVTKFILTERSGENERAL_EXPORT svtkDataSetTriangleFilter : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkDataSetTriangleFilter* New();
  svtkTypeMacro(svtkDataSetTriangleFilter, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * When On this filter will cull all 1D and 2D cells from the output.
   * The default is Off.
   */
  svtkSetMacro(TetrahedraOnly, svtkTypeBool);
  svtkGetMacro(TetrahedraOnly, svtkTypeBool);
  svtkBooleanMacro(TetrahedraOnly, svtkTypeBool);
  //@}

protected:
  svtkDataSetTriangleFilter();
  ~svtkDataSetTriangleFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Used to triangulate 3D cells
  svtkOrderedTriangulator* Triangulator;

  // Different execute methods depending on whether input is structured or not
  void StructuredExecute(svtkDataSet*, svtkUnstructuredGrid*);
  void UnstructuredExecute(svtkDataSet*, svtkUnstructuredGrid*);

  svtkTypeBool TetrahedraOnly;

private:
  svtkDataSetTriangleFilter(const svtkDataSetTriangleFilter&) = delete;
  void operator=(const svtkDataSetTriangleFilter&) = delete;
};

#endif
