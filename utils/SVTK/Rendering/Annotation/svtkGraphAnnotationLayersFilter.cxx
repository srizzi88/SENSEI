/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkGraphAnnotationLayersFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "svtkGraphAnnotationLayersFilter.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkAppendPolyData.h"
#include "svtkCellData.h"
#include "svtkConvexHull2D.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkGraphAnnotationLayersFilter);

//-----------------------------------------------------------------------------
svtkGraphAnnotationLayersFilter::svtkGraphAnnotationLayersFilter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);

  this->HullAppend = svtkSmartPointer<svtkAppendPolyData>::New();
  this->OutlineAppend = svtkSmartPointer<svtkAppendPolyData>::New();
  this->ConvexHullFilter = svtkSmartPointer<svtkConvexHull2D>::New();
}

//-----------------------------------------------------------------------------
svtkGraphAnnotationLayersFilter::~svtkGraphAnnotationLayersFilter()
{
  //
}

//-----------------------------------------------------------------------------
int svtkGraphAnnotationLayersFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::OutlineOn()
{
  this->ConvexHullFilter->OutlineOn();
}

void svtkGraphAnnotationLayersFilter::OutlineOff()
{
  this->ConvexHullFilter->OutlineOff();
}

void svtkGraphAnnotationLayersFilter::SetOutline(bool b)
{
  this->ConvexHullFilter->SetOutline(b);
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::SetScaleFactor(double scale)
{
  this->ConvexHullFilter->SetScaleFactor(scale);
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::SetHullShapeToBoundingRectangle()
{
  this->ConvexHullFilter->SetHullShape(svtkConvexHull2D::BoundingRectangle);
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::SetHullShapeToConvexHull()
{
  this->ConvexHullFilter->SetHullShape(svtkConvexHull2D::ConvexHull);
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::SetMinHullSizeInWorld(double size)
{
  this->ConvexHullFilter->SetMinHullSizeInWorld(size);
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::SetMinHullSizeInDisplay(int size)
{
  this->ConvexHullFilter->SetMinHullSizeInDisplay(size);
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::SetRenderer(svtkRenderer* renderer)
{
  this->ConvexHullFilter->SetRenderer(renderer);
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkGraphAnnotationLayersFilter::GetMTime()
{
  if (this->ConvexHullFilter)
  {
    return this->ConvexHullFilter->GetMTime();
  }
  else
  {
    return this->MTime;
  }
}

//-----------------------------------------------------------------------------
int svtkGraphAnnotationLayersFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the input and output.
  svtkInformation* inGraphInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* inLayersInfo = inputVector[1]->GetInformationObject(0);

  svtkGraph* graph = svtkGraph::SafeDownCast(inGraphInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPoints* inputPoints = graph->GetPoints();
  svtkAnnotationLayers* layers =
    svtkAnnotationLayers::SafeDownCast(inLayersInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  svtkInformation* outInfo1 = outputVector->GetInformationObject(1);

  svtkPolyData* outputHull = svtkPolyData::SafeDownCast(outInfo0->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* outputOutline =
    svtkPolyData::SafeDownCast(outInfo1->Get(svtkDataObject::DATA_OBJECT()));

  this->HullAppend->RemoveAllInputs();
  this->OutlineAppend->RemoveAllInputs();

  unsigned numberOfAnnotations = layers->GetNumberOfAnnotations();

  // Generate one hull/polydata per selection node
  svtkIdType hullId = 0;
  for (unsigned annotationId = 0; annotationId < numberOfAnnotations; ++annotationId)
  {
    svtkAnnotation* annotation = layers->GetAnnotation(annotationId);
    if (annotation->GetInformation()->Get(svtkAnnotation::ENABLE()) == 0)
    {
      continue;
    }

    svtkSelection* selection = annotation->GetSelection();
    unsigned numberOfSelectionNodes = selection->GetNumberOfNodes();

    for (unsigned selectionNodeId = 0; selectionNodeId < numberOfSelectionNodes; ++selectionNodeId)
    {
      svtkSmartPointer<svtkPoints> hullPoints = svtkSmartPointer<svtkPoints>::New();

      hullId++;
      svtkSelectionNode* selectionNode = selection->GetNode(selectionNodeId);
      if (selectionNode->GetFieldType() != svtkSelectionNode::VERTEX)
      {
        continue;
      }
      svtkIdTypeArray* vertexIds =
        svtkArrayDownCast<svtkIdTypeArray>(selectionNode->GetSelectionList());

      // Get points from graph
      svtkIdType numberOfNodePoints = vertexIds->GetNumberOfTuples();
      if (numberOfNodePoints == 0)
      {
        continue;
      }
      for (svtkIdType i = 0; i < numberOfNodePoints; ++i)
      {
        hullPoints->InsertNextPoint(inputPoints->GetPoint(vertexIds->GetValue(i)));
      }

      // Create filled polygon
      svtkSmartPointer<svtkPolyData> hullPolyData = svtkSmartPointer<svtkPolyData>::New();
      hullPolyData->SetPoints(hullPoints);
      ConvexHullFilter->SetInputData(hullPolyData);
      ConvexHullFilter->Update();
      hullPolyData->ShallowCopy(ConvexHullFilter->GetOutput());

      // Add data arrays to the polydata
      svtkIdType representativeVertex = vertexIds->GetValue(0);
      svtkIdType numberOfCells = hullPolyData->GetNumberOfCells();

      svtkUnsignedCharArray* outColors = svtkUnsignedCharArray::New();
      outColors->SetNumberOfComponents(4);
      outColors->SetName("Hull color");
      double* color = annotation->GetInformation()->Get(svtkAnnotation::COLOR());
      double opacity = annotation->GetInformation()->Get(svtkAnnotation::OPACITY());
      unsigned char outColor[4] = { static_cast<unsigned char>(color[0] * 255),
        static_cast<unsigned char>(color[1] * 255), static_cast<unsigned char>(color[2] * 255),
        static_cast<unsigned char>(opacity * 255) };
      for (svtkIdType i = 0; i < numberOfCells; ++i)
      {
        outColors->InsertNextTypedTuple(outColor);
      }
      hullPolyData->GetCellData()->AddArray(outColors);
      outColors->Delete();

      svtkIdTypeArray* hullIds = svtkIdTypeArray::New();
      hullIds->SetName("Hull id");
      for (svtkIdType i = 0; i < numberOfCells; ++i)
      {
        hullIds->InsertNextValue(hullId);
      }
      hullPolyData->GetCellData()->AddArray(hullIds);
      hullIds->Delete();

      svtkStringArray* hullName = svtkStringArray::New();
      hullName->SetName("Hull name");
      for (svtkIdType i = 0; i < numberOfCells; ++i)
      {
        hullName->InsertNextValue(annotation->GetInformation()->Get(svtkAnnotation::LABEL()));
      }
      hullPolyData->GetCellData()->AddArray(hullName);
      hullName->Delete();

      svtkDoubleArray* hullCentreVertex = svtkDoubleArray::New();
      hullCentreVertex->SetName("Hull point");
      hullCentreVertex->SetNumberOfComponents(3);
      for (svtkIdType i = 0; i < numberOfCells; ++i)
      {
        hullCentreVertex->InsertNextTuple(inputPoints->GetPoint(representativeVertex));
      }
      hullPolyData->GetCellData()->AddArray(hullCentreVertex);
      hullCentreVertex->Delete();

      this->HullAppend->AddInputData(hullPolyData);

      if (this->ConvexHullFilter->GetOutline())
      {
        svtkSmartPointer<svtkPolyData> outlinePolyData = svtkSmartPointer<svtkPolyData>::New();
        outlinePolyData->ShallowCopy(ConvexHullFilter->GetOutput(1));
        this->OutlineAppend->AddInputData(outlinePolyData);
      }
    } // Next selection node.
  }   // Next annotation.

  // Send data to output
  if (this->HullAppend->GetNumberOfInputConnections(0) > 0)
  {
    this->HullAppend->Update();
    outputHull->ShallowCopy(this->HullAppend->GetOutput());
  }
  if (this->OutlineAppend->GetNumberOfInputConnections(0) > 0)
  {
    this->OutlineAppend->Update();
    outputOutline->ShallowCopy(this->OutlineAppend->GetOutput());
  }
  return 1;
}

//-----------------------------------------------------------------------------
void svtkGraphAnnotationLayersFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConvexHull2D: ";
  if (this->ConvexHullFilter)
  {
    os << endl;
    this->ConvexHullFilter->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
