/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperTreeGridOutlineFilter
 * @brief   create wireframe outline for arbitrary data set
 *
 * svtkHyperTreeGridOutlineFilter is a filter that generates a wireframe outline of
 * HyperTreeGrid. The outline consists of the twelve edges of the dataset
 * bounding box.
 *
 * @sa
 * svtkOutlineFilter
 *
 * This class was optimized by Jacques-Bernard Lekien, 2019.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkHyperTreeGridOutlineFilter_h
#define svtkHyperTreeGridOutlineFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkHyperTreeGridAlgorithm.h"

class svtkOutlineSource;

class SVTKFILTERSMODELING_EXPORT svtkHyperTreeGridOutlineFilter : public svtkHyperTreeGridAlgorithm
{
public:
  static svtkHyperTreeGridOutlineFilter* New();
  svtkTypeMacro(svtkHyperTreeGridOutlineFilter, svtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Generate solid faces for the box. This is off by default.
   */
  svtkSetMacro(GenerateFaces, svtkTypeBool);
  svtkBooleanMacro(GenerateFaces, svtkTypeBool);
  svtkGetMacro(GenerateFaces, svtkTypeBool);
  //@}

protected:
  svtkHyperTreeGridOutlineFilter();
  ~svtkHyperTreeGridOutlineFilter() override;

  svtkTypeBool GenerateFaces;
  svtkOutlineSource* OutlineSource;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  // JBL Pour moi, c'est un defaut de design de svtkHyperTreeGridAlgorithm
  int ProcessTrees(svtkHyperTreeGrid* input, svtkDataObject* outputDO) override;

private:
  svtkHyperTreeGridOutlineFilter(const svtkHyperTreeGridOutlineFilter&) = delete;
  void operator=(const svtkHyperTreeGridOutlineFilter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkHyperTreeGridOutlineFilter.h
