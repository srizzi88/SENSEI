/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedLocations.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectedLocations
 * @brief   extract cells within a dataset that
 * contain the locations listen in the svtkSelection.
 *
 * svtkExtractSelectedLocations extracts all cells whose volume contain at least
 * one point listed in the LOCATIONS content of the svtkSelection. This filter
 * adds a scalar array called svtkOriginalCellIds that says what input cell
 * produced each output cell. This is an example of a Pedigree ID which helps
 * to trace back results.
 * @sa
 * svtkSelection svtkExtractSelection
 */

#ifndef svtkExtractSelectedLocations_h
#define svtkExtractSelectedLocations_h

#include "svtkExtractSelectionBase.h"
#include "svtkFiltersExtractionModule.h" // For export macro

class svtkSelection;
class svtkSelectionNode;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelectedLocations : public svtkExtractSelectionBase
{
public:
  static svtkExtractSelectedLocations* New();
  svtkTypeMacro(svtkExtractSelectedLocations, svtkExtractSelectionBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkExtractSelectedLocations();
  ~svtkExtractSelectedLocations() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int ExtractCells(svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output);
  int ExtractPoints(svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output);

private:
  svtkExtractSelectedLocations(const svtkExtractSelectedLocations&) = delete;
  void operator=(const svtkExtractSelectedLocations&) = delete;
};

#endif
