/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWindowToImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWindowToImageFilter.h"

#include "svtkActor2D.h"
#include "svtkActor2DCollection.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <vector>

#define BORDER_PIXELS 2

svtkStandardNewMacro(svtkWindowToImageFilter);

class svtkWTI2DHelperClass
{
public:
  // maintain a list of 2D actors
  svtkActor2DCollection* StoredActors;
  // maintain lists of their svtkCoordinate objects
  svtkCollection* Coord1s;
  svtkCollection* Coord2s;
  // Store the display coords for adjustment during tiling
  std::vector<std::pair<int, int> > Coords1;
  std::vector<std::pair<int, int> > Coords2;
  //
  svtkWTI2DHelperClass()
  {
    this->StoredActors = svtkActor2DCollection::New();
    this->Coord1s = svtkCollection::New();
    this->Coord2s = svtkCollection::New();
  }
  ~svtkWTI2DHelperClass()
  {
    this->Coord1s->RemoveAllItems();
    this->Coord2s->RemoveAllItems();
    this->StoredActors->RemoveAllItems();
    this->Coord1s->Delete();
    this->Coord2s->Delete();
    this->StoredActors->Delete();
  }
};

//----------------------------------------------------------------------------
svtkWindowToImageFilter::svtkWindowToImageFilter()
{
  this->Input = nullptr;
  this->Scale[0] = this->Scale[1] = 1;
  this->ReadFrontBuffer = 1;
  this->ShouldRerender = 1;
  this->Viewport[0] = 0;
  this->Viewport[1] = 0;
  this->Viewport[2] = 1;
  this->Viewport[3] = 1;
  this->InputBufferType = SVTK_RGB;
  this->FixBoundary = false;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->StoredData = new svtkWTI2DHelperClass;
}

//----------------------------------------------------------------------------
svtkWindowToImageFilter::~svtkWindowToImageFilter()
{
  if (this->Input)
  {
    this->Input->UnRegister(this);
    this->Input = nullptr;
  }
  delete this->StoredData;
}

