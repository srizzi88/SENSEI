/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextDevice2D.h"
#include "svtkAbstractMapper.h" // for SVTK_SCALAR_MODE defines
#include "svtkBrush.h"
#include "svtkCellIterator.h"
#include "svtkMathTextUtilities.h"
#include "svtkPen.h"
#include "svtkPolyData.h"
#include "svtkRect.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkUnsignedCharArray.h"

#include "svtkObjectFactory.h"
#include <cassert>
#include <vector>

svtkAbstractObjectFactoryNewMacro(svtkContextDevice2D);

svtkContextDevice2D::svtkContextDevice2D()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->BufferId = nullptr;
  this->Pen = svtkPen::New();
  this->Brush = svtkBrush::New();
  this->TextProp = svtkTextProperty::New();
}

//-----------------------------------------------------------------------------
svtkContextDevice2D::~svtkContextDevice2D()
{
  this->Pen->Delete();
  this->Brush->Delete();
  this->TextProp->Delete();
}

//-----------------------------------------------------------------------------
bool svtkContextDevice2D::MathTextIsSupported()
{
  return svtkMathTextUtilities::GetInstance() != nullptr;
}

//-----------------------------------------------------------------------------
void svtkContextDevice2D::DrawPolyData(
  float p[2], float scale, svtkPolyData* polyData, svtkUnsignedCharArray* colors, int scalarMode)
{
  std::vector<float> verts;
  std::vector<unsigned char> vertColors;

  svtkCellIterator* cell = polyData->NewCellIterator();
  cell->InitTraversal();
  for (; !cell->IsDoneWithTraversal(); cell->GoToNextCell())
  {
    // To match the original implementation on the OpenGL2 backend, we only
    // handle polygons and lines:
    int cellType = cell->GetCellType();
    switch (cellType)
    {
      case SVTK_LINE:
      case SVTK_POLY_LINE:
      case SVTK_TRIANGLE:
      case SVTK_QUAD:
      case SVTK_POLYGON:
        break;

      default:
        continue;
    }

    // Allocate temporary arrays:
    svtkIdType numPoints = cell->GetNumberOfPoints();
    if (numPoints == 0)
    {
      continue;
    }
    verts.resize(static_cast<size_t>(numPoints) * 2);
    vertColors.resize(static_cast<size_t>(numPoints) * 4);

    svtkIdType cellId = cell->GetCellId();
    svtkIdList* pointIds = cell->GetPointIds();
    svtkPoints* points = cell->GetPoints();

    for (svtkIdType i = 0; i < numPoints; ++i)
    {
      const size_t vertsIdx = 2 * static_cast<size_t>(i);
      const size_t colorIdx = 4 * static_cast<size_t>(i);

      const double* point = points->GetPoint(i);
      verts[vertsIdx] = (static_cast<float>(point[0]) + p[0]) * scale;
      verts[vertsIdx + 1] = (static_cast<float>(point[1]) + p[1]) * scale;

      if (scalarMode == SVTK_SCALAR_MODE_USE_POINT_DATA)
      {
        colors->GetTypedTuple(pointIds->GetId(i), vertColors.data() + colorIdx);
      }
      else
      {
        colors->GetTypedTuple(cellId, vertColors.data() + colorIdx);
      }
    }

    if (cellType == SVTK_LINE || cellType == SVTK_POLY_LINE )
    {
      this->DrawPoly(verts.data(), numPoints, vertColors.data(), 4);
    }
    else
    {
      this->DrawColoredPolygon(verts.data(), numPoints, vertColors.data(), 4);
    }
  }
  cell->Delete();
}

//-----------------------------------------------------------------------------
void svtkContextDevice2D::ApplyPen(svtkPen* pen)
{
  this->Pen->DeepCopy(pen);
}

//-----------------------------------------------------------------------------
void svtkContextDevice2D::ApplyBrush(svtkBrush* brush)
{
  this->Brush->DeepCopy(brush);
}

//-----------------------------------------------------------------------------
void svtkContextDevice2D::ApplyTextProp(svtkTextProperty* prop)
{
  // This is a deep copy, but is called shallow for some reason...
  this->TextProp->ShallowCopy(prop);
}

// ----------------------------------------------------------------------------
bool svtkContextDevice2D::GetBufferIdMode() const
{
  return this->BufferId != nullptr;
}

// ----------------------------------------------------------------------------
void svtkContextDevice2D::BufferIdModeBegin(svtkAbstractContextBufferId* bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId != nullptr);

  this->BufferId = bufferId;

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void svtkContextDevice2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  this->BufferId = nullptr;

  assert("post: done" && !this->GetBufferIdMode());
}

//-----------------------------------------------------------------------------
void svtkContextDevice2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Pen: ";
  this->Pen->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Brush: ";
  this->Brush->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Text Property: ";
  this->TextProp->PrintSelf(os, indent.GetNextIndent());
}

//-----------------------------------------------------------------------------
void svtkContextDevice2D::DrawMarkers(int, bool, float*, int, unsigned char*, int) {}

//-----------------------------------------------------------------------------
void svtkContextDevice2D::DrawColoredPolygon(float*, int, unsigned char*, int)
{
  svtkErrorMacro("DrawColoredPolygon not implemented on this device.");
}
