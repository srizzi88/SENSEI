/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDynamic2DLabelMapper.cxx

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

#include "svtkDynamic2DLabelMapper.h"

#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkKdTree.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSortDataArray.h"
#include "svtkStringArray.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkTimerLog.h"
#include "svtkTypeTraits.h"
#include "svtkUnicodeStringArray.h"
#include "svtkViewport.h"
#include "svtksys/FStream.hxx"

#include <fstream>

svtkStandardNewMacro(svtkDynamic2DLabelMapper);

//----------------------------------------------------------------------------
// Creates a new label mapper

svtkDynamic2DLabelMapper::svtkDynamic2DLabelMapper()
{
  this->LabelWidth = nullptr;
  this->LabelHeight = nullptr;
  this->Cutoff = nullptr;

  this->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "priority");
  this->ReversePriority = false;
  this->LabelHeightPadding = 50;
  this->LabelWidthPadding = 10;
  this->ReferenceScale = 1.0;

  // Set new default property
  svtkTextProperty* prop = svtkTextProperty::New();
  prop->SetFontSize(12);
  prop->SetBold(1);
  prop->SetItalic(0);
  prop->SetShadow(1);
  prop->SetFontFamilyToArial();
  prop->SetJustificationToCentered();
  prop->SetVerticalJustificationToCentered();
  prop->SetColor(1, 1, 1);
  this->SetLabelTextProperty(prop);
  prop->Delete();
}

//----------------------------------------------------------------------------
svtkDynamic2DLabelMapper::~svtkDynamic2DLabelMapper()
{
  delete[] this->LabelWidth;
  delete[] this->LabelHeight;
  delete[] this->Cutoff;
}

//----------------------------------------------------------------------------
void svtkDynamic2DLabelMapper::SetPriorityArrayName(const char* name)
{
  this->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, name);
}

//----------------------------------------------------------------------------
template <typename T>
void svtkDynamic2DLabelMapper_PrintComponent(
  char* output, size_t outputSize, const char* format, int index, const T* array)
{
  snprintf(output, outputSize, format, array[index]);
}

