/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataLIC2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageDataLIC2D.h"

#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkImageCast.h"
#include "svtkImageData.h"
#include "svtkImageNoiseSource.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLineIntegralConvolution2D.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLState.h"
#include "svtkPixelBufferObject.h"
#include "svtkPixelExtent.h"
#include "svtkPixelTransfer.h"
#include "svtkPointData.h"
#include "svtkRenderbuffer.h"
#include "svtkShaderProgram.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"
#include "svtkStructuredExtent.h"
#include "svtkTextureObject.h"
#include "svtkUnsignedCharArray.h"

#include <deque>
using std::deque;

#include "svtkOpenGLHelper.h"
#include "svtkTextureObjectVS.h"

#define svtkImageDataLIC2DDEBUG 0
#if (svtkImageDataLIC2DDEBUG >= 1)
#include "svtkTextureIO.h"
#endif

#define PRINTEXTENT(ext)                                                                           \
  ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkImageDataLIC2D);

//----------------------------------------------------------------------------
svtkImageDataLIC2D::svtkImageDataLIC2D()
{
  this->Context = nullptr;
  this->OwnWindow = false;
  this->OpenGLExtensionsSupported = 0;

  this->Steps = 20;
  this->StepSize = 1.0;
  this->Magnification = 1;

  this->NoiseSource = svtkImageNoiseSource::New();
  this->NoiseSource->SetWholeExtent(0, 127, 0, 127, 0, 0);
  this->NoiseSource->SetMinimum(0.0);
  this->NoiseSource->SetMaximum(1.0);

  this->ImageCast = svtkImageCast::New();
  this->ImageCast->SetOutputScalarTypeToFloat();
  this->ImageCast->SetInputConnection(this->NoiseSource->GetOutputPort(0));

  this->SetNumberOfInputPorts(2);

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
svtkImageDataLIC2D::~svtkImageDataLIC2D()
{
  this->NoiseSource->Delete();
  this->ImageCast->Delete();
  this->SetContext(nullptr);
}

//----------------------------------------------------------------------------
int svtkImageDataLIC2D::SetContext(svtkRenderWindow* renWin)
{
  svtkOpenGLRenderWindow* rw = svtkOpenGLRenderWindow::SafeDownCast(renWin);

  if (this->Context == rw)
  {
    return this->OpenGLExtensionsSupported;
  }

  if (this->Context && this->OwnWindow)
  {
    this->Context->Delete();
  }
  this->Modified();
  this->Context = nullptr;
  this->OwnWindow = false;
  this->OpenGLExtensionsSupported = 0;

  svtkOpenGLRenderWindow* context = svtkOpenGLRenderWindow::SafeDownCast(renWin);
  if (context)
  {
    context->Render();
    context->MakeCurrent();

    bool featureSupport = svtkLineIntegralConvolution2D::IsSupported(context) &&
      svtkPixelBufferObject::IsSupported(context) &&
      svtkOpenGLFramebufferObject::IsSupported(context) && svtkRenderbuffer::IsSupported(context) &&
      svtkTextureObject::IsSupported(context);

    if (!featureSupport)
    {
      svtkErrorMacro("Required OpenGL extensions not supported.");
      return 0;
    }

    this->OpenGLExtensionsSupported = 1;
    this->Context = context;
  }

  return 1;
}

//----------------------------------------------------------------------------
svtkRenderWindow* svtkImageDataLIC2D::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
int svtkImageDataLIC2D::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }

  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkImageDataLIC2D::TranslateInputExtent(
  const int* inExt, const int* inWholeExt, int* resultExt)
{
  int nPlanar = 0;
  for (int q = 0; q < 3; ++q)
  {
    int qq = 2 * q;
    if (inWholeExt[qq] == inWholeExt[qq + 1])
    {
      resultExt[qq] = inExt[qq];
      resultExt[qq + 1] = inExt[qq];
      nPlanar += 1;
    }
    else
    {
      resultExt[qq] = inExt[qq] * this->Magnification;
      resultExt[qq + 1] = (inExt[qq + 1] + 1) * this->Magnification - 1;
    }
  }
  if (nPlanar != 1)
  {
    svtkErrorMacro("Non-planar dataset");
  }
}

