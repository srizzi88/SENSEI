/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelection
 * @brief   extract a subset from a svtkDataSet.
 *
 * svtkExtractSelection extracts some subset of cells and points from
 * its input dataobject. The dataobject is given on its first input port.
 * The subset is described by the contents of the svtkSelection on its
 * second input port.  Depending on the contents of the svtkSelection
 * this will create various svtkSelectors to identify the
 * selected elements.
 *
 * This filter supports svtkCompositeDataSet (output is svtkMultiBlockDataSet),
 * svtkTable and svtkDataSet (output is svtkUnstructuredGrid).
 * Other types of input are not processed and the corresponding output is a
 * default constructed object of the input type.
 *
 * @sa
 * svtkSelection svtkSelector svtkSelectionNode
 */

#ifndef svtkExtractSelection_h
#define svtkExtractSelection_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersExtractionModule.h" // For export macro

#include "svtkSelectionNode.h" // for svtkSelectionNode::SelectionContent
#include "svtkSmartPointer.h"  // for smart pointer

class svtkSignedCharArray;
class svtkSelection;
class svtkSelectionNode;
class svtkSelector;
class svtkUnstructuredGrid;
class svtkTable;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelection : public svtkDataObjectAlgorithm
{
public:
  static svtkExtractSelection* New();
  svtkTypeMacro(svtkExtractSelection, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  //@{
  /**
   * This flag tells the extraction filter not to extract a subset of the
   * data, but instead to produce a svtkInsidedness array and add it to the
   * input dataset. Default value is false(0).
   */
  svtkSetMacro(PreserveTopology, bool);
  svtkGetMacro(PreserveTopology, bool);
  svtkBooleanMacro(PreserveTopology, bool);
  //@}

protected:
  svtkExtractSelection();
  ~svtkExtractSelection() override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  /**
   * Sets up empty output dataset
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // Gets the attribute association of the selection.  Currently we support ROW, POINT, and CELL.
  // If the selection types are mismatched the boolean parameter will be set to false, otherwise
  // it will be true after the function returns.
  svtkDataObject::AttributeTypes GetAttributeTypeOfSelection(svtkSelection* sel, bool& sane);

  /**
   * Creates a new svtkSelector for the given content type.
   * May return null if not supported.
   */
  virtual svtkSmartPointer<svtkSelector> NewSelectionOperator(
    svtkSelectionNode::SelectionContent type);

  /**
   * Given a non-composite input data object (either a block of a larger composite
   * or the whole input), along with the element type being extracted and the
   * computed insidedness array this method either copies the input and adds the
   * insidedness array (if PreserveTopology is on) or returns a new data object
   * containing only the elements to be extracted.
   */
  svtkSmartPointer<svtkDataObject> ExtractElements(svtkDataObject* block,
    svtkDataObject::AttributeTypes elementType, svtkSignedCharArray* insidednessArray);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Given a svtkDataSet and an array of which cells to extract, this populates
   * the given svtkUnstructuredGrid with the selected cells.
   */
  void ExtractSelectedCells(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkSignedCharArray* cellInside);
  /**
   * Given a svtkDataSet and an array of which points to extract, the populates
   * the given svtkUnstructuredGrid with the selected points and a cell of type vertex
   * for each point.
   */
  void ExtractSelectedPoints(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkSignedCharArray* pointInside);
  /**
   * Given an input svtkTable and an array of which rows to extract, this populates
   * the output table with the selected rows.
   */
  void ExtractSelectedRows(svtkTable* input, svtkTable* output, svtkSignedCharArray* rowsInside);

  bool PreserveTopology = false;

private:
  svtkExtractSelection(const svtkExtractSelection&) = delete;
  void operator=(const svtkExtractSelection&) = delete;
};

#endif
