/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkBlockSelector
 * @brief Selects cells or points contained in a block as defined in the
 * svtkSelectionNode used to initialize this operator.
 *
 */

#ifndef svtkBlockSelector_h
#define svtkBlockSelector_h

#include "svtkSelector.h"

class SVTKFILTERSEXTRACTION_EXPORT svtkBlockSelector : public svtkSelector
{
public:
  static svtkBlockSelector* New();
  svtkTypeMacro(svtkBlockSelector, svtkSelector);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Initialize(svtkSelectionNode* node) override;

protected:
  svtkBlockSelector();
  ~svtkBlockSelector() override;

  bool ComputeSelectedElements(svtkDataObject* input, svtkSignedCharArray* insidednessArray) override;
  SelectionMode GetAMRBlockSelection(unsigned int level, unsigned int index) override;
  SelectionMode GetBlockSelection(unsigned int compositeIndex) override;

private:
  svtkBlockSelector(const svtkBlockSelector&) = delete;
  void operator=(const svtkBlockSelector&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
