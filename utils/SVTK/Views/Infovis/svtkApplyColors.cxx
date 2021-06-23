/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkApplyColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkApplyColors.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkCellData.h"
#include "svtkConvertSelection.h"
#include "svtkDataSet.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkScalarsToColors.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkApplyColors);
svtkCxxSetObjectMacro(svtkApplyColors, PointLookupTable, svtkScalarsToColors);
svtkCxxSetObjectMacro(svtkApplyColors, CellLookupTable, svtkScalarsToColors);

svtkApplyColors::svtkApplyColors()
{
  this->PointLookupTable = nullptr;
  this->CellLookupTable = nullptr;
  this->DefaultPointColor[0] = 0.0;
  this->DefaultPointColor[1] = 0.0;
  this->DefaultPointColor[2] = 0.0;
  this->DefaultPointOpacity = 1.0;
  this->DefaultCellColor[0] = 0.0;
  this->DefaultCellColor[1] = 0.0;
  this->DefaultCellColor[2] = 0.0;
  this->DefaultCellOpacity = 1.0;
  this->SelectedPointColor[0] = 0.0;
  this->SelectedPointColor[1] = 0.0;
  this->SelectedPointColor[2] = 0.0;
  this->SelectedPointOpacity = 1.0;
  this->SelectedCellColor[0] = 0.0;
  this->SelectedCellColor[1] = 0.0;
  this->SelectedCellColor[2] = 0.0;
  this->SelectedCellOpacity = 1.0;
  this->SetNumberOfInputPorts(2);
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, svtkDataSetAttributes::SCALARS);
  this->SetInputArrayToProcess(
    1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_EDGES, svtkDataSetAttributes::SCALARS);
  this->ScalePointLookupTable = true;
  this->ScaleCellLookupTable = true;
  this->UsePointLookupTable = false;
  this->UseCellLookupTable = false;
  this->PointColorOutputArrayName = nullptr;
  this->CellColorOutputArrayName = nullptr;
  this->SetPointColorOutputArrayName("svtkApplyColors color");
  this->SetCellColorOutputArrayName("svtkApplyColors color");
  this->UseCurrentAnnotationColor = false;
}

svtkApplyColors::~svtkApplyColors()
{
  this->SetPointLookupTable(nullptr);
  this->SetCellLookupTable(nullptr);
  this->SetPointColorOutputArrayName(nullptr);
  this->SetCellColorOutputArrayName(nullptr);
}

int svtkApplyColors::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

