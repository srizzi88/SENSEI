/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAdjacencyMatrixToEdgeTable.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkAdjacencyMatrixToEdgeTable
 *
 *
 * Treats a dense 2-way array of doubles as an adacency matrix and converts it into a
 * svtkTable suitable for use as an edge table with svtkTableToGraph.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkAdjacencyMatrixToEdgeTable_h
#define svtkAdjacencyMatrixToEdgeTable_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkAdjacencyMatrixToEdgeTable : public svtkTableAlgorithm
{
public:
  static svtkAdjacencyMatrixToEdgeTable* New();
  svtkTypeMacro(svtkAdjacencyMatrixToEdgeTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specifies whether rows or columns become the "source" in the output edge table.
   * 0 = rows, 1 = columns.  Default: 0
   */
  svtkGetMacro(SourceDimension, svtkIdType);
  svtkSetMacro(SourceDimension, svtkIdType);
  //@}

  //@{
  /**
   * Controls the name of the output table column that contains edge weights.
   * Default: "value"
   */
  svtkGetStringMacro(ValueArrayName);
  svtkSetStringMacro(ValueArrayName);
  //@}

  //@{
  /**
   * Specifies the minimum number of adjacent edges to include for each source vertex.
   * Default: 0
   */
  svtkGetMacro(MinimumCount, svtkIdType);
  svtkSetMacro(MinimumCount, svtkIdType);
  //@}

  //@{
  /**
   * Specifies a minimum threshold that an edge weight must exceed to be included in
   * the output.
   * Default: 0.5
   */
  svtkGetMacro(MinimumThreshold, double);
  svtkSetMacro(MinimumThreshold, double);
  //@}

protected:
  svtkAdjacencyMatrixToEdgeTable();
  ~svtkAdjacencyMatrixToEdgeTable() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIdType SourceDimension;
  char* ValueArrayName;
  svtkIdType MinimumCount;
  double MinimumThreshold;

private:
  svtkAdjacencyMatrixToEdgeTable(const svtkAdjacencyMatrixToEdgeTable&) = delete;
  void operator=(const svtkAdjacencyMatrixToEdgeTable&) = delete;
};

#endif
