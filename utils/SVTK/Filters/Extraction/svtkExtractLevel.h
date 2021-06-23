/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractLevel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractLevel
 * @brief   extract levels between min and max from a
 * hierarchical box dataset.
 *
 * svtkExtractLevel filter extracts the levels between (and including) the user
 * specified min and max levels.
 */

#ifndef svtkExtractLevel_h
#define svtkExtractLevel_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractLevel : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkExtractLevel* New();
  svtkTypeMacro(svtkExtractLevel, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Select the levels that should be extracted. All other levels will have no
   * datasets in them.
   */
  void AddLevel(unsigned int level);
  void RemoveLevel(unsigned int level);
  void RemoveAllLevels();
  //@}

protected:
  svtkExtractLevel();
  ~svtkExtractLevel() override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /// Implementation of the algorithm.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkExtractLevel(const svtkExtractLevel&) = delete;
  void operator=(const svtkExtractLevel&) = delete;

  class svtkSet;
  svtkSet* Levels;
};

#endif
