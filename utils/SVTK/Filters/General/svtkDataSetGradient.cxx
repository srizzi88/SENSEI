/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetGradient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .SECTION Thanks
// This file is part of the generalized Youngs material interface reconstruction algorithm
// contributed by CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
// F-91297 Arpajon, France. <br> Implementation by Thierry Carrard (CEA)

#include "svtkDataSetGradient.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetGradientPrecompute.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

// utility macros
#define ADD_VEC(a, b)                                                                              \
  a[0] += b[0];                                                                                    \
  a[1] += b[1];                                                                                    \
  a[2] += b[2]
#define SCALE_VEC(a, b)                                                                            \
  a[0] *= b;                                                                                       \
  a[1] *= b;                                                                                       \
  a[2] *= b
#define ZERO_VEC(a)                                                                                \
  a[0] = 0;                                                                                        \
  a[1] = 0;                                                                                        \
  a[2] = 0
#define COPY_VEC(a, b)                                                                             \
  a[0] = b[0];                                                                                     \
  a[1] = b[1];                                                                                     \
  a[2] = b[2];
#define MAX_CELL_POINTS 128
#define SVTK_CQS_EPSILON 1e-12

// standard constructors and factory
svtkStandardNewMacro(svtkDataSetGradient);

/*!
  The default constructor
  \sa ~svtkDataSetGradient()
*/
svtkDataSetGradient::svtkDataSetGradient()
  : ResultArrayName(nullptr)
{
  this->SetResultArrayName("gradient");
}

/*!
  The destructor
  \sa svtkDataSetGradient()
*/
svtkDataSetGradient::~svtkDataSetGradient()
{
  this->SetResultArrayName(nullptr);
}

void svtkDataSetGradient::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Result array name: " << this->ResultArrayName << "\n";
}

int svtkDataSetGradient::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get connected input & output
  svtkDataSet* _output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* _input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (_input == nullptr || _output == nullptr)
  {
    svtkErrorMacro(<< "Missing input or output \n");
    return 0;
  }

  // get array to compute gradient from
  svtkDataArray* inArray = this->GetInputArrayToProcess(0, _input);
  if (inArray == nullptr)
  {
    inArray = _input->GetPointData()->GetScalars();
  }
  if (inArray == nullptr)
  {
    inArray = _input->GetCellData()->GetScalars();
  }

  if (inArray == nullptr)
  {
    svtkErrorMacro(<< "no input array to process\n");
    return 0;
  }

  svtkDebugMacro(<< "Input array to process : " << inArray->GetName() << "\n");

  bool pointData;
  if (_input->GetCellData()->GetArray(inArray->GetName()) == inArray)
  {
    pointData = false;
    svtkDebugMacro(<< "cell data to point gradient\n");
  }
  else if (_input->GetPointData()->GetArray(inArray->GetName()) == inArray)
  {
    pointData = true;
    svtkDebugMacro(<< "point data to cell gradient\n");
  }
  else
  {
    svtkErrorMacro(<< "input array must be cell or point data\n");
    return 0;
  }

  // we're just adding a scalar field
  _output->ShallowCopy(_input);

  svtkDataArray* cqsArray = _output->GetFieldData()->GetArray("GradientPrecomputation");
  svtkDataArray* sizeArray = _output->GetCellData()->GetArray("CellSize");
  if (cqsArray == nullptr || sizeArray == nullptr)
  {
    svtkDebugMacro(
      << "Couldn't find field array 'GradientPrecomputation', computing it right now.\n");
    svtkDataSetGradientPrecompute::GradientPrecompute(_output);
    cqsArray = _output->GetFieldData()->GetArray("GradientPrecomputation");
    sizeArray = _output->GetCellData()->GetArray("CellSize");
    if (cqsArray == nullptr || sizeArray == nullptr)
    {
      svtkErrorMacro(
        << "Computation of field array 'GradientPrecomputation' or 'CellSize' failed.\n");
      return 0;
    }
  }

  svtkIdType nCells = _input->GetNumberOfCells();
  svtkIdType nPoints = _input->GetNumberOfPoints();

  svtkDoubleArray* gradientArray = svtkDoubleArray::New();
  gradientArray->SetName(this->ResultArrayName);
  gradientArray->SetNumberOfComponents(3);

  if (pointData) // compute cell gradient from point data
  {
    gradientArray->SetNumberOfTuples(nCells);
    svtkIdType cellPoint = 0;
    for (svtkIdType i = 0; i < nCells; i++)
    {
      svtkCell* cell = _input->GetCell(i);
      int np = cell->GetNumberOfPoints();
      double gradient[3] = { 0, 0, 0 };
      for (int p = 0; p < np; p++)
      {
        double cqs[3], scalar;
        cqsArray->GetTuple(cellPoint++, cqs);
        scalar = inArray->GetTuple1(cell->GetPointId(p));
        SCALE_VEC(cqs, scalar);
        ADD_VEC(gradient, cqs);
      }
      SCALE_VEC(gradient, (1.0 / sizeArray->GetTuple1(i)));
      gradientArray->SetTuple(i, gradient);
    }

    _output->GetCellData()->AddArray(gradientArray);
    //_output->GetCellData()->SetVectors( gradientArray );
  }
  else // compute point gradient from cell data
  {
    gradientArray->SetNumberOfTuples(nPoints);
    gradientArray->FillComponent(0, 0.0);
    gradientArray->FillComponent(1, 0.0);
    gradientArray->FillComponent(2, 0.0);
    double* gradient = gradientArray->WritePointer(0, nPoints * 3);
    double* gradientDivisor = new double[nPoints];
    for (svtkIdType i = 0; i < nPoints; i++)
    {
      gradientDivisor[i] = 0.0;
    }
    svtkIdType cellPoint = 0;
    for (svtkIdType i = 0; i < nCells; i++)
    {
      svtkCell* cell = _input->GetCell(i);
      int np = cell->GetNumberOfPoints();
      double scalar = inArray->GetTuple1(i);
      for (int p = 0; p < np; p++)
      {
        double cqs[3];
        double pointCoord[3];
        svtkIdType pointId = cell->GetPointId(p);
        cqsArray->GetTuple(cellPoint++, cqs);
        _input->GetPoint(cell->GetPointId(p), pointCoord);
        scalar *= cell->GetCellDimension();
        SCALE_VEC((gradient + pointId * 3), scalar);
        gradientDivisor[pointId] += svtkMath::Dot(cqs, pointCoord);
      }
    }
    for (svtkIdType i = 0; i < nPoints; i++)
    {
      SCALE_VEC((gradient + i * 3), (1.0 / gradientDivisor[i]));
    }
    delete[] gradientDivisor;
    _output->GetPointData()->AddArray(gradientArray);
    //_output->GetPointData()->SetVectors( gradientArray );
  }
  gradientArray->Delete();

  svtkDebugMacro(<< _output->GetClassName() << " @ " << _output << " :\n");

  return 1;
}
