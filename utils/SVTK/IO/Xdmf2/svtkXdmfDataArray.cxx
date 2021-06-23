/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include "svtkXdmfDataArray.h"

#include "svtkCommand.h"
#include "svtkObjectFactory.h"

#include "svtkCharArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkShortArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedShortArray.h"

#include "svtk_xdmf2.h"
#include SVTKXDMF2_HEADER(XdmfArray.h)

using namespace xdmf2;

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkXdmfDataArray);

//----------------------------------------------------------------------------
svtkXdmfDataArray::svtkXdmfDataArray()
{
  this->Array = nullptr;
  this->svtkArray = nullptr;
}

//----------------------------------------------------------------------------
svtkDataArray* svtkXdmfDataArray::FromXdmfArray(
  char* ArrayName, int CopyShape, int rank, int Components, int MakeCopy)
{
  xdmf2::XdmfArray* array = this->Array;
  XdmfInt64 components = 1;
  XdmfInt64 tuples = 0;
  if (ArrayName != nullptr)
  {
    array = TagNameToArray(ArrayName);
  }
  if (array == nullptr)
  {
    XdmfErrorMessage("Array is nullptr");
    return (nullptr);
  }
  if (this->svtkArray)
  {
    this->svtkArray->Delete();
    this->svtkArray = 0;
  }
  switch (array->GetNumberType())
  {
    case XDMF_INT8_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkCharArray::New();
      }
      break;
    case XDMF_UINT8_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkUnsignedCharArray::New();
      }
      break;
    case XDMF_INT16_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkShortArray::New();
      }
      break;
    case XDMF_UINT16_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkUnsignedShortArray::New();
      }
      break;
    case XDMF_UINT32_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkUnsignedIntArray::New();
      }
      break;
    case XDMF_INT32_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkIntArray::New();
      }
      break;
    case XDMF_INT64_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkLongArray::New();
      }
      break;
    case XDMF_FLOAT32_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkFloatArray::New();
      }
      break;
    case XDMF_FLOAT64_TYPE:
      if (this->svtkArray == nullptr)
      {
        this->svtkArray = svtkDoubleArray::New();
      }
      break;
    default:
      svtkErrorMacro("Cannot create SVTK data array: " << array->GetNumberType());
      return 0;
  }
  if (CopyShape)
  {
    if (array->GetRank() > rank + 1)
    {
      this->svtkArray->Delete();
      this->svtkArray = 0;
      svtkErrorMacro("Rank of Xdmf array is more than 1 + rank of dataset");
      return 0;
    }
    if (array->GetRank() > rank)
    {
      components = array->GetDimension(rank);
    }
    tuples = array->GetNumberOfElements() / components;
    /// this breaks
    components = Components;
    tuples = array->GetNumberOfElements() / components;
    // cout << "Tuples: " << tuples << " components: " << components << endl;
    // cout << "Rank: " << rank << endl;
    this->svtkArray->SetNumberOfComponents(components);
    if (MakeCopy)
      this->svtkArray->SetNumberOfTuples(tuples);
  }
  else
  {
    this->svtkArray->SetNumberOfComponents(1);
    if (MakeCopy)
      this->svtkArray->SetNumberOfTuples(array->GetNumberOfElements());
  }
  // cout << "Number type: " << array->GetNumberType() << endl;
  if (MakeCopy)
  {
    switch (array->GetNumberType())
    {
      case XDMF_INT8_TYPE:
        array->GetValues(
          0, (XDMF_8_INT*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_UINT8_TYPE:
        array->GetValues(
          0, (XDMF_8_U_INT*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_INT16_TYPE:
        array->GetValues(
          0, (XDMF_16_INT*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_UINT16_TYPE:
        array->GetValues(
          0, (XDMF_16_U_INT*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_INT32_TYPE:
        array->GetValues(
          0, (XDMF_32_INT*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_UINT32_TYPE:
        array->GetValues(
          0, (XDMF_32_U_INT*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_INT64_TYPE:
        array->GetValues(
          0, (XDMF_64_INT*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_FLOAT32_TYPE:
        array->GetValues(
          0, (float*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      case XDMF_FLOAT64_TYPE:
        array->GetValues(
          0, (double*)this->svtkArray->GetVoidPointer(0), array->GetNumberOfElements());
        break;
      default:
        if (array->GetNumberOfElements() > 0)
        {
          // cout << "Manual idx" << endl;
          // cout << "Tuples: " << svtkArray->GetNumberOfTuples() << endl;
          // cout << "Components: " << svtkArray->GetNumberOfComponents() << endl;
          // cout << "Elements: " << array->GetNumberOfElements() << endl;
          svtkIdType jj, kk;
          svtkIdType idx = 0;
          for (jj = 0; jj < svtkArray->GetNumberOfTuples(); jj++)
          {
            for (kk = 0; kk < svtkArray->GetNumberOfComponents(); kk++)
            {
              double val = array->GetValueAsFloat64(idx);
              // cout << "Value: " << val << endl;
              svtkArray->SetComponent(jj, kk, val);
              idx++;
            }
          }
        }
        break;
    }
  }
  else
  {
    switch (array->GetNumberType())
    {
      case XDMF_INT8_TYPE:
      {
        svtkCharArray* chara = svtkArrayDownCast<svtkCharArray>(this->svtkArray);
        if (!chara)
        {
          XdmfErrorMessage("Cannot downcast data array");
          return (0);
        }
        chara->SetArray((char*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_UINT8_TYPE:
      {
        svtkUnsignedCharArray* uchara = svtkArrayDownCast<svtkUnsignedCharArray>(this->svtkArray);
        if (!uchara)
        {
          XdmfErrorMessage("Cannot downcast ucharata array");
          return (0);
        }
        uchara->SetArray((unsigned char*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_INT16_TYPE:
      {
        svtkShortArray* shorta = svtkArrayDownCast<svtkShortArray>(this->svtkArray);
        if (!shorta)
        {
          XdmfErrorMessage("Cannot downcast data array");
          return (0);
        }
        shorta->SetArray((short*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_UINT16_TYPE:
      {
        svtkUnsignedShortArray* ushorta = svtkArrayDownCast<svtkUnsignedShortArray>(this->svtkArray);
        if (!ushorta)
        {
          XdmfErrorMessage("Cannot downcast ushortata array");
          return (0);
        }
        ushorta->SetArray((unsigned short*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_INT32_TYPE:
      {
        svtkIntArray* inta = svtkArrayDownCast<svtkIntArray>(this->svtkArray);
        if (!inta)
        {
          XdmfErrorMessage("Cannot downcast intata array");
          return (0);
        }
        inta->SetArray((int*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_UINT32_TYPE:
      {
        svtkUnsignedIntArray* uinta = svtkArrayDownCast<svtkUnsignedIntArray>(this->svtkArray);
        if (!uinta)
        {
          XdmfErrorMessage("Cannot downcast uintata array");
          return (0);
        }
        uinta->SetArray((unsigned int*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_INT64_TYPE:
      {
        svtkLongArray* longa = svtkArrayDownCast<svtkLongArray>(this->svtkArray);
        if (!longa)
        {
          XdmfErrorMessage("Cannot downcast longa array");
          return (0);
        }
        longa->SetArray((long*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_FLOAT32_TYPE:
      {
        svtkFloatArray* floata = svtkArrayDownCast<svtkFloatArray>(this->svtkArray);
        if (!floata)
        {
          XdmfErrorMessage("Cannot downcast floatata array");
          return (0);
        }
        floata->SetArray((float*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      case XDMF_FLOAT64_TYPE:
      {
        svtkDoubleArray* doublea = svtkArrayDownCast<svtkDoubleArray>(this->svtkArray);
        if (!doublea)
        {
          XdmfErrorMessage("Cannot downcast doubleata array");
          return (0);
        }
        doublea->SetArray((double*)array->GetDataPointer(), components * tuples, 0);
      }
      break;
      default:
        XdmfErrorMessage("Can't handle number type");
        return (0);
    }
    array->Reset();
  }
  return (this->svtkArray);
}

//----------------------------------------------------------------------------
char* svtkXdmfDataArray::ToXdmfArray(svtkDataArray* DataArray, int CopyShape)
{
  xdmf2::XdmfArray* array;
  if (DataArray == nullptr)
  {
    DataArray = this->svtkArray;
  }
  if (DataArray == nullptr)
  {
    svtkDebugMacro(<< "Array is nullptr");
    return (nullptr);
  }
  if (this->Array == nullptr)
  {
    this->Array = new xdmf2::XdmfArray();
    switch (DataArray->GetDataType())
    {
      case SVTK_CHAR:
      case SVTK_UNSIGNED_CHAR:
        this->Array->SetNumberType(XDMF_INT8_TYPE);
        break;
      case SVTK_SHORT:
      case SVTK_UNSIGNED_SHORT:
      case SVTK_INT:
      case SVTK_UNSIGNED_INT:
      case SVTK_LONG:
      case SVTK_UNSIGNED_LONG:
        this->Array->SetNumberType(XDMF_INT32_TYPE);
        break;
      case SVTK_FLOAT:
        this->Array->SetNumberType(XDMF_FLOAT32_TYPE);
        break;
      case SVTK_DOUBLE:
        this->Array->SetNumberType(XDMF_FLOAT64_TYPE);
        break;
      default:
        XdmfErrorMessage("Can't handle Data Type");
        return (nullptr);
    }
  }
  array = this->Array;
  if (CopyShape)
  {
    XdmfInt64 Shape[3];

    Shape[0] = DataArray->GetNumberOfTuples();
    Shape[1] = DataArray->GetNumberOfComponents();
    if (Shape[1] == 1)
    {
      array->SetShape(1, Shape);
    }
    else
    {
      array->SetShape(2, Shape);
    }
  }
  switch (array->GetNumberType())
  {
    case XDMF_INT8_TYPE:
      array->SetValues(
        0, (unsigned char*)DataArray->GetVoidPointer(0), array->GetNumberOfElements());
      break;
    case XDMF_INT32_TYPE:
    case XDMF_INT64_TYPE:
      array->SetValues(0, (int*)DataArray->GetVoidPointer(0), array->GetNumberOfElements());
      break;
    case XDMF_FLOAT32_TYPE:
      array->SetValues(0, (float*)DataArray->GetVoidPointer(0), array->GetNumberOfElements());
      break;
    default:
      array->SetValues(0, (double*)DataArray->GetVoidPointer(0), array->GetNumberOfElements());
      break;
  }
  return (array->GetTagName());
}

//------------------------------------------------------------------------------
svtkDataArray* svtkXdmfDataArray::FromArray(void)
{
  return (this->FromXdmfArray());
}

//------------------------------------------------------------------------------
char* svtkXdmfDataArray::ToArray(void)
{
  return (this->ToXdmfArray());
}

//------------------------------------------------------------------------------
void svtkXdmfDataArray::SetArray(char* TagName)
{
  this->Array = TagNameToArray(TagName);
  if (this->Array)
  {
    this->FromXdmfArray();
  }
}

//------------------------------------------------------------------------------
char* svtkXdmfDataArray::GetArray(void)
{
  if (this->Array != nullptr)
  {
    return (this->Array->GetTagName());
  }
  return (nullptr);
}

//------------------------------------------------------------------------------
void svtkXdmfDataArray::SetVtkArray(svtkDataArray* array)
{
  this->svtkArray = array;
  this->ToXdmfArray(array);
}

//------------------------------------------------------------------------------
svtkDataArray* svtkXdmfDataArray::GetVtkArray(void)
{
  return (this->svtkArray);
}

//----------------------------------------------------------------------------
void svtkXdmfDataArray::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
