/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableToStructuredGrid
 * @brief   converts svtkTable to a svtkStructuredGrid.
 *
 * svtkTableToStructuredGrid is a filter that converts an input
 * svtkTable to a svtkStructuredGrid. It provides API to select columns to use as
 * points in the output structured grid. The specified dimensions of the output
 * (specified using SetWholeExtent()) must match the number of rows in the input
 * table.
 */

#ifndef svtkTableToStructuredGrid_h
#define svtkTableToStructuredGrid_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class svtkTable;

class SVTKFILTERSGENERAL_EXPORT svtkTableToStructuredGrid : public svtkStructuredGridAlgorithm
{
public:
  static svtkTableToStructuredGrid* New();
  svtkTypeMacro(svtkTableToStructuredGrid, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the whole extents for the image to produce. The size of the image
   * must match the number of rows in the input table.
   */
  svtkSetVector6Macro(WholeExtent, int);
  svtkGetVector6Macro(WholeExtent, int);
  //@}

  //@{
  /**
   * Set the name of the column to use as the X coordinate for the points.
   */
  svtkSetStringMacro(XColumn);
  svtkGetStringMacro(XColumn);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetXColumn() to
   * use as the xcoordinate in case the column is a multi-component array.
   * Default is 0.
   */
  svtkSetClampMacro(XComponent, int, 0, SVTK_INT_MAX);
  svtkGetMacro(XComponent, int);
  //@}

  //@{
  /**
   * Set the name of the column to use as the Y coordinate for the points.
   * Default is 0.
   */
  svtkSetStringMacro(YColumn);
  svtkGetStringMacro(YColumn);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetYColumn() to
   * use as the Ycoordinate in case the column is a multi-component array.
   */
  svtkSetClampMacro(YComponent, int, 0, SVTK_INT_MAX);
  svtkGetMacro(YComponent, int);
  //@}

  //@{
  /**
   * Set the name of the column to use as the Z coordinate for the points.
   * Default is 0.
   */
  svtkSetStringMacro(ZColumn);
  svtkGetStringMacro(ZColumn);
  //@}

  //@{
  /**
   * Specify the component for the column specified using SetZColumn() to
   * use as the Zcoordinate in case the column is a multi-component array.
   */
  svtkSetClampMacro(ZComponent, int, 0, SVTK_INT_MAX);
  svtkGetMacro(ZComponent, int);
  //@}

protected:
  svtkTableToStructuredGrid();
  ~svtkTableToStructuredGrid() override;

  int Convert(svtkTable*, svtkStructuredGrid*, int extent[6]);

  /**
   * Overridden to specify that input must be a svtkTable.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Convert input svtkTable to svtkStructuredGrid.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Request information -- pass whole extent to the pipeline.
   */
  int RequestInformation(svtkInformation* svtkNotUsed(request),
    svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector) override;

  char* XColumn;
  char* YColumn;
  char* ZColumn;
  int XComponent;
  int YComponent;
  int ZComponent;
  int WholeExtent[6];

private:
  svtkTableToStructuredGrid(const svtkTableToStructuredGrid&) = delete;
  void operator=(const svtkTableToStructuredGrid&) = delete;
};

#endif
