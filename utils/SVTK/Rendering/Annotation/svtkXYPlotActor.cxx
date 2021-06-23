/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkXYPlotActor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkXYPlotActor.h"

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAppendPolyData.h"
#include "svtkAxisActor2D.h"
#include "svtkCellArray.h"
#include "svtkDataObjectCollection.h"
#include "svtkDataSetCollection.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkGlyph2D.h"
#include "svtkGlyphSource2D.h"
#include "svtkIntArray.h"
#include "svtkLegendBoxActor.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPlanes.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkTextActor.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkTrivialProducer.h"
#include "svtkViewport.h"

struct _xmlNode;
#define SVTK_MAX_PLOTS 50

svtkStandardNewMacro(svtkXYPlotActor);

svtkCxxSetObjectMacro(svtkXYPlotActor, TitleTextProperty, svtkTextProperty);
svtkCxxSetObjectMacro(svtkXYPlotActor, AxisLabelTextProperty, svtkTextProperty);

class svtkXYPlotActorConnections : public svtkAlgorithm
{
public:
  static svtkXYPlotActorConnections* New();
  svtkTypeMacro(svtkXYPlotActorConnections, svtkAlgorithm);

  svtkXYPlotActorConnections() { this->SetNumberOfInputPorts(1); }
};

svtkStandardNewMacro(svtkXYPlotActorConnections);

//----------------------------------------------------------------------------
// Instantiate object
svtkXYPlotActor::svtkXYPlotActor()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(.25, .25);
  this->Position2Coordinate->SetValue(.5, .5);

  this->InputConnectionHolder = svtkXYPlotActorConnections::New();
  this->SelectedInputScalars = nullptr;
  this->SelectedInputScalarsComponent = svtkIntArray::New();

  this->DataObjectInputConnectionHolder = svtkXYPlotActorConnections::New();

  this->Title = nullptr;
  this->XTitle = new char[7];
  snprintf(this->XTitle, 7, "%s", "X Axis");

  this->YTitleActor = svtkTextActor::New();
  this->YTitleActor->SetInput("Y Axis");
  this->YTitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->YTitleActor->GetPosition2Coordinate()->SetCoordinateSystemToViewport();

  this->YTitlePosition = SVTK_XYPLOT_Y_AXIS_HCENTER;
  this->YTitleDelta = 0;

  this->XValues = SVTK_XYPLOT_INDEX;

  this->NumberOfXLabels = 5;
  this->NumberOfYLabels = 5;

  this->TitleTextProperty = svtkTextProperty::New();
  this->TitleTextProperty->SetBold(1);
  this->TitleTextProperty->SetItalic(1);
  this->TitleTextProperty->SetShadow(1);
  this->TitleTextProperty->SetFontFamilyToArial();

  this->AxisLabelTextProperty = svtkTextProperty::New();
  this->AxisLabelTextProperty->SetBold(0);
  this->AxisLabelTextProperty->SetItalic(1);
  this->AxisLabelTextProperty->SetShadow(1);
  this->AxisLabelTextProperty->SetFontFamilyToArial();

  this->AxisTitleTextProperty = svtkTextProperty::New();
  this->AxisTitleTextProperty->SetBold(0);
  this->AxisTitleTextProperty->SetItalic(1);
  this->AxisTitleTextProperty->SetShadow(1);
  this->AxisTitleTextProperty->SetFontFamilyToArial();

  this->XLabelFormat = new char[8];
  snprintf(this->XLabelFormat, 8, "%s", "%-#6.3g");

  this->YLabelFormat = new char[8];
  snprintf(this->YLabelFormat, 8, "%s", "%-#6.3g");

  this->Logx = 0;

  this->XRange[0] = 0.;
  this->XRange[1] = 0.;
  this->YRange[0] = 0.;
  this->YRange[1] = 0.;

  this->Border = 5;
  this->PlotLines = 1;
  this->PlotPoints = 0;
  this->PlotCurveLines = 0;
  this->PlotCurvePoints = 0;
  this->ExchangeAxes = 0;
  this->ReverseXAxis = 0;
  this->ReverseYAxis = 0;

  this->TitleMapper = svtkTextMapper::New();
  this->TitleActor = svtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();

  this->XAxis = svtkAxisActor2D::New();
  this->XAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->XAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->XAxis->SetProperty(this->GetProperty());

  this->YAxis = svtkAxisActor2D::New();
  this->YAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->YAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->YAxis->SetProperty(this->GetProperty());

  this->NumberOfInputs = 0;
  this->PlotData = nullptr;
  this->PlotGlyph = nullptr;
  this->PlotAppend = nullptr;
  this->PlotMapper = nullptr;
  this->PlotActor = nullptr;

  this->ViewportCoordinate[0] = 0.;
  this->ViewportCoordinate[1] = 0.;
  this->PlotCoordinate[0] = 0.;
  this->PlotCoordinate[1] = 0.;

  this->DataObjectPlotMode = SVTK_XYPLOT_COLUMN;
  this->XComponent = svtkIntArray::New();
  this->XComponent->SetNumberOfValues(SVTK_MAX_PLOTS);
  this->YComponent = svtkIntArray::New();
  this->YComponent->SetNumberOfValues(SVTK_MAX_PLOTS);

  this->LinesOn = svtkIntArray::New();
  this->LinesOn->SetNumberOfValues(SVTK_MAX_PLOTS);
  this->PointsOn = svtkIntArray::New();
  this->PointsOn->SetNumberOfValues(SVTK_MAX_PLOTS);
  for (int i = 0; i < SVTK_MAX_PLOTS; i++)
  {
    this->XComponent->SetValue(i, 0);
    this->YComponent->SetValue(i, 0);
    this->LinesOn->SetValue(i, this->PlotLines);
    this->PointsOn->SetValue(i, this->PlotPoints);
  }

  this->Legend = 0;
  this->LegendPosition[0] = .85;
  this->LegendPosition[1] = .75;
  this->LegendPosition2[0] = .15;
  this->LegendPosition2[1] = .20;
  this->LegendActor = svtkLegendBoxActor::New();
  this->LegendActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->LegendActor->GetPosition2Coordinate()->SetReferenceCoordinate(nullptr);
  this->LegendActor->BorderOff();
  this->LegendActor->SetNumberOfEntries(SVTK_MAX_PLOTS); // initial allocation
  this->GlyphSource = svtkGlyphSource2D::New();
  this->GlyphSource->SetGlyphTypeToNone();
  this->GlyphSource->DashOn();
  this->GlyphSource->FilledOff();
  this->GlyphSource->Update();
  this->GlyphSize = 0.020;

  this->ClipPlanes = svtkPlanes::New();
  svtkPoints* pts = svtkPoints::New();
  pts->SetNumberOfPoints(4);
  this->ClipPlanes->SetPoints(pts);
  pts->Delete();
  svtkDoubleArray* n = svtkDoubleArray::New();
  n->SetNumberOfComponents(3);
  n->SetNumberOfTuples(4);
  this->ClipPlanes->SetNormals(n);
  n->Delete();

  // Construct the box
  this->ChartBox = 0;
  this->ChartBoxPolyData = svtkPolyData::New();
  svtkPoints* points = svtkPoints::New();
  points->SetNumberOfPoints(4);
  this->ChartBoxPolyData->SetPoints(points);
  points->Delete();
  svtkCellArray* polys = svtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->ChartBoxPolyData->SetPolys(polys);
  polys->Delete();
  this->ChartBoxMapper = svtkPolyDataMapper2D::New();
  this->ChartBoxMapper->SetInputData(this->ChartBoxPolyData);
  this->ChartBoxActor = svtkActor2D::New();
  this->ChartBoxActor->SetMapper(this->ChartBoxMapper);

  // Box border
  this->ChartBorder = 0;
  this->ChartBorderPolyData = svtkPolyData::New();
  this->ChartBorderPolyData->SetPoints(points);
  svtkCellArray* lines = svtkCellArray::New();
  lines->InsertNextCell(5);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(0);
  this->ChartBorderPolyData->SetLines(lines);
  lines->Delete();
  this->ChartBorderMapper = svtkPolyDataMapper2D::New();
  this->ChartBorderMapper->SetInputData(this->ChartBorderPolyData);
  this->ChartBorderActor = svtkActor2D::New();
  this->ChartBorderActor->SetMapper(this->ChartBorderMapper);

  // Reference lines
  this->ShowReferenceXLine = 0;
  this->ShowReferenceYLine = 0;
  this->ReferenceXValue = 0.;
  this->ReferenceYValue = 0.;
  points = svtkPoints::New();
  points->SetNumberOfPoints(4);
  lines = svtkCellArray::New();
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertNextCell(2);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  this->ReferenceLinesPolyData = svtkPolyData::New();
  this->ReferenceLinesPolyData->SetPoints(points);
  this->ReferenceLinesPolyData->SetLines(lines);
  points->Delete();
  lines->Delete();
  this->ReferenceLinesMapper = svtkPolyDataMapper2D::New();
  this->ReferenceLinesMapper->SetInputData(this->ReferenceLinesPolyData);
  this->ReferenceLinesActor = svtkActor2D::New();
  this->ReferenceLinesActor->SetMapper(this->ReferenceLinesMapper);

  this->CachedSize[0] = 0;
  this->CachedSize[1] = 0;

  this->AdjustXLabels = 1;
  this->AdjustYLabels = 1;
  this->AdjustTitlePosition = 1;
  this->TitlePosition[0] = .5;
  this->TitlePosition[1] = .9;
  this->AdjustTitlePositionMode = svtkXYPlotActor::AlignHCenter | svtkXYPlotActor::AlignTop |
    svtkXYPlotActor::AlignAxisHCenter | svtkXYPlotActor::AlignAxisVCenter;
}

