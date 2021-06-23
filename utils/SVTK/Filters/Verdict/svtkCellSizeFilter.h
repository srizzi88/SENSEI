/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellSizeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellSizeFilter
 * @brief   Computes cell sizes.
 *
 * Computes the cell sizes for all types of cells in SVTK. For triangles,
 * quads, tets and hexes the static methods in svtkMeshQuality are used.
 * This is done through Verdict for higher accuracy.
 * Other cell types are individually done analytically where possible
 * and breaking into triangles or tets when not possible. When cells are
 * broken into triangles or tets the accuracy may be diminished. By default
 * all sizes are computed but vertex count, length, area and volumetric cells
 * can each be optionally ignored. Individual arrays are used for each
 * requested size (e.g. if length and volume are requested there will be
 * two arrays outputted from this filter). The 4 arrays can be individually
 * named with defaults of VertexCount, Length, Area and Volme. For dimensions
 * of cells that do not have their size computed, a value of 0 will be given.
 * For cells that should have their size computed but can't, the filter will return -1.
 * The ComputeSum option will sum the cell sizes (excluding ghost cells)
 * and put the value into svtkFieldData arrays named with the corresponding cell
 * data array name. For composite datasets the total sum over all blocks will
 * also be added to the top-level block's field data for the summation.
 */

#ifndef svtkCellSizeFilter_h
#define svtkCellSizeFilter_h

#include "svtkFiltersVerdictModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class svtkDataSet;
class svtkDoubleArray;
class svtkIdList;
class svtkImageData;
class svtkPointSet;

class SVTKFILTERSVERDICT_EXPORT svtkCellSizeFilter : public svtkPassInputTypeAlgorithm
{
public:
  svtkTypeMacro(svtkCellSizeFilter, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkCellSizeFilter* New();

  //@{
  /**
   * Specify whether or not to compute sizes for vertex and polyvertex
   * cells. The computed value is the number of points in the cell.
   * This option is enabled by default.
   */
  svtkSetMacro(ComputeVertexCount, bool);
  svtkGetMacro(ComputeVertexCount, bool);
  svtkBooleanMacro(ComputeVertexCount, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 1D cells
   * cells. The computed value is the length of the cell.
   * This option is enabled by default.
   */
  svtkSetMacro(ComputeLength, bool);
  svtkGetMacro(ComputeLength, bool);
  svtkBooleanMacro(ComputeLength, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 2D cells
   * cells. The computed value is the area of the cell.
   * This option is enabled by default.
   */
  svtkSetMacro(ComputeArea, bool);
  svtkGetMacro(ComputeArea, bool);
  svtkBooleanMacro(ComputeArea, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 3D cells
   * cells. The computed value is the volume of the cell.
   * This option is enabled by default.
   */
  svtkSetMacro(ComputeVolume, bool);
  svtkGetMacro(ComputeVolume, bool);
  svtkBooleanMacro(ComputeVolume, bool);
  //@}

  //@{
  /**
   * Specify whether to sum the computed sizes and put the result in
   * a field data array. This option is disabled by default.
   */
  svtkSetMacro(ComputeSum, bool);
  svtkGetMacro(ComputeSum, bool);
  svtkBooleanMacro(ComputeSum, bool);
  //@}

  //@{
  /**
   * Set/Get the name of the computed arrays. Default names are VertexCount,
   * Length, Area and Volume.
   */
  svtkSetStringMacro(VertexCountArrayName);
  svtkGetStringMacro(VertexCountArrayName);
  svtkSetStringMacro(LengthArrayName);
  svtkGetStringMacro(LengthArrayName);
  svtkSetStringMacro(AreaArrayName);
  svtkGetStringMacro(AreaArrayName);
  svtkSetStringMacro(VolumeArrayName);
  svtkGetStringMacro(VolumeArrayName);
  //@}

protected:
  svtkCellSizeFilter();
  ~svtkCellSizeFilter() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  bool ComputeDataSet(svtkDataSet* input, svtkDataSet* output, double sum[4]);

  void IntegrateImageData(svtkImageData* input, svtkImageData* output, double sum[4]);
  void ExecuteBlock(svtkDataSet* input, svtkDataSet* output, double sum[4]);

  //@{
  /**
   * Specify whether to sum the computed sizes and put the result in
   * a field data array. This option is disabled by default.
   */
  double IntegratePolyLine(svtkDataSet* input, svtkIdList* cellPtIds);
  double IntegratePolygon(svtkPointSet* input, svtkIdList* cellPtIds);
  double IntegrateTriangleStrip(svtkPointSet* input, svtkIdList* cellPtIds);
  double IntegratePixel(svtkDataSet* input, svtkIdList* cellPtIds);
  double IntegrateVoxel(svtkDataSet* input, svtkIdList* cellPtIds);
  double IntegrateGeneral1DCell(svtkDataSet* input, svtkIdList* cellPtIds);
  double IntegrateGeneral2DCell(svtkPointSet* input, svtkIdList* cellPtIds);
  double IntegrateGeneral3DCell(svtkPointSet* input, svtkIdList* cellPtIds);
  //@}

  //@{
  /**
   * Method to add the computed sum to the field data of the data object.
   */
  void AddSumFieldData(svtkDataObject*, double sum[4]);
  //@}

  //@{
  /**
   * Method to compute the global sum information. For serial operation this is a no-op.
   */
  virtual void ComputeGlobalSum(double sum[4]) { (void)sum; }
  //@}

private:
  svtkCellSizeFilter(const svtkCellSizeFilter&) = delete;
  void operator=(const svtkCellSizeFilter&) = delete;

  bool ComputeVertexCount;
  bool ComputeLength;
  bool ComputeArea;
  bool ComputeVolume;
  bool ComputeSum;

  char* VertexCountArrayName;
  char* LengthArrayName;
  char* AreaArrayName;
  char* VolumeArrayName;
};

#endif
