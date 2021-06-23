/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataItem.h"
#include "svtkAbstractMapper.h"
#include "svtkContext2D.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPolyData.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkPolyDataItem);

svtkCxxSetObjectMacro(svtkPolyDataItem, PolyData, svtkPolyData);

svtkCxxSetObjectMacro(svtkPolyDataItem, MappedColors, svtkUnsignedCharArray);

class svtkPolyDataItem::DrawHintsHelper
{

public:
  DrawHintsHelper()
  {
    this->previousLineType = 0;
    this->previousLineWidth = 0.0f;
  }

  /**
   * Retrieve drawing hints as field data from the polydata and use the
   * provided context2D to apply them
   */
  void ApplyDrawHints(svtkContext2D* painter, svtkPolyData* polyData)
  {
    svtkFieldData* fieldData = polyData->GetFieldData();

    svtkIntArray* stippleArray =
      svtkIntArray::SafeDownCast(fieldData->GetAbstractArray("StippleType"));

    svtkFloatArray* lineWidthArray =
      svtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("LineWidth"));

    svtkPen* pen = painter->GetPen();

    this->previousLineType = pen->GetLineType();
    this->previousLineWidth = pen->GetWidth();

    if (stippleArray != nullptr)
    {
      pen->SetLineType(stippleArray->GetValue(0));
    }

    if (lineWidthArray != nullptr)
    {
      pen->SetWidth(lineWidthArray->GetValue(0));
    }
  }

  /**
   * "Un-apply" hints by restoring saved drawing state
   */
  void RemoveDrawHints(svtkContext2D* painter)
  {
    svtkPen* pen = painter->GetPen();
    pen->SetLineType(this->previousLineType);
    pen->SetWidth(this->previousLineWidth);
  };

private:
  DrawHintsHelper(const DrawHintsHelper&) = delete;
  void operator=(const DrawHintsHelper&) = delete;

  int previousLineType;
  float previousLineWidth;
};

//-----------------------------------------------------------------------------
svtkPolyDataItem::svtkPolyDataItem()
  : PolyData(nullptr)
  , MappedColors(nullptr)
  , ScalarMode(SVTK_SCALAR_MODE_USE_POINT_DATA)
{
  this->Position[0] = this->Position[1] = 0;
  this->HintHelper = new svtkPolyDataItem::DrawHintsHelper();
}

//-----------------------------------------------------------------------------
svtkPolyDataItem::~svtkPolyDataItem()
{
  this->SetPolyData(nullptr);
  this->SetMappedColors(nullptr);
  delete this->HintHelper;
}

//-----------------------------------------------------------------------------
bool svtkPolyDataItem::Paint(svtkContext2D* painter)
{
  if (this->PolyData && this->MappedColors)
  {
    this->HintHelper->ApplyDrawHints(painter, this->PolyData);

    // Draw the PolyData in the bottom left corner of the item.
    painter->DrawPolyData(
      this->Position[0], this->Position[1], this->PolyData, this->MappedColors, this->ScalarMode);

    this->HintHelper->RemoveDrawHints(painter);
  }

  return true;
}

//-----------------------------------------------------------------------------
void svtkPolyDataItem::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