//----------------------------------------------------------------------------
svtkXYPlotActor::~svtkXYPlotActor()
{
  // Get rid of the list of array names.
  int num = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  if (this->SelectedInputScalars)
  {
    for (int i = 0; i < num; ++i)
    {
      delete[] this->SelectedInputScalars[i];
      this->SelectedInputScalars[i] = nullptr;
    }
    delete[] this->SelectedInputScalars;
    this->SelectedInputScalars = nullptr;
  }
  this->SelectedInputScalarsComponent->Delete();
  this->SelectedInputScalarsComponent = nullptr;

  //  Now we can get rid of the inputs.
  this->InputConnectionHolder->Delete();
  this->InputConnectionHolder = nullptr;

  this->DataObjectInputConnectionHolder->Delete();

  this->TitleMapper->Delete();
  this->TitleMapper = nullptr;
  this->TitleActor->Delete();
  this->TitleActor = nullptr;

  this->SetTitle(nullptr);
  this->SetXTitle(nullptr);
  this->SetXLabelFormat(nullptr);
  this->SetYLabelFormat(nullptr);

  this->XAxis->Delete();
  this->YAxis->Delete();

  this->InitializeEntries();

  this->LegendActor->Delete();
  this->GlyphSource->Delete();
  this->ClipPlanes->Delete();

  this->ChartBoxActor->Delete();
  this->ChartBoxMapper->Delete();
  this->ChartBoxPolyData->Delete();

  this->ChartBorderActor->Delete();
  this->ChartBorderMapper->Delete();
  this->ChartBorderPolyData->Delete();

  this->ReferenceLinesActor->Delete();
  this->ReferenceLinesMapper->Delete();
  this->ReferenceLinesPolyData->Delete();

  this->XComponent->Delete();
  this->YComponent->Delete();

  this->LinesOn->Delete();
  this->PointsOn->Delete();

  this->TitleTextProperty->Delete();
  this->TitleTextProperty = nullptr;
  this->AxisLabelTextProperty->Delete();
  this->AxisLabelTextProperty = nullptr;
  this->AxisTitleTextProperty->Delete();
  this->AxisTitleTextProperty = nullptr;

  this->YTitleActor->Delete();
  this->YTitleActor = nullptr;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::InitializeEntries()
{
  if (this->NumberOfInputs > 0)
  {
    for (int i = 0; i < this->NumberOfInputs; i++)
    {
      this->PlotData[i]->Delete();
      this->PlotGlyph[i]->Delete();
      this->PlotAppend[i]->Delete();
      this->PlotMapper[i]->Delete();
      this->PlotActor[i]->Delete();
    } // for all entries

    delete[] this->PlotData;
    this->PlotData = nullptr;

    delete[] this->PlotGlyph;
    this->PlotGlyph = nullptr;

    delete[] this->PlotAppend;
    this->PlotAppend = nullptr;

    delete[] this->PlotMapper;
    this->PlotMapper = nullptr;

    delete[] this->PlotActor;
    this->PlotActor = nullptr;
    this->NumberOfInputs = 0;
  } // if entries have been defined
}

//----------------------------------------------------------------------------
bool svtkXYPlotActor::DoesConnectionMatch(int i, svtkAlgorithmOutput* in)
{
  svtkAlgorithmOutput* conn = this->InputConnectionHolder->GetInputConnection(0, i);
  if (conn->GetProducer() == in->GetProducer() && conn->GetIndex() == in->GetIndex())
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::IsInputPresent(svtkAlgorithmOutput* in, const char* arrayName, int component)
{
  int numConns = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  for (int idx = 0; idx < numConns; idx++)
  {
    if (this->DoesConnectionMatch(idx, in))
    {
      if (arrayName == nullptr && this->SelectedInputScalars[idx] == nullptr &&
        component == this->SelectedInputScalarsComponent->GetValue(idx))
      {
        return idx + 1;
      }
      if (arrayName != nullptr && this->SelectedInputScalars[idx] != nullptr &&
        strcmp(arrayName, this->SelectedInputScalars[idx]) == 0 &&
        component == this->SelectedInputScalarsComponent->GetValue(idx))
      {
        return idx + 1;
      }
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::AddDataSetInput(svtkDataSet* ds, const char* arrayName, int component)
{
  svtkTrivialProducer* tp = svtkTrivialProducer::New();
  tp->SetOutput(ds);
  this->AddDataSetInputConnection(tp->GetOutputPort(), arrayName, component);
  tp->Delete();
}

//----------------------------------------------------------------------------
// Add a dataset and array to the list of data to plot.
void svtkXYPlotActor::AddDataSetInputConnection(
  svtkAlgorithmOutput* in, const char* arrayName, int component)
{
  int idx, num;
  char** newNames;

  // I cannot change the input list, because the user has direct
  // access to the collection.  I cannot store the index of the array,
  // because the index might change from render to render ...
  // I have to store the list of string array names.

  idx = this->IsInputPresent(in, arrayName, component);
  // idx starts at 1 and goes to "NumberOfItems".
  if (idx != 0)
  {
    return;
  }

  // The input/array/component must be a unique combination.  Add it to our input list.

  // Now reallocate the list of strings and add the new value.
  num = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  newNames = new char*[num + 1];
  for (idx = 0; idx < num; ++idx)
  {
    newNames[idx] = this->SelectedInputScalars[idx];
  }
  if (arrayName == nullptr)
  {
    newNames[num] = nullptr;
  }
  else
  {
    newNames[num] = new char[strlen(arrayName) + 1];
    strcpy(newNames[num], arrayName);
  }
  delete[] this->SelectedInputScalars;
  this->SelectedInputScalars = newNames;

  // Save the component in the int array.
  this->SelectedInputScalarsComponent->InsertValue(num, component);

  // Add the data set to the collection
  this->InputConnectionHolder->AddInputConnection(0, in);

  // In case of multiple use of a XYPlotActor the NumberOfEntries could be set
  // to n. Then when a call to SetEntryString( n+1, bla ) was done the string was lost
  // Need to update the number of entries for the legend actor
  this->LegendActor->SetNumberOfEntries(this->LegendActor->GetNumberOfEntries() + 1);

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::RemoveAllDataSetInputConnections()
{
  int idx, num;

  num = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  this->InputConnectionHolder->RemoveAllInputs();

  for (idx = 0; idx < num; ++idx)
  {
    delete[] this->SelectedInputScalars[idx];
    this->SelectedInputScalars[idx] = nullptr;
  }
  this->SelectedInputScalarsComponent->Reset();

  this->DataObjectInputConnectionHolder->RemoveAllInputs();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::RemoveDataSetInput(svtkDataSet* ds, const char* arrayName, int component)
{
  int numConns = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  for (int idx = 0; idx < numConns; idx++)
  {
    svtkAlgorithmOutput* aout = this->InputConnectionHolder->GetInputConnection(0, idx);
    svtkAlgorithm* alg = aout ? aout->GetProducer() : nullptr;
    if (alg)
    {
      if (ds == alg->GetOutputDataObject(aout->GetIndex()))
      {
        this->RemoveDataSetInputConnection(aout, arrayName, component);
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to plot.
void svtkXYPlotActor::RemoveDataSetInputConnection(
  svtkAlgorithmOutput* in, const char* arrayName, int component)
{
  // IsInputPresent returns 0 on failure, index+1 on success.
  // Subtract 1 for the actual index.
  int found = this->IsInputPresent(in, arrayName, component) - 1;
  if (found == -1)
  {
    return;
  }

  this->Modified();

  int num = this->InputConnectionHolder->GetNumberOfInputConnections(0);

  this->InputConnectionHolder->RemoveInputConnection(0, found);

  // Do not bother reallocating the SelectedInputScalars
  // string array to make it smaller.
  delete[] this->SelectedInputScalars[found];
  this->SelectedInputScalars[found] = nullptr;
  for (int idx = found + 1; idx < num; ++idx)
  {
    this->SelectedInputScalars[idx - 1] = this->SelectedInputScalars[idx];
    this->SelectedInputScalarsComponent->SetValue(
      idx - 1, this->SelectedInputScalarsComponent->GetValue(idx));
  }
  // Resetting the last item is not really necessary,
  // but to be clean we do it anyway.
  this->SelectedInputScalarsComponent->SetValue(num - 1, -1);
  this->SelectedInputScalars[num - 1] = nullptr;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::AddDataObjectInputConnection(svtkAlgorithmOutput* aout)
{
  // Return if the connection already exists
  int numDO = this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0);
  for (int i = 0; i < numDO; i++)
  {
    svtkAlgorithmOutput* port = this->DataObjectInputConnectionHolder->GetInputConnection(0, i);
    if (port == aout)
    {
      return;
    }
  }

  this->DataObjectInputConnectionHolder->AddInputConnection(aout);
}

//----------------------------------------------------------------------------
// Add a data object to the list of data to plot.
void svtkXYPlotActor::AddDataObjectInput(svtkDataObject* in)
{
  svtkTrivialProducer* tp = svtkTrivialProducer::New();
  tp->SetOutput(in);
  this->AddDataObjectInputConnection(tp->GetOutputPort());
  tp->Delete();
}

//----------------------------------------------------------------------------
// Remove a data object from the list of data to plot.
void svtkXYPlotActor::RemoveDataObjectInputConnection(svtkAlgorithmOutput* aout)
{
  int numDO = this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0);
  for (int i = 0; i < numDO; i++)
  {
    svtkAlgorithmOutput* port = this->DataObjectInputConnectionHolder->GetInputConnection(0, i);
    if (port == aout)
    {
      this->DataObjectInputConnectionHolder->RemoveInputConnection(0, i);
      break;
    }
  }
}

//----------------------------------------------------------------------------
// Remove a data object from the list of data to plot.
void svtkXYPlotActor::RemoveDataObjectInput(svtkDataObject* in)
{
  int numDO = this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0);
  for (int i = 0; i < numDO; i++)
  {
    svtkAlgorithmOutput* port = this->DataObjectInputConnectionHolder->GetInputConnection(0, i);
    svtkAlgorithm* alg = port->GetProducer();
    int portIdx = port->GetIndex();
    if (alg->GetOutputDataObject(portIdx) == in)
    {
      this->DataObjectInputConnectionHolder->RemoveInputConnection(0, i);
      break;
    }
  }
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int svtkXYPlotActor::RenderOverlay(svtkViewport* viewport)
{
  int renderedSomething = 0;

  // Make sure input is up to date.
  if (this->InputConnectionHolder->GetNumberOfInputConnections(0) < 1 &&
    this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0) < 1)
  {
    svtkErrorMacro(<< "Nothing to plot!");
    return 0;
  }

  if (this->ChartBox)
  {
    renderedSomething += this->ChartBoxActor->RenderOverlay(viewport);
  }
  if (this->ChartBorder)
  {
    renderedSomething += this->ChartBorderActor->RenderOverlay(viewport);
  }

  renderedSomething += this->XAxis->RenderOverlay(viewport);
  renderedSomething += this->YAxis->RenderOverlay(viewport);
  if (this->Title)
  {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
  }
  for (int i = 0; i < this->NumberOfInputs; i++)
  {
    renderedSomething += this->PlotActor[i]->RenderOverlay(viewport);
  }
  if (this->ShowReferenceXLine || this->ShowReferenceYLine)
  {
    renderedSomething += this->ReferenceLinesActor->RenderOverlay(viewport);
  }
  if (this->Legend)
  {
    renderedSomething += this->LegendActor->RenderOverlay(viewport);
  }
  if (this->YTitleActor)
  {
    renderedSomething += this->YTitleActor->RenderOverlay(viewport);
  }

  return renderedSomething;
}

//----------------------------------------------------------------------------
// Plot scalar data for each input dataset.
int svtkXYPlotActor::RenderOpaqueGeometry(svtkViewport* viewport)
{
  svtkMTimeType mtime, dsMtime;
  svtkDataObject* dobj;
  int numDS, numDO, renderedSomething = 0;

  // Initialize
  // Make sure input is up to date.
  numDS = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  numDO = this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0);
  if (numDS > 0)
  {
    svtkDebugMacro(<< "Plotting input data sets");
    mtime = 0;
    for (int i = 0; i < numDS; i++)
    {
      svtkAlgorithmOutput* port = this->InputConnectionHolder->GetInputConnection(0, i);
      svtkAlgorithm* alg = port->GetProducer();
      int portIdx = port->GetIndex();
      alg->Update(portIdx);
      dobj = alg->GetOutputDataObject(portIdx);
      dsMtime = dobj->GetMTime();
      if (dsMtime > mtime)
      {
        mtime = dsMtime;
      }
    }
  }
  else if (numDO > 0)
  {
    svtkDebugMacro(<< "Plotting input data objects");
    mtime = 0;
    for (int i = 0; i < numDO; i++)
    {
      svtkAlgorithmOutput* port = this->DataObjectInputConnectionHolder->GetInputConnection(0, i);
      svtkAlgorithm* alg = port->GetProducer();
      int portIdx = port->GetIndex();
      alg->Update(portIdx);
      dobj = alg->GetOutputDataObject(portIdx);
      dsMtime = dobj->GetMTime();
      if (dsMtime > mtime)
      {
        mtime = dsMtime;
      }
    }
  }
  else
  {
    svtkErrorMacro(<< "Nothing to plot!");
    return 0;
  }

  if (this->Title && this->Title[0] && !this->TitleTextProperty)
  {
    svtkErrorMacro(<< "Need a title text property to render plot title");
    return 0;
  }

  // Check modified time to see whether we have to rebuild.
  // Pay attention that GetMTime() has been redefined ( see below )

  const int* size = viewport->GetSize();
  if (mtime > this->BuildTime || size[0] != this->CachedSize[0] || size[1] != this->CachedSize[1] ||
    this->GetMTime() > this->BuildTime ||
    (this->Title && this->Title[0] && this->TitleTextProperty->GetMTime() > this->BuildTime) ||
    (this->AxisLabelTextProperty && this->AxisLabelTextProperty->GetMTime() > this->BuildTime) ||
    (this->AxisTitleTextProperty && this->AxisTitleTextProperty->GetMTime() > this->BuildTime))
  {
    double range[2], yrange[2], xRange[2], yRange[2], interval, *lengths = nullptr;
    int pos[2], pos2[2], numTicks;
    int stringSize[2];
    int num = (numDS > 0 ? numDS : numDO);

    svtkDebugMacro(<< "Rebuilding plot");
    this->CachedSize[0] = size[0];
    this->CachedSize[1] = size[1];

    // manage legend
    svtkDebugMacro(<< "Rebuilding legend");
    if (this->Legend)
    {
      int legPos[2], legPos2[2];
      int* p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
      int* p2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
      legPos[0] = (int)(p1[0] + this->LegendPosition[0] * (p2[0] - p1[0]));
      legPos2[0] = (int)(legPos[0] + this->LegendPosition2[0] * (p2[0] - p1[0]));
      legPos[1] = (int)(p1[1] + this->LegendPosition[1] * (p2[1] - p1[1]));
      legPos2[1] = (int)(legPos[1] + this->LegendPosition2[1] * (p2[1] - p1[1]));

      this->LegendActor->GetPositionCoordinate()->SetValue((double)legPos[0], (double)legPos[1]);
      this->LegendActor->GetPosition2Coordinate()->SetValue((double)legPos2[0], (double)legPos2[1]);
      this->LegendActor->SetNumberOfEntries(num);
      for (int i = 0; i < num; i++)
      {
        if (!this->LegendActor->GetEntrySymbol(i))
        {
          this->LegendActor->SetEntrySymbol(i, this->GlyphSource->GetOutput());
        }
        if (!this->LegendActor->GetEntryString(i))
        {
          char legendString[18];
          snprintf(legendString, sizeof(legendString), "%s%d", "Curve ", i);
          this->LegendActor->SetEntryString(i, legendString);
        }
      }

      this->LegendActor->SetPadding(2);
      this->LegendActor->GetProperty()->DeepCopy(this->GetProperty());
      this->LegendActor->ScalarVisibilityOff();
    }

    // Rebuid text props
    // Perform shallow copy here since each individual axis can be
    // accessed through the class API ( i.e. each individual axis text prop
    // can be changed ). Therefore, we can not just assign pointers otherwise
    // each individual axis text prop would point to the same text prop.

    if (this->AxisLabelTextProperty && this->AxisLabelTextProperty->GetMTime() > this->BuildTime)
    {
      if (this->XAxis->GetLabelTextProperty())
      {
        this->XAxis->GetLabelTextProperty()->ShallowCopy(this->AxisLabelTextProperty);
      }
      if (this->YAxis->GetLabelTextProperty())
      {
        this->YAxis->GetLabelTextProperty()->ShallowCopy(this->AxisLabelTextProperty);
      }
    }

    if (this->AxisTitleTextProperty && this->AxisTitleTextProperty->GetMTime() > this->BuildTime)
    {
      if (this->XAxis->GetTitleTextProperty())
      {
        this->XAxis->GetTitleTextProperty()->ShallowCopy(this->AxisTitleTextProperty);
      }
      if (this->YAxis->GetTitleTextProperty())
      {
        this->YAxis->GetTitleTextProperty()->ShallowCopy(this->AxisTitleTextProperty);
      }
      if (this->YTitleActor->GetTextProperty())
      {
        this->YTitleActor->GetTextProperty()->ShallowCopy(this->AxisTitleTextProperty);
      }
    }

    // setup x-axis
    svtkDebugMacro(<< "Rebuilding x-axis");

    this->XAxis->SetTitle(this->XTitle);
    this->XAxis->SetNumberOfLabels(this->NumberOfXLabels);
    this->XAxis->SetProperty(this->GetProperty());

    lengths = new double[num];
    if (numDS > 0) // plotting data sets
    {
      this->ComputeXRange(range, lengths);
    }
    else
    {
      this->ComputeDORange(range, yrange, lengths);
    }
    if (this->XRange[0] < this->XRange[1])
    {
      range[0] = this->XRange[0];
      range[1] = this->XRange[1];
    }

    if (this->AdjustXLabels)
    {
      svtkAxisActor2D::ComputeRange(range, xRange, this->NumberOfXLabels, numTicks, interval);
    }
    else
    {
      xRange[0] = range[0];
      xRange[1] = range[1];
    }

    if (!this->ExchangeAxes)
    {
      this->XComputedRange[0] = xRange[0];
      this->XComputedRange[1] = xRange[1];
      if (this->ReverseXAxis)
      {
        this->XAxis->SetRange(range[1], range[0]);
      }
      else
      {
        this->XAxis->SetRange(range[0], range[1]);
      }
    }
    else
    {
      this->XComputedRange[1] = xRange[0];
      this->XComputedRange[0] = xRange[1];
      if (this->ReverseYAxis)
      {
        this->XAxis->SetRange(range[0], range[1]);
      }
      else
      {
        this->XAxis->SetRange(range[1], range[0]);
      }
    }

    // setup y-axis
    svtkDebugMacro(<< "Rebuilding y-axis");
    this->YAxis->SetNumberOfLabels(this->NumberOfYLabels);

    if (this->YRange[0] >= this->YRange[1])
    {
      if (numDS > 0) // plotting data sets
      {
        this->ComputeYRange(yrange);
      }
    }
    else
    {
      yrange[0] = this->YRange[0];
      yrange[1] = this->YRange[1];
    }

    if (this->AdjustYLabels)
    {
      svtkAxisActor2D::ComputeRange(yrange, yRange, this->NumberOfYLabels, numTicks, interval);
    }
    else
    {
      yRange[0] = yrange[0];
      yRange[1] = yrange[1];
    }

    if (!this->ExchangeAxes)
    {
      this->YComputedRange[0] = yRange[0];
      this->YComputedRange[1] = yRange[1];
      if (this->ReverseYAxis)
      {
        this->YAxis->SetRange(yrange[0], yrange[1]);
      }
      else
      {
        this->YAxis->SetRange(yrange[1], yrange[0]);
      }
    }
    else
    {
      this->YComputedRange[1] = yRange[0];
      this->YComputedRange[0] = yRange[1];
      if (this->ReverseXAxis)
      {
        this->YAxis->SetRange(yrange[1], yrange[0]);
      }
      else
      {
        this->YAxis->SetRange(yrange[0], yrange[1]);
      }
    }

    this->PlaceAxes(viewport, size, pos, pos2);

    // Update y axis title position
    // NB: Must be done after call to PlaceAxes() which calculates YTitleSize and YAxisTitleSize
    if (strcmp(this->YTitleActor->GetInput(), ""))
    {
      this->YTitleActor->GetTextProperty()->SetFontSize(this->YAxisTitleSize);

      int* p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);

      // Retrieve lower endpoint of Y axis
      int* yaxis_p1 = this->YAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);

      // Retrieve upper endpoint of Y axis
      int* yaxis_p2 = this->YAxis->GetPosition2Coordinate()->GetComputedViewportValue(viewport);

      int yaxis = yaxis_p1[1] - yaxis_p2[1];
      int yaxis_ymiddle = (int)(yaxis * .5);
      int ytitle_half_height = (int)(this->YTitleSize[1] * .5);
      int ytitle_width = this->YTitleSize[0];
      int ytitlePos[2];
      ytitlePos[0] = 0;
      ytitlePos[1] = 0;
      switch (this->YTitlePosition)
      {
        case SVTK_XYPLOT_Y_AXIS_TOP:
        {
          this->YTitleActor->SetOrientation(0.);
          // Make sure that title does not exceed actor bounds
          int val = yaxis_p1[0] - this->YTitleDelta - ytitle_width;
          ytitlePos[0] = val < p1[0] ? p1[0] : val;
          ytitlePos[1] = yaxis_p1[1] + 10;
          break;
        }
        case SVTK_XYPLOT_Y_AXIS_HCENTER:
        {
          this->YTitleActor->SetOrientation(0.);
          // YTitleActor might exceed actor bounds
          ytitlePos[0] = yaxis_p1[0] - this->YTitleDelta - this->YTitleSize[0];
          ytitlePos[1] = (int)(yaxis_p2[1] + yaxis_ymiddle - ytitle_half_height);
          break;
        }
        case SVTK_XYPLOT_Y_AXIS_VCENTER:
        {
          this->YTitleActor->SetOrientation(90.);
          int val = (int)((yaxis - ytitle_width) * .4);
          ytitlePos[0] = yaxis_p1[0] - this->YTitleDelta;
          ytitlePos[1] = ytitle_width > yaxis ? yaxis_p2[1] : yaxis_p2[1] + val;
          break;
        }
      }
      this->YTitleActor->GetPositionCoordinate()->SetValue(
        (double)ytitlePos[0], (double)ytitlePos[1]);
    }

    // manage title
    if (this->Title != nullptr && this->Title[0])
    {
      this->TitleMapper->SetInput(this->Title);
      if (this->TitleTextProperty->GetMTime() > this->BuildTime)
      {
        this->TitleMapper->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
      }

      svtkTextMapper::SetRelativeFontSize(this->TitleMapper, viewport, size, stringSize, 0.015);

      if (this->AdjustTitlePosition)
      {
        this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();
        double titlePos[2];
        switch (this->AdjustTitlePositionMode & (AlignLeft | AlignRight | AlignHCenter))
        {
          default:
          case AlignLeft:
            titlePos[0] = pos[0];
            break;
          case AlignRight:
            titlePos[0] = pos2[0];
            break;
          case AlignHCenter:
            titlePos[0] = pos[0] + .5 * (pos2[0] - pos[0]);
            break;
        };
        switch (this->AdjustTitlePositionMode & (AlignAxisLeft | AlignAxisRight | AlignAxisHCenter))
        {
          case AlignAxisLeft:
            titlePos[0] -= stringSize[0];
            break;
          case AlignAxisRight:
            break;
          case AlignAxisHCenter:
            titlePos[0] -= stringSize[0] / 2;
            break;
          default:
            titlePos[0] -= (this->AdjustTitlePositionMode & AlignLeft) ? stringSize[0] : 0;
            break;
        };
        switch (this->AdjustTitlePositionMode & (AlignTop | AlignBottom | AlignVCenter))
        {
          default:
          case AlignTop:
            titlePos[1] = pos2[1];
            break;
          case AlignBottom:
            titlePos[1] = pos[1];
            break;
          case AlignVCenter:
            titlePos[1] = pos[1] + .5 * (pos2[1] - pos[1]);
        };

        switch (this->AdjustTitlePositionMode & (AlignAxisTop | AlignAxisBottom | AlignAxisVCenter))
        {
          case AlignAxisTop:
            titlePos[1] +=
              (this->AdjustTitlePositionMode & AlignTop) ? this->Border : -this->Border;
            break;
          case AlignAxisBottom:
            titlePos[1] -= stringSize[1];
            break;
          case AlignAxisVCenter:
            titlePos[1] -= stringSize[1] / 2;
            break;
          default:
            titlePos[1] += (this->AdjustTitlePositionMode & AlignTop) ? stringSize[1] : 0;
            break;
        };
        this->TitleActor->GetPositionCoordinate()->SetValue(titlePos[0], titlePos[1]);
        // this->TitleActor->GetPositionCoordinate()->SetValue(
        //  pos[0] + .5 * ( pos2[0] - pos[0] ) - stringSize[0] / 2.0,
        //  pos2[1] - stringSize[1] / 2.0 );
      }
      else
      {
        this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
        this->TitleActor->GetPositionCoordinate()->SetValue(
          this->TitlePosition[0], this->TitlePosition[1]);
      }
    }

    // Border and box - may adjust spacing based on font size relationship
    // to the proportions relative to the border
    //
    if (this->ChartBox || this->ChartBorder)
    {
      double doubleP1[3], doubleP2[3];

      doubleP1[0] = static_cast<double>(pos[0]);
      doubleP1[1] = static_cast<double>(pos[1]);
      doubleP1[2] = 0.;
      doubleP2[0] = static_cast<double>(pos2[0]);
      doubleP2[1] = static_cast<double>(pos2[1]);
      doubleP2[2] = 0.;

      svtkPoints* pts = this->ChartBoxPolyData->GetPoints();
      pts->SetPoint(0, doubleP1);
      pts->SetPoint(1, doubleP2[0], doubleP1[1], 0.0);
      pts->SetPoint(2, doubleP2);
      pts->SetPoint(3, doubleP1[0], doubleP2[1], 0.0);

      this->ChartBorderActor->SetProperty(this->GetProperty());
    }
    // Reference lines
    if (this->ShowReferenceXLine || this->ShowReferenceYLine)
    {
      double doubleP1[3], doubleP2[3];

      doubleP1[0] = static_cast<double>(pos[0]);
      doubleP1[1] = static_cast<double>(pos[1]);
      doubleP1[2] = 0.;
      doubleP2[0] = static_cast<double>(pos2[0]);
      doubleP2[1] = static_cast<double>(pos2[1]);
      doubleP2[2] = 0.;

      svtkPoints* pts = this->ReferenceLinesPolyData->GetPoints();
      if (this->ShowReferenceXLine && this->ReferenceXValue >= xRange[0] &&
        this->ReferenceXValue < xRange[1])
      {
        double xRefPos = doubleP1[0] +
          (this->ReferenceXValue - xRange[0]) / (xRange[1] - xRange[0]) *
            (doubleP2[0] - doubleP1[0]);
        pts->SetPoint(0, xRefPos, doubleP1[1], 0.0);
        pts->SetPoint(1, xRefPos, doubleP2[1], 0.0);
      }
      else
      {
        pts->SetPoint(0, doubleP1);
        pts->SetPoint(1, doubleP1);
      }
      if (this->ShowReferenceYLine && this->ReferenceYValue >= yRange[0] &&
        this->ReferenceYValue < yRange[1])
      {
        double yRefPos = doubleP1[1] +
          (this->ReferenceYValue - yRange[0]) / (yRange[1] - yRange[0]) *
            (doubleP2[1] - doubleP1[1]);
        pts->SetPoint(2, doubleP1[0], yRefPos, 0.);
        pts->SetPoint(3, doubleP2[0], yRefPos, 0.);
      }
      else
      {
        pts->SetPoint(2, doubleP1);
        pts->SetPoint(3, doubleP1);
      }
      // copy the color/linewidth/opacity...
      this->ReferenceLinesActor->SetProperty(this->GetProperty());
    }
    svtkDebugMacro(<< "Creating Plot Data");
    // Okay, now create the plot data and set up the pipeline
    this->CreatePlotData(pos, pos2, xRange, yRange, lengths, numDS, numDO);
    delete[] lengths;

    this->BuildTime.Modified();

  } // if need to rebuild the plot

  svtkDebugMacro(<< "Rendering Box");
  if (this->ChartBox)
  {
    renderedSomething += this->ChartBoxActor->RenderOpaqueGeometry(viewport);
  }
  if (this->ChartBorder)
  {
    renderedSomething += this->ChartBorderActor->RenderOpaqueGeometry(viewport);
  }
  if (this->ShowReferenceXLine || this->ShowReferenceYLine)
  {
    renderedSomething += this->ReferenceLinesActor->RenderOpaqueGeometry(viewport);
  }
  svtkDebugMacro(<< "Rendering Axes");
  renderedSomething += this->XAxis->RenderOpaqueGeometry(viewport);
  renderedSomething += this->YAxis->RenderOpaqueGeometry(viewport);
  if (this->YTitleActor)
  {
    svtkDebugMacro(<< "Rendering ytitleactor");
    renderedSomething += this->YTitleActor->RenderOpaqueGeometry(viewport);
  }
  for (int i = 0; i < this->NumberOfInputs; ++i)
  {
    svtkDebugMacro(<< "Rendering plotactors");
    renderedSomething += this->PlotActor[i]->RenderOpaqueGeometry(viewport);
  }
  if (this->Title)
  {
    svtkDebugMacro(<< "Rendering titleactors");
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
  }
  if (this->Legend)
  {
    svtkDebugMacro(<< "Rendering legendeactors");
    renderedSomething += this->LegendActor->RenderOpaqueGeometry(viewport);
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
svtkTypeBool svtkXYPlotActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
const char* svtkXYPlotActor::GetXValuesAsString()
{
  switch (this->XValues)
  {
    case SVTK_XYPLOT_INDEX:
      return "Index";
    case SVTK_XYPLOT_ARC_LENGTH:
      return "ArcLength";
    case SVTK_XYPLOT_NORMALIZED_ARC_LENGTH:
      return "NormalizedArcLength";
    default:
      return "Value";
  }
}

//----------------------------------------------------------------------------
const char* svtkXYPlotActor::GetDataObjectPlotModeAsString()
{
  if (this->DataObjectPlotMode == SVTK_XYPLOT_ROW)
  {
    return "Plot Rows";
  }
  else
  {
    return "Plot Columns";
  }
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void svtkXYPlotActor::ReleaseGraphicsResources(svtkWindow* win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->XAxis->ReleaseGraphicsResources(win);
  this->YAxis->ReleaseGraphicsResources(win);
  for (int i = 0; i < this->NumberOfInputs; i++)
  {
    this->PlotActor[i]->ReleaseGraphicsResources(win);
  }
  this->LegendActor->ReleaseGraphicsResources(win);
  if (this->ChartBoxActor)
  {
    this->ChartBoxActor->ReleaseGraphicsResources(win);
  }
  if (this->ChartBorderActor)
  {
    this->ChartBorderActor->ReleaseGraphicsResources(win);
  }
  if (this->ReferenceLinesActor)
  {
    this->ReferenceLinesActor->ReleaseGraphicsResources(win);
  }
  if (this->YTitleActor)
  {
    this->YTitleActor->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkXYPlotActor::GetMTime()
{
  svtkMTimeType mtime, mtime2;
  mtime = this->svtkActor2D::GetMTime();

  if (this->Legend)
  {
    mtime2 = this->LegendActor->GetMTime();
    if (mtime2 > mtime)
    {
      mtime = mtime2;
    }
  }

  return mtime;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkIndent i2 = indent.GetNextIndent();
  svtkAlgorithmOutput* input;
  char* array;
  int component;
  int idx, num;

  this->Superclass::PrintSelf(os, indent);

  num = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  os << indent << "DataSetInputs: " << endl;
  for (idx = 0; idx < num; ++idx)
  {
    input = this->InputConnectionHolder->GetInputConnection(0, idx);
    array = this->SelectedInputScalars[idx];
    component = this->SelectedInputScalarsComponent->GetValue((svtkIdType)idx);
    if (array == nullptr)
    {
      os << i2 << "(" << input << ") Default Scalars,  Component = " << component << endl;
    }
    else
    {
      os << i2 << "(" << input << ") " << array << ",  Component = " << component << endl;
    }
  }

  os << indent << "Input DataObjects:\n";
  num = this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0);
  for (idx = 0; idx < num; ++idx)
  {
    input = this->DataObjectInputConnectionHolder->GetInputConnection(0, idx);
    os << i2 << input << endl;
  }

  if (this->TitleTextProperty)
  {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Title Text Property: ( none )\n";
  }

  if (this->AxisTitleTextProperty)
  {
    os << indent << "Axis Title Text Property:\n";
    this->AxisTitleTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Axis Title Text Property: ( none )\n";
  }

  if (this->AxisLabelTextProperty)
  {
    os << indent << "Axis Label Text Property:\n";
    this->AxisLabelTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Axis Label Text Property: ( none )\n";
  }

  os << indent << "Data Object Plot Mode: " << this->GetDataObjectPlotModeAsString() << endl;

  os << indent << "Title: " << (this->Title ? this->Title : "( none )") << "\n";
  os << indent << "X Title: " << (this->XTitle ? this->XTitle : "( none )") << "\n";

  os << indent << "X Values: " << this->GetXValuesAsString() << endl;
  os << indent << "Log X Values: " << (this->Logx ? "On\n" : "Off\n");

  os << indent << "Plot global-points: " << (this->PlotPoints ? "On\n" : "Off\n");
  os << indent << "Plot global-lines: " << (this->PlotLines ? "On\n" : "Off\n");
  os << indent << "Plot per-curve points: " << (this->PlotCurvePoints ? "On\n" : "Off\n");
  os << indent << "Plot per-curve lines: " << (this->PlotCurveLines ? "On\n" : "Off\n");
  os << indent << "Exchange Axes: " << (this->ExchangeAxes ? "On\n" : "Off\n");
  os << indent << "Reverse X Axis: " << (this->ReverseXAxis ? "On\n" : "Off\n");
  os << indent << "Reverse Y Axis: " << (this->ReverseYAxis ? "On\n" : "Off\n");

  os << indent << "Number Of X Labels: " << this->NumberOfXLabels << "\n";
  os << indent << "Number Of Y Labels: " << this->NumberOfYLabels << "\n";

  os << indent << "X Label Format: " << this->XLabelFormat << "\n";
  os << indent << "Y Label Format: " << this->YLabelFormat << "\n";
  os << indent << "Border: " << this->Border << "\n";

  os << indent << "X Range: ";
  if (this->XRange[0] >= this->XRange[1])
  {
    os << indent << "( Automatically Computed )\n";
  }
  else
  {
    os << "(" << this->XRange[0] << ", " << this->XRange[1] << ")\n";
  }

  os << indent << "Y Range: ";
  if (this->XRange[0] >= this->YRange[1])
  {
    os << indent << "( Automatically Computed )\n";
  }
  else
  {
    os << "(" << this->YRange[0] << ", " << this->YRange[1] << ")\n";
  }

  os << indent << "Viewport Coordinate: (" << this->ViewportCoordinate[0] << ", "
     << this->ViewportCoordinate[1] << ")\n";

  os << indent << "Plot Coordinate: (" << this->PlotCoordinate[0] << ", " << this->PlotCoordinate[1]
     << ")\n";

  os << indent << "Legend: " << (this->Legend ? "On\n" : "Off\n");
  os << indent << "Legend Position: (" << this->LegendPosition[0] << ", " << this->LegendPosition[1]
     << ")\n";
  os << indent << "Legend Position2: (" << this->LegendPosition2[0] << ", "
     << this->LegendPosition2[1] << ")\n";

  os << indent << "Glyph Size: " << this->GlyphSize << endl;

  os << indent << "Legend Actor:";
  this->LegendActor->PrintSelf(os << endl, i2);
  os << indent << "Glyph Source:";
  this->GlyphSource->PrintSelf(os << endl, i2);

  os << indent << "AdjustXLabels: " << this->AdjustXLabels << endl;
  os << indent << "AdjustYLabels: " << this->AdjustYLabels << endl;
  os << indent << "AdjustTitlePosition: " << this->AdjustTitlePosition << endl;
  os << indent << "TitlePosition: " << this->TitlePosition[0] << " " << this->TitlePosition[1]
     << " " << endl;
  os << indent << "AdjustTitlePositionMode: " << this->AdjustTitlePositionMode << endl;
  os << indent << "ChartBox: " << (this->ChartBox ? "On\n" : "Off\n");
  os << indent << "ChartBorder: " << (this->ChartBorder ? "On\n" : "Off\n");
  os << indent << "ShowReferenceXLine: " << (this->ShowReferenceXLine ? "On\n" : "Off\n");
  os << indent << "ReferenceXValue: " << this->ReferenceXValue << endl;
  os << indent << "ShowReferenceYLine: " << (this->ShowReferenceYLine ? "On\n" : "Off\n");
  os << indent << "ReferenceYValue: " << this->ReferenceYValue << endl;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::ComputeXRange(double range[2], double* lengths)
{
  int dsNum;
  svtkIdType numPts, ptId, maxNum;
  double maxLength = 0.0, xPrev[3], x[3];
  svtkDataSet* ds;

  range[0] = SVTK_DOUBLE_MAX;
  range[1] = SVTK_DOUBLE_MIN;

  int numDS = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  for (dsNum = 0, maxNum = 0; dsNum < numDS; dsNum++)
  {
    svtkAlgorithmOutput* port = this->InputConnectionHolder->GetInputConnection(0, dsNum);
    svtkAlgorithm* alg = port->GetProducer();
    int portIndex = port->GetIndex();
    ds = svtkDataSet::SafeDownCast(alg->GetOutputDataObject(portIndex));
    numPts = ds->GetNumberOfPoints();
    if (numPts == 0)
    {
      svtkErrorMacro(<< "No scalar data to plot!");
      continue;
    }

    if (this->XValues != SVTK_XYPLOT_INDEX)
    {
      ds->GetPoint(0, xPrev);
      for (lengths[dsNum] = 0.0, ptId = 0; ptId < numPts; ptId++)
      {
        ds->GetPoint(ptId, x);
        switch (this->XValues)
        {
          case SVTK_XYPLOT_VALUE:
            if (this->GetLogx() == 0)
            {
              if (x[this->XComponent->GetValue(dsNum)] < range[0])
              {
                range[0] = x[this->XComponent->GetValue(dsNum)];
              }
              if (x[this->XComponent->GetValue(dsNum)] > range[1])
              {
                range[1] = x[this->XComponent->GetValue(dsNum)];
              }
            }
            else
            {
              // ensure range strictly > 0 for log
              if ((x[this->XComponent->GetValue(dsNum)]) < range[0] &&
                (x[this->XComponent->GetValue(dsNum)] > 0))
              {
                range[0] = x[this->XComponent->GetValue(dsNum)];
              }
              if ((x[this->XComponent->GetValue(dsNum)] > range[1]) &&
                (x[this->XComponent->GetValue(dsNum)] > 0))
              {
                range[1] = x[this->XComponent->GetValue(dsNum)];
              }
            }
            break;
          default:
            lengths[dsNum] += sqrt(svtkMath::Distance2BetweenPoints(x, xPrev));
            xPrev[0] = x[0];
            xPrev[1] = x[1];
            xPrev[2] = x[2];
        }
      } // for all points
      if (lengths[dsNum] > maxLength)
      {
        maxLength = lengths[dsNum];
      }
    } // if need to visit all points

    else // if ( this->XValues == SVTK_XYPLOT_INDEX )
    {
      if (numPts > maxNum)
      {
        maxNum = numPts;
      }
    }
  } // over all datasets

  // determine the range
  switch (this->XValues)
  {
    case SVTK_XYPLOT_ARC_LENGTH:
      range[0] = 0.;
      range[1] = maxLength;
      break;
    case SVTK_XYPLOT_NORMALIZED_ARC_LENGTH:
      range[0] = 0.;
      range[1] = 1.;
      break;
    case SVTK_XYPLOT_INDEX:
      range[0] = 0.;
      range[1] = (double)(maxNum - 1);
      break;
    case SVTK_XYPLOT_VALUE:
      if (this->GetLogx() == 1)
      {
        if (range[0] > range[1])
        {
          range[0] = 0;
          range[1] = 0;
        }
        else
        {
          range[0] = log10(range[0]);
          range[1] = log10(range[1]);
        }
      }
      break; // range computed in for loop above
    default:
      svtkErrorMacro(<< "Unknown X-Value option.");
      return;
  }
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::ComputeYRange(double range[2])
{
  svtkDataSet* ds;
  svtkDataArray* scalars;
  double sRange[2];
  int component;

  range[0] = SVTK_DOUBLE_MAX;
  range[1] = SVTK_DOUBLE_MIN;

  int numDS = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  for (int dsNum = 0, count = 0; dsNum < numDS; dsNum++, count++)
  {
    svtkAlgorithmOutput* port = this->InputConnectionHolder->GetInputConnection(0, dsNum);
    svtkAlgorithm* alg = port->GetProducer();
    int portIndex = port->GetIndex();
    ds = svtkDataSet::SafeDownCast(alg->GetOutputDataObject(portIndex));
    scalars = ds->GetPointData()->GetScalars(this->SelectedInputScalars[count]);
    component = this->SelectedInputScalarsComponent->GetValue(count);
    if (!scalars)
    {
      svtkErrorMacro(<< "No scalar data to plot!");
      continue;
    }
    if (component < 0 || component >= scalars->GetNumberOfComponents())
    {
      svtkErrorMacro(<< "Bad component!");
      continue;
    }

    scalars->GetRange(sRange, component);
    if (sRange[0] < range[0])
    {
      range[0] = sRange[0];
    }

    if (sRange[1] > range[1])
    {
      range[1] = sRange[1];
    }
  } // over all datasets
}

//----------------------------------------------------------------------------
static inline int svtkXYPlotActorGetComponent(
  svtkFieldData* field, svtkIdType tuple, int component, double* val)
{
  int array_comp;
  int array_index = field->GetArrayContainingComponent(component, array_comp);
  if (array_index < 0)
  {
    return 0;
  }
  svtkDataArray* da = field->GetArray(array_index);
  if (!da)
  {
    // non-numeric array.
    return 0;
  }
  *val = da->GetComponent(tuple, array_comp);
  return 1;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::ComputeDORange(double xrange[2], double yrange[2], double* lengths)
{
  int i;
  svtkDataObject* dobj;
  svtkFieldData* field;
  int doNum, numColumns;
  svtkIdType numTuples, numRows, num, ptId, maxNum;
  double maxLength = 0.;
  double x = 0.;
  double y = 0.;
  double xPrev = 0.;
  svtkDataArray* array;

  // NOTE: FieldData can have non-numeric arrays. However, XY plot can only
  // work on numeric arrays ( or svtkDataArray subclasses ).

  xrange[0] = yrange[0] = SVTK_DOUBLE_MAX;
  xrange[1] = yrange[1] = -SVTK_DOUBLE_MAX;
  maxNum = 0;
  int numDOs = this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0);
  for (doNum = 0; doNum < numDOs; doNum++)
  {
    svtkAlgorithmOutput* port = this->DataObjectInputConnectionHolder->GetInputConnection(0, doNum);
    svtkAlgorithm* alg = port->GetProducer();
    int portIdx = port->GetIndex();
    dobj = alg->GetOutputDataObject(portIdx);

    lengths[doNum] = 0.;
    field = dobj->GetFieldData();
    numColumns = field->GetNumberOfComponents(); // number of "columns"
    // numColumns includes the components for non-numeric arrays as well.
    for (numRows = SVTK_ID_MAX, i = 0; i < field->GetNumberOfArrays(); i++)
    {
      array = field->GetArray(i);
      if (!array)
      {
        // non-numeric array, skip.
        continue;
      }
      numTuples = array->GetNumberOfTuples();
      if (numTuples < numRows)
      {
        numRows = numTuples;
      }
    }

    num = (this->DataObjectPlotMode == SVTK_XYPLOT_ROW ? numColumns : numRows);

    if (this->XValues != SVTK_XYPLOT_INDEX)
    {
      // gather the information to form a plot
      for (ptId = 0; ptId < num; ptId++)
      {
        int status = 0;

        if (this->DataObjectPlotMode == SVTK_XYPLOT_ROW)
        {
          // x = field->GetComponent( this->XComponent->GetValue( doNum ), ptId );
          status = ::svtkXYPlotActorGetComponent(field, this->XComponent->GetValue(doNum), ptId, &x);
        }
        else // if ( this->DataObjectPlotMode == SVTK_XYPLOT_COLUMN )
        {
          // x = field->GetComponent( ptId, this->XComponent->GetValue( doNum ) );
          status = ::svtkXYPlotActorGetComponent(field, ptId, this->XComponent->GetValue(doNum), &x);
        }
        if (!status)
        {
          // requested component falls in a non-numeric array, skip it.
          continue;
        }
        if (ptId == 0)
        {
          xPrev = x;
        }

        switch (this->XValues)
        {
          case SVTK_XYPLOT_VALUE:
            if (this->GetLogx() == 0)
            {
              if (x < xrange[0])
              {
                xrange[0] = x;
              }
              if (x > xrange[1])
              {
                xrange[1] = x;
              }
            }
            else // ensure positive values
            {
              if ((x < xrange[0]) && (x > 0))
              {
                xrange[0] = x;
              }
              if (x > xrange[1] && (x > 0))
              {
                xrange[1] = x;
              }
            }
            break;
          default:
            lengths[doNum] += fabs(x - xPrev);
            xPrev = x;
        }
      } // for all points
      if (lengths[doNum] > maxLength)
      {
        maxLength = lengths[doNum];
      }
    } // if all data has to be visited

    else // if ( this->XValues == SVTK_XYPLOT_INDEX )
    {
      if (num > maxNum)
      {
        maxNum = num;
      }
    }

    // Get the y-values
    for (ptId = 0; ptId < num; ptId++)
    {
      int status = 0;
      if (this->DataObjectPlotMode == SVTK_XYPLOT_ROW)
      {
        // y = field->GetComponent( this->YComponent->GetValue( doNum ), ptId );
        status = ::svtkXYPlotActorGetComponent(field, this->YComponent->GetValue(doNum), ptId, &y);
      }
      else // if ( this->DataObjectPlotMode == SVTK_XYPLOT_COLUMN )
      {
        // y = field->GetComponent( ptId, this->YComponent->GetValue( doNum ) );
        status = ::svtkXYPlotActorGetComponent(field, ptId, this->YComponent->GetValue(doNum), &y);
      }
      if (!status)
      {
        // requested component falls in non-numeric array.
        // skip.
        continue;
      }
      if (y < yrange[0])
      {
        yrange[0] = y;
      }
      if (y > yrange[1])
      {
        yrange[1] = y;
      }
    } // over all y values
  }   // over all dataobjects

  // determine the range
  switch (this->XValues)
  {
    case SVTK_XYPLOT_ARC_LENGTH:
      xrange[0] = 0.;
      xrange[1] = maxLength;
      break;
    case SVTK_XYPLOT_NORMALIZED_ARC_LENGTH:
      xrange[0] = 0.;
      xrange[1] = 1.;
      break;
    case SVTK_XYPLOT_INDEX:
      xrange[0] = 0.;
      xrange[1] = (double)(maxNum - 1);
      break;
    case SVTK_XYPLOT_VALUE:
      if (this->GetLogx() == 1)
      {
        xrange[0] = log10(xrange[0]);
        xrange[1] = log10(xrange[1]);
      }
      break;
    default:
      svtkErrorMacro(<< "Unknown X-Value option");
      return;
  }
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::CreatePlotData(
  int* pos, int* pos2, double xRange[2], double yRange[2], double* lengths, int numDS, int numDO)
{
  double xyz[3];
  xyz[2] = 0.;
  int i, numLinePts, doNum, num;
  svtkIdType numPts, ptId, id;
  double length, x[3], xPrev[3];
  svtkDataArray* scalars;
  int component;
  svtkDataSet* ds;
  svtkCellArray* lines;
  svtkPoints* pts;
  int clippingRequired = 0;

  // Allocate resources for the polygonal plots
  //
  num = (numDS > numDO ? numDS : numDO);
  this->InitializeEntries();
  this->NumberOfInputs = num;
  this->PlotData = new svtkPolyData*[num];
  this->PlotGlyph = new svtkGlyph2D*[num];
  this->PlotAppend = new svtkAppendPolyData*[num];
  this->PlotMapper = new svtkPolyDataMapper2D*[num];
  this->PlotActor = new svtkActor2D*[num];
  for (i = 0; i < num; i++)
  {
    this->PlotData[i] = svtkPolyData::New();
    this->PlotGlyph[i] = svtkGlyph2D::New();
    this->PlotGlyph[i]->SetInputData(this->PlotData[i]);
    this->PlotGlyph[i]->SetScaleModeToDataScalingOff();
    this->PlotAppend[i] = svtkAppendPolyData::New();
    this->PlotAppend[i]->AddInputData(this->PlotData[i]);
    if (this->LegendActor->GetEntrySymbol(i) != nullptr &&
      this->LegendActor->GetEntrySymbol(i) != this->GlyphSource->GetOutput())
    {
      this->PlotGlyph[i]->SetSourceData(this->LegendActor->GetEntrySymbol(i));
      this->PlotGlyph[i]->SetScaleFactor(this->ComputeGlyphScale(i, pos, pos2));
      this->PlotAppend[i]->AddInputConnection(this->PlotGlyph[i]->GetOutputPort());
    }
    this->PlotMapper[i] = svtkPolyDataMapper2D::New();
    this->PlotMapper[i]->SetInputConnection(this->PlotAppend[i]->GetOutputPort());
    this->PlotMapper[i]->ScalarVisibilityOff();
    this->PlotActor[i] = svtkActor2D::New();
    this->PlotActor[i]->SetMapper(this->PlotMapper[i]);
    this->PlotActor[i]->GetProperty()->DeepCopy(this->GetProperty());
    if (this->LegendActor->GetEntryColor(i)[0] < 0.0)
    {
      this->PlotActor[i]->GetProperty()->SetColor(this->GetProperty()->GetColor());
    }
    else
    {
      this->PlotActor[i]->GetProperty()->SetColor(this->LegendActor->GetEntryColor(i));
    }
  }

  // Prepare to receive data
  this->GenerateClipPlanes(pos, pos2);
  for (i = 0; i < this->NumberOfInputs; i++)
  {
    lines = svtkCellArray::New();
    pts = svtkPoints::New();

    lines->AllocateEstimate(10, 10);
    pts->Allocate(10, 10);
    this->PlotData[i]->SetPoints(pts);
    this->PlotData[i]->SetVerts(lines);
    this->PlotData[i]->SetLines(lines);

    pts->Delete();
    lines->Delete();
  }

  // Okay, for each input generate plot data. Depending on the input
  // we use either dataset or data object.
  //
  if (numDS > 0)
  {
    for (int dsNum = 0; dsNum < numDS; dsNum++)
    {
      svtkAlgorithmOutput* port = this->InputConnectionHolder->GetInputConnection(0, dsNum);
      svtkAlgorithm* alg = port->GetProducer();
      int portIndex = port->GetIndex();
      ds = svtkDataSet::SafeDownCast(alg->GetOutputDataObject(portIndex));
      clippingRequired = 0;
      numPts = ds->GetNumberOfPoints();
      scalars = ds->GetPointData()->GetScalars(this->SelectedInputScalars[dsNum]);
      if (!scalars)
      {
        continue;
      }
      if (scalars->GetNumberOfTuples() < numPts)
      {
        svtkErrorMacro("Number of points: " << numPts << " exceeds number of scalar tuples: "
                                           << scalars->GetNumberOfTuples());
        continue;
      }
      component = this->SelectedInputScalarsComponent->GetValue(dsNum);
      if (component < 0 || component >= scalars->GetNumberOfComponents())
      {
        continue;
      }

      pts = this->PlotData[dsNum]->GetPoints();
      lines = this->PlotData[dsNum]->GetLines();
      lines->InsertNextCell(0); // update the count later

      ds->GetPoint(0, xPrev);
      for (numLinePts = 0, length = 0.0, ptId = 0; ptId < numPts; ptId++)
      {
        xyz[1] = scalars->GetComponent(ptId, component);
        ds->GetPoint(ptId, x);
        switch (this->XValues)
        {
          case SVTK_XYPLOT_NORMALIZED_ARC_LENGTH:
            length += sqrt(svtkMath::Distance2BetweenPoints(x, xPrev));
            xyz[0] = length / lengths[dsNum];
            xPrev[0] = x[0];
            xPrev[1] = x[1];
            xPrev[2] = x[2];
            break;
          case SVTK_XYPLOT_INDEX:
            xyz[0] = (double)ptId;
            break;
          case SVTK_XYPLOT_ARC_LENGTH:
            length += sqrt(svtkMath::Distance2BetweenPoints(x, xPrev));
            xyz[0] = length;
            xPrev[0] = x[0];
            xPrev[1] = x[1];
            xPrev[2] = x[2];
            break;
          case SVTK_XYPLOT_VALUE:
            xyz[0] = x[this->XComponent->GetValue(dsNum)];
            break;
          default:
            svtkErrorMacro(<< "Unknown X-Component option");
        }

        if (this->GetLogx() == 1)
        {
          if (xyz[0] > 0)
          {
            xyz[0] = log10(xyz[0]);
            // normalize and position
            if (xyz[0] < xRange[0] || xyz[0] > xRange[1] || xyz[1] < yRange[0] ||
              xyz[1] > yRange[1])
            {
              clippingRequired = 1;
            }

            numLinePts++;
            xyz[0] = pos[0] + (xyz[0] - xRange[0]) / (xRange[1] - xRange[0]) * (pos2[0] - pos[0]);
            xyz[1] = pos[1] + (xyz[1] - yRange[0]) / (yRange[1] - yRange[0]) * (pos2[1] - pos[1]);
            id = pts->InsertNextPoint(xyz);
            lines->InsertCellPoint(id);
          }
        }
        else
        {
          // normalize and position
          if (xyz[0] < xRange[0] || xyz[0] > xRange[1] || xyz[1] < yRange[0] || xyz[1] > yRange[1])
          {
            clippingRequired = 1;
          }

          numLinePts++;
          xyz[0] = pos[0] + (xyz[0] - xRange[0]) / (xRange[1] - xRange[0]) * (pos2[0] - pos[0]);
          xyz[1] = pos[1] + (xyz[1] - yRange[0]) / (yRange[1] - yRange[0]) * (pos2[1] - pos[1]);
          id = pts->InsertNextPoint(xyz);
          lines->InsertCellPoint(id);
        }
      } // for all input points

      lines->UpdateCellCount(numLinePts);
      if (clippingRequired)
      {
        this->ClipPlotData(pos, pos2, this->PlotData[dsNum]);
      }
    } // loop over all input data sets
  }   // if plotting datasets

  else // plot data from data objects
  {
    svtkDataObject* dobj;
    int numColumns;
    svtkIdType numRows, numTuples;
    svtkDataArray* array;
    svtkFieldData* field;
    int numDOs = this->DataObjectInputConnectionHolder->GetNumberOfInputConnections(0);
    for (doNum = 0; doNum < numDOs; doNum++)
    {
      svtkAlgorithmOutput* port =
        this->DataObjectInputConnectionHolder->GetInputConnection(0, doNum);
      svtkAlgorithm* alg = port->GetProducer();
      int portIdx = port->GetIndex();
      dobj = alg->GetOutputDataObject(portIdx);

      // determine the shape of the field
      field = dobj->GetFieldData();
      numColumns = field->GetNumberOfComponents(); // number of "columns"
      // numColumns also includes non-numeric array components.
      for (numRows = SVTK_ID_MAX, i = 0; i < field->GetNumberOfArrays(); i++)
      {
        array = field->GetArray(i);
        if (!array)
        {
          // skip non-numeric arrays.
          continue;
        }
        numTuples = array->GetNumberOfTuples();
        if (numTuples < numRows)
        {
          numRows = numTuples;
        }
      }

      pts = this->PlotData[doNum]->GetPoints();
      lines = this->PlotData[doNum]->GetLines();
      lines->InsertNextCell(0); // update the count later

      numPts = (this->DataObjectPlotMode == SVTK_XYPLOT_ROW ? numColumns : numRows);

      // gather the information to form a plot
      for (numLinePts = 0, length = 0.0, ptId = 0; ptId < numPts; ptId++)
      {
        int status1, status2;
        if (this->DataObjectPlotMode == SVTK_XYPLOT_ROW)
        {
          // x[0] = field->GetComponent( this->XComponent->GetValue( doNum ),ptId );
          // xyz[1] = field->GetComponent( this->YComponent->GetValue( doNum ),ptId );
          status1 =
            ::svtkXYPlotActorGetComponent(field, this->XComponent->GetValue(doNum), ptId, &x[0]);
          status2 =
            ::svtkXYPlotActorGetComponent(field, this->YComponent->GetValue(doNum), ptId, &xyz[1]);
        }
        else // if ( this->DataObjectPlotMode == SVTK_XYPLOT_COLUMN )
        {
          // x[0] = field->GetComponent( ptId, this->XComponent->GetValue( doNum ) );
          // xyz[1] = field->GetComponent( ptId, this->YComponent->GetValue( doNum ) );

          status1 =
            ::svtkXYPlotActorGetComponent(field, ptId, this->XComponent->GetValue(doNum), &x[0]);

          if (!status1)
          {
            svtkWarningMacro(<< this->XComponent->GetValue(doNum) << " is a non-numeric component.");
          }

          status2 =
            ::svtkXYPlotActorGetComponent(field, ptId, this->YComponent->GetValue(doNum), &xyz[1]);

          if (!status2)
          {
            svtkWarningMacro(<< this->YComponent->GetValue(doNum) << " is a non-numeric component.");
          }
        }
        if (!status1 || !status2)
        {
          // component is non-numeric.
          // Skip it.
          continue;
        }

        switch (this->XValues)
        {
          case SVTK_XYPLOT_NORMALIZED_ARC_LENGTH:
            length += fabs(x[0] - xPrev[0]);
            xyz[0] = length / lengths[doNum];
            xPrev[0] = x[0];
            break;
          case SVTK_XYPLOT_INDEX:
            xyz[0] = (double)ptId;
            break;
          case SVTK_XYPLOT_ARC_LENGTH:
            length += fabs(x[0] - xPrev[0]);
            xyz[0] = length;
            xPrev[0] = x[0];
            break;
          case SVTK_XYPLOT_VALUE:
            xyz[0] = x[0];
            break;
          default:
            svtkErrorMacro(<< "Unknown X-Value option");
        }

        if (this->GetLogx() == 1)
        {
          if (xyz[0] > 0)
          {
            xyz[0] = log10(xyz[0]);
            // normalize and position
            if (xyz[0] < xRange[0] || xyz[0] > xRange[1] || xyz[1] < yRange[0] ||
              xyz[1] > yRange[1])
            {
              clippingRequired = 1;
            }
            numLinePts++;
            xyz[0] = pos[0] + (xyz[0] - xRange[0]) / (xRange[1] - xRange[0]) * (pos2[0] - pos[0]);
            xyz[1] = pos[1] + (xyz[1] - yRange[0]) / (yRange[1] - yRange[0]) * (pos2[1] - pos[1]);
            id = pts->InsertNextPoint(xyz);
            lines->InsertCellPoint(id);
          }
        }
        else
        {
          // normalize and position
          if (xyz[0] < xRange[0] || xyz[0] > xRange[1] || xyz[1] < yRange[0] || xyz[1] > yRange[1])
          {
            clippingRequired = 1;
          }
          numLinePts++;
          xyz[0] = pos[0] + (xyz[0] - xRange[0]) / (xRange[1] - xRange[0]) * (pos2[0] - pos[0]);
          xyz[1] = pos[1] + (xyz[1] - yRange[0]) / (yRange[1] - yRange[0]) * (pos2[1] - pos[1]);
          id = pts->InsertNextPoint(xyz);
          lines->InsertCellPoint(id);
        }
      } // for all input points

      lines->UpdateCellCount(numLinePts);
      if (clippingRequired)
      {
        this->ClipPlotData(pos, pos2, this->PlotData[doNum]);
      }
    } // loop over all input data sets
  }

  // Remove points/lines as directed by the user
  for (i = 0; i < num; i++)
  {
    if (!this->PlotCurveLines)
    {
      if (!this->PlotLines)
      {
        this->PlotData[i]->SetLines(nullptr);
      }
    }
    else
    {
      if (this->GetPlotLines(i) == 0)
      {
        this->PlotData[i]->SetLines(nullptr);
      }
    }

    if (!this->PlotCurvePoints)
    {
      if (!this->PlotPoints ||
        (this->LegendActor->GetEntrySymbol(i) &&
          this->LegendActor->GetEntrySymbol(i) != this->GlyphSource->GetOutput()))
      {
        this->PlotData[i]->SetVerts(nullptr);
      }
    }
    else
    {
      if (this->GetPlotPoints(i) == 0 ||
        (this->LegendActor->GetEntrySymbol(i) &&
          this->LegendActor->GetEntrySymbol(i) != this->GlyphSource->GetOutput()))
      {
        this->PlotData[i]->SetVerts(nullptr);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Position the axes taking into account the expected padding due to labels
// and titles. We want the result to fit in the box specified. This method
// knows something about how the svtkAxisActor2D functions, so it may have
// to change if that class changes dramatically.
//
void svtkXYPlotActor::PlaceAxes(svtkViewport* viewport, const int* size, int pos[2], int pos2[2])
{
  int titleSizeX[2], titleSizeY[2], labelSizeX[2], labelSizeY[2];
  double labelFactorX, labelFactorY;
  double fontFactorX, fontFactorY;
  double tickOffsetX, tickOffsetY;
  double tickLengthX, tickLengthY;

  svtkAxisActor2D* axisX;
  svtkAxisActor2D* axisY;

  char str1[512], str2[512];

  if (this->ExchangeAxes)
  {
    axisX = this->YAxis;
    axisY = this->XAxis;
  }
  else
  {
    axisX = this->XAxis;
    axisY = this->YAxis;
  }

  fontFactorY = axisY->GetFontFactor();
  fontFactorX = axisX->GetFontFactor();

  labelFactorY = axisY->GetLabelFactor();
  labelFactorX = axisX->GetLabelFactor();

  // Create a dummy text mapper for getting font sizes
  svtkTextMapper* textMapper = svtkTextMapper::New();
  svtkTextProperty* tprop = textMapper->GetTextProperty();

  // Get the location of the corners of the box
  int* p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
  int* p2 = this->Position2Coordinate->GetComputedViewportValue(viewport);

  // Estimate the padding around the X and Y axes
  tprop->ShallowCopy(axisX->GetTitleTextProperty());
  textMapper->SetInput(axisX->GetTitle());
  svtkTextMapper::SetRelativeFontSize(textMapper, viewport, size, titleSizeX, 0.015 * fontFactorX);

  tprop->ShallowCopy(axisY->GetTitleTextProperty());
  textMapper->SetInput(axisY->GetTitle());
  svtkTextMapper::SetRelativeFontSize(textMapper, viewport, size, titleSizeY, 0.015 * fontFactorY);

  // Retrieve X axis title font
  tprop->ShallowCopy(axisX->GetTitleTextProperty());
  // Calculate string length from YTitleActor,
  //  + 1 for the case where there is only one character
  //  + 1 for the final \0
  int len = int((strlen(YTitleActor->GetInput()) + 1) * .5) + 1;
  char* tmp = new char[len];
  switch (this->YTitlePosition)
  {
    case SVTK_XYPLOT_Y_AXIS_TOP:
      snprintf(tmp, len, "%s", YTitleActor->GetInput());
      textMapper->SetInput(tmp);
      break;
    case SVTK_XYPLOT_Y_AXIS_HCENTER:
      textMapper->SetInput(YTitleActor->GetInput());
      break;
    case SVTK_XYPLOT_Y_AXIS_VCENTER:
      // Create a dummy title to ensure that the added YTitleActor is visible
      textMapper->SetInput("AABB");
      break;
  }
  delete[] tmp;
  this->YAxisTitleSize =
    svtkTextMapper::SetRelativeFontSize(textMapper, viewport, size, titleSizeY, 0.015 * fontFactorY);

  this->YTitleSize[0] = titleSizeY[0];
  this->YTitleSize[1] = titleSizeY[1];

  // At this point the thing to do would be to actually ask the Y axis
  // actor to return the largest label.
  // In the meantime, let's try with the min and max
  snprintf(str1, sizeof(str1), axisY->GetLabelFormat(), axisY->GetAdjustedRange()[0]);
  snprintf(str2, sizeof(str2), axisY->GetLabelFormat(), axisY->GetAdjustedRange()[1]);
  tprop->ShallowCopy(axisY->GetLabelTextProperty());
  textMapper->SetInput(strlen(str1) > strlen(str2) ? str1 : str2);
  svtkTextMapper::SetRelativeFontSize(
    textMapper, viewport, size, labelSizeY, 0.015 * labelFactorY * fontFactorY);

  // We do only care of the height of the label in the X axis, so let's
  // use the min for example
  snprintf(str1, sizeof(str1), axisX->GetLabelFormat(), axisX->GetAdjustedRange()[0]);
  tprop->ShallowCopy(axisX->GetLabelTextProperty());
  textMapper->SetInput(str1);
  svtkTextMapper::SetRelativeFontSize(
    textMapper, viewport, size, labelSizeX, 0.015 * labelFactorX * fontFactorX);

  tickOffsetX = axisX->GetTickOffset();
  tickOffsetY = axisY->GetTickOffset();
  tickLengthX = axisX->GetTickLength();
  tickLengthY = axisY->GetTickLength();

  // Okay, estimate the size
  pos[0] =
    (int)(p1[0] + titleSizeY[0] + 2.0 * tickOffsetY + tickLengthY + labelSizeY[0] + this->Border);

  pos[1] =
    (int)(p1[1] + titleSizeX[1] + 2.0 * tickOffsetX + tickLengthX + labelSizeX[1] + this->Border);

  pos2[0] = (int)(p2[0] - labelSizeY[0] / 2 - tickOffsetY - this->Border);

  pos2[1] = (int)(p2[1] - labelSizeX[1] / 2 - tickOffsetX - this->Border);

  // Save estimated axis size to avoid recomputing of YTitleActor displacement
  if (this->YTitlePosition == SVTK_XYPLOT_Y_AXIS_TOP)
  {
    this->YTitleDelta = (int)(2 * tickOffsetY + tickLengthY + this->Border);
  }
  else
  {
    this->YTitleDelta = (int)(2 * tickOffsetY + tickLengthY + .75 * labelSizeY[0] + this->Border);
  }

  // Now specify the location of the axes
  axisX->GetPositionCoordinate()->SetValue((double)pos[0], (double)pos[1]);
  axisX->GetPosition2Coordinate()->SetValue((double)pos2[0], (double)pos[1]);
  axisY->GetPositionCoordinate()->SetValue((double)pos[0], (double)pos2[1]);
  axisY->GetPosition2Coordinate()->SetValue((double)pos[0], (double)pos[1]);

  textMapper->Delete();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::ViewportToPlotCoordinate(svtkViewport* viewport, double& u, double& v)
{
  int *p0, *p1, *p2;

  // XAxis, YAxis are in viewport coordinates already
  p0 = this->XAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPosition2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  u =
    ((u - p0[0]) / (double)(p1[0] - p0[0])) * (this->XComputedRange[1] - this->XComputedRange[0]) +
    this->XComputedRange[0];
  v =
    ((v - p0[1]) / (double)(p2[1] - p0[1])) * (this->YComputedRange[1] - this->YComputedRange[0]) +
    this->YComputedRange[0];
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::PlotToViewportCoordinate(svtkViewport* viewport, double& u, double& v)
{
  int *p0, *p1, *p2;

  // XAxis, YAxis are in viewport coordinates already
  p0 = this->XAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPosition2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  u = (((u - this->XComputedRange[0]) / (this->XComputedRange[1] - this->XComputedRange[0])) *
        (double)(p1[0] - p0[0])) +
    p0[0];
  v = (((v - this->YComputedRange[0]) / (this->YComputedRange[1] - this->YComputedRange[0])) *
        (double)(p2[1] - p0[1])) +
    p0[1];
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::ViewportToPlotCoordinate(svtkViewport* viewport)
{
  this->ViewportToPlotCoordinate(
    viewport, this->ViewportCoordinate[0], this->ViewportCoordinate[1]);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::PlotToViewportCoordinate(svtkViewport* viewport)
{
  this->PlotToViewportCoordinate(viewport, this->PlotCoordinate[0], this->PlotCoordinate[1]);
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::IsInPlot(svtkViewport* viewport, double u, double v)
{
  int *p0, *p1, *p2;

  // Bounds of the plot are based on the axes...
  p0 = this->XAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  p1 = this->XAxis->GetPosition2Coordinate()->GetComputedViewportValue(viewport);
  p2 = this->YAxis->GetPositionCoordinate()->GetComputedViewportValue(viewport);

  if (u >= p0[0] && u <= p1[0] && v >= p0[1] && v <= p2[1])
  {
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetPlotLines(int i, int isOn)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  int val = this->LinesOn->GetValue(i);
  if (val != isOn)
  {
    this->Modified();
    this->LinesOn->SetValue(i, isOn);
  }
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::GetPlotLines(int i)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  return this->LinesOn->GetValue(i);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetPlotPoints(int i, int isOn)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  int val = this->PointsOn->GetValue(i);
  if (val != isOn)
  {
    this->Modified();
    this->PointsOn->SetValue(i, isOn);
  }
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::GetPlotPoints(int i)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  return this->PointsOn->GetValue(i);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetPlotColor(int i, double r, double g, double b)
{
  this->LegendActor->SetEntryColor(i, r, g, b);
}

//----------------------------------------------------------------------------
double* svtkXYPlotActor::GetPlotColor(int i)
{
  return this->LegendActor->GetEntryColor(i);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetPlotSymbol(int i, svtkPolyData* input)
{
  this->LegendActor->SetEntrySymbol(i, input);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkXYPlotActor::GetPlotSymbol(int i)
{
  return this->LegendActor->GetEntrySymbol(i);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetPlotLabel(int i, const char* label)
{
  this->LegendActor->SetEntryString(i, label);
}

//----------------------------------------------------------------------------
const char* svtkXYPlotActor::GetPlotLabel(int i)
{
  return this->LegendActor->GetEntryString(i);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::GenerateClipPlanes(int* pos, int* pos2)
{
  double n[3], x[3];
  svtkPoints* pts = this->ClipPlanes->GetPoints();
  svtkDataArray* normals = this->ClipPlanes->GetNormals();

  n[2] = x[2] = 0.;

  // first
  n[0] = 0.;
  n[1] = -1.;
  normals->SetTuple(0, n);
  x[0] = (double).5 * (pos[0] + pos2[0]);
  x[1] = (double)pos[1];
  pts->SetPoint(0, x);

  // second
  n[0] = 1.;
  n[1] = 0.;
  normals->SetTuple(1, n);
  x[0] = (double)pos2[0];
  x[1] = (double).5 * (pos[1] + pos2[1]);
  pts->SetPoint(1, x);

  // third
  n[0] = 0.;
  n[1] = 1.;
  normals->SetTuple(2, n);
  x[0] = (double).5 * (pos[0] + pos2[0]);
  x[1] = (double)pos2[1];
  pts->SetPoint(2, x);

  // fourth
  n[0] = -1.;
  n[1] = 0.;
  normals->SetTuple(3, n);
  x[0] = (double)pos[0];
  x[1] = (double).5 * (pos[1] + pos2[1]);
  pts->SetPoint(3, x);
}

//----------------------------------------------------------------------------
double svtkXYPlotActor::ComputeGlyphScale(int i, int* pos, int* pos2)
{
  svtkPolyData* pd = this->LegendActor->GetEntrySymbol(i);
  // pd->Update();
  double length = pd->GetLength();
  double sf = this->GlyphSize *
    sqrt(
      (double)(pos[0] - pos2[0]) * (pos[0] - pos2[0]) + (pos[1] - pos2[1]) * (pos[1] - pos2[1])) /
    length;

  return sf;
}

//----------------------------------------------------------------------------
// This assumes that there are multiple polylines
void svtkXYPlotActor::ClipPlotData(int* pos, int* pos2, svtkPolyData* pd)
{
  svtkPoints* points = pd->GetPoints();
  svtkPoints* newPoints;
  svtkCellArray* lines = pd->GetLines();
  svtkCellArray *newLines, *newVerts;
  svtkIdType numPts = pd->GetNumberOfPoints();
  svtkIdType npts = 0;
  svtkIdType newPts[2];
  const svtkIdType* pts = nullptr;
  svtkIdType i, id;
  int j;
  double x1[3], x2[3], px[3], n[3], xint[3], t;
  double p1[2], p2[2];

  p1[0] = (double)pos[0];
  p1[1] = (double)pos[1];
  p2[0] = (double)pos2[0];
  p2[1] = (double)pos2[1];

  newPoints = svtkPoints::New();
  newPoints->Allocate(numPts);
  newVerts = svtkCellArray::New();
  newVerts->AllocateCopy(lines);
  newLines = svtkCellArray::New();
  newLines->AllocateCopy(lines);
  int* pointMap = new int[numPts];
  for (i = 0; i < numPts; i++)
  {
    pointMap[i] = -1;
  }

  // Loop over polyverts eliminating those that are outside
  for (lines->InitTraversal(); lines->GetNextCell(npts, pts);)
  {
    // loop over verts keeping only those that are not clipped
    for (i = 0; i < npts; i++)
    {
      points->GetPoint(pts[i], x1);

      if (x1[0] >= p1[0] && x1[0] <= p2[0] && x1[1] >= p1[1] && x1[1] <= p2[1])
      {
        id = newPoints->InsertNextPoint(x1);
        pointMap[i] = id;
        newPts[0] = id;
        newVerts->InsertNextCell(1, newPts);
      }
    }
  }

  // Loop over polylines clipping each line segment
  for (lines->InitTraversal(); lines->GetNextCell(npts, pts);)
  {
    // loop over line segment making up the polyline
    for (i = 0; i < (npts - 1); i++)
    {
      points->GetPoint(pts[i], x1);
      points->GetPoint(pts[i + 1], x2);

      // intersect each segment with the four planes
      if ((x1[0] < p1[0] && x2[0] < p1[0]) || (x1[0] > p2[0] && x2[0] > p2[0]) ||
        (x1[1] < p1[1] && x2[1] < p1[1]) || (x1[1] > p2[1] && x2[1] > p2[1]))
      {
        ; // trivial rejection
      }
      else if (x1[0] >= p1[0] && x2[0] >= p1[0] && x1[0] <= p2[0] && x2[0] <= p2[0] &&
        x1[1] >= p1[1] && x2[1] >= p1[1] && x1[1] <= p2[1] && x2[1] <= p2[1])
      { // trivial acceptance
        newPts[0] = pointMap[pts[i]];
        newPts[1] = pointMap[pts[i + 1]];
        newLines->InsertNextCell(2, newPts);
      }
      else
      {
        newPts[0] = -1;
        newPts[1] = -1;
        if (x1[0] >= p1[0] && x1[0] <= p2[0] && x1[1] >= p1[1] && x1[1] <= p2[1])
        { // first point in
          newPts[0] = pointMap[pts[i]];
        }
        else if (x2[0] >= p1[0] && x2[0] <= p2[0] && x2[1] >= p1[1] && x2[1] <= p2[1])
        { // second point in
          newPts[0] = pointMap[pts[i + 1]];
        }

        // only create cell if either x1 or x2 is inside the range
        if (newPts[0] >= 0)
        {
          for (j = 0; j < 4; j++)
          {
            this->ClipPlanes->GetPoints()->GetPoint(j, px);
            this->ClipPlanes->GetNormals()->GetTuple(j, n);
            if (svtkPlane::IntersectWithLine(x1, x2, n, px, t, xint) && t >= 0 && t <= 1.0)
            {
              newPts[1] = newPoints->InsertNextPoint(xint);
              break;
            }
          }
          if (newPts[1] >= 0)
          {
            newLines->InsertNextCell(2, newPts);
          }
        }
      }
    }
  }
  delete[] pointMap;

  // Update the lines
  pd->SetPoints(newPoints);
  pd->SetVerts(newVerts);
  pd->SetLines(newLines);

  newPoints->Delete();
  newVerts->Delete();
  newLines->Delete();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetDataObjectXComponent(int i, int comp)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  int val = this->XComponent->GetValue(i);
  if (val != comp)
  {
    this->Modified();
    this->XComponent->SetValue(i, comp);
  }
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::GetDataObjectXComponent(int i)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  return this->XComponent->GetValue(i);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetDataObjectYComponent(int i, int comp)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  int val = this->YComponent->GetValue(i);
  if (val != comp)
  {
    this->Modified();
    this->YComponent->SetValue(i, comp);
  }
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::GetDataObjectYComponent(int i)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  return this->YComponent->GetValue(i);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetPointComponent(int i, int comp)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  int val = this->XComponent->GetValue(i);
  if (val != comp)
  {
    this->Modified();
    this->XComponent->SetValue(i, comp);
  }
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::GetPointComponent(int i)
{
  i = (i < 0 ? 0 : (i >= SVTK_MAX_PLOTS ? SVTK_MAX_PLOTS - 1 : i));
  return this->XComponent->GetValue(i);
}

//----------------------------------------------------------------------------
double* svtkXYPlotActor::TransformPoint(int pos[2], int pos2[2], double x[3], double xNew[3])
{
  // First worry about exchanging axes
  if (this->ExchangeAxes)
  {
    double sx = (x[0] - pos[0]) / (pos2[0] - pos[0]);
    double sy = (x[1] - pos[1]) / (pos2[1] - pos[1]);
    xNew[0] = sy * (pos2[0] - pos[0]) + pos[0];
    xNew[1] = sx * (pos2[1] - pos[1]) + pos[1];
    xNew[2] = x[2];
  }
  else
  {
    xNew[0] = x[0];
    xNew[1] = x[1];
    xNew[2] = x[2];
  }

  // Okay, now swap the axes around if reverse is on
  if (this->ReverseXAxis)
  {
    xNew[0] = pos[0] + (pos2[0] - xNew[0]);
  }
  if (this->ReverseYAxis)
  {
    xNew[1] = pos[1] + (pos2[1] - xNew[1]);
  }

  return xNew;
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetYTitle(const char* ytitle)
{
  this->YTitleActor->SetInput(ytitle);
  this->Modified();
}

//----------------------------------------------------------------------------
char* svtkXYPlotActor::GetYTitle()
{
  return this->YTitleActor->GetInput();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetXTitlePosition(double position)
{
  this->XAxis->SetTitlePosition(position);
  this->Modified();
}

//----------------------------------------------------------------------------
double svtkXYPlotActor::GetXTitlePosition()
{
  return this->XAxis->GetTitlePosition();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAdjustXLabels(int adjust)
{
  this->AdjustXLabels = adjust;
  this->XAxis->SetAdjustLabels(adjust);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAdjustYLabels(int adjust)
{
  this->AdjustYLabels = adjust;
  this->YAxis->SetAdjustLabels(adjust);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetLabelFormat(const char* _arg)
{
  this->SetXLabelFormat(_arg);
  this->SetYLabelFormat(_arg);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetXLabelFormat(const char* _arg)
{
  if (this->XLabelFormat == nullptr && _arg == nullptr)
  {
    return;
  }

  if (this->XLabelFormat && _arg && (!strcmp(this->XLabelFormat, _arg)))
  {
    return;
  }

  delete[] this->XLabelFormat;

  if (_arg)
  {
    this->XLabelFormat = new char[strlen(_arg) + 1];
    strcpy(this->XLabelFormat, _arg);
  }
  else
  {
    this->XLabelFormat = nullptr;
  }

  this->XAxis->SetLabelFormat(this->XLabelFormat);

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetYLabelFormat(const char* _arg)
{
  if (this->YLabelFormat == nullptr && _arg == nullptr)
  {
    return;
  }

  if (this->YLabelFormat && _arg && (!strcmp(this->YLabelFormat, _arg)))
  {
    return;
  }

  delete[] this->YLabelFormat;

  if (_arg)
  {
    this->YLabelFormat = new char[strlen(_arg) + 1];
    strcpy(this->YLabelFormat, _arg);
  }
  else
  {
    this->YLabelFormat = nullptr;
  }

  this->YAxis->SetLabelFormat(this->YLabelFormat);

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetNumberOfXMinorTicks(int num)
{
  this->XAxis->SetNumberOfMinorTicks(num);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::GetNumberOfXMinorTicks()
{
  return this->XAxis->GetNumberOfMinorTicks();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetNumberOfYMinorTicks(int num)
{
  this->YAxis->SetNumberOfMinorTicks(num);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXYPlotActor::GetNumberOfYMinorTicks()
{
  return this->YAxis->GetNumberOfMinorTicks();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::PrintAsCSV(ostream& os)
{
  svtkDataArray* scalars;
  svtkDataSet* ds;
  double s;
  int component;
  int numDS = this->InputConnectionHolder->GetNumberOfInputConnections(0);
  for (int dsNum = 0; dsNum < numDS; dsNum++)
  {
    svtkAlgorithmOutput* port = this->InputConnectionHolder->GetInputConnection(0, dsNum);
    svtkAlgorithm* alg = port->GetProducer();
    int portIndex = port->GetIndex();
    ds = svtkDataSet::SafeDownCast(alg->GetOutputDataObject(portIndex));
    svtkIdType numPts = ds->GetNumberOfPoints();
    scalars = ds->GetPointData()->GetScalars(this->SelectedInputScalars[dsNum]);
    os << this->SelectedInputScalars[dsNum] << ",";

    component = this->SelectedInputScalarsComponent->GetValue(dsNum);
    for (svtkIdType ptId = 0; ptId < numPts; ptId++)
    {
      s = scalars->GetComponent(ptId, component);
      if (ptId == 0)
      {
        os << s;
      }
      else
      {
        os << "," << s;
      }
    }
    os << endl;

    if (dsNum == numDS - 1)
    {
      os << "X or T,";
      for (svtkIdType ptId = 0; ptId < numPts; ptId++)
      {
        double* x = ds->GetPoint(ptId);
        if (ptId == 0)
        {
          os << x[0];
        }
        else
        {
          os << "," << x[0];
        }
      }
      os << endl;
    }
  }
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::AddUserCurvesPoint(double c_dbl, double x, double y)
{
  int c = static_cast<int>(c_dbl);
  if (this->ActiveCurveIndex != c)
  {
    svtkPolyData* dataObj = svtkPolyData::New();
    dataObj->GetFieldData()->AddArray(this->ActiveCurve);
    this->AddDataObjectInput(dataObj);
    this->SetDataObjectXComponent(this->ActiveCurveIndex, 0);
    this->SetDataObjectYComponent(this->ActiveCurveIndex, 1);
    dataObj->Delete();
    this->ActiveCurve = svtkSmartPointer<svtkDoubleArray>::New();
    this->ActiveCurve->SetNumberOfComponents(2);
    this->ActiveCurveIndex = c;
  }
  this->ActiveCurve->InsertNextTuple2(x, y);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::RemoveAllActiveCurves()
{
  this->ActiveCurveIndex = 0;
  this->ActiveCurve = svtkSmartPointer<svtkDoubleArray>::New();
  this->ActiveCurve->SetNumberOfComponents(2);
  this->Modified();
}

//----------------------------------------------------------------------------
//  Glyph type
//  \li 0 : nothing
//  \li 1 : vertex - not visible
//  \li 2 : line
//  \li 3 : cross
//  \li 4 : thick cross
//  \li 5 : triangle
//  \li 6 : square
//  \li 7 : circle
//  \li 8 : diamond
//  \li 9 : arrow
//  \li 10 : thick arrow
//  \li 11 : curved arrow
//  \li 12 : arrow
//  \li 13 : nothing
//  \li 14 : nothing
//  \li 15 : 2 + fillOff
//  \li 16 : nothing
//  \li 17 : 4 + fillOff
//  \li 18 : 5 + fillOff
//  \li 19 : 6 + fillOff
//  \li 20 : 7 + fillOff
//  \li 21 : 8 + fillOff
//  \li 22 : nothing
//  \li 23 : 10 + fillOff
//  \li 24 : 11 + fillOff
//  \li 25 : 12 + fillOff*
void svtkXYPlotActor::SetPlotGlyphType(int curve, int i)
{
  static const int good_glyph_type[26] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0, 0, 2, 0, 4,
    5, 6, 7, 8, 0, 10, 11, 12 };
  svtkSmartPointer<svtkGlyphSource2D> source = svtkSmartPointer<svtkGlyphSource2D>::New();
  source->SetGlyphType(good_glyph_type[i]);
  source->SetFilled((i > 12) ? 0 : 1);
  source->Update();

  svtkPolyData* glyph = svtkPolyData::SafeDownCast(source->GetOutputDataObject(0));
  this->SetPlotSymbol(curve, glyph);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetXAxisColor(double r, double g, double b)
{
  this->XAxis->GetProperty()->SetColor(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetYAxisColor(double r, double g, double b)
{
  this->YAxis->GetProperty()->SetColor(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetLegendBorder(int b)
{
  this->LegendActor->SetBorder(b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetLegendBox(int b)
{
  this->LegendActor->SetBox(b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetLegendUseBackground(int b)
{
  this->LegendActor->SetUseBackground(b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetLegendBackgroundColor(double r, double g, double b)
{
  this->LegendActor->SetBackgroundColor(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetLineWidth(double w)
{
  this->GetProperty()->SetLineWidth(w);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleColor(double r, double g, double b)
{
  this->GetTitleTextProperty()->SetColor(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleFontFamily(int x)
{
  this->GetTitleTextProperty()->SetFontFamily(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleBold(int x)
{
  this->GetTitleTextProperty()->SetBold(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleItalic(int x)
{
  this->GetTitleTextProperty()->SetItalic(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleShadow(int x)
{
  this->GetTitleTextProperty()->SetShadow(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleFontSize(int x)
{
  this->GetTitleTextProperty()->SetFontSize(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleJustification(int x)
{
  this->GetTitleTextProperty()->SetJustification(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetTitleVerticalJustification(int x)
{
  this->GetTitleTextProperty()->SetVerticalJustification(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleColor(double r, double g, double b)
{
  this->GetAxisTitleTextProperty()->SetColor(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleFontFamily(int x)
{
  this->GetAxisTitleTextProperty()->SetFontFamily(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleBold(int x)
{
  this->GetAxisTitleTextProperty()->SetBold(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleItalic(int x)
{
  this->GetAxisTitleTextProperty()->SetItalic(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleShadow(int x)
{
  this->GetAxisTitleTextProperty()->SetShadow(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleFontSize(int x)
{
  this->GetAxisTitleTextProperty()->SetFontSize(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleJustification(int x)
{
  this->GetAxisTitleTextProperty()->SetJustification(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleVerticalJustification(int x)
{
  this->GetAxisTitleTextProperty()->SetVerticalJustification(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisTitleTextProperty(svtkTextProperty* p)
{
  // NB: Perform shallow copy here since each individual axis can be
  // accessed through the class API ( i.e. each individual axis text prop
  // can be changed ). Therefore, we can not just assign pointers otherwise
  // each individual axis text prop would point to the same text prop.
  this->AxisTitleTextProperty->ShallowCopy(p);
  this->YTitleActor->GetTextProperty()->ShallowCopy(p);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelColor(double r, double g, double b)
{
  this->GetAxisLabelTextProperty()->SetColor(r, g, b);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelFontFamily(int x)
{
  this->GetAxisLabelTextProperty()->SetFontFamily(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelBold(int x)
{
  this->GetAxisLabelTextProperty()->SetBold(x);
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelItalic(int x)
{
  this->GetAxisLabelTextProperty()->SetItalic(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelShadow(int x)
{
  this->GetAxisLabelTextProperty()->SetShadow(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelFontSize(int x)
{
  this->GetAxisLabelTextProperty()->SetFontSize(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelJustification(int x)
{
  this->GetAxisLabelTextProperty()->SetJustification(x);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkXYPlotActor::SetAxisLabelVerticalJustification(int x)
{
  this->GetAxisLabelTextProperty()->SetVerticalJustification(x);
  this->Modified();
}
