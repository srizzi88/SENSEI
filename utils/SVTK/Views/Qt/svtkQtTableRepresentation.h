/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTableRepresentation.h

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
 * @class   svtkQtTableRepresentation
 * @brief   set up a svtkTable in a Qt model
 *
 *
 *
 * This class is a wrapper around svtkQtTableModelAdapter.  It
 * performs the following functions:
 *
 * <ul>
 * <li>Keep track of the key column, first data column, and last data column.
 *     Populate the appropriate ivars on the Qt adapter.
 * <li>Assign colors to each of the data series using a svtkLookupTable.
 *     A default lookup table is provided or the user can supply one
 *     using SetColorTable().
 * </ul>
 *
 * The user must supply the following items:
 * <ul>
 * <li>the name of the column that contains the series names,
 * <li>the names of the first and last data columns
 *     (this range should not contain the key column), and
 * <li>(optionally) a svtkLookupTable to use when assigning colors.
 * </ul>
 *
 *
 * @warning
 * Call SetInputConnection with a table connection
 * BEFORE the representation is added to a view or strange things
 * may happen, including segfaults.
 */

#ifndef svtkQtTableRepresentation_h
#define svtkQtTableRepresentation_h

#include "svtkDataRepresentation.h"
#include "svtkViewsQtModule.h" // For export macro

class svtkDoubleArray;
class svtkLookupTable;
class svtkQtTableModelAdapter;

// ----------------------------------------------------------------------

class SVTKVIEWSQT_EXPORT svtkQtTableRepresentation : public svtkDataRepresentation
{
public:
  svtkTypeMacro(svtkQtTableRepresentation, svtkDataRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the lookup table that will be used to determine colors
   * for each series.  The table's range should be [0, 1).
   */
  void SetColorTable(svtkLookupTable* t);
  svtkGetObjectMacro(ColorTable, svtkLookupTable);
  //@}

  //@{
  /**
   * Set/get the name of the column that contains series names.  This
   * must be called BEFORE the representation is added to a view.
   */
  void SetKeyColumn(const char* col);
  char* GetKeyColumn();
  //@}

  //@{
  /**
   * Set/get the name of the first data column.  This must be called
   * BEFORE the representation is added to a view.
   */
  svtkSetStringMacro(FirstDataColumn);
  svtkGetStringMacro(FirstDataColumn);
  //@}

  //@{
  /**
   * Set/get the name of the last data column.  This must be called
   * BEFORE the representation is added to a view.
   */
  svtkSetStringMacro(LastDataColumn);
  svtkGetStringMacro(LastDataColumn);
  //@}

protected:
  svtkQtTableRepresentation();
  ~svtkQtTableRepresentation() override;

  /**
   * Update the table representation
   */
  void UpdateTable();

  svtkSetStringMacro(KeyColumnInternal);
  svtkGetStringMacro(KeyColumnInternal);

  // ----------------------------------------------------------------------
  svtkQtTableModelAdapter* ModelAdapter;
  svtkLookupTable* ColorTable;
  svtkDoubleArray* SeriesColors;
  char* KeyColumnInternal;
  char* FirstDataColumn;
  char* LastDataColumn;

  /**
   * Prepare the input connections to this representation.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  virtual void ResetModel();
  virtual void CreateSeriesColors();

  /**
   * This should set the model type to DATA, METADATA or FULL
   * depending on what you want.
   */
  virtual void SetModelType() {}

private:
  svtkQtTableRepresentation(const svtkQtTableRepresentation&) = delete;
  void operator=(const svtkQtTableRepresentation&) = delete;
};

#endif
