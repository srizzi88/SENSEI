/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabeledDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLabeledDataMapper.h"

#include "svtkActor2D.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPlaneCollection.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkTransform.h"
#include "svtkTypeTraits.h"
#include "svtkUnicodeStringArray.h"

#include <map>

class svtkLabeledDataMapper::Internals
{
public:
  std::map<int, svtkSmartPointer<svtkTextProperty> > TextProperties;
};

svtkStandardNewMacro(svtkLabeledDataMapper);

svtkCxxSetObjectMacro(svtkLabeledDataMapper, Transform, svtkTransform);

// ----------------------------------------------------------------------

template <typename T>
void svtkLabeledDataMapper_PrintComponent(
  char* output, size_t outputSize, const char* format, int index, const T* array)
{
  snprintf(output, outputSize, format, array[index]);
}

//----------------------------------------------------------------------------
// Creates a new label mapper

svtkLabeledDataMapper::svtkLabeledDataMapper()
{
  this->Implementation = new Internals;

  this->Input = nullptr;
  this->LabelMode = SVTK_LABEL_IDS;

  this->LabelFormat = nullptr;

  this->LabeledComponent = (-1);
  this->FieldDataArray = 0;
  this->FieldDataName = nullptr;

  this->NumberOfLabels = 0;
  this->NumberOfLabelsAllocated = 0;

  this->LabelPositions = nullptr;
  this->TextMappers = nullptr;
  this->AllocateLabels(50);

  this->ComponentSeparator = ' ';

  svtkSmartPointer<svtkTextProperty> prop = svtkSmartPointer<svtkTextProperty>::New();
  prop->SetFontSize(12);
  prop->SetBold(1);
  prop->SetItalic(1);
  prop->SetShadow(1);
  prop->SetFontFamilyToArial();
  this->Implementation->TextProperties[0] = prop;

  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "type");

  this->Transform = nullptr;
  this->CoordinateSystem = svtkLabeledDataMapper::WORLD;
}

