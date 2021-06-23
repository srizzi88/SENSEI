/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractDataSets
 * @brief   extracts a number of datasets.
 *
 * svtkExtractDataSets accepts a svtkHierarchicalBoxDataSet as input and extracts
 * different datasets from different levels. The output is
 * svtkMultiBlockDataSet of svtkMultiPiece datasets. Each block corresponds to
 * a level in the vktHierarchicalBoxDataSet. Individual datasets, within a level,
 * are stored in a svtkMultiPiece dataset.
 *
 * @sa
 * svtkHierarchicalBoxDataSet, svtkMultiBlockDataSet svtkMultiPieceDataSet
 */

#ifndef svtkExtractDataSets_h
#define svtkExtractDataSets_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractDataSets : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkExtractDataSets* New();
  svtkTypeMacro(svtkExtractDataSets, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a dataset to be extracted.
   */
  void AddDataSet(unsigned int level, unsigned int idx);

  /**
   * Remove all entries from the list of datasets to be extracted.
   */
  void ClearDataSetList();

protected:
  svtkExtractDataSets();
  ~svtkExtractDataSets() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkExtractDataSets(const svtkExtractDataSets&) = delete;
  void operator=(const svtkExtractDataSets&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