//----------------------------------------------------------------------------
svtkImageData* svtkWindowToImageFilter::GetOutput()
{
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void svtkWindowToImageFilter::SetInput(svtkWindow* input)
{
  if (input != this->Input)
  {
    if (this->Input)
    {
      this->Input->UnRegister(this);
    }
    this->Input = input;
    if (this->Input)
    {
      this->Input->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkWindowToImageFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Input)
  {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Input: (none)\n";
  }
  os << indent << "ReadFrontBuffer: " << this->ReadFrontBuffer << "\n";
  os << indent << "Scale: " << this->Scale[0] << ", " << this->Scale[1] << "\n";
  os << indent << "ShouldRerender: " << this->ShouldRerender << "\n";
  os << indent << "Viewport: " << this->Viewport[0] << "," << this->Viewport[1] << ","
     << this->Viewport[2] << "," << this->Viewport[3] << "\n";
  os << indent << "InputBufferType: " << this->InputBufferType << "\n";
  os << indent << "FixBoundary: " << this->FixBoundary << endl;
}

//----------------------------------------------------------------------------
void svtkWindowToImageFilter::SetViewport(double a1, double a2, double a3, double a4)
{
  a1 = svtkMath::ClampValue(a1, 0.0, 1.0);
  a2 = svtkMath::ClampValue(a2, 0.0, 1.0);
  a3 = svtkMath::ClampValue(a3, 0.0, 1.0);
  a4 = svtkMath::ClampValue(a4, 0.0, 1.0);

  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Viewport to (" << a1 << ","
                << a2 << "," << a3 << "," << a4 << ")");
  if ((this->Viewport[0] != a1) || (this->Viewport[1] != a2) || (this->Viewport[2] != a3) ||
    (this->Viewport[3] != a4))
  {
    this->Viewport[0] = a1;
    this->Viewport[1] = a2;
    this->Viewport[2] = a3;
    this->Viewport[3] = a4;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkWindowToImageFilter::SetViewport(double vp[4])
{
  this->SetViewport(vp[0], vp[1], vp[2], vp[3]);
}

//----------------------------------------------------------------------------
// This method returns the largest region that can be generated.
void svtkWindowToImageFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (this->Input == nullptr)
  {
    svtkErrorMacro(<< "Please specify a renderer as input!");
    return;
  }

  if (this->Scale[0] < 1)
  {
    svtkWarningMacro("Scale[0] cannot be less than 1. Clamping to 1.");
    this->Scale[0] = 1;
  }

  if (this->Scale[1] < 1)
  {
    svtkWarningMacro("Scale[1] cannot be less than 1. Clamping to 1.");
    this->Scale[1] = 1;
  }

  int tileScale[2];
  this->Input->GetTileScale(tileScale);
  int magTileScale[2] = { tileScale[0] * this->Scale[0], tileScale[1] * this->Scale[1] };

  if ((magTileScale[0] > 1 || magTileScale[1] > 1) &&
    (this->Viewport[0] != 0 || this->Viewport[1] != 0 || this->Viewport[2] != 1 ||
      this->Viewport[3] != 1))
  {
    svtkWarningMacro(<< "Viewport extents are not used when scale factors > 1 "
                       "or tiled displays are used.");
    this->Viewport[0] = 0;
    this->Viewport[1] = 0;
    this->Viewport[2] = 1;
    this->Viewport[3] = 1;
  }

  // set the extent
  const int* size = this->Input->GetSize();
  int wExtent[6];
  wExtent[0] = 0;
  wExtent[1] =
    (int(this->Viewport[2] * size[0] + 0.5) - int(this->Viewport[0] * size[0])) * this->Scale[0] -
    1;
  wExtent[2] = 0;
  wExtent[3] =
    (int(this->Viewport[3] * size[1] + 0.5) - int(this->Viewport[1] * size[1])) * this->Scale[1] -
    1;
  wExtent[4] = 0;
  wExtent[5] = 0;

  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent, 6);

  switch (this->InputBufferType)
  {
    case SVTK_RGB:
      svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_UNSIGNED_CHAR, 3);
      break;
    case SVTK_RGBA:
      svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_UNSIGNED_CHAR, 4);
      break;
    case SVTK_ZBUFFER:
      svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_FLOAT, 1);
      break;
    default:
      // SVTK_RGB configuration by default
      svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_UNSIGNED_CHAR, 3);
      break;
  }
}

