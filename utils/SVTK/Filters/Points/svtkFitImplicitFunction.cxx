/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFitImplicitFunction.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFitImplicitFunction.h"

#include "svtkImplicitFunction.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"

svtkStandardNewMacro(svtkFitImplicitFunction);
svtkCxxSetObjectMacro(svtkFitImplicitFunction, ImplicitFunction, svtkImplicitFunction);

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
  double Threshold;
  svtkIdType* PointMap;

  ExtractPoints(T* points, svtkImplicitFunction* f, double thresh, svtkIdType* map)
    : Points(points)
    , Function(f)
    , Threshold(thresh)
    , PointMap(map)
  {
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const T* p = this->Points + 3 * ptId;
    svtkIdType* map = this->PointMap + ptId;
    svtkImplicitFunction* f = this->Function;
    double x[3], val;
    double tMin = (-this->Threshold);
    double tMax = this->Threshold;

    for (; ptId < endPtId; ++ptId)
    {
      x[0] = static_cast<double>(*p++);
      x[1] = static_cast<double>(*p++);
      x[2] = static_cast<double>(*p++);

      val = f->FunctionValue(x);
      *map++ = ((val >= tMin && val < tMax) ? 1 : -1);
    }
  }

  static void Execute(svtkFitImplicitFunction* self, svtkIdType numPts, T* points, svtkIdType* map)
  {
    ExtractPoints extract(points, self->GetImplicitFunction(), self->GetThreshold(), map);
    svtkSMPTools::For(0, numPts, extract);
  }

}; // ExtractPoints

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkFitImplicitFunction::svtkFitImplicitFunction()
{
  this->ImplicitFunction = nullptr;
  this->Threshold = 0.01;
}

//----------------------------------------------------------------------------
svtkFitImplicitFunction::~svtkFitImplicitFunction()
{
  this->SetImplicitFunction(nullptr);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
svtkMTimeType svtkFitImplicitFunction::GetMTime()
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
// Traverse all the input points and extract those that lie near the surface
// of an implicit function.
int svtkFitImplicitFunction::FilterPoints(svtkPointSet* input)
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
void svtkFitImplicitFunction::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Implicit Function: " << static_cast<void*>(this->ImplicitFunction) << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
}