//----------------------------------------------------------------------------
int svtkImageDataLIC2D::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int ext[6];
  int wholeExtent[6];
  double spacing[3];

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  inInfo->Get(svtkDataObject::SPACING(), spacing);
  svtkDebugMacro(<< "Input WHOLE_EXTENT: " << PRINTEXTENT(wholeExtent) << endl);
  this->TranslateInputExtent(wholeExtent, wholeExtent, ext);

  for (int axis = 0; axis < 3; axis++)
  {
    // Change the data spacing
    spacing[axis] /= this->Magnification;
  }
  svtkDebugMacro(<< "WHOLE_EXTENT: " << PRINTEXTENT(ext) << endl);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageDataLIC2D::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Tell the vector field input the extents that we need from it.
  // The downstream request needs to be downsized based on the Magnification.
  int ext[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext);

  svtkDebugMacro(<< "Requested UPDATE_EXTENT: " << PRINTEXTENT(ext) << endl);
  for (int axis = 0; axis < 3; axis++)
  {
    int wholeMin = ext[axis * 2];
    int wholeMax = ext[axis * 2 + 1];

    // Scale the output extent
    wholeMin = wholeMin / this->Magnification;
    wholeMax = wholeMax / this->Magnification;

    ext[axis * 2] = wholeMin;
    ext[axis * 2 + 1] = wholeMax;
  }
  svtkDebugMacro(<< "UPDATE_EXTENT: " << PRINTEXTENT(ext) << endl);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);

  inInfo = inputVector[1]->GetInformationObject(0);
  if (inInfo)
  {
    // always request the whole noise image.
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageDataLIC2D::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    svtkErrorMacro("Empty input");
    return 0;
  }

  int dims[3];
  input->GetDimensions(dims);

  int dataDescription = svtkStructuredData::GetDataDescription(dims);

  if (svtkStructuredData::GetDataDimension(dataDescription) != 2)
  {
    svtkErrorMacro("Input is not a 2D image.");
    return 0;
  }

  svtkIdType numPoints = input->GetNumberOfPoints();
  svtkDataArray* inVectors = this->GetInputArrayToProcess(0, inputVector);
  if (!inVectors)
  {
    svtkErrorMacro("Vectors are required for line integral convolution.");
    return 0;
  }

  if (inVectors->GetNumberOfTuples() != numPoints)
  {
    svtkErrorMacro("Only point vectors are supported.");
    return 0;
  }

  if (!this->Context)
  {
    svtkRenderWindow* renWin = svtkRenderWindow::New();
    if (this->SetContext(renWin) == 0)
    {
      svtkErrorMacro("Missing required OpenGL extensions");
      renWin->Delete();
      return 0;
    }
    this->OwnWindow = true;
  }

  this->Context->MakeCurrent();
  svtkOpenGLClearErrorMacro();

  // Noise.
  svtkInformation* noiseInfo = inputVector[1]->GetInformationObject(0);
  svtkImageData* noise = nullptr;
  if (noiseInfo)
  {
    noise = svtkImageData::SafeDownCast(noiseInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (!noise)
    {
      svtkErrorMacro("Invalid noise dataset on input "
                    "Default noise dataset is used");
    }

    if ((noise->GetPointData() == nullptr) || (noise->GetPointData()->GetScalars() == nullptr))
    {
      svtkErrorMacro("Noise dataset missing point data scalars. "
                    "Default noise dataset is used");
      noise = nullptr;
    }

    double noiseRange[2];
    svtkDataArray* inVals = noise->GetPointData()->GetScalars();
    inVals->GetRange(noiseRange);
    if ((noiseRange[0] < 0.0) || (noiseRange[1] > 1.0))
    {
      svtkErrorMacro("Noise dataset has values out of range 0.0 to 1.0."
                    "Default noise dataset is used");
      noise = nullptr;
    }
  }

  if (!noise)
  {
    this->ImageCast->Update();
    noise = this->ImageCast->GetOutput();
  }

  int comp[3] = { 0, 0, 0 };
  switch (dataDescription)
  {
    case SVTK_XY_PLANE:
      comp[0] = 0;
      comp[1] = 1;
      comp[2] = 2;
      break;

    case SVTK_YZ_PLANE:
      comp[0] = 1;
      comp[1] = 2;
      comp[2] = 0;
      break;

    case SVTK_XZ_PLANE:
      comp[0] = 0;
      comp[1] = 2;
      comp[2] = 1;
      break;
  }

  // size of output
  int magDims[3];
  magDims[0] = this->Magnification * dims[0];
  magDims[1] = this->Magnification * dims[1];
  magDims[2] = this->Magnification * dims[2];

  // send vector data to a texture
  int inputExt[6];
  input->GetExtent(inputExt);

  svtkPixelExtent inVectorExtent(dims[comp[0]], dims[comp[1]]);

  svtkPixelBufferObject* vecPBO = svtkPixelBufferObject::New();
  vecPBO->SetContext(this->Context);

  svtkPixelTransfer::Blit(inVectorExtent, inVectorExtent, inVectorExtent, inVectorExtent, 3,
    inVectors->GetDataType(), inVectors->GetVoidPointer(0), 4, SVTK_FLOAT,
    vecPBO->MapUnpackedBuffer(SVTK_FLOAT, static_cast<unsigned int>(inVectorExtent.Size()), 4));

  vecPBO->UnmapUnpackedBuffer();

  svtkTextureObject* vectorTex = svtkTextureObject::New();
  vectorTex->SetContext(this->Context);
  vectorTex->Create2D(dims[comp[0]], dims[comp[1]], 4, vecPBO, false);
  svtkLineIntegralConvolution2D::SetVectorTexParameters(vectorTex);

  vecPBO->Delete();

#if (svtkImageDataLIC2DDEBUG >= 1)
  svtkTextureIO::Write("idlic2d_vectors.svtk", vectorTex, nullptr, nullptr);
#endif

  // magnify vectors
  svtkPixelExtent magVectorExtent(magDims[comp[0]], magDims[comp[1]]);
  int magVectorSize[2];
  magVectorExtent.Size(magVectorSize);

  svtkTextureObject* magVectorTex = vectorTex;
  if (this->Magnification > 1)
  {
    svtkOpenGLState* ostate = this->Context->GetState();
    magVectorTex = svtkTextureObject::New();
    magVectorTex->SetContext(this->Context);
    magVectorTex->Create2D(magVectorSize[0], magVectorSize[1], 4, SVTK_FLOAT, false);
    svtkLineIntegralConvolution2D::SetVectorTexParameters(magVectorTex);

    svtkOpenGLFramebufferObject* drawFbo = svtkOpenGLFramebufferObject::New();
    drawFbo->SetContext(this->Context);
    ostate->PushFramebufferBindings();
    drawFbo->Bind();
    drawFbo->AddColorAttachment(0U, magVectorTex);
    drawFbo->ActivateDrawBuffer(0U);
    // drawFbo->AddColorAttachment(svtkgl::FRAMEBUFFER_EXT, 0U, vectorTex);
    // drawFbo->ActivateReadBuffer(0U);
    drawFbo->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    drawFbo->InitializeViewport(magVectorSize[0], magVectorSize[1]);

    this->Context->GetState()->svtkglClearColor(0.0, 0.0, 0.0, 0.0);
    this->Context->GetState()->svtkglClear(GL_COLOR_BUFFER_BIT);

    float tcoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

    float verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f };

    svtkOpenGLHelper shaderHelper;

    // build the shader source code
    shaderHelper.Program = this->Context->GetShaderCache()->ReadyShaderProgram(svtkTextureObjectVS,
      "//SVTK::System::Dec\n"
      "in vec2 tcoordVC;\n"
      "uniform sampler2D source;\n"
      "//SVTK::Output::Dec\n"
      "void main(void) {\n"
      "  gl_FragData[0] = texture2D(source,tcoordVC); }\n",
      "");

    // bind and activate this texture
    vectorTex->Activate();
    int sourceId = vectorTex->GetTextureUnit();
    shaderHelper.Program->SetUniformi("source", sourceId);
    vectorTex->CopyToFrameBuffer(tcoords, verts, shaderHelper.Program, shaderHelper.VAO);
    vectorTex->Deactivate();
    vectorTex->Delete();
    shaderHelper.ReleaseGraphicsResources(this->Context);

    ostate->PopFramebufferBindings();
    drawFbo->Delete();
  }

