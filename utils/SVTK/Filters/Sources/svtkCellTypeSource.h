/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellTypeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellTypeSource
 * @brief   Create cells of a given type
 *
 * svtkCellTypeSource is a source object that creates cells of the given
 * input type. BlocksDimensions specifies the number of cell "blocks" in each
 * direction. A cell block may be divided into multiple cells based on
 * the chosen cell type (e.g. 6 pyramid cells make up a single cell block).
 * If a 1D cell is selected then only the first dimension is
 * used to specify how many cells are generated. If a 2D cell is
 * selected then only the first and second dimensions are used to
 * determine how many cells are created. The source respects pieces.
 */

#ifndef svtkCellTypeSource_h
#define svtkCellTypeSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkMergePoints;

class SVTKFILTERSSOURCES_EXPORT svtkCellTypeSource : public svtkUnstructuredGridAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type and printing instance values.
   */
  static svtkCellTypeSource* New();
  svtkTypeMacro(svtkCellTypeSource, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the type of cells to be generated.
   */
  void SetCellType(int cellType);
  svtkGetMacro(CellType, int);
  //@}

  //@{
  /**
   * Set/Get the order of Lagrange interpolation to be used.
   *
   * This is only used when the cell type is a Lagrange element.
   * The default is cubic (order 3).
   * Lagrange elements are the same order along all axes
   * (i.e., you cannot specify a different interpolation order
   * for the i, j, and k axes of a hexahedron).
   */
  svtkSetMacro(CellOrder, int);
  svtkGetMacro(CellOrder, int);
  //@}

  //@{
  /**
   * Set/Get whether quadratic cells with simplicial shapes should be "completed".
   *
   * By default, quadratic Lagrange cells with simplicial shapes
   * do not completely span the basis of all polynomial of the maximal
   * degree. This can be corrected by adding mid-face and body-centered
   * nodes. Setting this option to true will generate cells with these
   * additional nodes.
   *
   * This is only used when
   * (1) the cell type is a Lagrange triangle, tetrahedron, or wedge;
   * and (2) \a CellOrder is set to 2 (quadratic elements).
   * The default is false.
   *
   * When true, generated
   * (1) triangles will have 7 nodes instead of 6;
   * (2) tetrahedra will have 15 nodes instead of 10;
   * (3) wedges will have 21 nodes instead of 18.
   */
  svtkSetMacro(CompleteQuadraticSimplicialElements, bool);
  svtkGetMacro(CompleteQuadraticSimplicialElements, bool);
  svtkBooleanMacro(CompleteQuadraticSimplicialElements, bool);
  //@}

  //@{
  /**
   * Set/Get the polynomial order of the "Polynomial" point field.
   * The default is 1.
   */
  svtkSetClampMacro(PolynomialFieldOrder, int, 0, SVTK_INT_MAX);
  svtkGetMacro(PolynomialFieldOrder, int);
  //@}

  //@{
  /**
   * Get the dimension of the cell blocks to be generated
   */
  int GetCellDimension();
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION (0) - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION (1) - Output double-precision floating point.
   */
  svtkSetClampMacro(OutputPrecision, int, 0, 1);
  svtkGetMacro(OutputPrecision, int);
  //@}

  //@{
  /**
   * Set the number of cells in each direction. If a 1D cell type is
   * selected then only the first dimension is used and if a 2D cell
   * type is selected then the first and second dimensions are used.
   * Default is (1, 1, 1), which results in a single block of cells.
   */
  void SetBlocksDimensions(int*);
  void SetBlocksDimensions(int, int, int);
  svtkGetVector3Macro(BlocksDimensions, int);
  //@}

protected:
  svtkCellTypeSource();
  ~svtkCellTypeSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void GenerateTriangles(svtkUnstructuredGrid*, int extent[6]);
  void GenerateQuads(svtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticTriangles(svtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticQuads(svtkUnstructuredGrid*, int extent[6]);
  void GenerateTetras(svtkUnstructuredGrid*, int extent[6]);
  void GenerateHexahedron(svtkUnstructuredGrid*, int extent[6]);
  void GenerateWedges(svtkUnstructuredGrid*, int extent[6]);
  void GeneratePyramids(svtkUnstructuredGrid*, int extent[6]);
  void GeneratePentagonalPrism(svtkUnstructuredGrid*, int extent[6]);
  void GenerateHexagonalPrism(svtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticTetras(svtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticHexahedron(svtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticWedges(svtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticPyramids(svtkUnstructuredGrid*, int extent[6]);

  void GenerateLagrangeCurves(svtkUnstructuredGrid*, int extent[6]);
  void GenerateLagrangeTris(svtkUnstructuredGrid*, int extent[6]);
  void GenerateLagrangeQuads(svtkUnstructuredGrid*, int extent[6]);
  void GenerateLagrangeTets(svtkUnstructuredGrid*, int extent[6]);
  void GenerateLagrangeHexes(svtkUnstructuredGrid*, int extent[6]);
  void GenerateLagrangeWedges(svtkUnstructuredGrid*, int extent[6]);

  void GenerateBezierCurves(svtkUnstructuredGrid*, int extent[6]);
  void GenerateBezierTris(svtkUnstructuredGrid*, int extent[6]);
  void GenerateBezierQuads(svtkUnstructuredGrid*, int extent[6]);
  void GenerateBezierTets(svtkUnstructuredGrid*, int extent[6]);
  void GenerateBezierHexes(svtkUnstructuredGrid*, int extent[6]);
  void GenerateBezierWedges(svtkUnstructuredGrid*, int extent[6]);

  virtual void ComputeFields(svtkUnstructuredGrid*);
  double GetValueOfOrder(int order, double coords[3]);

  int BlocksDimensions[3];
  int CellType;
  int CellOrder;
  bool CompleteQuadraticSimplicialElements;
  int OutputPrecision;
  int PolynomialFieldOrder;
  svtkMergePoints* Locator; // Only valid during RequestData.

private:
  svtkCellTypeSource(const svtkCellTypeSource&) = delete;
  void operator=(const svtkCellTypeSource&) = delete;
};

#endif