//----------------------------------------------------------------------------
void svtkDynamic2DLabelMapper::RenderOpaqueGeometry(svtkViewport* viewport, svtkActor2D* actor)
{
  int i, j, numComp = 0, pointIdLabels, activeComp = 0;
  double x[3];
  svtkAbstractArray* abstractData;
  svtkDataArray* numericData;
  svtkStringArray* stringData;
  svtkUnicodeStringArray* uStringData;
  svtkDataObject* input = this->GetExecutive()->GetInputData(0, 0);

  if (!input)
  {
    svtkErrorMacro(<< "Need input data to render labels (2)");
    return;
  }

  svtkTextProperty* tprop = this->GetLabelTextProperty();
  if (!tprop)
  {
    svtkErrorMacro(<< "Need text property to render labels");
    return;
  }

  this->GetInputAlgorithm()->Update();

  // Input might have changed
  input = this->GetExecutive()->GetInputData(0, 0);

  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  svtkGraph* gInput = svtkGraph::SafeDownCast(input);
  if (!dsInput && !gInput)
  {
    svtkErrorMacro(<< "Input must be svtkDataSet or svtkGraph.");
    return;
  }
  svtkDataSetAttributes* pd = dsInput ? dsInput->GetPointData() : gInput->GetVertexData();

  // If no labels we are done
  svtkIdType numItems = dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();
  if (numItems == 0)
  {
    return;
  }

  // Check to see whether we have to rebuild everything
  if (this->GetMTime() > this->BuildTime || input->GetMTime() > this->BuildTime)
  {
    svtkDebugMacro(<< "Rebuilding labels");

    svtkIntArray* typeArr =
      svtkArrayDownCast<svtkIntArray>(this->GetInputAbstractArrayToProcess(0, input));

    // figure out what to label, and if we can label it
    pointIdLabels = 0;
    abstractData = nullptr;
    numericData = nullptr;
    stringData = nullptr;
    uStringData = nullptr;
    switch (this->LabelMode)
    {
      case SVTK_LABEL_IDS:
        pointIdLabels = 1;
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
      ;
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
    else if (uStringData)
    {
      svtkWarningMacro(
        "Unicode string arrays are not adequately supported by the svtkDynamic2DLabelMapper.  "
        "Unicode strings will be converted to svtkStdStrings for rendering.");
    }
    else if (!stringData)
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
          case SVTK_SHORT:
          case SVTK_UNSIGNED_SHORT:
          case SVTK_INT:
          case SVTK_UNSIGNED_INT:
            FormatString = "%d";
            break;

          case SVTK_CHAR:
          case SVTK_SIGNED_CHAR:
          case SVTK_UNSIGNED_CHAR:
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
        FormatString = "unicode";
      }
      else
      {
        FormatString = "BUG - COULDN'T DETECT DATA TYPE";
      }

      svtkDebugMacro(<< "Using default format string " << FormatString.c_str());
    } // Done building default format string

    this->NumberOfLabels = dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();
    if (this->NumberOfLabels > this->NumberOfLabelsAllocated)
    {
      // delete old stuff
      for (i = 0; i < this->NumberOfLabelsAllocated; i++)
      {
        this->TextMappers[i]->Delete();
      }
      delete[] this->TextMappers;

      this->NumberOfLabelsAllocated = this->NumberOfLabels;
      this->TextMappers = new svtkTextMapper*[this->NumberOfLabelsAllocated];
      for (i = 0; i < this->NumberOfLabelsAllocated; i++)
      {
        this->TextMappers[i] = svtkTextMapper::New();
      }
    } // if we have to allocate new text mappers

    // ----------------------------------------
    // Now we actually construct the label strings
    //

    const char* LiveFormatString = FormatString.c_str();
    char TempString[1024];

    for (i = 0; i < this->NumberOfLabels; i++)
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
          void* rawData = numericData->GetVoidPointer(i);

          if (numComp == 1)
          {
            switch (numericData->GetDataType())
            {
              svtkTemplateMacro(svtkDynamic2DLabelMapper_PrintComponent(TempString,
                sizeof(TempString), LiveFormatString, activeComp, static_cast<SVTK_TT*>(rawData)));
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
                svtkTemplateMacro(svtkDynamic2DLabelMapper_PrintComponent(TempString,
                  sizeof(TempString), LiveFormatString, j, static_cast<SVTK_TT*>(rawData)));
              }
              ResultString += TempString;

              if (j < (numComp - 1))
              {
                ResultString += ' ';
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

      this->TextMappers[i]->SetInput(ResultString.c_str());

      // Find the correct property type
      int type = 0;
      if (typeArr)
      {
        type = typeArr->GetValue(i);
      }
      svtkTextProperty* prop = this->GetLabelTextProperty(type);
      if (!prop)
      {
        prop = this->GetLabelTextProperty(0);
      }
      this->TextMappers[i]->SetTextProperty(prop);
    }

    this->BuildTime.Modified();

    //
    // Perform the label layout preprocessing
    //

    // Calculate height and width padding
    float widthPadding = 0, heightPadding = 0;
    if (this->NumberOfLabels > 0)
    {
      widthPadding = this->TextMappers[0]->GetHeight(viewport) * this->LabelWidthPadding / 100.0;
      heightPadding = this->TextMappers[0]->GetHeight(viewport) * this->LabelHeightPadding / 100.0;
    }

    // Calculate label widths / heights
    delete[] this->LabelWidth;
    this->LabelWidth = new float[this->NumberOfLabels];
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      this->LabelWidth[i] = this->TextMappers[i]->GetWidth(viewport) + widthPadding;
    }

    delete[] this->LabelHeight;
    this->LabelHeight = new float[this->NumberOfLabels];
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      this->LabelHeight[i] = this->TextMappers[i]->GetHeight(viewport) + heightPadding;
    }

    // Determine cutoff scales of each point
    delete[] this->Cutoff;
    this->Cutoff = new float[this->NumberOfLabels];

    svtkTimerLog* timer = svtkTimerLog::New();
    timer->StartTimer();

    svtkCoordinate* coord = svtkCoordinate::New();
    coord->SetViewport(viewport);
    svtkPoints* pts = svtkPoints::New();
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      double* dc;
      double pti[3];
      if (dsInput)
      {
        dsInput->GetPoint(i, pti);
      }
      else
      {
        gInput->GetPoint(i, pti);
      }
      coord->SetValue(pti);
      dc = coord->GetComputedDoubleDisplayValue(nullptr);
      pts->InsertNextPoint(dc[0], dc[1], 0);
    }
    coord->Delete();

    timer->StopTimer();
    svtkDebugMacro("svtkDynamic2DLabelMapper computed display coordinates for "
      << timer->GetElapsedTime() << "s");
    timer->StartTimer();

    // Announce progress
    double progress = 0;
    this->InvokeEvent(svtkCommand::ProgressEvent, static_cast<void*>(&progress));
    int current = 0;
    int total = this->NumberOfLabels * (this->NumberOfLabels - 1) / 2;

    // Create an index array to store the offsets of the sorted elements.
    svtkIdTypeArray* index = svtkIdTypeArray::New();
    index->SetNumberOfValues(this->NumberOfLabels);
    for (i = 0; i < this->NumberOfLabels; i++)
    {
      index->SetValue(i, i);
    }

    // If the array is found, sort it and rearrange the corresponding index array.
    svtkAbstractArray* inputArr = this->GetInputAbstractArrayToProcess(1, input);
    if (inputArr)
    {
      // Don't sort the original array, instead make a copy.
      svtkAbstractArray* arr = svtkAbstractArray::CreateArray(inputArr->GetDataType());
      arr->DeepCopy(inputArr);
      svtkSortDataArray::Sort(arr, index);
      arr->Delete();
    }

    // We normally go from highest (at the end) to lowest (at the beginning).
    // If priorities are reversed, we go from lowest to highest.
    // If no sorted array was used, we just go from index 0 to index n-1.
    svtkIdType begin = this->NumberOfLabels - 1;
    svtkIdType end = -1;
    svtkIdType step = -1;
    if ((this->ReversePriority && inputArr) || (!this->ReversePriority && !inputArr))
    {
      begin = 0;
      end = this->NumberOfLabels;
      step = 1;
    }
    for (i = begin; i != end; i += step)
    {
      svtkIdType indexI = index->GetValue(i);
      float* pti = reinterpret_cast<float*>(pts->GetVoidPointer(3 * indexI));
      this->Cutoff[indexI] = SVTK_FLOAT_MAX;
      for (j = begin; j != i; j += step)
      {
        svtkIdType indexJ = index->GetValue(j);
        float* ptj = reinterpret_cast<float*>(pts->GetVoidPointer(3 * indexJ));
        float absX = (pti[0] - ptj[0]) > 0 ? (pti[0] - ptj[0]) : -(pti[0] - ptj[0]);
        float absY = (pti[1] - ptj[1]) > 0 ? (pti[1] - ptj[1]) : -(pti[1] - ptj[1]);
        float xScale = 2 * absX / (this->LabelWidth[indexI] + this->LabelWidth[indexJ]);
        float yScale = 2 * absY / (this->LabelHeight[indexI] + this->LabelHeight[indexJ]);
        float maxScale = xScale < yScale ? yScale : xScale;
        if (maxScale < this->Cutoff[indexJ] && maxScale < this->Cutoff[indexI])
        {
          this->Cutoff[indexI] = maxScale;
        }
        if (current % 100000 == 0)
        {
          progress = static_cast<double>(current) / total;
          this->InvokeEvent(svtkCommand::ProgressEvent, static_cast<void*>(&progress));
        }
        current++;
      }
    }
    index->Delete();
    progress = 1.0;
    this->InvokeEvent(svtkCommand::ProgressEvent, static_cast<void*>(&progress));

    pts->Delete();

    // Determine the reference scale
    this->ReferenceScale = this->GetCurrentScale(viewport);

    timer->StopTimer();
    svtkDebugMacro(
      "svtkDynamic2DLabelMapper computed label cutoffs for " << timer->GetElapsedTime() << "s");
    timer->Delete();
  }

  //
  // Draw labels visible in the current scale
  //

  // Determine the current scale
  double scale = 1.0;
  if (this->ReferenceScale != 0.0)
  {
    scale = this->GetCurrentScale(viewport) / this->ReferenceScale;
  }

  for (i = 0; i < this->NumberOfLabels; i++)
  {
    if (dsInput)
    {
      dsInput->GetPoint(i, x);
    }
    else
    {
      gInput->GetPoint(i, x);
    }
    if ((1.0 / scale) < this->Cutoff[i])
    {
      actor->GetPositionCoordinate()->SetCoordinateSystemToWorld();
      actor->GetPositionCoordinate()->SetValue(x);
      this->TextMappers[i]->RenderOpaqueGeometry(viewport, actor);
    }
  }
}

