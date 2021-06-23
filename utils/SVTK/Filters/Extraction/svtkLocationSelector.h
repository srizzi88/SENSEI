/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLocationSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkLocationSelector
 * @brief selects cells containing or points near chosen point locations.
 *
 * svtkLocationSelector is svtkSelector that can select elements
 * containing or near matching elements. It handles svtkSelectionNode::LOCATIONS
 */

#ifndef svtkLocationSelector_h
#define svtkLocationSelector_h

#include "svtkSelector.h"

#include <memory> // unique_ptr

class SVTKFILTERSEXTRACTION_EXPORT svtkLocationSelector : public svtkSelector
{
public:
  static svtkLocationSelector* New();
  svtkTypeMacro(svtkLocationSelector, svtkSelector);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Initialize(svtkSelectionNode* node) override;
  void Finalize() override;

protected:
  svtkLocationSelector();
  ~svtkLocationSelector() override;

  bool ComputeSelectedElements(svtkDataObject* input, svtkSignedCharArray* insidednessArray) override;

private:
  svtkLocationSelector(const svtkLocationSelector&) = delete;
  void operator=(const svtkLocationSelector&) = delete;

  class svtkInternals;
  class svtkInternalsForPoints;
  class svtkInternalsForCells;
  std::unique_ptr<svtkInternals> Internals;
};

#endif
