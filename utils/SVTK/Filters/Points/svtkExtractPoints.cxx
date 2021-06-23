/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPoints.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractPoints.h"

#include "svtkImplicitFunction.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"

svtkStandardNewMacro(svtkExtractPoints);
svtkCxxSetObjectMacro(svtkExtractPoints, ImplicitFunction, svtkImplicitFunction);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm
template <typename T>
struct ExtractPoints
{
  const T* Points;
  svtkImplicitFunction* Function;
  bool ExtractInside;
  svtkIdType* PointMap;

  ExtractPoints(T* points, svtkImplicitFunction* f, bool inside, svtkIdType* map)
    : Points(points)
    , Function(f)
    , ExtractInside(inside)
    , PointMap(map)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const T* p = this->Points + 3 * ptId;
    svtkIdType* map = this->PointMap + ptId;
    svtkImplicitFunction* f = this->Function;
    double x[3];
    double inside = (this->ExtractInside ? 1.0 : -1.0);

    for (; ptId < endPtId; ++ptId)
    {
      x[0] = static_cast<double>(*p++);
      x[1] = static_cast<double>(*p++);
      x[2] = static_cast<double>(*p++);

      *map++ = ((f->FunctionValue(x) * inside) <= 0.0 ? 1 : -1);
    }
  }

  static void Execute(svtkExtractPoints* self, svtkIdType numPts, T* points, svtkIdType* map)
  {
    ExtractPoints extract(points, self->GetImplicitFunction(), self->GetExtractInside(), map);
    svtkSMPTools::For(0, numPts, extract);
  }

}; // ExtractPoints

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkExtractPoints::svtkExtractPoints()
{
  this->ImplicitFunction = nullptr;
  this->ExtractInside = true;
}

//----------------------------------------------------------------------------
svtkExtractPoints::~svtkExtractPoints()
{
  this->SetImplicitFunction(nullptr);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
svtkMTimeType svtkExtractPoints::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
// Traverse all the input points and extract points that are contained within
// and implicit function.
int svtkExtractPoints::FilterPoints(svtkPointSet* input)
{
  // Check the input.
  if (!this->ImplicitFunction)
  {
    svtkErrorMacro(<< "Implicit function required\n");
    return 0;
  }

  // Determine which points, if any, should be removed. We create a map
  // to keep track. The bulk of the algorithmic work is done in this pass.
  svtkIdType numPts = input->GetNumberOfPoints();
  void* inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    svtkTemplateMacro(ExtractPoints<SVTK_TT>::Execute(this, numPts, (SVTK_TT*)inPtr, this->PointMap));
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Implicit Function: " << static_cast<void*>(this->ImplicitFunction) << "\n";
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
}
