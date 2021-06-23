/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOutlineFilter
 * @brief   create wireframe outline for an arbitrary data set or composite dataset
 *
 * svtkOutlineFilter is a filter that generates a wireframe outline of any
 * dataset or composite dataset. An outline consists of the twelve edges of
 * the dataset bounding box. An option exists for generating faces instead of
 * a wireframe outline.
 *
 * @warning
 * When an input composite dataset is provided, options exist for producing
 * different styles of outline(s). Also, if the composite dataset has
 * non-geometric members (like tables) the result is unpredictable.
 */

#ifndef svtkOutlineFilter_h
#define svtkOutlineFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSMODELING_EXPORT svtkOutlineFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation. type information, and printing.
   */
  static svtkOutlineFilter* New();
  svtkTypeMacro(svtkOutlineFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Generate solid faces for the box. This is off by default.
   */
  svtkSetMacro(GenerateFaces, svtkTypeBool);
  svtkBooleanMacro(GenerateFaces, svtkTypeBool);
  svtkGetMacro(GenerateFaces, svtkTypeBool);
  //@}

  enum CompositeOutlineStyle
  {
    ROOT_LEVEL = 0,
    LEAF_DATASETS = 1,
    ROOT_AND_LEAFS = 2,
    SPECIFIED_INDEX = 3
  };

  //@{
  /**
   * Specify a style for creating bounding boxes around input composite
   * datasets. (If the filter input is a svtkDataSet type these options have
   * no effect.) There are four choices: 1) place a bounding box around the
   * root of the svtkCompositeDataSet (i.e., all of the data); 2) place
   * separate bounding boxes around each svtkDataSet leaf of the composite
   * dataset; 3) place a bounding box around the root and all dataset leaves;
   * and 4) place a bounding box around each (flat) index of the composite
   * dataset. The default behavior is both root and leafs.
   */
  svtkSetMacro(CompositeStyle, int);
  svtkGetMacro(CompositeStyle, int);
  void SetCompositeStyleToRoot() { this->SetCompositeStyle(ROOT_LEVEL); }
  void SetCompositeStyleToLeafs() { this->SetCompositeStyle(LEAF_DATASETS); }
  void SetCompositeStyleToRootAndLeafs() { this->SetCompositeStyle(ROOT_AND_LEAFS); }
  void SetCompositeStyleToSpecifiedIndex() { this->SetCompositeStyle(SPECIFIED_INDEX); }
  //@}

  //@{
  /**
   * If the composite style is set to SpecifiedIndex, then one or more flat
   * indices can be specified, and bounding boxes will be drawn around those
   * pieces of the composite dataset. (Recall that the flat index is a
   * non-negative integer, with root index=0, increasing in perorder
   * (depth-first) traversal order.
   */
  void AddIndex(unsigned int index);
  void RemoveIndex(unsigned int index);
  void RemoveAllIndices();
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkOutlineFilter();
  ~svtkOutlineFilter() override;

  svtkTypeBool GenerateFaces;
  int CompositeStyle;
  int OutputPointsPrecision;

  class svtkIndexSet;
  svtkIndexSet* Indices;

  void AppendOutline(svtkPoints* pts, svtkCellArray* lines, svtkCellArray* faces, double bds[6]);

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkOutlineFilter(const svtkOutlineFilter&) = delete;
  void operator=(const svtkOutlineFilter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkOutlineFilter.h
