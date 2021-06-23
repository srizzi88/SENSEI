/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSelector.h
 * @brief   Computes the portion of a dataset which is inside a selection
 *
 * This is an abstract superclass for types of selection operations. Subclasses
 * generally only need to override `ComputeSelectedElements`.
 */

#ifndef svtkSelector_h
#define svtkSelector_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkObject.h"
#include "svtkSmartPointer.h" // For svtkSmartPointer

class svtkCompositeDataSet;
class svtkDataObject;
class svtkSelectionNode;
class svtkSignedCharArray;
class svtkTable;
class svtkDataObjectTree;
class svtkUniformGridAMR;

class SVTKFILTERSEXTRACTION_EXPORT svtkSelector : public svtkObject
{
public:
  svtkTypeMacro(svtkSelector, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Sets the svtkSelectionNode used by this selection operator and initializes
   * the data structures in the selection operator based on the selection.
   *
   * (for example in the svtkFrustumSelector this creates the svtkPlanes
   * implicit function to represent the frustum).
   *
   * @param node The selection node that determines the behavior of this operator.
   */
  virtual void Initialize(svtkSelectionNode* node);

  /**
   * Does any cleanup of objects created in Initialize
   */
  virtual void Finalize() {}

  /**
   * Given an input and the svtkSelectionNode passed into the Initialize() method, add to the
   * output a svtkSignedChar attribute array indicating whether each element is inside (1)
   * or outside (0) the selection. The attribute (point data or cell data) is determined
   * by the svtkSelection that owns the svtkSelectionNode set in Initialize(). The insidedness
   * array is named with the value of InsidednessArrayName. If input is a svtkCompositeDataSet,
   * the insidedness array is added to each block.
   *
   */
  virtual void Execute(svtkDataObject* input, svtkDataObject* output);

  //@{
  /**
   * Get/Set the name of the array to use for the insidedness array to add to
   * the output in `Execute` call.
   */
  svtkSetMacro(InsidednessArrayName, std::string);
  svtkGetMacro(InsidednessArrayName, std::string);
  //@}
protected:
  svtkSelector();
  virtual ~svtkSelector() override;

  // Contains the selection criteria.
  svtkSelectionNode* Node = nullptr;

  // Name of the insidedness array added to the output when the selection criteria is
  // evaluated by this operator.
  std::string InsidednessArrayName;

  /**
   * This method computes whether or not each element in the dataset is inside the selection
   * and populates the given array with 0 (outside the selection) or 1 (inside the selection).
   *
   * The svtkDataObject passed in will never be a `svtkCompositeDataSet` subclass.
   *
   * What type of elements are operated over is determined by the svtkSelectionNode's
   * field association. The insidednessArray passed in should have the correct number of elements
   * for that field type or it will be resized.
   *
   * Returns true for successful completion. The operator should only return false
   * when it cannot operate on the inputs. In which case, it is assumed that the
   * insidednessArray may have been left untouched by this method and the calling code
   * will fill it with 0.
   */
  virtual bool ComputeSelectedElements(
    svtkDataObject* input, svtkSignedCharArray* insidednessArray) = 0;

  enum SelectionMode
  {
    INCLUDE,
    EXCLUDE,
    INHERIT
  };

  /**
   * Returns whether the AMR block is to be processed. Return `INCLUDE` to
   * indicate it must be processed or `EXCLUDE` to indicate it must not be
   * processed. If the selector cannot make an exact determination for the given
   * level, index it should return `INHERIT`. If the selection did not specify
   * which AMR block to extract, then too return `INHERIT`.
   */
  virtual SelectionMode GetAMRBlockSelection(unsigned int level, unsigned int index);

  /**
   * Returns whether the block is to be processed. Return `INCLUDE` to
   * indicate it must be processed or `EXCLUDE` to indicate it must not be
   * processed. If the selector cannot make an exact determination for the given
   * level and index, it should return `INHERIT`. Note, returning `INCLUDE` or
   * `EXCLUDE` has impact on all nodes in the subtree unless any of the node
   * explicitly overrides the block selection mode.
   */
  virtual SelectionMode GetBlockSelection(unsigned int compositeIndex);

  /**
   * Creates an array suitable for storing insideness. The array is named using
   * this->InsidednessArrayName and is sized to exactly `numElems` values.
   */
  svtkSmartPointer<svtkSignedCharArray> CreateInsidednessArray(svtkIdType numElems);

  /**
   * Given a data object and selected points, return an array indicating the
   * insidedness of cells that contain at least one of the selected points.
   */
  svtkSmartPointer<svtkSignedCharArray> ComputeCellsContainingSelectedPoints(
    svtkDataObject* data, svtkSignedCharArray* selectedPoints);

  /**
   * Handle expanding to connected cells or point, if requested. This method is
   * called in `Execute`. Subclass that override `Execute` should ensure they
   * call this method to handle expanding to connected elements, as requested.
   *
   * Note: this method will modify `output`.
   */
  void ExpandToConnectedElements(svtkDataObject* output);

private:
  svtkSelector(const svtkSelector&) = delete;
  void operator=(const svtkSelector&) = delete;

  void ProcessBlock(svtkDataObject* inputBlock, svtkDataObject* outputBlock, bool forceFalse);
  void ProcessAMR(svtkUniformGridAMR* input, svtkCompositeDataSet* output);
  void ProcessDataObjectTree(svtkDataObjectTree* input, svtkDataObjectTree* output,
    SelectionMode inheritedSelectionMode, unsigned int compositeIndex = 0);
};

#endif
