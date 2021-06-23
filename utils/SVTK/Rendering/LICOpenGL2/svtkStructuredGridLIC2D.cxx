/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridLIC2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredGridLIC2D.h"

#include "svtkOpenGLHelper.h"

#include "svtkDataTransferHelper.h"
#include "svtkFloatArray.h"
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
#include "svtkPointData.h"
#include "svtkShaderProgram.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredExtent.h"
#include "svtkTextureObject.h"

#include "svtkPixelBufferObject.h"
#include "svtkPixelExtent.h"
#include "svtkPixelTransfer.h"

#include <cassert>

#include "svtkStructuredGridLIC2D_fs.h"
#include "svtkTextureObjectVS.h"

#define PRINTEXTENT(ext)                                                                           \
  ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]

svtkStandardNewMacro(svtkStructuredGridLIC2D);
//----------------------------------------------------------------------------
svtkStructuredGridLIC2D::svtkStructuredGridLIC2D()
{
  this->Context = nullptr;
  this->Steps = 1;
  this->StepSize = 1.0;
  this->Magnification = 1;
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
  this->OwnWindow = false;
  this->FBOSuccess = 0;
  this->LICSuccess = 0;

  this->NoiseSource = svtkImageNoiseSource::New();
  this->NoiseSource->SetWholeExtent(0, 127, 0, 127, 0, 0);
  this->NoiseSource->SetMinimum(0.0);
  this->NoiseSource->SetMaximum(1.0);

  this->LICProgram = nullptr;
}

//----------------------------------------------------------------------------
svtkStructuredGridLIC2D::~svtkStructuredGridLIC2D()
{
  this->NoiseSource->Delete();
  this->SetContext(nullptr);
}

//----------------------------------------------------------------------------
svtkRenderWindow* svtkStructuredGridLIC2D::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
int svtkStructuredGridLIC2D::SetContext(svtkRenderWindow* context)
{
  if (this->Context && this->OwnWindow)
  {
    this->Context->Delete();
    this->Context = nullptr;
  }
  this->OwnWindow = false;

  svtkOpenGLRenderWindow* openGLRenWin = svtkOpenGLRenderWindow::SafeDownCast(context);
  this->Context = openGLRenWin;

  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
// Description:
// Fill the input port information objects for this algorithm.  This
// is invoked by the first call to GetInputPortInformation for each
// port so subclasses can specify what they can handle.
// Redefined from the superclass.
int svtkStructuredGridLIC2D::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

// ----------------------------------------------------------------------------
// Description:
// Fill the output port information objects for this algorithm.
// This is invoked by the first call to GetOutputPortInformation for
// each port so subclasses can specify what they can handle.
// Redefined from the superclass.
int svtkStructuredGridLIC2D::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    // input+texcoords
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredGrid");
  }
  else
  {
    // LIC texture
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  }

  return 1;
}
//----------------------------------------------------------------------------
// We need to report output extent after taking into consideration the
// magnification.
int svtkStructuredGridLIC2D::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int ext[6];
  double spacing[3];

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(1);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);

  spacing[0] = 1.0;
  spacing[1] = 1.0;
  spacing[2] = 1.0;

  for (int axis = 0; axis < 3; axis++)
  {
    int wholeMin = ext[axis * 2];
    int wholeMax = ext[axis * 2 + 1];
    int dimension = wholeMax - wholeMin + 1;

    // Scale the output extent
    wholeMin = static_cast<int>(ceil(static_cast<double>(wholeMin * this->Magnification)));
    wholeMax = (dimension != 1)
      ? wholeMin + static_cast<int>(floor(static_cast<double>(dimension * this->Magnification))) - 1
      : wholeMin;

    ext[axis * 2] = wholeMin;
    ext[axis * 2 + 1] = wholeMax;
  }

  svtkDebugMacro(<< "request info whole ext = " << PRINTEXTENT(ext) << endl);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//----------------------------------------------------------------------------
