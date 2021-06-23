/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiagonalMatrixSource.h

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
 * @class   svtkDiagonalMatrixSource
 * @brief   generates a sparse or dense square matrix
 * with user-specified values for the diagonal, superdiagonal, and subdiagonal.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkDiagonalMatrixSource_h
#define svtkDiagonalMatrixSource_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkFiltersSourcesModule.h" // For export macro

class SVTKFILTERSSOURCES_EXPORT svtkDiagonalMatrixSource : public svtkArrayDataAlgorithm
{
public:
  static svtkDiagonalMatrixSource* New();
  svtkTypeMacro(svtkDiagonalMatrixSource, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Determines whether the output matrix will be dense or sparse
  enum StorageType
  {
    DENSE,
    SPARSE
  };

  svtkGetMacro(ArrayType, int);
  svtkSetMacro(ArrayType, int);

  //@{
  /**
   * Stores the extents of the output matrix (which is square)
   */
  svtkGetMacro(Extents, svtkIdType);
  svtkSetMacro(Extents, svtkIdType);
  //@}

  //@{
  /**
   * Stores the value that will be assigned to diagonal elements (default: 1)
   */
  svtkGetMacro(Diagonal, double);
  svtkSetMacro(Diagonal, double);
  //@}

  //@{
  /**
   * Stores the value that will be assigned to superdiagonal elements (default: 0)
   */
  svtkGetMacro(SuperDiagonal, double);
  svtkSetMacro(SuperDiagonal, double);
  //@}

  //@{
  /**
   * Stores the value that will be assigned to subdiagonal elements (default: 0)
   */
  svtkGetMacro(SubDiagonal, double);
  svtkSetMacro(SubDiagonal, double);
  //@}

  //@{
  /**
   * Controls the output matrix row dimension label.
   * Default: "rows"
   */
  svtkGetStringMacro(RowLabel);
  svtkSetStringMacro(RowLabel);
  //@}

  //@{
  /**
   * Controls the output matrix column dimension label.
   * Default: "columns"
   */
  svtkGetStringMacro(ColumnLabel);
  svtkSetStringMacro(ColumnLabel);
  //@}

protected:
  svtkDiagonalMatrixSource();
  ~svtkDiagonalMatrixSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkDiagonalMatrixSource(const svtkDiagonalMatrixSource&) = delete;
  void operator=(const svtkDiagonalMatrixSource&) = delete;

  svtkArray* GenerateDenseArray();
  svtkArray* GenerateSparseArray();

  int ArrayType;
  svtkIdType Extents;
  double Diagonal;
  double SuperDiagonal;
  double SubDiagonal;
  char* RowLabel;
  char* ColumnLabel;
};

#endif