#if (svtkImageDataLIC2DDEBUG >= 1)
  svtkTextureIO::Write("idlic2d_magvectors.svtk", magVectorTex, nullptr, nullptr);
#endif

  // send noise data to a texture
  svtkDataArray* inNoise = noise->GetPointData()->GetScalars();

  svtkPixelExtent noiseExt(noise->GetExtent());

  svtkPixelBufferObject* noisePBO = svtkPixelBufferObject::New();
  noisePBO->SetContext(this->Context);
  int noiseComp = inNoise->GetNumberOfComponents();

  if (inNoise->GetDataType() != SVTK_FLOAT)
  {
    svtkErrorMacro("noise dataset was not float");
  }

  svtkPixelTransfer::Blit(noiseExt, noiseComp, inNoise->GetDataType(), inNoise->GetVoidPointer(0),
    SVTK_FLOAT,
    noisePBO->MapUnpackedBuffer(SVTK_FLOAT, static_cast<unsigned int>(noiseExt.Size()), noiseComp));

  noisePBO->UnmapUnpackedBuffer();

  int noiseTexSize[2];
  noiseExt.Size(noiseTexSize);

  svtkTextureObject* noiseTex = svtkTextureObject::New();
  noiseTex->SetContext(this->Context);
  noiseTex->Create2D(noiseTexSize[0], noiseTexSize[1], noiseComp, noisePBO, false);

  noisePBO->Delete();

