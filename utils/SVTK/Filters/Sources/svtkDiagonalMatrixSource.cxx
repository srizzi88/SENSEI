/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiagonalMatrixSource.cxx

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

#include "svtkDiagonalMatrixSource.h"
#include "svtkDenseArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkDiagonalMatrixSource);

// ----------------------------------------------------------------------

svtkDiagonalMatrixSource::svtkDiagonalMatrixSource()
  : ArrayType(DENSE)
  , Extents(3)
  , Diagonal(1.0)
  , SuperDiagonal(0.0)
  , SubDiagonal(0.0)
  , RowLabel(nullptr)
  , ColumnLabel(nullptr)
{
  this->SetRowLabel("rows");
  this->SetColumnLabel("columns");

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkDiagonalMatrixSource::~svtkDiagonalMatrixSource()
{
  this->SetRowLabel(nullptr);
  this->SetColumnLabel(nullptr);
}

// ----------------------------------------------------------------------

void svtkDiagonalMatrixSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayType: " << this->ArrayType << endl;
  os << indent << "Extents: " << this->Extents << endl;
  os << indent << "Diagonal: " << this->Diagonal << endl;
  os << indent << "SuperDiagonal: " << this->SuperDiagonal << endl;
  os << indent << "SubDiagonal: " << this->SubDiagonal << endl;
  os << indent << "RowLabel: " << (this->RowLabel ? this->RowLabel : "") << endl;
  os << indent << "ColumnLabel: " << (this->ColumnLabel ? this->ColumnLabel : "") << endl;
}

// ----------------------------------------------------------------------

int svtkDiagonalMatrixSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (this->Extents < 1)
  {
    svtkErrorMacro(<< "Invalid matrix extents: " << this->Extents << "x" << this->Extents
                  << " array is not supported.");
    return 0;
  }

  svtkArray* array = nullptr;
  switch (this->ArrayType)
  {
    case DENSE:
      array = this->GenerateDenseArray();
      break;
    case SPARSE:
      array = this->GenerateSparseArray();
      break;
    default:
      svtkErrorMacro(<< "Invalid array type: " << this->ArrayType << ".");
      return 0;
  }

  svtkArrayData* const output = svtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}

svtkArray* svtkDiagonalMatrixSource::GenerateDenseArray()
{
  svtkDenseArray<double>* const array = svtkDenseArray<double>::New();
  array->Resize(svtkArrayExtents::Uniform(2, this->Extents));
  array->SetDimensionLabel(0, this->RowLabel);
  array->SetDimensionLabel(1, this->ColumnLabel);

  array->Fill(0.0);

  if (this->Diagonal != 0.0)
  {
    for (svtkIdType i = 0; i != this->Extents; ++i)
      array->SetValue(svtkArrayCoordinates(i, i), this->Diagonal);
  }

  if (this->SuperDiagonal != 0.0)
  {
    for (svtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->SetValue(svtkArrayCoordinates(i, i + 1), this->SuperDiagonal);
  }

  if (this->SubDiagonal != 0.0)
  {
    for (svtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->SetValue(svtkArrayCoordinates(i + 1, i), this->SubDiagonal);
  }

  return array;
}

svtkArray* svtkDiagonalMatrixSource::GenerateSparseArray()
{
  svtkSparseArray<double>* const array = svtkSparseArray<double>::New();
  array->Resize(svtkArrayExtents::Uniform(2, this->Extents));
  array->SetDimensionLabel(0, this->RowLabel);
  array->SetDimensionLabel(1, this->ColumnLabel);

  if (this->Diagonal != 0.0)
  {
    for (svtkIdType i = 0; i != this->Extents; ++i)
      array->AddValue(svtkArrayCoordinates(i, i), this->Diagonal);
  }

  if (this->SuperDiagonal != 0.0)
  {
    for (svtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->AddValue(svtkArrayCoordinates(i, i + 1), this->SuperDiagonal);
  }

  if (this->SubDiagonal != 0.0)
  {
    for (svtkIdType i = 0; i + 1 != this->Extents; ++i)
      array->AddValue(svtkArrayCoordinates(i + 1, i), this->SubDiagonal);
  }

  return array;
}
