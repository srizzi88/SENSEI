/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedRows.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkExtractSelectedRows
 * @brief   return selected rows of a table
 *
 *
 * The first input is a svtkTable to extract rows from.
 * The second input is a svtkSelection containing the selected indices.
 * The third input is a svtkAnnotationLayers containing selected indices.
 * The field type of the input selection is ignored when converted to row
 * indices.
 */

#ifndef svtkExtractSelectedRows_h
#define svtkExtractSelectedRows_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelectedRows : public svtkTableAlgorithm
{
public:
  static svtkExtractSelectedRows* New();
  svtkTypeMacro(svtkExtractSelectedRows, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * A convenience method for setting the second input (i.e. the selection).
   */
  void SetSelectionConnection(svtkAlgorithmOutput* in);

  /**
   * A convenience method for setting the third input (i.e. the annotation layers).
   */
  void SetAnnotationLayersConnection(svtkAlgorithmOutput* in);

  /**
   * Specify the first svtkGraph input and the second svtkSelection input.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  //@{
  /**
   * When set, a column named svtkOriginalRowIds will be added to the output.
   * False by default.
   */
  svtkSetMacro(AddOriginalRowIdsArray, bool);
  svtkGetMacro(AddOriginalRowIdsArray, bool);
  svtkBooleanMacro(AddOriginalRowIdsArray, bool);
  //@}

protected:
  svtkExtractSelectedRows();
  ~svtkExtractSelectedRows() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool AddOriginalRowIdsArray;

private:
  svtkExtractSelectedRows(const svtkExtractSelectedRows&) = delete;
  void operator=(const svtkExtractSelectedRows&) = delete;
};

#endif