//----------------------------------------------------------------------------
double svtkDynamic2DLabelMapper::GetCurrentScale(svtkViewport* viewport)
{
  // The current scale is the size on the screen of 1 unit in the xy plane

  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  if (!ren)
  {
    svtkErrorMacro("svtkDynamic2DLabelMapper only works in a svtkRenderer or subclass");
    return 1.0;
  }
  svtkCamera* camera = ren->GetActiveCamera();
  if (camera->GetParallelProjection())
  {
    // For parallel projection, the scale depends on the parallel scale
    double scale = (ren->GetSize()[1] / 2.0) / camera->GetParallelScale();
    return scale;
  }
  else
  {
    // For perspective projection, the scale depends on the view angle
    double viewAngle = camera->GetViewAngle();
    double distZ =
      camera->GetPosition()[2] > 0 ? camera->GetPosition()[2] : -camera->GetPosition()[2];
    double unitAngle = svtkMath::DegreesFromRadians(atan2(1.0, distZ));
    double scale = ren->GetSize()[1] * unitAngle / viewAngle;
    return scale;
  }
}

//----------------------------------------------------------------------------
void svtkDynamic2DLabelMapper::RenderOverlay(svtkViewport* viewport, svtkActor2D* actor)
{
  int i;
  double x[3];
  svtkDataObject* input = this->GetExecutive()->GetInputData(0, 0);
  svtkGraph* gInput = svtkGraph::SafeDownCast(input);
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  svtkIdType numPts = dsInput ? dsInput->GetNumberOfPoints() : gInput->GetNumberOfVertices();

  // Determine the current scale
  double scale = 1.0;
  if (this->ReferenceScale != 0.0)
  {
    scale = this->GetCurrentScale(viewport) / this->ReferenceScale;
  }

  if (!input)
  {
    svtkErrorMacro(<< "Need input data to render labels (1)");
    return;
  }

  svtkTimerLog* timer = svtkTimerLog::New();
  timer->StartTimer();

  for (i = 0; i < this->NumberOfLabels && i < numPts; i++)
  {
    if (dsInput)
    {
      dsInput->GetPoint(i, x);
    }
    else
    {
      gInput->GetPoint(i, x);
    }
    actor->SetPosition(x);
    double* display = actor->GetPositionCoordinate()->GetComputedDoubleDisplayValue(viewport);
    double screenX = display[0];
    double screenY = display[1];

    bool inside = viewport->IsInViewport(static_cast<int>(screenX + this->LabelWidth[i]),
                    static_cast<int>(screenY + this->LabelHeight[i])) ||
      viewport->IsInViewport(static_cast<int>(screenX + this->LabelWidth[i]),
        static_cast<int>(screenY - this->LabelHeight[i])) ||
      viewport->IsInViewport(static_cast<int>(screenX - this->LabelWidth[i]),
        static_cast<int>(screenY + this->LabelHeight[i])) ||
      viewport->IsInViewport(static_cast<int>(screenX - this->LabelWidth[i]),
        static_cast<int>(screenY - this->LabelHeight[i]));
    if (inside && (1.0f / scale) < this->Cutoff[i])
    {
      this->TextMappers[i]->RenderOverlay(viewport, actor);
    }
  }

  timer->StopTimer();
  svtkDebugMacro("svtkDynamic2DLabelMapper interactive time: " << timer->GetElapsedTime() << "s");
  timer->Delete();
}

//----------------------------------------------------------------------------
void svtkDynamic2DLabelMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ReversePriority: " << (this->ReversePriority ? "on" : "off") << endl;
  os << indent << "LabelHeightPadding: " << (this->LabelHeightPadding ? "on" : "off") << endl;
  os << indent << "LabelWidthPadding: " << (this->LabelWidthPadding ? "on" : "off") << endl;
}
