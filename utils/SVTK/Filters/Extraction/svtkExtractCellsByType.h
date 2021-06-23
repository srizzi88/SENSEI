/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractCellsByType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkExtractCellsByType
 * @brief   extract cells of a specified type
 *
 *
 * Given an input svtkDataSet and a list of cell types, produce an output
 * dataset containing only cells of the specified type(s). Note that if the
 * input dataset is homogeneous (e.g., all cells are of the same type) and
 * the cell type is one of the cells specified, then the input dataset is
 * shallow copied to the output.
 *
 * The type of output dataset is always the same as the input type. Since
 * structured types of data (i.e., svtkImageData, svtkStructuredGrid,
 * svtkRectilnearGrid, svtkUniformGrid) are all composed of a cell of the same
 * type, the output is either empty, or a shallow copy of the
 * input. Unstructured data (svtkUnstructuredGrid, svtkPolyData) input may
 * produce a subset of the input data (depending on the selected cell types).
 *
 * Note this filter can be used in a pipeline with composite datasets to
 * extract blocks of (a) particular cell type(s).
 *
 * @warning
 * Unlike the filter svtkExtractCells which always produces
 * svtkUnstructuredGrid output, this filter produces the same output type as
 * input type (i.e., it is a svtkDataSetAlgorithm). Also, svtkExtractCells
 * extracts cells based on their ids.

 * @sa
 * svtkExtractBlock svtkExtractCells
 */

#ifndef svtkExtractCellsByType_h
#define svtkExtractCellsByType_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersExtractionModule.h" // For export macro

struct svtkCellTypeSet;
class svtkIdTypeArray;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractCellsByType : public svtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static svtkExtractCellsByType* New();
  svtkTypeMacro(svtkExtractCellsByType, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the cell types to extract. Any cells of the type specified are
   * extracted. Methods for clearing the set of cells, adding all cells, and
   * determining if a cell is in the set are also provided.
   */
  void AddCellType(unsigned int type);
  void AddAllCellTypes();
  void RemoveCellType(unsigned int type);
  void RemoveAllCellTypes();
  bool ExtractCellType(unsigned int type);
  //@}

protected:
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void ExtractUnstructuredData(svtkDataSet* input, svtkDataSet* output);
  void ExtractPolyDataCells(
    svtkDataSet* input, svtkDataSet* output, svtkIdType* ptMap, svtkIdType& numNewPts);
  void ExtractUnstructuredGridCells(
    svtkDataSet* input, svtkDataSet* output, svtkIdType* ptMap, svtkIdType& numNewPts);

  svtkExtractCellsByType();
  ~svtkExtractCellsByType() override;

private:
  svtkExtractCellsByType(const svtkExtractCellsByType&) = delete;
  void operator=(const svtkExtractCellsByType&) = delete;

  svtkCellTypeSet* CellTypes;
};

#endif