#if (svtkImageDataLIC2DDEBUG >= 1)
  svtkTextureIO::Write("idlic2d_noise.svtk", noiseTex, nullptr, nullptr);
#endif

  // step size conversion to normalize image space
  double* spacing = input->GetSpacing();
  spacing[comp[0]] /= this->Magnification;
  spacing[comp[1]] /= this->Magnification;

  double cellLength =
    sqrt(spacing[comp[0]] * spacing[comp[0]] + spacing[comp[1]] * spacing[comp[1]]);

  double w = spacing[comp[0]] * dims[comp[0]];
  double h = spacing[comp[1]] * dims[comp[1]];
  double normalizationFactor = sqrt(w * w + h * h);
  double stepSize = this->StepSize * cellLength / normalizationFactor;

  // compute the LIC
  int updateExt[6];
  inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExt);

  int magUpdateExt[6];
  magUpdateExt[2 * comp[0]] = updateExt[2 * comp[0]] * this->Magnification;
  magUpdateExt[2 * comp[1]] = updateExt[2 * comp[1]] * this->Magnification;
  magUpdateExt[2 * comp[2]] = updateExt[comp[2]];
  magUpdateExt[2 * comp[0] + 1] = (updateExt[2 * comp[0] + 1] + 1) * this->Magnification - 1;
  magUpdateExt[2 * comp[1] + 1] = (updateExt[2 * comp[1] + 1] + 1) * this->Magnification - 1;
  magUpdateExt[2 * comp[2] + 1] = updateExt[comp[2]];

  svtkPixelExtent magLicExtent(magUpdateExt[2 * comp[0]], magUpdateExt[2 * comp[0] + 1],
    magUpdateExt[2 * comp[1]], magUpdateExt[2 * comp[1] + 1]);

  // add ghosts
  double rk4fac = 3.0;
  int nGhosts = static_cast<int>(this->Steps * this->StepSize * rk4fac);
  nGhosts = nGhosts < 1 ? 1 : nGhosts;
  nGhosts *= 2; // for second ee lic pass

  svtkPixelExtent magLicGuardExtent(magLicExtent);
  magLicGuardExtent.Grow(nGhosts);
  magLicGuardExtent &= magVectorExtent;

  svtkLineIntegralConvolution2D* LICer = svtkLineIntegralConvolution2D::New();
  LICer->SetContext(this->Context);
  LICer->SetNumberOfSteps(this->Steps);
  LICer->SetStepSize(stepSize);
  LICer->SetComponentIds(comp[0], comp[1]);
  // LICer->SetGridSpacings(spacing[comp[0]], spacing[comp[1]]);

  deque<svtkPixelExtent> magLicExtents(1, magLicExtent);
  deque<svtkPixelExtent> magLicGuardExtents(1, magLicGuardExtent);

  svtkTextureObject* licTex = LICer->Execute(
    magVectorExtent, magLicGuardExtents, magLicExtents, magVectorTex, nullptr, noiseTex);

  LICer->Delete();
  noiseTex->Delete();
  magVectorTex->Delete();

  if (!licTex)
  {
    svtkErrorMacro("Failed to compute LIC");
    return 0;
  }

