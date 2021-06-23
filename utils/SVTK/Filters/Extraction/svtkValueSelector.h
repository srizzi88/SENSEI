/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkValueSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkValueSelector
 * @brief selects elements matching chosen values.
 *
 * svtkValueSelector is a svtkSelector that can select elements matching
 * values. This can handle a wide array of svtkSelectionNode::SelectionContent types.
 * These include svtkSelectionNode::GLOBALIDS, svtkSelectionNode::PEDIGREEIDS,
 * svtkSelectionNode::VALUES, svtkSelectionNode::INDICES, and
 * svtkSelectionNode::THRESHOLDS.
 *
 * A few things to note:
 *
 * * svtkSelectionNode::SelectionList must be 2-component array for
 *   content-type = svtkSelectionNode::THRESHOLDS and 1-component array for all
 *   other support content-types. For 1-component selection list, this will
 *   match items where the field array (or index) value matches any value in the
 *   selection list. For 2-component selection list, this will match those items
 *   with values in inclusive-range specified by the two components.
 *
 * * For svtkSelectionNode::VALUES or svtkSelectionNode::THRESHOLDS, the field
 *   array to select on is defined by the name given the SelectionList itself.
 *   If the SelectionList has no name (or is an empty string), then the active
 *   scalars from the dataset will be chosen.
 */

#ifndef svtkValueSelector_h
#define svtkValueSelector_h

#include "svtkSelector.h"

#include <memory> // unique_ptr

class SVTKFILTERSEXTRACTION_EXPORT svtkValueSelector : public svtkSelector
{
public:
  static svtkValueSelector* New();
  svtkTypeMacro(svtkValueSelector, svtkSelector);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Initialize(svtkSelectionNode* node) override;
  void Finalize() override;

protected:
  svtkValueSelector();
  ~svtkValueSelector() override;

  bool ComputeSelectedElements(svtkDataObject* input, svtkSignedCharArray* insidednessArray) override;

private:
  svtkValueSelector(const svtkValueSelector&) = delete;
  void operator=(const svtkValueSelector&) = delete;

  class svtkInternals;
  std::unique_ptr<svtkInternals> Internals;
};

#endif
