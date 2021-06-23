/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkApplyIcons.cxx

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
#include "svtkApplyIcons.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkCellData.h"
#include "svtkConvertSelection.h"
#include "svtkDataSet.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include <map>

svtkStandardNewMacro(svtkApplyIcons);

class svtkApplyIcons::Internals
{
public:
  std::map<svtkVariant, int> LookupTable;
};

svtkApplyIcons::svtkApplyIcons()
{
  this->Implementation = new Internals();
  this->DefaultIcon = -1;
  this->SelectedIcon = 0;
  this->SetNumberOfInputPorts(2);
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, svtkDataSetAttributes::SCALARS);
  this->UseLookupTable = false;
  this->IconOutputArrayName = nullptr;
  this->SetIconOutputArrayName("svtkApplyIcons icon");
  this->SelectionMode = IGNORE_SELECTION;
  this->AttributeType = svtkDataObject::VERTEX;
}

svtkApplyIcons::~svtkApplyIcons()
{
  delete Implementation;
  this->SetIconOutputArrayName(nullptr);
}

void svtkApplyIcons::SetIconType(svtkVariant v, int icon)
{
  this->Implementation->LookupTable[v] = icon;
}

void svtkApplyIcons::ClearAllIconTypes()
{
  this->Implementation->LookupTable.clear();
}

int svtkApplyIcons::FillInputPortInformation(int port, svtkInformation* info)
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

int svtkApplyIcons::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects.
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* layersInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->IconOutputArrayName)
  {
    svtkErrorMacro("Output array name must be valid");
    return 0;
  }

  // Get the input and output.
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkAnnotationLayers* layers = nullptr;
  if (layersInfo)
  {
    layers = svtkAnnotationLayers::SafeDownCast(layersInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  // Initialize icon array.
  svtkAbstractArray* arr = this->GetInputAbstractArrayToProcess(0, inputVector);
  svtkSmartPointer<svtkIntArray> iconArr = svtkSmartPointer<svtkIntArray>::New();
  iconArr->SetName(this->IconOutputArrayName);

  // If we have an input array, use its attribute type, otherwise use the
  // AttributeType ivar.
  int attribType = this->AttributeType;
  if (arr)
  {
    attribType = output->GetAttributeTypeForArray(arr);
  }

  // Error if the attribute type is not defined on the data.
  if (!output->GetAttributes(attribType))
  {
    svtkErrorMacro("The input array is not found, and the AttributeType parameter is not valid for "
                  "this data object.");
    return 1;
  }

  // Size the array and add it to the correct attributes.
  svtkIdType numTuples = input->GetNumberOfElements(attribType);
  iconArr->SetNumberOfTuples(numTuples);
  output->GetAttributes(attribType)->AddArray(iconArr);

  // Process the icon array.
  if (this->UseLookupTable && arr)
  {
    // Map the data values through the lookup table.
    std::map<svtkVariant, int>::iterator itEnd = this->Implementation->LookupTable.end();
    for (svtkIdType i = 0; i < iconArr->GetNumberOfTuples(); ++i)
    {
      svtkVariant val = arr->GetVariantValue(i);
      int mappedIcon = this->DefaultIcon;
      if (this->Implementation->LookupTable.find(val) != itEnd)
      {
        mappedIcon = this->Implementation->LookupTable[val];
      }
      iconArr->SetValue(i, mappedIcon);
    }
  }
  else if (arr)
  {
    // If no lookup table, pass the input array values.
    for (svtkIdType i = 0; i < iconArr->GetNumberOfTuples(); ++i)
    {
      iconArr->SetValue(i, arr->GetVariantValue(i).ToInt());
    }
  }
  else
  {
    // If no lookup table or array, use default icon.
    for (svtkIdType i = 0; i < iconArr->GetNumberOfTuples(); ++i)
    {
      iconArr->SetValue(i, this->DefaultIcon);
    }
  }

  // Convert to a selection attribute type.
  int attribTypeSel = -1;
  switch (attribType)
  {
    case svtkDataObject::POINT:
      attribTypeSel = svtkSelectionNode::POINT;
      break;
    case svtkDataObject::CELL:
      attribTypeSel = svtkSelectionNode::CELL;
      break;
    case svtkDataObject::VERTEX:
      attribTypeSel = svtkSelectionNode::VERTEX;
      break;
    case svtkDataObject::EDGE:
      attribTypeSel = svtkSelectionNode::EDGE;
      break;
    case svtkDataObject::ROW:
      attribTypeSel = svtkSelectionNode::ROW;
      break;
    case svtkDataObject::FIELD:
      attribTypeSel = svtkSelectionNode::FIELD;
      break;
  }

  if (layers)
  {
    // Set annotated icons.
    svtkSmartPointer<svtkIdTypeArray> list1 = svtkSmartPointer<svtkIdTypeArray>::New();
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
      svtkSelection* sel = ann->GetSelection();
      int curIcon = -1;
      if (ann->GetInformation()->Has(svtkAnnotation::ICON_INDEX()))
      {
        curIcon = ann->GetInformation()->Get(svtkAnnotation::ICON_INDEX());
      }
      else
      {
        continue;
      }
      svtkConvertSelection::GetSelectedItems(sel, input, attribTypeSel, list1);
      svtkIdType numIds = list1->GetNumberOfTuples();
      for (svtkIdType i = 0; i < numIds; ++i)
      {
        if (list1->GetValue(i) >= iconArr->GetNumberOfTuples())
        {
          continue;
        }
        iconArr->SetValue(list1->GetValue(i), curIcon);
      }
    }

    // Set selected icons.
    if (svtkAnnotation* ann = layers->GetCurrentAnnotation())
    {
      svtkSelection* selection = ann->GetSelection();
      list1 = svtkSmartPointer<svtkIdTypeArray>::New();
      int selectedIcon = -1;
      bool changeSelected = false;
      switch (this->SelectionMode)
      {
        case SELECTED_ICON:
        case SELECTED_OFFSET:
          selectedIcon = this->SelectedIcon;
          changeSelected = true;
          break;
        case ANNOTATION_ICON:
          if (ann->GetInformation()->Has(svtkAnnotation::ICON_INDEX()))
          {
            selectedIcon = ann->GetInformation()->Get(svtkAnnotation::ICON_INDEX());
            changeSelected = true;
          }
          break;
      }
      if (changeSelected)
      {
        svtkConvertSelection::GetSelectedItems(selection, input, attribTypeSel, list1);
        svtkIdType numIds = list1->GetNumberOfTuples();
        for (svtkIdType i = 0; i < numIds; ++i)
        {
          if (list1->GetValue(i) >= iconArr->GetNumberOfTuples())
          {
            continue;
          }
          if (this->SelectionMode == SELECTED_OFFSET)
          {
            // Use selected icon as an offset into the icon sheet.
            selectedIcon = iconArr->GetValue(list1->GetValue(i)) + this->SelectedIcon;
          }
          iconArr->SetValue(list1->GetValue(i), selectedIcon);
        }
      }
    } // if changeSelected
  }   // if current ann not nullptr

  return 1;
}

void svtkApplyIcons::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultIcon: " << this->DefaultIcon << endl;
  os << indent << "SelectedIcon: " << this->SelectedIcon << endl;
  os << indent << "UseLookupTable: " << (this->UseLookupTable ? "on" : "off") << endl;
  os << indent << "IconOutputArrayName: "
     << (this->IconOutputArrayName ? this->IconOutputArrayName : "(none)") << endl;
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}
