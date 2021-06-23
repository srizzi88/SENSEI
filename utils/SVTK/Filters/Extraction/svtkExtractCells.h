/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractCells.h

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
 * @class   svtkExtractCells
 * @brief   subset a svtkDataSet to create a svtkUnstructuredGrid
 *
 *
 *    Given a svtkDataSet and a list of cell ids, create a svtkUnstructuredGrid
 *    composed of these cells.  If the cell list is empty when svtkExtractCells
 *    executes, it will set up the ugrid, point and cell arrays, with no points,
 *    cells or data.
 */

#ifndef svtkExtractCells_h
#define svtkExtractCells_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkIdList;
class svtkExtractCellsSTLCloak;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractCells : public svtkUnstructuredGridAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  svtkTypeMacro(svtkExtractCells, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkExtractCells* New();
  //@}

  /**
   * Set the list of cell IDs that the output svtkUnstructuredGrid will be
   * composed of.  Replaces any other cell ID list supplied so far.  (Set to
   * nullptr to free memory used by cell list.)  The cell ids should be >=0.
   */
  void SetCellList(svtkIdList* l);

  /**
   * Add the supplied list of cell IDs to those that will be included in the
   * output svtkUnstructuredGrid. The cell ids should be >=0.
   */
  void AddCellList(svtkIdList* l);

  /**
   * Add this range of cell IDs to those that will be included in the output
   * svtkUnstructuredGrid. Note that (from < to), and (from >= 0).
   */
  void AddCellRange(svtkIdType from, svtkIdType to);

  //@{
  /**
   * Another way to provide ids using a pointer to svtkIdType array.
   */
  void SetCellIds(const svtkIdType* ptr, svtkIdType numValues);
  void AddCellIds(const svtkIdType* ptr, svtkIdType numValues);
  //@}

  //@{
  /**
   * If all cells are being extracted, this filter can use fast path to speed up
   * the extraction. In that case, one can set this flag to true. When set to
   * true, cell ids added via the various methods are simply ignored.
   * Defaults to false.
   */
  svtkSetMacro(ExtractAllCells, bool);
  svtkGetMacro(ExtractAllCells, bool);
  svtkBooleanMacro(ExtractAllCells, bool);
  //@}

  //@{
  /**
   * If the cell ids specified are already sorted and unique, then set this to
   * true to avoid the filter from doing time-consuming sorts and uniquification
   * operations. Defaults to false.
   */
  svtkSetMacro(AssumeSortedAndUniqueIds, bool);
  svtkGetMacro(AssumeSortedAndUniqueIds, bool);
  svtkBooleanMacro(AssumeSortedAndUniqueIds, bool);
  //@}
protected:
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkExtractCells();
  ~svtkExtractCells() override;

  void Copy(svtkDataSet* input, svtkUnstructuredGrid* output);
  svtkIdType ReMapPointIds(svtkDataSet* grid);

  void CopyCellsDataSet(svtkDataSet* input, svtkUnstructuredGrid* output);
  void CopyCellsUnstructuredGrid(svtkDataSet* input, svtkUnstructuredGrid* output);

  svtkExtractCellsSTLCloak* CellList = nullptr;
  svtkIdType SubSetUGridCellArraySize = 0;
  svtkIdType SubSetUGridFacesArraySize = 0;
  bool InputIsUgrid = false;
  bool ExtractAllCells = false;
  bool AssumeSortedAndUniqueIds = false;

private:
  svtkExtractCells(const svtkExtractCells&) = delete;
  void operator=(const svtkExtractCells&) = delete;
};

#endif