//----------------------------------------------------------------------------
svtkTypeBool svtkWindowToImageFilter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->RequestData(request, inputVector, outputVector);
    return 1;
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void svtkWindowToImageFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* out = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  out->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  out->AllocateScalars(outInfo);

  if (!this->Input)
  {
    return;
  }

  if (this->Scale[0] < 1)
  {
    svtkWarningMacro("Scale[0] cannot be less than 1. Clamping to 1.");
    this->Scale[0] = 1;
  }

  if (this->Scale[1] < 1)
  {
    svtkWarningMacro("Scale[1] cannot be less than 1. Clamping to 1.");
    this->Scale[1] = 1;
  }

  svtkRenderWindow* renWin = svtkRenderWindow::SafeDownCast(this->Input);
  if (!renWin)
  {
    svtkWarningMacro(
      "The window passed to window to image should be a RenderWindow or one of its subclasses");
    return;
  }

  if (!((out->GetScalarType() == SVTK_UNSIGNED_CHAR &&
          (this->InputBufferType == SVTK_RGB || this->InputBufferType == SVTK_RGBA)) ||
        (out->GetScalarType() == SVTK_FLOAT && this->InputBufferType == SVTK_ZBUFFER)))
  {
    svtkErrorMacro("mismatch in scalar types!");
    return;
  }

  int tileScale[2];
  this->Input->GetTileScale(tileScale);
  int magTileScale[2] = { this->Scale[0] * tileScale[0], this->Scale[1] * tileScale[1] };

  int tileSize[2] = { this->Input->GetActualSize()[0], this->Input->GetActualSize()[1] };

  // This is the size of the window with all tiles accounted for:
  int winSize[2] = { this->Input->GetSize()[0], this->Input->GetSize()[1] };

  int imageBounds[4] = { int(this->Viewport[0] * tileSize[0]), int(this->Viewport[1] * tileSize[1]),
    int(this->Viewport[2] * tileSize[0] + 0.5) - 1,
    int(this->Viewport[3] * tileSize[1] + 0.5) - 1 };

  int vpSize[2] = { imageBounds[2] - imageBounds[0] + 1, imageBounds[3] - imageBounds[1] + 1 };

  int imageSize[2] = { vpSize[0] * magTileScale[0], vpSize[1] * magTileScale[1] };

  int inIncrY = vpSize[0];
  inIncrY *= (this->InputBufferType == SVTK_RGB) ? 3 : ((this->InputBufferType == SVTK_RGBA) ? 4 : 1);

  int outIncrY = imageSize[0] * out->GetNumberOfScalarComponents();

  svtkRendererCollection* rc = renWin->GetRenderers();
  svtkRenderer* aren;
  svtkCamera* cam;
  int numRenderers = rc->GetNumberOfItems();

  // for each renderer
  svtkCamera** cams = new svtkCamera*[numRenderers];
  float* viewAngles = new float[numRenderers];
  double* windowCenters = new double[numRenderers * 2];
  double* parallelScale = new double[numRenderers];
  svtkCollectionSimpleIterator rsit;
  rc->InitTraversal(rsit);
  for (int i = 0; i < numRenderers; ++i)
  {
    aren = rc->GetNextRenderer(rsit);
    cams[i] = aren->GetActiveCamera();
    cams[i]->Register(this);
    cams[i]->GetWindowCenter(windowCenters + i * 2);
    viewAngles[i] = svtkMath::RadiansFromDegrees(cams[i]->GetViewAngle());
    parallelScale[i] = cams[i]->GetParallelScale();
    cam = cams[i]->NewInstance();
    cam->ShallowCopy(cams[i]);
    aren->SetActiveCamera(cam);
  }

  // render each of the tiles required to fill this request
  this->Input->SetTileScale(magTileScale);
  this->Input->GetSize();

  // this->Rescale2DActors();

  int num_iterations[2] = { magTileScale[0], magTileScale[1] };
  bool overlap_viewports = false;
  if (this->FixBoundary)
  {
    if ((magTileScale[0] > 1 || magTileScale[1] > 1) && winSize[0] >= 50)
    {
      ++num_iterations[0];
      ++num_iterations[1];
      overlap_viewports = true;
    }
  }

  // Precompute the tile this->Viewport for each iteration.
  double* viewports = new double[4 * num_iterations[0] * num_iterations[1]];
  for (int y = 0; y < num_iterations[1]; y++)
  {
    for (int x = 0; x < num_iterations[0]; x++)
    {
      double* cur_viewport = &viewports[(num_iterations[0] * y + x) * 4];
      cur_viewport[0] = static_cast<double>(x) / magTileScale[0];
      cur_viewport[1] = static_cast<double>(y) / magTileScale[1];
      cur_viewport[2] = (x + 1.0) / magTileScale[0];
      cur_viewport[3] = (y + 1.0) / magTileScale[1];

      if (overlap_viewports)
      {
        if (x > 0 && x < num_iterations[0] - 1)
        {
          cur_viewport[0] -= x * (BORDER_PIXELS * 2.0) / tileSize[0];
          cur_viewport[2] -= x * (BORDER_PIXELS * 2.0) / tileSize[0];
        }
        if (x == num_iterations[0] - 1)
        {
          cur_viewport[0] = static_cast<double>(x - 1) / magTileScale[0];
          cur_viewport[2] = static_cast<double>(x) / magTileScale[0];
        }
        if (y > 0 && y < num_iterations[1] - 1)
        {
          cur_viewport[1] -= y * (BORDER_PIXELS * 2.0) / tileSize[1];
          cur_viewport[3] -= y * (BORDER_PIXELS * 2.0) / tileSize[1];
        }
        if (y == num_iterations[1] - 1)
        {
          cur_viewport[1] = static_cast<double>(y - 1) / magTileScale[1];
          cur_viewport[3] = static_cast<double>(y) / magTileScale[1];
        }
      }
    }
  }

  for (int y = 0; y < num_iterations[1]; y++)
  {
    for (int x = 0; x < num_iterations[0]; x++)
    {
      // setup the Window ivars
      double* cur_viewport = &viewports[(num_iterations[0] * y + x) * 4];
      this->Input->SetTileViewport(cur_viewport);
      double* tvp = this->Input->GetTileViewport();

      // for each renderer, setup camera
      rc->InitTraversal(rsit);
      for (int i = 0; i < numRenderers; ++i)
      {
        aren = rc->GetNextRenderer(rsit);
        cam = aren->GetActiveCamera();
        double* vp = aren->GetViewport();
        double visVP[4];
        visVP[0] = (vp[0] < tvp[0]) ? tvp[0] : vp[0];
        visVP[0] = (visVP[0] > tvp[2]) ? tvp[2] : visVP[0];
        visVP[1] = (vp[1] < tvp[1]) ? tvp[1] : vp[1];
        visVP[1] = (visVP[1] > tvp[3]) ? tvp[3] : visVP[1];
        visVP[2] = (vp[2] > tvp[2]) ? tvp[2] : vp[2];
        visVP[2] = (visVP[2] < tvp[0]) ? tvp[0] : visVP[2];
        visVP[3] = (vp[3] > tvp[3]) ? tvp[3] : vp[3];
        visVP[3] = (visVP[3] < tvp[1]) ? tvp[1] : visVP[3];

        // compute magnification
        double mag = (visVP[3] - visVP[1]) / (vp[3] - vp[1]);
        // compute the delta
        double deltax = (visVP[2] + visVP[0]) / 2.0 - (vp[2] + vp[0]) / 2.0;
        double deltay = (visVP[3] + visVP[1]) / 2.0 - (vp[3] + vp[1]) / 2.0;
        // scale by original window size
        if (visVP[2] - visVP[0] > 0)
        {
          deltax = 2.0 * deltax / (visVP[2] - visVP[0]);
        }
        if (visVP[3] - visVP[1] > 0)
        {
          deltay = 2.0 * deltay / (visVP[3] - visVP[1]);
        }
        cam->SetWindowCenter(windowCenters[i * 2] + deltax, windowCenters[i * 2 + 1] + deltay);
        double angle = 2.0 * atan(tan(viewAngles[i] / 2.0) * mag);
        cam->SetViewAngle(svtkMath::DegreesFromRadians(angle));
        cam->SetParallelScale(parallelScale[i] * mag);
      }

      // Shift 2d actors just before rendering
      // this->Shift2DActors(size[0]*x, size[1]*y);
      // now render the tile and get the data
      if (this->ShouldRerender || num_iterations[0] > 1 || num_iterations[1] > 1)
      {
        this->Render();
      }
      this->Input->MakeCurrent();

      int buffer = this->ReadFrontBuffer;
      if (!this->Input->GetDoubleBuffer())
      {
        buffer = 1;
      }
      if (this->InputBufferType == SVTK_RGB || this->InputBufferType == SVTK_RGBA)
      {
        unsigned char *pixels, *pixels1, *outPtr;
        if (this->InputBufferType == SVTK_RGB)
        {
          pixels = this->Input->GetPixelData(
            imageBounds[0], imageBounds[1], imageBounds[2], imageBounds[3], buffer);
        }
        else
        {
          pixels = renWin->GetRGBACharPixelData(
            imageBounds[0], imageBounds[1], imageBounds[2], imageBounds[3], buffer);
        }
        pixels1 = pixels;

        // now write the data to the output image
        if (overlap_viewports)
        {
          int xpos = int(cur_viewport[0] * imageSize[0] + 0.5);
          int ypos = int(cur_viewport[1] * imageSize[1] + 0.5);

          outPtr = static_cast<unsigned char*>(out->GetScalarPointer(xpos, ypos, 0));

          // We skip padding pixels around internal borders.
          int ncomp = out->GetNumberOfScalarComponents();
          int start_x_offset = (x != 0) ? BORDER_PIXELS : 0;
          int end_x_offset = (x != num_iterations[0] - 1 && x != 0) ? BORDER_PIXELS : 0;
          int start_y_offset = (y != 0) ? BORDER_PIXELS : 0;
          int end_y_offset = (y != num_iterations[1] - 1) ? BORDER_PIXELS : 0;
          start_x_offset *= ncomp;
          end_x_offset *= ncomp;

          for (int idxY = 0; idxY < tileSize[1]; idxY++)
          {
            if (idxY >= start_y_offset && idxY < tileSize[1] - end_y_offset)
            {
              memcpy(outPtr + start_x_offset, pixels1 + start_x_offset,
                inIncrY - (start_x_offset + end_x_offset));
            }
            outPtr += outIncrY;
            pixels1 += inIncrY;
          }
        }
        else
        {
          outPtr =
            static_cast<unsigned char*>(out->GetScalarPointer(x * vpSize[0], y * vpSize[1], 0));

          for (int idxY = 0; idxY < vpSize[1]; idxY++)
          {
            memcpy(outPtr, pixels1, inIncrY);
            outPtr += outIncrY;
            pixels1 += inIncrY;
          }
        }

        // free the memory
        delete[] pixels;
      }
      else
      { // SVTK_ZBUFFER
        float* pixels =
          renWin->GetZbufferData(imageBounds[0], imageBounds[1], imageBounds[2], imageBounds[3]);
        float* pixels1 = pixels;

        // now write the data to the output image
        float* outPtr = static_cast<float*>(out->GetScalarPointer(x * vpSize[0], y * vpSize[1], 0));

        for (int idxY = 0; idxY < vpSize[1]; idxY++)
        {
          memcpy(outPtr, pixels1, inIncrY * sizeof(float));
          outPtr += outIncrY;
          pixels1 += inIncrY;
        }

        // free the memory
        delete[] pixels;
      }
    }
  }

  // restore settings
  // for each renderer
  rc->InitTraversal(rsit);
  for (int i = 0; i < numRenderers; ++i)
  {
    aren = rc->GetNextRenderer(rsit);
    // store the old view angle & set the new
    cam = aren->GetActiveCamera();
    aren->SetActiveCamera(cams[i]);
    cams[i]->UnRegister(this);
    cam->Delete();
  }
  delete[] viewAngles;
  delete[] windowCenters;
  delete[] parallelScale;
  delete[] cams;
  delete[] viewports;

  // render each of the tiles required to fill this request
  this->Input->SetTileScale(tileScale);
  this->Input->SetTileViewport(0.0, 0.0, 1.0, 1.0);
  this->Input->GetSize();
  // restore every 2d actors
  // this->Restore2DActors();
}