//----------------------------------------------------------------------------
svtkLabeledDataMapper::~svtkLabeledDataMapper()
{
  delete[] this->LabelFormat;

  delete[] this->LabelPositions;
  if (this->TextMappers != nullptr)
  {
    for (int i = 0; i < this->NumberOfLabelsAllocated; i++)
    {
      this->TextMappers[i]->Delete();
    }
    delete[] this->TextMappers;
  }

  this->SetFieldDataName(nullptr);
  this->SetTransform(nullptr);
  delete this->Implementation;
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::AllocateLabels(int numLabels)
{
  if (numLabels > this->NumberOfLabelsAllocated)
  {
    int i;
    // delete old stuff
    delete[] this->LabelPositions;
    this->LabelPositions = nullptr;
    for (i = 0; i < this->NumberOfLabelsAllocated; i++)
    {
      this->TextMappers[i]->Delete();
    }
    delete[] this->TextMappers;
    this->TextMappers = nullptr;

    this->NumberOfLabelsAllocated = numLabels;

    // Allocate and initialize new stuff
    this->LabelPositions = new double[this->NumberOfLabelsAllocated * 3];
    this->TextMappers = new svtkTextMapper*[this->NumberOfLabelsAllocated];
    for (i = 0; i < this->NumberOfLabelsAllocated; i++)
    {
      this->TextMappers[i] = svtkTextMapper::New();
      this->LabelPositions[3 * i] = 0;
      this->LabelPositions[3 * i + 1] = 0;
      this->LabelPositions[3 * i + 2] = 0;
    }
  }
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::SetLabelTextProperty(svtkTextProperty* prop, int type)
{
  this->Implementation->TextProperties[type] = prop;
  this->Modified();
}

//----------------------------------------------------------------------------
svtkTextProperty* svtkLabeledDataMapper::GetLabelTextProperty(int type)
{
  if (this->Implementation->TextProperties.find(type) != this->Implementation->TextProperties.end())
  {
    return this->Implementation->TextProperties[type];
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::SetInputData(svtkDataObject* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
svtkDataSet* svtkLabeledDataMapper::GetInput()
{
  return svtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this mapper.
void svtkLabeledDataMapper::ReleaseGraphicsResources(svtkWindow* win)
{
  if (this->TextMappers != nullptr)
  {
    for (int i = 0; i < this->NumberOfLabelsAllocated; i++)
    {
      this->TextMappers[i]->ReleaseGraphicsResources(win);
    }
  }
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::RenderOverlay(svtkViewport* viewport, svtkActor2D* actor)
{
  for (int i = 0; i < this->NumberOfLabels; i++)
  {
    double x[3];
    x[0] = this->LabelPositions[3 * i];
    x[1] = this->LabelPositions[3 * i + 1];
    x[2] = this->LabelPositions[3 * i + 2];

    double* pos = x;
    if (this->Transform)
    {
      pos = this->Transform->TransformDoublePoint(x);
    }

    if (this->CoordinateSystem == svtkLabeledDataMapper::WORLD)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(pos);
    }
    else if (this->CoordinateSystem == svtkLabeledDataMapper::DISPLAY)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
      actor->GetPositionCoordinate()->SetValue(pos);
    }

    bool show = true;
    if (this->ClippingPlanes)
    {
      for (int p = 0; p < this->GetNumberOfClippingPlanes(); ++p)
      {
        if (this->ClippingPlanes->GetItem(p)->FunctionValue(pos) < 0.0)
        {
          show = false;
        }
      }
    }
    if (show)
    {
      this->TextMappers[i]->RenderOverlay(viewport, actor);
    }
  }
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::RenderOpaqueGeometry(svtkViewport* viewport, svtkActor2D* actor)
{
  svtkTextProperty* tprop = this->Implementation->TextProperties[0];
  if (!tprop)
  {
    svtkErrorMacro(<< "Need default text property to render labels");
    return;
  }

  // Updates the input pipeline if needed.
  this->Update();

  svtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  if (!inputDO)
  {
    this->NumberOfLabels = 0;
    svtkErrorMacro(<< "Need input data to render labels (2)");
    return;
  }

  // Check for property updates.
  svtkMTimeType propMTime = 0;
  std::map<int, svtkSmartPointer<svtkTextProperty> >::iterator it, itEnd;
  it = this->Implementation->TextProperties.begin();
  itEnd = this->Implementation->TextProperties.end();
  for (; it != itEnd; ++it)
  {
    svtkTextProperty* prop = it->second;
    if (prop && prop->GetMTime() > propMTime)
    {
      propMTime = prop->GetMTime();
    }
  }

  // Check to see whether we have to rebuild everything
  if (this->GetMTime() > this->BuildTime || inputDO->GetMTime() > this->BuildTime ||
    propMTime > this->BuildTime)
  {
    this->BuildLabels();
  }

  for (int i = 0; i < this->NumberOfLabels; i++)
  {
    double* pos = &this->LabelPositions[3 * i];
    if (this->Transform)
    {
      pos = this->Transform->TransformDoublePoint(pos);
    }

    if (this->CoordinateSystem == svtkLabeledDataMapper::WORLD)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(pos);
    }
    else if (this->CoordinateSystem == svtkLabeledDataMapper::DISPLAY)
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
      actor->GetPositionCoordinate()->SetValue(pos);
    }
    bool show = true;
    if (this->ClippingPlanes)
    {
      for (int p = 0; p < this->GetNumberOfClippingPlanes(); ++p)
      {
        if (this->ClippingPlanes->GetItem(p)->FunctionValue(pos) < 0.0)
        {
          show = false;
        }
      }
    }
    if (show)
    {
      this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
    }
  }
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::BuildLabels()
{
  svtkDebugMacro(<< "Rebuilding labels");
  svtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(inputDO);
  svtkDataSet* ds = svtkDataSet::SafeDownCast(inputDO);
  if (ds)
  {
    this->AllocateLabels(ds->GetNumberOfPoints());
    this->NumberOfLabels = 0;
    this->BuildLabelsInternal(ds);
  }
  else if (cd)
  {
    this->AllocateLabels(cd->GetNumberOfPoints());
    this->NumberOfLabels = 0;
    svtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        this->BuildLabelsInternal(ds);
      }
    }
    iter->Delete();
  }
  else
  {
    svtkErrorMacro("Unsupported data type: " << inputDO->GetClassName());
  }

  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::BuildLabelsInternal(svtkDataSet* input)
{
  int i, j, numComp = 0, pointIdLabels = 0, activeComp = 0;
  svtkAbstractArray* abstractData = nullptr;
  svtkDataArray* numericData = nullptr;
  svtkStringArray* stringData = nullptr;
  svtkUnicodeStringArray* uStringData = nullptr;

  if (input->GetNumberOfPoints() == 0)
  {
    return;
  }

  svtkPointData* pd = input->GetPointData();
  // figure out what to label, and if we can label it
  pointIdLabels = 0;
  switch (this->LabelMode)
  {
    case SVTK_LABEL_IDS:
    {
      pointIdLabels = 1;
    };
    break;
    case SVTK_LABEL_SCALARS:
      if (pd->GetScalars())
      {
        numericData = pd->GetScalars();
      }
      break;
    case SVTK_LABEL_VECTORS:
      if (pd->GetVectors())
      {
        numericData = pd->GetVectors();
      }
      break;
    case SVTK_LABEL_NORMALS:
      if (pd->GetNormals())
      {
        numericData = pd->GetNormals();
      }
      break;
    case SVTK_LABEL_TCOORDS:
      if (pd->GetTCoords())
      {
        numericData = pd->GetTCoords();
      }
      break;
    case SVTK_LABEL_TENSORS:
      if (pd->GetTensors())
      {
        numericData = pd->GetTensors();
      }
      break;
    case SVTK_LABEL_FIELD_DATA:
    {
      int arrayNum;
      if (this->FieldDataName != nullptr)
      {
        svtkDebugMacro(<< "Labeling field data array " << this->FieldDataName);
        abstractData = pd->GetAbstractArray(this->FieldDataName, arrayNum);
      }
      else
      {
        arrayNum = (this->FieldDataArray < pd->GetNumberOfArrays() ? this->FieldDataArray
                                                                   : pd->GetNumberOfArrays() - 1);
        abstractData = pd->GetAbstractArray(arrayNum);
      }
      numericData = svtkArrayDownCast<svtkDataArray>(abstractData);
      stringData = svtkArrayDownCast<svtkStringArray>(abstractData);
      uStringData = svtkArrayDownCast<svtkUnicodeStringArray>(abstractData);
    };
    break;
  }

  // determine number of components and check input
  if (pointIdLabels)
  {
    numComp = 1;
  }
  else if (numericData)
  {
    numComp = numericData->GetNumberOfComponents();
    activeComp = 0;
    if (this->LabeledComponent >= 0)
    {
      activeComp = (this->LabeledComponent < numComp ? this->LabeledComponent : numComp - 1);
      numComp = 1;
    }
  }
  else
  {
    if (stringData)
    {
      numComp = stringData->GetNumberOfComponents();
    }
    else if (uStringData)
    {
      numComp = uStringData->GetNumberOfComponents();
    }
    else
    {
      if (this->FieldDataName)
      {
        svtkWarningMacro(<< "Could not find label array (" << this->FieldDataName << ") "
                        << "in input.");
      }
      else
      {
        svtkWarningMacro(<< "Could not find label array ("
                        << "index " << this->FieldDataArray << ") "
                        << "in input.");
      }

      return;
    }
  }

  svtkStdString FormatString;
  if (this->LabelFormat)
  {
    // The user has specified a format string.
    svtkDebugMacro(<< "Using user-specified format string " << this->LabelFormat);
    FormatString = this->LabelFormat;
  }
  else
  {
    // Try to come up with some sane default.
    if (pointIdLabels)
    {
      FormatString = "%d";
    }
    else if (numericData)
    {
      switch (numericData->GetDataType())
      {
        case SVTK_VOID:
          FormatString = "0x%x";
          break;

          // don't use svtkTypeTraits::ParseFormat for character types as parse formats
          // aren't the same as print formats for these types.
        case SVTK_BIT:
        case SVTK_SIGNED_CHAR:
        case SVTK_UNSIGNED_CHAR:
        case SVTK_SHORT:
        case SVTK_UNSIGNED_SHORT:
        case SVTK_INT:
        case SVTK_UNSIGNED_INT:
          FormatString = "%d";
          break;

        case SVTK_CHAR:
          FormatString = "%c";
          break;

        case SVTK_LONG:
          FormatString = svtkTypeTraits<long>::ParseFormat();
          break;
        case SVTK_UNSIGNED_LONG:
          FormatString = svtkTypeTraits<unsigned long>::ParseFormat();
          break;

        case SVTK_ID_TYPE:
          FormatString = svtkTypeTraits<svtkIdType>::ParseFormat();
          break;

        case SVTK_LONG_LONG:
          FormatString = svtkTypeTraits<long long>::ParseFormat();
          break;
        case SVTK_UNSIGNED_LONG_LONG:
          FormatString = svtkTypeTraits<unsigned long long>::ParseFormat();
          break;

        case SVTK_FLOAT:
          FormatString = svtkTypeTraits<float>::ParseFormat();
          break;

        case SVTK_DOUBLE:
          FormatString = svtkTypeTraits<double>::ParseFormat();
          break;

        default:
          FormatString = "BUG - UNKNOWN DATA FORMAT";
          break;
      }
    }
    else if (stringData)
    {
      FormatString = "";
    }
    else if (uStringData)
    {
      svtkWarningMacro(
        "Unicode string arrays are not adequately supported by the svtkLabeledDataMapper.  Unicode "
        "strings will be converted to svtkStdStrings for rendering.");
      FormatString = "unicode";
    }
    else
    {
      FormatString = "BUG - COULDN'T DETECT DATA TYPE";
    }

    svtkDebugMacro(<< "Using default format string " << FormatString.c_str());

  } // Done building default format string

  int numCurLabels = input->GetNumberOfPoints();
  // We are assured that
  // this->NumberOfLabelsAllocated >= (this->NumberOfLabels + numCurLabels)
  if (this->NumberOfLabelsAllocated < (this->NumberOfLabels + numCurLabels))
  {
    svtkErrorMacro("Number of labels must be allocated before this method is called.");
    return;
  }

  // ----------------------------------------
  // Now we actually construct the label strings
  //

  const char* LiveFormatString = FormatString.c_str();
  char TempString[1024];

  svtkIntArray* typeArr =
    svtkArrayDownCast<svtkIntArray>(this->GetInputAbstractArrayToProcess(0, input));
  for (i = 0; i < numCurLabels; i++)
  {
    svtkStdString ResultString;

    if (pointIdLabels)
    {
      snprintf(TempString, sizeof(TempString), LiveFormatString, i);
      ResultString = TempString;
    }
    else
    {
      if (numericData)
      {
        void* rawData = numericData->GetVoidPointer(i * numComp);

        if (numComp == 1)
        {
          switch (numericData->GetDataType())
          {
            svtkTemplateMacro(svtkLabeledDataMapper_PrintComponent(TempString, sizeof(TempString),
              LiveFormatString, activeComp, static_cast<SVTK_TT*>(rawData)));
          }
          ResultString = TempString;
        }
        else // numComp != 1
        {
          ResultString = "(";

          // Print each component in turn and add it to the string.
          for (j = 0; j < numComp; ++j)
          {
            switch (numericData->GetDataType())
            {
              svtkTemplateMacro(svtkLabeledDataMapper_PrintComponent(TempString, sizeof(TempString),
                LiveFormatString, j, static_cast<SVTK_TT*>(rawData)));
            }
            ResultString += TempString;

            if (j < (numComp - 1))
            {
              ResultString += this->GetComponentSeparator();
            }
            else
            {
              ResultString += ')';
            }
          }
        }
      }
      else // rendering string data
      {
        // If the user hasn't given us a custom format string then
        // we'll sidestep a lot of snprintf nonsense.
        if (this->LabelFormat == nullptr)
        {
          if (uStringData)
          {
            ResultString = uStringData->GetValue(i).utf8_str();
          }
          else
          {
            ResultString = stringData->GetValue(i);
          }
        }
        else // the user specified a label format
        {
          snprintf(TempString, 1023, LiveFormatString, stringData->GetValue(i).c_str());
          ResultString = TempString;
        } // done printing strings with label format
      }   // done printing strings
    }     // done creating string

    this->TextMappers[i + this->NumberOfLabels]->SetInput(ResultString.c_str());

    // Find the correct property type
    int type = 0;
    if (typeArr)
    {
      type = typeArr->GetValue(i);
    }
    svtkTextProperty* prop = this->Implementation->TextProperties[type];
    if (!prop)
    {
      prop = this->Implementation->TextProperties[0];
    }
    this->TextMappers[i + this->NumberOfLabels]->SetTextProperty(prop);

    double x[3];
    input->GetPoint(i, x);
    this->LabelPositions[3 * (i + this->NumberOfLabels)] = x[0];
    this->LabelPositions[3 * (i + this->NumberOfLabels) + 1] = x[1];
    this->LabelPositions[3 * (i + this->NumberOfLabels) + 2] = x[2];
  }

  this->NumberOfLabels += numCurLabels;
}

//----------------------------------------------------------------------------
int svtkLabeledDataMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // Can handle composite datasets.
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void svtkLabeledDataMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Input)
  {
    os << indent << "Input: (" << this->Input << ")\n";
  }
  else
  {
    os << indent << "Input: (none)\n";
  }

  std::map<int, svtkSmartPointer<svtkTextProperty> >::iterator it, itEnd;
  it = this->Implementation->TextProperties.begin();
  itEnd = this->Implementation->TextProperties.end();
  for (; it != itEnd; ++it)
  {
    svtkTextProperty* prop = it->second;
    if (prop)
    {
      os << indent << "LabelTextProperty " << it->first << ":\n";
      prop->PrintSelf(os, indent.GetNextIndent());
    }
    else
    {
      os << indent << "LabelTextProperty " << it->first << ": (none)\n";
    }
  }

  os << indent << "Label Mode: ";
  if (this->LabelMode == SVTK_LABEL_IDS)
  {
    os << "Label Ids\n";
  }
  else if (this->LabelMode == SVTK_LABEL_SCALARS)
  {
    os << "Label Scalars\n";
  }
  else if (this->LabelMode == SVTK_LABEL_VECTORS)
  {
    os << "Label Vectors\n";
  }
  else if (this->LabelMode == SVTK_LABEL_NORMALS)
  {
    os << "Label Normals\n";
  }
  else if (this->LabelMode == SVTK_LABEL_TCOORDS)
  {
    os << "Label TCoords\n";
  }
  else if (this->LabelMode == SVTK_LABEL_TENSORS)
  {
    os << "Label Tensors\n";
  }
  else
  {
    os << "Label Field Data\n";
  }

  os << indent << "Label Format: " << (this->LabelFormat ? this->LabelFormat : "Null") << "\n";

  os << indent << "Labeled Component: ";
  if (this->LabeledComponent < 0)
  {
    os << "(All Components)\n";
  }
  else
  {
    os << this->LabeledComponent << "\n";
  }

  os << indent << "Field Data Array: " << this->FieldDataArray << "\n";
  os << indent << "Field Data Name: " << (this->FieldDataName ? this->FieldDataName : "Null")
     << "\n";

  os << indent << "Transform: " << (this->Transform ? "" : "(none)") << endl;
  if (this->Transform)
  {
    this->Transform->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "CoordinateSystem: " << this->CoordinateSystem << endl;
}

// ----------------------------------------------------------------------
void svtkLabeledDataMapper::SetFieldDataArray(int arrayIndex)
{
  delete[] this->FieldDataName;
  this->FieldDataName = nullptr;

  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting FieldDataArray to "
                << arrayIndex);

  if (this->FieldDataArray !=
    (arrayIndex < 0 ? 0 : (arrayIndex > SVTK_INT_MAX ? SVTK_INT_MAX : arrayIndex)))
  {
    this->FieldDataArray =
      (arrayIndex < 0 ? 0 : (arrayIndex > SVTK_INT_MAX ? SVTK_INT_MAX : arrayIndex));
    this->Modified();
  }
}

// ----------------------------------------------------------------------
svtkMTimeType svtkLabeledDataMapper::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();
  std::map<int, svtkSmartPointer<svtkTextProperty> >::iterator it, itEnd;
  it = this->Implementation->TextProperties.begin();
  itEnd = this->Implementation->TextProperties.end();
  for (; it != itEnd; ++it)
  {
    svtkTextProperty* p = it->second;
    svtkMTimeType curMTime = p->GetMTime();
    if (curMTime > mtime)
    {
      mtime = curMTime;
    }
  }
  return mtime;
}

// ----------------------------------------------------------------------
const char* svtkLabeledDataMapper::GetLabelText(int label)
{
  assert("label index range" && label >= 0 && label < this->NumberOfLabels);
  return this->TextMappers[label]->GetInput();
}

// ----------------------------------------------------------------------
void svtkLabeledDataMapper::SetFieldDataName(const char* arrayName)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting "
                << "FieldDataName"
                << " to " << (arrayName ? arrayName : "(null)"));

  if (this->FieldDataName == nullptr && arrayName == nullptr)
  {
    return;
  }
  if (this->FieldDataName && arrayName && (!strcmp(this->FieldDataName, arrayName)))
  {
    return;
  }
  delete[] this->FieldDataName;
  if (arrayName)
  {
    this->FieldDataName = new char[strlen(arrayName) + 1];
    strcpy(this->FieldDataName, arrayName);
  }
  else
  {
    this->FieldDataName = nullptr;
  }
  this->Modified();
}