int svtkStructuredGridLIC2D::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(1);

  // Tell the vector field input the extents that we need from it.
  // The downstream request needs to be downsized based on the Magnification.
  int ext[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext);

  svtkDebugMacro(<< "request update extent, update ext = " << PRINTEXTENT(ext) << endl);

  for (int axis = 0; axis < 3; axis++)
  {
    int wholeMin = ext[axis * 2];
    int wholeMax = ext[axis * 2 + 1];
    int dimension = wholeMax - wholeMin + 1;

    // Scale the output extent
    wholeMin = static_cast<int>(ceil(static_cast<double>(wholeMin / this->Magnification)));
    wholeMax = dimension != 1
      ? wholeMin + static_cast<int>(floor(static_cast<double>(dimension / this->Magnification))) - 1
      :

      ext[axis * 2] = wholeMin;
    ext[axis * 2 + 1] = wholeMax;
  }
  svtkDebugMacro(<< "UPDATE_EXTENT: " << ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3]
                << ", " << ext[4] << ", " << ext[5] << endl);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);

  svtkDebugMacro(<< "request update extent, update ext2 = " << PRINTEXTENT(ext) << endl);

  if (inputVector[1] != nullptr &&
    inputVector[1]->GetInformationObject(0) != nullptr) // optional input
  {
    inInfo = inputVector[1]->GetInformationObject(0);
    // always request the whole extent
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Stolen from svtkImageAlgorithm. Should be in svtkStructuredGridAlgorithm.
void svtkStructuredGridLIC2D::AllocateOutputData(svtkDataObject* output, svtkInformation* outInfo)
{
  // set the extent to be the update extent
  svtkStructuredGrid* out = svtkStructuredGrid::SafeDownCast(output);
  if (out)
  {
    out->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  }
  else
  {
    svtkImageData* out2 = svtkImageData::SafeDownCast(output);
    if (out2)
    {
      out2->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
    }
  }
}

//----------------------------------------------------------------------------
// Stolen from svtkImageData. Should be in svtkStructuredGrid.
void svtkStructuredGridLIC2D::AllocateScalars(svtkStructuredGrid* sg, svtkInformation* outInfo)
{
  int newType = SVTK_DOUBLE;
  int newNumComp = 1;

  svtkInformation* scalarInfo = svtkDataObject::GetActiveFieldInformation(
    outInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
  if (scalarInfo)
  {
    newType = scalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());
    if (scalarInfo->Has(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
    {
      newNumComp = scalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
    }
  }

  svtkDataArray* scalars;

  // if the scalar type has not been set then we have a problem
  if (newType == SVTK_VOID)
  {
    svtkErrorMacro("Attempt to allocate scalars before scalar type was set!.");
    return;
  }

  const int* extent = sg->GetExtent();
  // Use svtkIdType to avoid overflow on large images
  svtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  svtkIdType imageSize = dims[0] * dims[1] * dims[2];

  // if we currently have scalars then just adjust the size
  scalars = sg->GetPointData()->GetScalars();
  if (scalars && scalars->GetDataType() == newType && scalars->GetReferenceCount() == 1)
  {
    scalars->SetNumberOfComponents(newNumComp);
    scalars->SetNumberOfTuples(imageSize);
    // Since the execute method will be modifying the scalars
    // directly.
    scalars->Modified();
    return;
  }

  // allocate the new scalars
  scalars = svtkDataArray::CreateDataArray(newType);
  scalars->SetNumberOfComponents(newNumComp);

  // allocate enough memory
  scalars->SetNumberOfTuples(imageSize);

  sg->GetPointData()->SetScalars(scalars);
  scalars->Delete();
}

//----------------------------------------------------------------------------
int svtkStructuredGridLIC2D::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // 3 passes:
  // pass 1: render to compute the transformed vector field for the points.
  // pass 2: perform LIC with the new vector field. This has to happen in a
  // different pass than computation of the transformed vector.
  // pass 3: Render structured slice quads with correct texture correct
  // tcoords and apply the LIC texture to it.

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkStructuredGrid* input =
    svtkStructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  int inputRequestedExtent[6];
  inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inputRequestedExtent);

  // Check if the input image is a 2D image (not 0D, not 1D, not 3D)
  int dims[3];
  svtkStructuredExtent::GetDimensions(inputRequestedExtent, dims);

  svtkDebugMacro(<< "dims = " << dims[0] << " " << dims[1] << " " << dims[2] << endl);
  svtkDebugMacro(<< "requested ext = " << inputRequestedExtent[0] << " " << inputRequestedExtent[1]
                << " " << inputRequestedExtent[2] << " " << inputRequestedExtent[3] << " "
                << inputRequestedExtent[4] << " " << inputRequestedExtent[5] << endl);

  if (!((dims[0] == 1) && (dims[1] > 1) && (dims[2] > 1)) &&
    !((dims[1] == 1) && (dims[0] > 1) && (dims[2] > 1)) &&
    !((dims[2] == 1) && (dims[0] > 1) && (dims[1] > 1)))
  {
    svtkErrorMacro(<< "input is not a 2D image." << endl);
    input = nullptr;
    inInfo = nullptr;
    return 0;
  }
  if (input->GetPointData() == nullptr)
  {
    svtkErrorMacro(<< "input does not have point data.");
    input = nullptr;
    inInfo = nullptr;
    return 0;
  }
  if (input->GetPointData()->GetVectors() == nullptr)
  {
    svtkErrorMacro(<< "input does not vectors on point data.");
    input = nullptr;
    inInfo = nullptr;
    return 0;
  }

  if (!this->Context)
  {
    svtkRenderWindow* renWin = svtkRenderWindow::New();
    if (this->SetContext(renWin) == 0)
    {
      svtkErrorMacro("Invalid render window");
      renWin->Delete();
      renWin = nullptr;
      input = nullptr;
      inInfo = nullptr;
      return 0;
    }

    renWin = nullptr; // to be released via this->context
    this->OwnWindow = true;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkStructuredGrid* output =
    svtkStructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  this->AllocateOutputData(output, outInfo);
  output->ShallowCopy(input);

  svtkInformation* outInfoTexture = outputVector->GetInformationObject(1);
  svtkImageData* outputTexture =
    svtkImageData::SafeDownCast(outInfoTexture->Get(svtkDataObject::DATA_OBJECT()));
  this->AllocateOutputData(outputTexture, outInfoTexture);

  // Noise.
  svtkInformation* noiseInfo = inputVector[1]->GetInformationObject(0);
  svtkImageData* noise = nullptr;
  if (noiseInfo == nullptr)
  {
    this->NoiseSource->Update();
    noise = this->NoiseSource->GetOutput();
  }
  else
  {
    noise = svtkImageData::SafeDownCast(noiseInfo->Get(svtkDataObject::DATA_OBJECT()));

    if (noise->GetPointData() == nullptr)
    {
      svtkErrorMacro(<< "provided noise does not have point data.");
      return 0;
    }
    if (noise->GetPointData()->GetScalars() == nullptr)
    {
      svtkErrorMacro(<< "provided noise does not have scalars on point data.");
      return 0;
    }
  }

  svtkOpenGLClearErrorMacro();

  int width;
  int height;
  int firstComponent;
  int secondComponent;
  int slice;
  if (dims[0] == 1)
  {
    svtkDebugMacro(<< "x" << endl);
    firstComponent = 1;
    secondComponent = 2;
    slice = 0;
  }
  else
  {
    if (dims[1] == 1)
    {
      svtkDebugMacro(<< "y" << endl);
      firstComponent = 0;
      secondComponent = 2;
      slice = 1;
    }
    else
    {
      svtkDebugMacro(<< "z" << endl);
      firstComponent = 0;
      secondComponent = 1;
      slice = 2;
    }
  }

  width = dims[firstComponent];
  height = dims[secondComponent];

  svtkDebugMacro(<< "w = " << width << " h = " << height << endl);

  svtkDataTransferHelper* vectorFieldBus = svtkDataTransferHelper::New();
  vectorFieldBus->SetContext(this->Context);
  vectorFieldBus->SetCPUExtent(inputRequestedExtent); // input->GetExtent());
  vectorFieldBus->SetGPUExtent(inputRequestedExtent); // input->GetExtent());
  //  vectorFieldBus->SetTextureExtent(input->GetExtent());
  vectorFieldBus->SetArray(input->GetPointData()->GetVectors());

  svtkDataTransferHelper* pointBus = svtkDataTransferHelper::New();
  pointBus->SetContext(this->Context);
  pointBus->SetCPUExtent(inputRequestedExtent); // input->GetExtent());
  pointBus->SetGPUExtent(inputRequestedExtent); // input->GetExtent());
  //  pointBus->SetTextureExtent(input->GetExtent());
  pointBus->SetArray(input->GetPoints()->GetData());

  // Vector field in image space.
  int magWidth = this->Magnification * width;
  int magHeight = this->Magnification * height;

  svtkOpenGLRenderWindow* renWin = svtkOpenGLRenderWindow::SafeDownCast(this->Context);
  svtkTextureObject* vector2 = svtkTextureObject::New();
  vector2->SetContext(renWin);
  vector2->Create2D(magWidth, magHeight, 3, SVTK_FLOAT, false);

  svtkDebugMacro(<< "Vector field in image space (target) textureId = " << vector2->GetHandle()
                << endl);

  svtkOpenGLState* ostate = renWin->GetState();
  ostate->PushFramebufferBindings();
  svtkOpenGLFramebufferObject* fbo = svtkOpenGLFramebufferObject::New();
  fbo->SetContext(renWin);
  fbo->Bind();
  fbo->AddColorAttachment(0, vector2);
  fbo->ActivateDrawBuffer(0);
  fbo->ActivateReadBuffer(0);

  // TODO --
  // step size is incorrect here
  // guard pixels are needed for parallel operations

  if (!fbo->Start(magWidth, magHeight))
  {
    ostate->PopFramebufferBindings();
    fbo->Delete();
    vector2->Delete();
    pointBus->Delete();
    vectorFieldBus->Delete();

    fbo = nullptr;
    vector2 = nullptr;
    pointBus = nullptr;
    vectorFieldBus = nullptr;

    noise = nullptr;
    input = nullptr;
    inInfo = nullptr;
    output = nullptr;
    outInfo = nullptr;
    noiseInfo = nullptr;
    outputTexture = nullptr;
    outInfoTexture = nullptr;

    this->FBOSuccess = 0;
    return 0;
  }
  this->FBOSuccess = 1;

  // build the shader program
  this->LICProgram = new svtkOpenGLHelper;
  std::string VSSource = svtkTextureObjectVS;
  std::string FSSource = svtkStructuredGridLIC2D_fs;
  std::string GSSource;
  this->LICProgram->Program = renWin->GetShaderCache()->ReadyShaderProgram(
    VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
  svtkShaderProgram* pgm = this->LICProgram->Program;

  float fvalues[3];
  fvalues[0] = static_cast<float>(dims[0]);
  fvalues[1] = static_cast<float>(dims[1]);
  fvalues[2] = static_cast<float>(dims[2]);
  pgm->SetUniform3f("uDimensions", fvalues);
  pgm->SetUniformi("uSlice", slice);

  pointBus->Upload(0, nullptr);
  svtkTextureObject* points = pointBus->GetTexture();
  points->SetWrapS(svtkTextureObject::ClampToEdge);
  points->SetWrapT(svtkTextureObject::ClampToEdge);
  points->SetWrapR(svtkTextureObject::ClampToEdge);

  vectorFieldBus->Upload(0, nullptr);
  svtkTextureObject* vectorField = vectorFieldBus->GetTexture();
  vectorField->SetWrapS(svtkTextureObject::ClampToEdge);
  vectorField->SetWrapT(svtkTextureObject::ClampToEdge);
  vectorField->SetWrapR(svtkTextureObject::ClampToEdge);

  points->Activate();
  pgm->SetUniformi("texPoints", points->GetTextureUnit());
  vectorField->Activate();
  pgm->SetUniformi("texVectorField", vectorField->GetTextureUnit());

  svtkOpenGLCheckErrorMacro("failed during config");

  svtkDebugMacro(<< "glFinish before rendering quad" << endl);

  fbo->RenderQuad(0, magWidth - 1, 0, magHeight - 1, pgm, this->LICProgram->VAO);
  svtkOpenGLCheckErrorMacro("StructuredGridLIC2D projection fialed");

  svtkDebugMacro(<< "glFinish after rendering quad" << endl);

  svtkLineIntegralConvolution2D* internal = svtkLineIntegralConvolution2D::New();
  if (!internal->IsSupported(this->Context))
  {
    this->LICProgram->ReleaseGraphicsResources(renWin);
    delete this->LICProgram;
    this->LICProgram = nullptr;

    ostate->PopFramebufferBindings();
    fbo->Delete();
    vector2->Delete();
    internal->Delete();
    pointBus->Delete();
    vectorFieldBus->Delete();

    fbo = nullptr;
    vector2 = nullptr;
    internal = nullptr;
    pointBus = nullptr;
    vectorFieldBus = nullptr;

    noise = nullptr;
    input = nullptr;
    inInfo = nullptr;
    points = nullptr;
    output = nullptr;
    outInfo = nullptr;
    noiseInfo = nullptr;
    vectorField = nullptr;
    outputTexture = nullptr;
    outInfoTexture = nullptr;

    this->LICSuccess = 0;
    return 0;
  }

  internal->SetContext(renWin);
  internal->SetNumberOfSteps(this->Steps);
  internal->SetStepSize(this->StepSize);
  internal->SetComponentIds(firstComponent, secondComponent);

  svtkDataTransferHelper* noiseBus = svtkDataTransferHelper::New();
  noiseBus->SetContext(this->Context);
  noiseBus->SetCPUExtent(noise->GetExtent());
  noiseBus->SetGPUExtent(noise->GetExtent());
  //  noiseBus->SetTextureExtent(noise->GetExtent());
  noiseBus->SetArray(noise->GetPointData()->GetScalars());
  noiseBus->Upload(0, nullptr);

  svtkTextureObject* licTex = internal->Execute(vector2, noiseBus->GetTexture());
  if (licTex == nullptr)
  {
    this->LICProgram->ReleaseGraphicsResources(renWin);
    delete this->LICProgram;
    this->LICProgram = nullptr;

    ostate->PopFramebufferBindings();
    fbo->Delete();
    vector2->Delete();
    internal->Delete();
    pointBus->Delete();
    noiseBus->Delete();
    vectorFieldBus->Delete();

    fbo = nullptr;
    vector2 = nullptr;
    internal = nullptr;
    pointBus = nullptr;
    noiseBus = nullptr;
    vectorFieldBus = nullptr;

    noise = nullptr;
    input = nullptr;
    inInfo = nullptr;
    points = nullptr;
    output = nullptr;
    outInfo = nullptr;
    noiseInfo = nullptr;
    vectorField = nullptr;
    outputTexture = nullptr;
    outInfoTexture = nullptr;

    this->LICSuccess = 0;
    return 0;
  }
  this->LICSuccess = 1;

  // transfer lic from texture to svtk array
  svtkPixelExtent magLicExtent(magWidth, magHeight);
  svtkIdType nOutTups = static_cast<svtkIdType>(magLicExtent.Size());

  svtkFloatArray* licOut = svtkFloatArray::New();
  licOut->SetNumberOfComponents(3);
  licOut->SetNumberOfTuples(nOutTups);
  licOut->SetName("LIC");

  svtkPixelBufferObject* licPBO = licTex->Download();

  svtkPixelTransfer::Blit<float, float>(magLicExtent, magLicExtent, magLicExtent, magLicExtent, 4,
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

  outputTexture->GetPointData()->SetScalars(licOut);
  licOut->Delete();

  // Pass three. Generate texture coordinates. Software.
  svtkFloatArray* tcoords = svtkFloatArray::New();
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
  output->GetPointData()->SetTCoords(tcoords);
  tcoords->Delete();

  double ddim[3];
  ddim[0] = static_cast<double>(dims[0] - 1);
  ddim[1] = static_cast<double>(dims[1] - 1);
  ddim[2] = static_cast<double>(dims[2] - 1);

  int tz = 0;
  while (tz < dims[slice])
  {
    int ty = 0;
    while (ty < dims[secondComponent])
    {
      int tx = 0;
      while (tx < dims[firstComponent])
      {
        tcoords->SetTuple2((tz * dims[secondComponent] + ty) * dims[firstComponent] + tx,
          tx / ddim[firstComponent], ty / ddim[secondComponent]);
        ++tx;
      }
      ++ty;
    }
    ++tz;
  }

  ostate->PopFramebufferBindings();

  internal->Delete();
  noiseBus->Delete();
  vectorFieldBus->Delete();
  pointBus->Delete();
  vector2->Delete();
  fbo->Delete();

  this->LICProgram->ReleaseGraphicsResources(renWin);
  delete this->LICProgram;
  this->LICProgram = nullptr;

  svtkOpenGLCheckErrorMacro("failed after RequestData");

  return 1;
}

//----------------------------------------------------------------------------
void svtkStructuredGridLIC2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Steps: " << this->Steps << "\n";
  os << indent << "StepSize: " << this->StepSize << "\n";
  os << indent << "FBOSuccess: " << this->FBOSuccess << "\n";
  os << indent << "LICSuccess: " << this->LICSuccess << "\n";
  os << indent << "Magnification: " << this->Magnification << "\n";
}
