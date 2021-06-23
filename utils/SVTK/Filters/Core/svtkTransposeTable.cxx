/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransposeTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTransposeTable.h"

#include "svtkAbstractArray.h"
#include "svtkCharArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkLongLongArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkShortArray.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedLongLongArray.h"
#include "svtkUnsignedShortArray.h"
#include "svtkVariantArray.h"

#include <sstream>

///////////////////////////////////////////////////////////////////////////////

class svtkTransposeTableInternal
{
public:
  svtkTransposeTableInternal(svtkTransposeTable* parent)
    : Parent(parent)
  {
  }

  bool TransposeTable(svtkTable* inTable, svtkTable* outTable);

protected:
  bool InsertColumn(int, svtkAbstractArray*);

  template <typename ArrayType, typename ValueType>
  bool TransposeColumn(int, bool);

  svtkTransposeTable* Parent;
  svtkTable* InTable;
  svtkTable* OutTable;
};

//----------------------------------------------------------------------------

template <typename ArrayType, typename ValueType>
bool svtkTransposeTableInternal::TransposeColumn(int columnId, bool useVariant)
{
  svtkAbstractArray* column = this->InTable->GetColumn(columnId);
  ArrayType* typeColumn = ArrayType::SafeDownCast(column);
  if (!typeColumn && !useVariant)
  {
    return false;
  }

  int numberOfRowsInTransposedColumn = this->InTable->GetNumberOfColumns();
  if (this->Parent->GetUseIdColumn())
  {
    columnId--;
    numberOfRowsInTransposedColumn--;
  }

  for (int r = 0; r < column->GetNumberOfTuples() * column->GetNumberOfComponents(); ++r)
  {
    svtkSmartPointer<ArrayType> transposedColumn;
    if (columnId == 0)
    {
      transposedColumn = svtkSmartPointer<ArrayType>::New();
      transposedColumn->SetNumberOfValues(numberOfRowsInTransposedColumn);
      this->OutTable->AddColumn(transposedColumn);
    }
    else
    {
      transposedColumn = ArrayType::SafeDownCast(this->OutTable->GetColumn(r));
    }

    if (!useVariant)
    {
      ValueType value = typeColumn->GetValue(r);
      transposedColumn->SetValue(columnId, value);
    }
    else
    {
      svtkVariant value = column->GetVariantValue(r);
      transposedColumn->SetVariantValue(columnId, value);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkTransposeTableInternal::InsertColumn(int pos, svtkAbstractArray* col)
{
  if (!col ||
    ((this->OutTable->GetNumberOfRows() !=
       col->GetNumberOfComponents() * col->GetNumberOfTuples()) &&
      (this->OutTable->GetNumberOfRows() != 0)))
  {
    return false;
  }

  int nbColsOutTable = this->OutTable->GetNumberOfColumns();

  svtkNew<svtkTable> updatedTable;
  for (int c = 0; c < nbColsOutTable; c++)
  {
    svtkAbstractArray* column = this->OutTable->GetColumn(c);
    if (c == pos)
    {
      updatedTable->AddColumn(col);
    }
    updatedTable->AddColumn(column);
  }
  if (pos == nbColsOutTable)
  {
    updatedTable->AddColumn(col);
  }

  this->OutTable->ShallowCopy(updatedTable);

  return true;
}

//----------------------------------------------------------------------------
bool svtkTransposeTableInternal::TransposeTable(svtkTable* inTable, svtkTable* outTable)
{
  this->InTable = inTable;
  this->OutTable = outTable;

  int idColOffset = this->Parent->GetUseIdColumn() ? 1 : 0;

  // Check column type consistency
  bool useVariant = false;
  svtkAbstractArray* firstCol = this->InTable->GetColumn(idColOffset);
  for (int c = idColOffset; c < this->InTable->GetNumberOfColumns(); c++)
  {
    if (strcmp(firstCol->GetClassName(), this->InTable->GetColumn(c)->GetClassName()) != 0)
    {
      useVariant = true;
      break;
    }
  }
  for (int c = idColOffset; c < this->InTable->GetNumberOfColumns(); c++)
  {
    svtkAbstractArray* column = this->InTable->GetColumn(c);
    if (!column)
    {
      return false;
    }
    if (!useVariant)
    {
#define TransposeTypedColumn(_vt, _ta, _t)                                                         \
  case _vt:                                                                                        \
    if (!this->TransposeColumn<_ta, _t>(c, useVariant))                                            \
    {                                                                                              \
      svtkErrorWithObjectMacro(this->Parent, << "Unable to transpose column " << c);                \
      return false;                                                                                \
    }                                                                                              \
    break;

      switch (column->GetDataType())
      {
        TransposeTypedColumn(SVTK_DOUBLE, svtkDoubleArray, double);
        TransposeTypedColumn(SVTK_FLOAT, svtkFloatArray, float);
        TransposeTypedColumn(SVTK_CHAR, svtkCharArray, char);
        TransposeTypedColumn(SVTK_SIGNED_CHAR, svtkSignedCharArray, signed char);
        TransposeTypedColumn(SVTK_SHORT, svtkShortArray, short);
        TransposeTypedColumn(SVTK_INT, svtkIntArray, int);
        TransposeTypedColumn(SVTK_LONG, svtkLongArray, long);
        TransposeTypedColumn(SVTK_LONG_LONG, svtkLongLongArray, long long);
        TransposeTypedColumn(SVTK_UNSIGNED_CHAR, svtkUnsignedCharArray, unsigned char);
        TransposeTypedColumn(SVTK_UNSIGNED_SHORT, svtkUnsignedShortArray, unsigned short);
        TransposeTypedColumn(SVTK_UNSIGNED_INT, svtkUnsignedIntArray, unsigned int);
        TransposeTypedColumn(SVTK_UNSIGNED_LONG, svtkUnsignedLongArray, unsigned long);
        TransposeTypedColumn(SVTK_UNSIGNED_LONG_LONG, svtkUnsignedLongLongArray, unsigned long long);
        TransposeTypedColumn(SVTK_ID_TYPE, svtkIdTypeArray, svtkIdType);
        TransposeTypedColumn(SVTK_STRING, svtkStringArray, svtkStdString);
#undef TransposeTypedColumn
        default:
          useVariant = true;
          break;
      }
    }
    if (useVariant)
    {
      if (!this->TransposeColumn<svtkVariantArray, svtkVariant>(c, useVariant))
      {
        svtkErrorWithObjectMacro(this->Parent, << "Unable to transpose column " << c);
        return false;
      }
    }
  }

  // Compute the number of chars needed to write the largest column id
  std::stringstream ss;
  ss << firstCol->GetNumberOfTuples();
  std::size_t maxBLen = ss.str().length();

  // Set id column on transposed table
  firstCol = this->InTable->GetColumn(0);
  for (int r = 0; r < firstCol->GetNumberOfComponents() * firstCol->GetNumberOfTuples(); r++)
  {
    svtkAbstractArray* destColumn = this->OutTable->GetColumn(r);
    if (this->Parent->GetUseIdColumn())
    {
      destColumn->SetName(firstCol->GetVariantValue(r).ToString());
    }
    else
    {
      // Set the column name to the (padded) row id.
      // We padd ids with 0 to avoid downstream dictionary sort issues.
      std::stringstream ss2;
      ss2 << std::setw(maxBLen) << std::setfill('0');
      ss2 << r;
      destColumn->SetName(ss2.str().c_str());
    }
  }

  // Create and insert the id column
  if (this->Parent->GetAddIdColumn())
  {
    svtkNew<svtkStringArray> stringArray;
    stringArray->SetName(this->Parent->GetUseIdColumn() ? this->InTable->GetColumn(0)->GetName()
                                                        : this->Parent->GetIdColumnName());
    stringArray->SetNumberOfValues(this->InTable->GetNumberOfColumns() - idColOffset);
    for (int c = idColOffset; c < this->InTable->GetNumberOfColumns(); ++c)
    {
      stringArray->SetValue(c - idColOffset, this->InTable->GetColumn(c)->GetName());
    }
    this->InsertColumn(0, stringArray);
  }

  return true;
}

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkTransposeTable);

//----------------------------------------------------------------------------
svtkTransposeTable::svtkTransposeTable()
{
  this->AddIdColumn = true;
  this->UseIdColumn = false;
  this->IdColumnName = nullptr;
  this->SetIdColumnName("ColName");
}

//----------------------------------------------------------------------------
svtkTransposeTable::~svtkTransposeTable()
{
  delete[] IdColumnName;
}

//----------------------------------------------------------------------------
void svtkTransposeTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkTransposeTable::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* inTable = svtkTable::GetData(inputVector[0]);
  svtkTable* outTable = svtkTable::GetData(outputVector, 0);

  if (inTable->GetNumberOfColumns() == 0)
  {
    svtkErrorMacro(<< "svtkTransposeTable requires svtkTable containing at least one column.");
    return 0;
  }

  svtkTransposeTableInternal intern(this);
  return intern.TransposeTable(inTable, outTable) ? 1 : 0;
}