//----------------------------------------------------------------------------
// On each tile we must subtract the origin of each actor to ensure
// it appears in the corrrect relative location
void svtkWindowToImageFilter::Restore2DActors()
{
  svtkActor2D* actor;
  svtkCoordinate *c1, *c2;
  svtkCoordinate *n1, *n2;
  int i;
  //
  for (this->StoredData->StoredActors->InitTraversal(), i = 0;
       (actor = this->StoredData->StoredActors->GetNextItem()); i++)
  {
    c1 = actor->GetPositionCoordinate();
    c2 = actor->GetPosition2Coordinate();
    n1 = svtkCoordinate::SafeDownCast(this->StoredData->Coord1s->GetItemAsObject(i));
    n2 = svtkCoordinate::SafeDownCast(this->StoredData->Coord2s->GetItemAsObject(i));
    c1->SetCoordinateSystem(n1->GetCoordinateSystem());
    c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
    c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
    c1->SetValue(n1->GetValue());
    c2->SetCoordinateSystem(n2->GetCoordinateSystem());
    c2->SetReferenceCoordinate(n2->GetReferenceCoordinate());
    c2->SetValue(n2->GetValue());
  }
  this->StoredData->Coord1s->RemoveAllItems();
  this->StoredData->Coord2s->RemoveAllItems();
  this->StoredData->StoredActors->RemoveAllItems();
}

