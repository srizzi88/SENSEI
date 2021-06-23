/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPeriodicFiler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkAngularPeriodicFilter.h"

#include "svtkAngularPeriodicDataArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPolyData.h"
#include "svtkStructuredGrid.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"
#include "svtkUnstructuredGrid.h"

#include <sstream>

svtkStandardNewMacro(svtkAngularPeriodicFilter);

//----------------------------------------------------------------------------
svtkAngularPeriodicFilter::svtkAngularPeriodicFilter()
{
  this->ComputeRotationsOnTheFly = true;
  this->RotationMode = SVTK_ROTATION_MODE_DIRECT_ANGLE;
  this->RotationAngle = 180.;
  this->RotationArrayName = nullptr;
  this->RotationAxis = static_cast<int>(SVTK_PERIODIC_ARRAY_AXIS_X);
  this->Center[0] = 0;
  this->Center[1] = 0;
  this->Center[2] = 0;
}

//----------------------------------------------------------------------------
svtkAngularPeriodicFilter::~svtkAngularPeriodicFilter()
{
  this->SetRotationArrayName(nullptr);
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Compute Rotations on-the-fly: " << this->ComputeRotationsOnTheFly << endl;
  if (this->RotationMode == SVTK_ROTATION_MODE_DIRECT_ANGLE)
  {
    os << indent << "Rotation Mode: Direct Angle" << endl;
    os << indent << "Rotation Angle: " << this->RotationAngle << endl;
  }
  else
  {
    os << indent << "Rotation Mode: Array Value" << endl;
    os << indent << "Rotation Angle Array Name: " << this->RotationArrayName << endl;
  }
  switch (this->RotationAxis)
  {
    case SVTK_PERIODIC_ARRAY_AXIS_X:
      os << indent << "Rotation Axis: X" << endl;
      break;
    case SVTK_PERIODIC_ARRAY_AXIS_Y:
      os << indent << "Rotation Axis: Y" << endl;
      break;
    case SVTK_PERIODIC_ARRAY_AXIS_Z:
      os << indent << "Rotation Axis: Z" << endl;
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::SetRotationAxisToX()
{
  this->SetRotationAxis(SVTK_PERIODIC_ARRAY_AXIS_X);
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::SetRotationAxisToY()
{
  this->SetRotationAxis(SVTK_PERIODIC_ARRAY_AXIS_Y);
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::SetRotationAxisToZ()
{
  this->SetRotationAxis(SVTK_PERIODIC_ARRAY_AXIS_Z);
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::CreatePeriodicDataSet(
  svtkCompositeDataIterator* loc, svtkCompositeDataSet* output, svtkCompositeDataSet* input)
{
  svtkDataObject* inputNode = input->GetDataSet(loc);
  svtkNew<svtkMultiPieceDataSet> multiPiece;

  // Number of periods
  int periodsNb = 0;

  // Rotation angle in degree
  double angle = this->GetRotationAngle();
  switch (this->GetRotationMode())
  {
    case SVTK_ROTATION_MODE_DIRECT_ANGLE:
      break;
    case SVTK_ROTATION_MODE_ARRAY_VALUE:
    {
      if (inputNode != nullptr)
      {
        svtkDataArray* angleArray =
          inputNode->GetFieldData()->GetArray(this->GetRotationArrayName());
        if (!angleArray)
        {
          svtkErrorMacro(<< "Bad rotation mode.");
          return;
        }
        double angleRad = angleArray->GetTuple1(0);
        angle = svtkMath::DegreesFromRadians(angleRad);
      }
      else
      {
        angle = 360;
      }
      break;
    }
    default:
    {
      svtkErrorMacro(<< "Bad rotation mode.");
      return;
    }
  }

  switch (this->GetIterationMode())
  {
    case SVTK_ITERATION_MODE_DIRECT_NB:
    {
      periodsNb = this->GetNumberOfPeriods();
      break;
    }

    case SVTK_ITERATION_MODE_MAX:
    {
      periodsNb = static_cast<int>(std::round(360. / std::abs(angle)));
      break;
    }

    default:
    {
      svtkErrorMacro(<< "Bad iteration mode.");
      return;
    }
  }

  multiPiece->SetNumberOfPieces(periodsNb);
  if (periodsNb > 0 && inputNode != nullptr)
  {
    // Shallow copy the first piece, it is not transformed
    svtkDataObject* firstDataSet = inputNode->NewInstance();
    firstDataSet->ShallowCopy(inputNode);
    multiPiece->SetPiece(0, firstDataSet);
    firstDataSet->Delete();
    this->GeneratePieceName(input, loc, multiPiece, 0);

    for (svtkIdType iPiece = 1; iPiece < periodsNb; iPiece++)
    {
      this->AppendPeriodicPiece(angle, iPiece, inputNode, multiPiece);
      this->GeneratePieceName(input, loc, multiPiece, iPiece);
    }
  }
  this->PeriodNumbers.push_back(periodsNb);
  output->SetDataSet(loc, multiPiece);
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::SetPeriodNumber(
  svtkCompositeDataIterator* loc, svtkCompositeDataSet* output, int nbPeriod)
{
  svtkMultiPieceDataSet* mp = svtkMultiPieceDataSet::SafeDownCast(output->GetDataSet(loc));
  if (mp)
  {
    mp->SetNumberOfPieces(nbPeriod);
  }
  else
  {
    svtkErrorMacro(<< "Setting period on a non existent svtkMultiPieceDataSet");
  }
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::AppendPeriodicPiece(
  double angle, svtkIdType iPiece, svtkDataObject* inputNode, svtkMultiPieceDataSet* multiPiece)
{
  svtkPointSet* dataset = svtkPointSet::SafeDownCast(inputNode);
  svtkPointSet* transformedDataset = nullptr;

  int pieceAlterner = ((iPiece % 2) * 2 - 1) * ((iPiece + 1) / 2);
  double pieceAngle = angle * pieceAlterner;

  // MappedData supported type are pointset
  if (dataset)
  {
    transformedDataset = dataset->NewInstance();

    // Transform periodic points and cells
    this->ComputePeriodicMesh(dataset, transformedDataset, pieceAngle);
    multiPiece->SetPiece(iPiece, transformedDataset);
    transformedDataset->Delete();
  }
  else
  {
    // Legacy non mapped code, for unsupported type dataset
    svtkWarningMacro("Unsupported Dataset Type for mapped array, using svtkTransformFilter instead.");
    svtkNew<svtkTransform> transform;
    switch (this->RotationAxis)
    {
      case SVTK_PERIODIC_ARRAY_AXIS_X:
        transform->RotateX(pieceAngle);
        break;
      case SVTK_PERIODIC_ARRAY_AXIS_Y:
        transform->RotateY(pieceAngle);
        break;
      case SVTK_PERIODIC_ARRAY_AXIS_Z:
        transform->RotateZ(pieceAngle);
        break;
    }

    svtkNew<svtkTransformFilter> transformFilter;
    transformFilter->SetInputData(inputNode);
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    multiPiece->SetPiece(iPiece, transformFilter->GetOutput());
  }
}

//----------------------------------------------------------------------------
svtkDataArray* svtkAngularPeriodicFilter::TransformDataArray(
  svtkDataArray* inputArray, double angle, bool useCenter, bool normalize)
{
  svtkDataArray* periodicArray = nullptr;
  switch (inputArray->GetDataType())
  {
    case SVTK_FLOAT:
    {
      svtkAngularPeriodicDataArray<float>* pArray = svtkAngularPeriodicDataArray<float>::New();
      pArray->SetAxis(this->RotationAxis);
      pArray->SetAngle(angle);
      if (useCenter)
      {
        pArray->SetCenter(this->Center);
      }
      pArray->SetNormalize(normalize);
      pArray->InitializeArray(svtkArrayDownCast<svtkFloatArray>(inputArray));
      if (this->ComputeRotationsOnTheFly)
      {
        periodicArray = pArray;
      }
      else
      {
        svtkFloatArray* concrete = svtkFloatArray::New();
        concrete->DeepCopy(pArray); // instantiate the array
        periodicArray = concrete;
        pArray->Delete();
      }
      break;
    }
    case SVTK_DOUBLE:
    {
      svtkAngularPeriodicDataArray<double>* pArray = svtkAngularPeriodicDataArray<double>::New();
      pArray->SetAxis(this->RotationAxis);
      pArray->SetAngle(angle);
      if (useCenter)
      {
        pArray->SetCenter(this->Center);
      }
      pArray->SetNormalize(normalize);
      pArray->InitializeArray(svtkArrayDownCast<svtkDoubleArray>(inputArray));
      if (this->ComputeRotationsOnTheFly)
      {
        periodicArray = pArray;
      }
      else
      {
        svtkDoubleArray* concrete = svtkDoubleArray::New();
        concrete->DeepCopy(pArray); // instantiate the array
        periodicArray = concrete;
        pArray->Delete();
      }
      break;
    }
    default:
    {
      svtkErrorMacro("Unknown data type " << inputArray->GetDataType());
      periodicArray = svtkDataArray::CreateDataArray(inputArray->GetDataType());
      periodicArray->DeepCopy(inputArray);
      break;
    }
  }

  return periodicArray;
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::ComputeAngularPeriodicData(
  svtkDataSetAttributes* data, svtkDataSetAttributes* transformedData, double angle)
{
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
  {
    int attribute = data->IsArrayAnAttribute(i);
    svtkDataArray* array = data->GetArray(i);
    svtkDataArray* transformedArray;
    // Perdiodic copy of vector (3 components) or symmectric tensor (6 component, converted to 9 )
    // or tensor (9 components) data
    int numComp = array->GetNumberOfComponents();
    if (numComp == 3 || numComp == 6 || numComp == 9)
    {
      transformedArray =
        this->TransformDataArray(array, angle, false, attribute == svtkDataSetAttributes::NORMALS);
    }
    else
    {
      transformedArray = array;
      array->Register(nullptr);
    }
    transformedData->AddArray(transformedArray);
    if (attribute >= 0)
    {
      transformedData->SetAttribute(transformedArray, attribute);
    }
    transformedArray->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::ComputePeriodicMesh(
  svtkPointSet* dataset, svtkPointSet* transformedDataset, double angle)
{
  // Shallow copy data structure
  transformedDataset->CopyStructure(dataset);

  // Transform points coordinates array
  svtkPoints* points = dataset->GetPoints();
  if (points != nullptr)
  {
    svtkDataArray* pointArray = dataset->GetPoints()->GetData();
    svtkNew<svtkPoints> rotatedPoints;
    svtkDataArray* transformedArray = this->TransformDataArray(pointArray, angle, true);
    rotatedPoints->SetData(transformedArray);
    transformedArray->Delete();
    // Set the points
    transformedDataset->SetPoints(rotatedPoints);
  }

  // Transform point data
  this->ComputeAngularPeriodicData(
    dataset->GetPointData(), transformedDataset->GetPointData(), angle);

  // Transform cell data
  this->ComputeAngularPeriodicData(
    dataset->GetCellData(), transformedDataset->GetCellData(), angle);

  // Shallow copy field data
  transformedDataset->GetFieldData()->ShallowCopy(dataset->GetFieldData());
}

//----------------------------------------------------------------------------
int svtkAngularPeriodicFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->GetRotationMode() == SVTK_ROTATION_MODE_ARRAY_VALUE &&
    this->GetIterationMode() == SVTK_ITERATION_MODE_MAX)
  {
    this->ReducePeriodNumbers = true;
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkAngularPeriodicFilter::GeneratePieceName(svtkCompositeDataSet* input,
  svtkCompositeDataIterator* inputLoc, svtkMultiPieceDataSet* output, svtkIdType outputId)
{
  svtkDataObjectTree* inputTree = svtkDataObjectTree::SafeDownCast(input);
  if (!inputTree)
  {
    return;
  }
  std::ostringstream ss;
  const char* parentName = inputTree->GetMetaData(inputLoc)->Get(svtkCompositeDataSet::NAME());
  if (parentName)
  {
    ss << parentName;
  }
  else
  {
    ss << "Piece";
  }
  ss << "_period" << outputId;
  output->GetMetaData(outputId)->Set(svtkCompositeDataSet::NAME(), ss.str().c_str());
}