int svtkApplyColors::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* layersInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->PointColorOutputArrayName || !this->CellColorOutputArrayName)
  {
    svtkErrorMacro("Point and cell array names must be valid");
    return 0;
  }

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkAnnotationLayers* layers = nullptr;
  if (layersInfo)
  {
    layers = svtkAnnotationLayers::SafeDownCast(layersInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  svtkGraph* graph = svtkGraph::SafeDownCast(output);
  svtkDataSet* dataSet = svtkDataSet::SafeDownCast(output);
  svtkTable* table = svtkTable::SafeDownCast(output);

  // initialize color arrays
  svtkSmartPointer<svtkUnsignedCharArray> colorArr1 = svtkSmartPointer<svtkUnsignedCharArray>::New();
  colorArr1->SetName(this->PointColorOutputArrayName);
  colorArr1->SetNumberOfComponents(4);
  if (graph)
  {
    colorArr1->SetNumberOfTuples(graph->GetNumberOfVertices());
    graph->GetVertexData()->AddArray(colorArr1);
  }
  else if (dataSet)
  {
    colorArr1->SetNumberOfTuples(dataSet->GetNumberOfPoints());
    dataSet->GetPointData()->AddArray(colorArr1);
  }
  else
  {
    colorArr1->SetNumberOfTuples(table->GetNumberOfRows());
    table->AddColumn(colorArr1);
  }
  svtkSmartPointer<svtkUnsignedCharArray> colorArr2 = svtkSmartPointer<svtkUnsignedCharArray>::New();
  colorArr2->SetName(this->CellColorOutputArrayName);
  colorArr2->SetNumberOfComponents(4);
  if (graph)
  {
    colorArr2->SetNumberOfTuples(graph->GetNumberOfEdges());
    graph->GetEdgeData()->AddArray(colorArr2);
  }
  else if (dataSet)
  {
    colorArr2->SetNumberOfTuples(dataSet->GetNumberOfCells());
    dataSet->GetCellData()->AddArray(colorArr2);
  }

  unsigned char pointColor[4];
  pointColor[0] = static_cast<unsigned char>(255 * this->DefaultPointColor[0]);
  pointColor[1] = static_cast<unsigned char>(255 * this->DefaultPointColor[1]);
  pointColor[2] = static_cast<unsigned char>(255 * this->DefaultPointColor[2]);
  pointColor[3] = static_cast<unsigned char>(255 * this->DefaultPointOpacity);
  svtkAbstractArray* arr1 = nullptr;
  if (this->PointLookupTable && this->UsePointLookupTable)
  {
    arr1 = this->GetInputAbstractArrayToProcess(0, inputVector);
  }
  this->ProcessColorArray(
    colorArr1, this->PointLookupTable, arr1, pointColor, this->ScalePointLookupTable);

  unsigned char cellColor[4];
  cellColor[0] = static_cast<unsigned char>(255 * this->DefaultCellColor[0]);
  cellColor[1] = static_cast<unsigned char>(255 * this->DefaultCellColor[1]);
  cellColor[2] = static_cast<unsigned char>(255 * this->DefaultCellColor[2]);
  cellColor[3] = static_cast<unsigned char>(255 * this->DefaultCellOpacity);
  svtkAbstractArray* arr2 = nullptr;
  if (this->CellLookupTable && this->UseCellLookupTable)
  {
    arr2 = this->GetInputAbstractArrayToProcess(1, inputVector);
  }
  this->ProcessColorArray(
    colorArr2, this->CellLookupTable, arr2, cellColor, this->ScaleCellLookupTable);

  if (layers)
  {
    svtkSmartPointer<svtkIdTypeArray> list1 = svtkSmartPointer<svtkIdTypeArray>::New();
    svtkSmartPointer<svtkIdTypeArray> list2 = svtkSmartPointer<svtkIdTypeArray>::New();
    unsigned char annColor[4] = { 0, 0, 0, 0 };
    unsigned char prev[4] = { 0, 0, 0, 0 };
    unsigned int numAnnotations = layers->GetNumberOfAnnotations();
    for (unsigned int a = 0; a < numAnnotations; ++a)
    {
      svtkAnnotation* ann = layers->GetAnnotation(a);
      if (ann->GetInformation()->Has(svtkAnnotation::ENABLE()) &&
        ann->GetInformation()->Get(svtkAnnotation::ENABLE()) == 0)
      {
        continue;
      }
      list1->Initialize();
      list2->Initialize();
      svtkSelection* sel = ann->GetSelection();
      bool hasColor = false;
      bool hasOpacity = false;
      if (ann->GetInformation()->Has(svtkAnnotation::COLOR()))
      {
        hasColor = true;
        double* color = ann->GetInformation()->Get(svtkAnnotation::COLOR());
        annColor[0] = static_cast<unsigned char>(255 * color[0]);
        annColor[1] = static_cast<unsigned char>(255 * color[1]);
        annColor[2] = static_cast<unsigned char>(255 * color[2]);
      }
      if (ann->GetInformation()->Has(svtkAnnotation::OPACITY()))
      {
        hasOpacity = true;
        double opacity = ann->GetInformation()->Get(svtkAnnotation::OPACITY());
        annColor[3] = static_cast<unsigned char>(255 * opacity);
      }
      if (!hasColor && !hasOpacity)
      {
        continue;
      }
      if (graph)
      {
        svtkConvertSelection::GetSelectedVertices(sel, graph, list1);
        svtkConvertSelection::GetSelectedEdges(sel, graph, list2);
      }
      else if (dataSet)
      {
        svtkConvertSelection::GetSelectedPoints(sel, dataSet, list1);
        svtkConvertSelection::GetSelectedCells(sel, dataSet, list2);
      }
      else
      {
        svtkConvertSelection::GetSelectedRows(sel, table, list1);
      }
      svtkIdType numIds = list1->GetNumberOfTuples();
      unsigned char curColor[4];
      for (svtkIdType i = 0; i < numIds; ++i)
      {
        if (list1->GetValue(i) >= colorArr1->GetNumberOfTuples())
        {
          continue;
        }
        colorArr1->GetTypedTuple(list1->GetValue(i), prev);
        if (hasColor)
        {
          curColor[0] = annColor[0];
          curColor[1] = annColor[1];
          curColor[2] = annColor[2];
        }
        else
        {
          curColor[0] = prev[0];
          curColor[1] = prev[1];
          curColor[2] = prev[2];
        }
        if (hasOpacity)
        {
          // Combine opacities
          curColor[3] = static_cast<unsigned char>((prev[3] / 255.0) * annColor[3]);
        }
        else
        {
          curColor[3] = prev[3];
        }
        colorArr1->SetTypedTuple(list1->GetValue(i), curColor);
      }
      numIds = list2->GetNumberOfTuples();
      for (svtkIdType i = 0; i < numIds; ++i)
      {
        if (list2->GetValue(i) >= colorArr2->GetNumberOfTuples())
        {
          continue;
        }
        colorArr2->GetTypedTuple(list2->GetValue(i), prev);
        if (hasColor)
        {
          curColor[0] = annColor[0];
          curColor[1] = annColor[1];
          curColor[2] = annColor[2];
        }
        else
        {
          curColor[0] = prev[0];
          curColor[1] = prev[1];
          curColor[2] = prev[2];
        }
        if (hasOpacity)
        {
          // Combine opacities
          curColor[3] = static_cast<unsigned char>((prev[3] / 255.0) * annColor[3]);
        }
        else
        {
          curColor[3] = prev[3];
        }
        colorArr2->SetTypedTuple(list2->GetValue(i), curColor);
      }
    }
    if (svtkAnnotation* ann = layers->GetCurrentAnnotation())
    {
      svtkSelection* selection = ann->GetSelection();
      list1 = svtkSmartPointer<svtkIdTypeArray>::New();
      list2 = svtkSmartPointer<svtkIdTypeArray>::New();
      unsigned char color1[4] = { 0, 0, 0, 255 };
      unsigned char color2[4] = { 0, 0, 0, 255 };
      if (this->UseCurrentAnnotationColor)
      {
        if (ann->GetInformation()->Has(svtkAnnotation::COLOR()))
        {
          double* color = ann->GetInformation()->Get(svtkAnnotation::COLOR());
          color1[0] = static_cast<unsigned char>(255 * color[0]);
          color1[1] = static_cast<unsigned char>(255 * color[1]);
          color1[2] = static_cast<unsigned char>(255 * color[2]);
        }
        if (ann->GetInformation()->Has(svtkAnnotation::OPACITY()))
        {
          double opacity = ann->GetInformation()->Get(svtkAnnotation::OPACITY());
          color1[3] = static_cast<unsigned char>(255 * opacity);
        }
        for (int c = 0; c < 4; ++c)
        {
          color2[c] = color1[c];
        }
      }
      else
      {
        color1[0] = static_cast<unsigned char>(255 * this->SelectedPointColor[0]);
        color1[1] = static_cast<unsigned char>(255 * this->SelectedPointColor[1]);
        color1[2] = static_cast<unsigned char>(255 * this->SelectedPointColor[2]);
        color1[3] = static_cast<unsigned char>(255 * this->SelectedPointOpacity);
        color2[0] = static_cast<unsigned char>(255 * this->SelectedCellColor[0]);
        color2[1] = static_cast<unsigned char>(255 * this->SelectedCellColor[1]);
        color2[2] = static_cast<unsigned char>(255 * this->SelectedCellColor[2]);
        color2[3] = static_cast<unsigned char>(255 * this->SelectedCellOpacity);
      }
      if (graph)
      {
        svtkConvertSelection::GetSelectedVertices(selection, graph, list1);
        svtkConvertSelection::GetSelectedEdges(selection, graph, list2);
      }
      else if (dataSet)
      {
        svtkConvertSelection::GetSelectedPoints(selection, dataSet, list1);
        svtkConvertSelection::GetSelectedCells(selection, dataSet, list2);
      }
      else
      {
        svtkConvertSelection::GetSelectedRows(selection, table, list1);
      }
      svtkIdType numIds = list1->GetNumberOfTuples();
      for (svtkIdType i = 0; i < numIds; ++i)
      {
        if (list1->GetValue(i) >= colorArr1->GetNumberOfTuples())
        {
          continue;
        }
        colorArr1->SetTypedTuple(list1->GetValue(i), color1);
      }
      numIds = list2->GetNumberOfTuples();
      for (svtkIdType i = 0; i < numIds; ++i)
      {
        if (list2->GetValue(i) >= colorArr2->GetNumberOfTuples())
        {
          continue;
        }
        colorArr2->SetTypedTuple(list2->GetValue(i), color2);
      }
    }
  } // end if (layers)

  return 1;
}

void svtkApplyColors::ProcessColorArray(svtkUnsignedCharArray* colorArr, svtkScalarsToColors* lut,
  svtkAbstractArray* arr, unsigned char color[4], bool scaleToArray)
{
  if (lut && arr)
  {
    // If scaling is on, use data min/max.
    // Otherwise, use the lookup table range.
    const double* rng = lut->GetRange();
    double minVal = rng[0];
    double maxVal = rng[1];
    if (scaleToArray)
    {
      minVal = SVTK_DOUBLE_MAX;
      maxVal = SVTK_DOUBLE_MIN;
      for (svtkIdType i = 0; i < colorArr->GetNumberOfTuples(); ++i)
      {
        double val = arr->GetVariantValue(i).ToDouble();
        if (val > maxVal)
        {
          maxVal = val;
        }
        if (val < minVal)
        {
          minVal = val;
        }
      }
    }

    // Map the data values through the lookup table.
    double scale = 1.0;
    if (minVal != maxVal)
    {
      scale = (rng[1] - rng[0]) / (maxVal - minVal);
    }
    unsigned char myColor[4] = { 0, 0, 0, 0 };
    for (svtkIdType i = 0; i < colorArr->GetNumberOfTuples(); ++i)
    {
      double val = arr->GetVariantValue(i).ToDouble();
      const unsigned char* mappedColor = lut->MapValue(rng[0] + scale * (val - minVal));
      myColor[0] = mappedColor[0];
      myColor[1] = mappedColor[1];
      myColor[2] = mappedColor[2];
      // Combine the opacity of the lookup table with the
      // default color opacity.
      myColor[3] = static_cast<unsigned char>((color[3] / 255.0) * mappedColor[3]);
      colorArr->SetTypedTuple(i, myColor);
    }
  }
  else
  {
    // If no lookup table, use default color.
    for (svtkIdType i = 0; i < colorArr->GetNumberOfTuples(); ++i)
    {
      colorArr->SetTypedTuple(i, color);
    }
  }
}

svtkMTimeType svtkApplyColors::GetMTime()
{
  svtkMTimeType mtime = Superclass::GetMTime();
  if (this->PointLookupTable && this->PointLookupTable->GetMTime() > mtime)
  {
    mtime = this->PointLookupTable->GetMTime();
  }
  if (this->CellLookupTable && this->CellLookupTable->GetMTime() > mtime)
  {
    mtime = this->CellLookupTable->GetMTime();
  }
  return mtime;
}

void svtkApplyColors::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PointLookupTable: " << (this->PointLookupTable ? "" : "(none)") << endl;
  if (this->PointLookupTable)
  {
    this->PointLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "CellLookupTable: " << (this->CellLookupTable ? "" : "(none)") << endl;
  if (this->CellLookupTable)
  {
    this->CellLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "DefaultPointColor: " << this->DefaultPointColor[0] << ","
     << this->DefaultPointColor[1] << "," << this->DefaultPointColor[2] << endl;
  os << indent << "DefaultPointOpacity: " << this->DefaultPointOpacity << endl;
  os << indent << "DefaultCellColor: " << this->DefaultCellColor[0] << ","
     << this->DefaultCellColor[1] << "," << this->DefaultCellColor[2] << endl;
  os << indent << "DefaultCellOpacity: " << this->DefaultCellOpacity << endl;
  os << indent << "SelectedPointColor: " << this->SelectedPointColor[0] << ","
     << this->SelectedPointColor[1] << "," << this->SelectedPointColor[2] << endl;
  os << indent << "SelectedPointOpacity: " << this->SelectedPointOpacity << endl;
  os << indent << "SelectedCellColor: " << this->SelectedCellColor[0] << ","
     << this->SelectedCellColor[1] << "," << this->SelectedCellColor[2] << endl;
  os << indent << "SelectedCellOpacity: " << this->SelectedCellOpacity << endl;
  os << indent << "ScalePointLookupTable: " << (this->ScalePointLookupTable ? "on" : "off") << endl;
  os << indent << "ScaleCellLookupTable: " << (this->ScaleCellLookupTable ? "on" : "off") << endl;
  os << indent << "UsePointLookupTable: " << (this->UsePointLookupTable ? "on" : "off") << endl;
  os << indent << "UseCellLookupTable: " << (this->UseCellLookupTable ? "on" : "off") << endl;
  os << indent << "PointColorOutputArrayName: "
     << (this->PointColorOutputArrayName ? this->PointColorOutputArrayName : "(none)") << endl;
  os << indent << "CellColorOutputArrayName: "
     << (this->CellColorOutputArrayName ? this->CellColorOutputArrayName : "(none)") << endl;
  os << indent << "UseCurrentAnnotationColor: " << (this->UseCurrentAnnotationColor ? "on" : "off")
     << endl;
}
