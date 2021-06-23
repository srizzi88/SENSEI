/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractEdges
 * @brief   extract cell edges from any type of data
 *
 * svtkExtractEdges is a filter to extract edges from a dataset. Edges
 * are extracted as lines or polylines.
 *
 * @sa
 * svtkFeatureEdges
 */

#ifndef svtkExtractEdges_h
#define svtkExtractEdges_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIncrementalPointLocator;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractEdges : public svtkPolyDataAlgorithm
{
public:
  static svtkExtractEdges* New();
  svtkTypeMacro(svtkExtractEdges, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return MTime also considering the locator.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkExtractEdges();
  ~svtkExtractEdges() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkIncrementalPointLocator* Locator;

private:
  svtkExtractEdges(const svtkExtractEdges&) = delete;
  void operator=(const svtkExtractEdges&) = delete;
};

#endif