#if (svtkImageDataLIC2DDEBUG >= 1)
  svtkTextureIO::Write("idlic2d_lic.svtk", licTex, nullptr, nullptr);
#endif

  // transfer lic from texture to svtk array
  svtkIdType nOutTups = static_cast<svtkIdType>(magLicExtent.Size());
  svtkFloatArray* licOut = svtkFloatArray::New();
  licOut->SetNumberOfComponents(3);
  licOut->SetNumberOfTuples(nOutTups);
  licOut->SetName("LIC");

  svtkPixelBufferObject* licPBO = licTex->Download();

  svtkPixelTransfer::Blit<float, float>(magVectorExtent, magLicExtent, magLicExtent, magLicExtent, 4,
    (float*)licPBO->MapPackedBuffer(), 3, licOut->GetPointer(0));

  licPBO->UnmapPackedBuffer();
  licPBO->Delete();
  licTex->Delete();

  // mask and convert to gray scale 3 components
  float* pLicOut = licOut->GetPointer(0);
  for (svtkIdType i = 0; i < nOutTups; ++i)
  {
    float lic = pLicOut[3 * i];
    float mask = pLicOut[3 * i + 1];
    if (mask)
    {
      pLicOut[3 * i + 1] = pLicOut[3 * i + 2] = pLicOut[3 * i] = 0.0f;
    }
    else
    {
      pLicOut[3 * i + 1] = pLicOut[3 * i + 2] = lic;
    }
  }

  // setup output
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    svtkErrorMacro("Empty output");
    return 1;
  }

  output->SetExtent(magUpdateExt);
  output->SetSpacing(spacing);
  output->GetPointData()->SetScalars(licOut);
  licOut->Delete();

  // Ensures that the output extent is exactly same as what was asked for.
  // output->Crop(outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));

  return 1;
}

//----------------------------------------------------------------------------
void svtkImageDataLIC2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Steps: " << this->Steps << "\n";
  os << indent << "StepSize: " << this->StepSize << "\n";
  os << indent << "Magnification: " << this->Magnification << "\n";
  os << indent << "OpenGLExtensionsSupported: " << this->OpenGLExtensionsSupported << "\n";
}
