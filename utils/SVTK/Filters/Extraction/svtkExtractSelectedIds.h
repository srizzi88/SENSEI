/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectedIds
 * @brief   extract a list of cells from a dataset
 *
 * svtkExtractSelectedIds extracts a set of cells and points from within a
 * svtkDataSet. The set of ids to extract are listed within a svtkSelection.
 * This filter adds a scalar array called svtkOriginalCellIds that says what
 * input cell produced each output cell. This is an example of a Pedigree ID
 * which helps to trace back results. Depending on whether the selection has
 * GLOBALIDS, VALUES or INDICES, the selection will use the contents of the
 * array named in the GLOBALIDS DataSetAttribute, and arbitrary array, or the
 * position (tuple id or number) within the cell or point array.
 * @sa
 * svtkSelection svtkExtractSelection
 */

#ifndef svtkExtractSelectedIds_h
#define svtkExtractSelectedIds_h

#include "svtkExtractSelectionBase.h"
#include "svtkFiltersExtractionModule.h" // For export macro

class svtkSelection;
class svtkSelectionNode;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelectedIds : public svtkExtractSelectionBase
{
public:
  static svtkExtractSelectedIds* New();
  svtkTypeMacro(svtkExtractSelectedIds, svtkExtractSelectionBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkExtractSelectedIds();
  ~svtkExtractSelectedIds() override;

  // Overridden to indicate that the input must be a svtkDataSet.
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int ExtractCells(svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output);
  int ExtractPoints(svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output);

private:
  svtkExtractSelectedIds(const svtkExtractSelectedIds&) = delete;
  void operator=(const svtkExtractSelectedIds&) = delete;
};

#endif
