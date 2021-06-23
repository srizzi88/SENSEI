/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUniformHyperTreeGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUniformHyperTreeGridSource
 * @brief   Create a synthetic grid of uniform hypertrees.
 *
 * This class uses input parameters, most notably a string descriptor,
 * to generate a svtkHyperTreeGrid instance representing the corresponding
 * tree-based AMR grid with uniform root cell sizes along each axis.
 *
 * @sa
 * svtkHyperTreeGridSource svtkUniformHyperTreeGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, NexGen Analytics 2017
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef svtkUniformHyperTreeGridSource_h
#define svtkUniformHyperTreeGridSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkHyperTreeGridSource.h"

class SVTKFILTERSSOURCES_EXPORT svtkUniformHyperTreeGridSource : public svtkHyperTreeGridSource
{
public:
  svtkTypeMacro(svtkUniformHyperTreeGridSource, svtkHyperTreeGridSource);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkUniformHyperTreeGridSource* New();

protected:
  svtkUniformHyperTreeGridSource();
  ~svtkUniformHyperTreeGridSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkUniformHyperTreeGridSource(const svtkUniformHyperTreeGridSource&) = delete;
  void operator=(const svtkUniformHyperTreeGridSource&) = delete;
};

#endif
