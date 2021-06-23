/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplitByCellScalarFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSplitByCellScalarFilter
 * @brief   splits input dataset according an integer cell scalar array.
 *
 * svtkSplitByCellScalarFilter is a filter that splits any dataset type
 * according an integer cell scalar value (typically a material identifier) to
 * a multiblock. Each block of the output contains cells that have the same
 * scalar value. Output blocks will be of type svtkUnstructuredGrid except if
 * input is of type svtkPolyData. In that case output blocks are of type
 * svtkPolyData.
 *
 * @sa
 * svtkThreshold
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux, Kitware 2016.
 */

#ifndef svtkSplitByCellScalarFilter_h
#define svtkSplitByCellScalarFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkSplitByCellScalarFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkSplitByCellScalarFilter* New();
  svtkTypeMacro(svtkSplitByCellScalarFilter, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify if input points array must be passed to output blocks. If so,
   * filter processing is faster but outblocks will contains more points than
   * what is needed by the cells it owns. If not, a new points array is created
   * for every block and it will only contains points for copied cells.
   * Note that this function is only possible for PointSet datasets.
   * The default is true.
   */
  svtkGetMacro(PassAllPoints, bool);
  svtkSetMacro(PassAllPoints, bool);
  svtkBooleanMacro(PassAllPoints, bool);
  //@}

protected:
  svtkSplitByCellScalarFilter();
  ~svtkSplitByCellScalarFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  bool PassAllPoints;

private:
  svtkSplitByCellScalarFilter(const svtkSplitByCellScalarFilter&) = delete;
  void operator=(const svtkSplitByCellScalarFilter&) = delete;
};

#endif
