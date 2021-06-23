/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConvertSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkConvertSelection
 * @brief   Convert a selection from one type to another
 *
 *
 * svtkConvertSelection converts an input selection from one type to another
 * in the context of a data object being selected. The first input is the
 * selection, while the second input is the data object that the selection
 * relates to.
 *
 * @sa
 * svtkSelection svtkSelectionNode svtkExtractSelection svtkExtractSelectedGraph
 */

#ifndef svtkConvertSelection_h
#define svtkConvertSelection_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"

class svtkCompositeDataSet;
class svtkGraph;
class svtkIdTypeArray;
class svtkSelection;
class svtkSelectionNode;
class svtkStringArray;
class svtkTable;
class svtkExtractSelection;

class SVTKFILTERSEXTRACTION_EXPORT svtkConvertSelection : public svtkSelectionAlgorithm
{
public:
  static svtkConvertSelection* New();
  svtkTypeMacro(svtkConvertSelection, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * A convenience method for setting the second input (i.e. the data object).
   */
  void SetDataObjectConnection(svtkAlgorithmOutput* in);

  //@{
  /**
   * The input field type.
   * If this is set to a number other than -1, ignores the input selection
   * field type and instead assumes that all selection nodes have the
   * field type specified.
   * This should be one of the constants defined in svtkSelectionNode.h.
   * Default is -1.
   */
  svtkSetMacro(InputFieldType, int);
  svtkGetMacro(InputFieldType, int);
  //@}

  //@{
  /**
   * The output selection content type.
   * This should be one of the constants defined in svtkSelectionNode.h.
   */
  svtkSetMacro(OutputType, int);
  svtkGetMacro(OutputType, int);
  //@}

  //@{
  /**
   * The output array name for value or threshold selections.
   */
  virtual void SetArrayName(const char*);
  virtual const char* GetArrayName();
  //@}

  //@{
  /**
   * The output array names for value selection.
   */
  virtual void SetArrayNames(svtkStringArray*);
  svtkGetObjectMacro(ArrayNames, svtkStringArray);
  //@}

  //@{
  /**
   * Convenience methods used by UI
   */
  void AddArrayName(const char*);
  void ClearArrayNames();
  //@}

  //@{
  /**
   * When on, creates a separate selection node for each array.
   * Defaults to OFF.
   */
  svtkSetMacro(MatchAnyValues, bool);
  svtkGetMacro(MatchAnyValues, bool);
  svtkBooleanMacro(MatchAnyValues, bool);
  //@}

  //@{
  /**
   * When enabled, not finding expected array will not return an error.
   * Defaults to OFF.
   */
  svtkSetMacro(AllowMissingArray, bool);
  svtkGetMacro(AllowMissingArray, bool);
  svtkBooleanMacro(AllowMissingArray, bool);
  //@}

  //@{
  /**
   * Set/get a selection extractor used in some conversions to
   * obtain IDs.
   */
  virtual void SetSelectionExtractor(svtkExtractSelection*);
  svtkGetObjectMacro(SelectionExtractor, svtkExtractSelection);
  //@}

  //@{
  /**
   * Static methods for easily converting between selection types.
   * NOTE: The returned selection pointer IS reference counted,
   * so be sure to Delete() it when you are done with it.
   */
  static svtkSelection* ToIndexSelection(svtkSelection* input, svtkDataObject* data);
  static svtkSelection* ToGlobalIdSelection(svtkSelection* input, svtkDataObject* data);
  static svtkSelection* ToPedigreeIdSelection(svtkSelection* input, svtkDataObject* data);
  static svtkSelection* ToValueSelection(
    svtkSelection* input, svtkDataObject* data, const char* arrayName);
  static svtkSelection* ToValueSelection(
    svtkSelection* input, svtkDataObject* data, svtkStringArray* arrayNames);
  //@}

  /**
   * Static generic method for obtaining selected items from a data object.
   * Other static methods (e.g. GetSelectedVertices) call this one.
   */
  static void GetSelectedItems(
    svtkSelection* input, svtkDataObject* data, int fieldType, svtkIdTypeArray* indices);

  //@{
  /**
   * Static methods for easily obtaining selected items from a data object.
   * The array argument will be filled with the selected items.
   */
  static void GetSelectedVertices(svtkSelection* input, svtkGraph* data, svtkIdTypeArray* indices);
  static void GetSelectedEdges(svtkSelection* input, svtkGraph* data, svtkIdTypeArray* indices);
  static void GetSelectedPoints(svtkSelection* input, svtkDataSet* data, svtkIdTypeArray* indices);
  static void GetSelectedCells(svtkSelection* input, svtkDataSet* data, svtkIdTypeArray* indices);
  static void GetSelectedRows(svtkSelection* input, svtkTable* data, svtkIdTypeArray* indices);
  //@}

  /**
   * A generic static method for converting selection types.
   * The type should be an integer constant defined in svtkSelectionNode.h.
   */
  static svtkSelection* ToSelectionType(svtkSelection* input, svtkDataObject* data, int type,
    svtkStringArray* arrayNames = nullptr, int inputFieldType = -1, bool allowMissingArray = false);

protected:
  svtkConvertSelection();
  ~svtkConvertSelection() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int Convert(svtkSelection* input, svtkDataObject* data, svtkSelection* output);

  int ConvertCompositeDataSet(svtkSelection* input, svtkCompositeDataSet* data, svtkSelection* output);

  int ConvertFromQueryNodeCompositeDataSet(
    svtkSelectionNode* input, svtkCompositeDataSet* data, svtkSelection* output);

  int ConvertToIndexSelection(svtkSelectionNode* input, svtkDataSet* data, svtkSelectionNode* output);

  int SelectTableFromTable(svtkTable* selTable, svtkTable* dataTable, svtkIdTypeArray* indices);

  int ConvertToBlockSelection(svtkSelection* input, svtkCompositeDataSet* data, svtkSelection* output);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int OutputType;
  int InputFieldType;
  svtkStringArray* ArrayNames;
  bool MatchAnyValues;
  bool AllowMissingArray;
  svtkExtractSelection* SelectionExtractor;

private:
  svtkConvertSelection(const svtkConvertSelection&) = delete;
  void operator=(const svtkConvertSelection&) = delete;
};

#endif
