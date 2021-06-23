/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostExtractLargestComponent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBoostExtractLargestComponent
 * @brief   Extract the largest connected
 * component of a graph
 *
 *
 * svtkBoostExtractLargestComponent finds the largest connected region of a
 * svtkGraph. For directed graphs, this returns the largest biconnected component.
 * See svtkBoostConnectedComponents for details.
 */

#ifndef svtkBoostExtractLargestComponent_h
#define svtkBoostExtractLargestComponent_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

class svtkGraph;

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostExtractLargestComponent
  : public svtkGraphAlgorithm
{
public:
  svtkTypeMacro(svtkBoostExtractLargestComponent, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct an instance of svtkBoostExtractLargestComponent with
   * InvertSelection set to false.
   */
  static svtkBoostExtractLargestComponent* New();

  //@{
  /**
   * Set the flag to determine if the selection should be inverted.
   */
  svtkSetMacro(InvertSelection, bool);
  svtkGetMacro(InvertSelection, bool);
  //@}

protected:
  svtkBoostExtractLargestComponent();
  ~svtkBoostExtractLargestComponent() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Store the choice of whether or not to invert the selection.
   */
  bool InvertSelection;

private:
  svtkBoostExtractLargestComponent(const svtkBoostExtractLargestComponent&) = delete;
  void operator=(const svtkBoostExtractLargestComponent&) = delete;
};

#endif