//----------------------------------------------------------------------------
void svtkWindowToImageFilter::Rescale2DActors()
{
  svtkActor2D* actor;
  svtkProp* aProp;
  svtkRenderer* aren;
  svtkPropCollection* pc;
  svtkRendererCollection* rc;
  svtkCoordinate *c1, *c2;
  svtkCoordinate *n1, *n2;
  int *p1, *p2;
  double d1[3], d2[3];
  //
  svtkRenderWindow* renWin = svtkRenderWindow::SafeDownCast(this->Input);
  rc = renWin->GetRenderers();
  for (rc->InitTraversal(); (aren = rc->GetNextItem());)
  {
    pc = aren->GetViewProps();
    if (pc)
    {
      for (pc->InitTraversal(); (aProp = pc->GetNextProp());)
      {
        actor = svtkActor2D::SafeDownCast((aProp));
        if (actor)
        {
          // put the actor in our list for retrieval later
          this->StoredData->StoredActors->AddItem(actor);
          // Copy all existing coordinate stuff
          n1 = actor->GetPositionCoordinate();
          n2 = actor->GetPosition2Coordinate();
          c1 = svtkCoordinate::New();
          c2 = svtkCoordinate::New();
          c1->SetCoordinateSystem(n1->GetCoordinateSystem());
          c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
          c1->SetReferenceCoordinate(n1->GetReferenceCoordinate());
          c1->SetValue(n1->GetValue());
          c2->SetCoordinateSystem(n2->GetCoordinateSystem());
          c2->SetReferenceCoordinate(n2->GetReferenceCoordinate());
          c2->SetValue(n2->GetValue());
          this->StoredData->Coord1s->AddItem(c1);
          this->StoredData->Coord2s->AddItem(c2);
          c1->Delete();
          c2->Delete();
          // work out the position in new magnified pixels
          p1 = n1->GetComputedDisplayValue(aren);
          p2 = n2->GetComputedDisplayValue(aren);
          d1[0] = p1[0] * this->Scale[0];
          d1[1] = p1[1] * this->Scale[1];
          d1[2] = 0.0;
          d2[0] = p2[0] * this->Scale[0];
          d2[1] = p2[1] * this->Scale[1];
          d2[2] = 0.0;
          this->StoredData->Coords1.push_back(
            std::pair<int, int>(static_cast<int>(d1[0]), static_cast<int>(d1[1])));
          this->StoredData->Coords2.push_back(
            std::pair<int, int>(static_cast<int>(d2[0]), static_cast<int>(d2[1])));
          // Make sure they have no dodgy offsets
          n1->SetCoordinateSystemToDisplay();
          n2->SetCoordinateSystemToDisplay();
          n1->SetReferenceCoordinate(nullptr);
          n2->SetReferenceCoordinate(nullptr);
          n1->SetValue(d1[0], d1[1]);
          n2->SetValue(d2[0], d2[1]);
          //
        }
      }
    }
  }
}
//----------------------------------------------------------------------------
// On each tile we must subtract the origin of each actor to ensure
// it appears in the correct relative location
void svtkWindowToImageFilter::Shift2DActors(int x, int y)
{
  svtkActor2D* actor;
  svtkCoordinate *c1, *c2;
  double d1[3], d2[3];
  int i;
  //
  for (this->StoredData->StoredActors->InitTraversal(), i = 0;
       (actor = this->StoredData->StoredActors->GetNextItem()); i++)
  {
    c1 = actor->GetPositionCoordinate();
    c2 = actor->GetPosition2Coordinate();
    c1->GetValue(d1);
    c2->GetValue(d2);
    d1[0] = this->StoredData->Coords1[i].first - x;
    d1[1] = this->StoredData->Coords1[i].second - y + 1;
    d2[0] = this->StoredData->Coords2[i].first - x;
    d2[1] = this->StoredData->Coords2[i].second - y + 1;
    c1->SetValue(d1);
    c2->SetValue(d2);
  }
}

//----------------------------------------------------------------------------
int svtkWindowToImageFilter::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}
//----------------------------------------------------------------------------
void svtkWindowToImageFilter::Render()
{
  if (svtkRenderWindow* renWin = svtkRenderWindow::SafeDownCast(this->Input))
  {
    // if interactor is present, trigger render through interactor. This
    // allows for custom applications that provide interactors that
    // customize rendering e.g. ParaView.
    if (renWin->GetInteractor())
    {
      renWin->GetInteractor()->Render();
    }
    else
    {
      renWin->Render();
    }
  }
}
