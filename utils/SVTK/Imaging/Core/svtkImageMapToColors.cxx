/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageMapToColors.h"

#include "svtkCharArray.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkScalarsToColors.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageMapToColors);
svtkCxxSetObjectMacro(svtkImageMapToColors, LookupTable, svtkScalarsToColors);

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImageMapToColors::svtkImageMapToColors()
{
  this->OutputFormat = 4;
  this->ActiveComponent = 0;
  this->PassAlphaToOutput = 0;
  this->LookupTable = nullptr;
  this->DataWasPassed = 0;

  // Black color
  this->NaNColor[0] = this->NaNColor[1] = this->NaNColor[2] = this->NaNColor[3] = 0;

  // Make sure the Scalars are used as default ArrayToProcess
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::POINT, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkImageMapToColors::~svtkImageMapToColors()
{
  if (this->LookupTable != nullptr)
  {
    this->LookupTable->UnRegister(this);
  }
}

//----------------------------------------------------------------------------
svtkMTimeType svtkImageMapToColors::GetMTime()
{
  svtkMTimeType t1, t2;

  t1 = this->Superclass::GetMTime();
  if (this->LookupTable)
  {
    t2 = this->LookupTable->GetMTime();
    if (t2 > t1)
    {
      t1 = t2;
    }
  }
  return t1;
}

//----------------------------------------------------------------------------
// This method checks to see if we can simply reference the input data
int svtkImageMapToColors::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkImageData* outData = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // If LookupTable is null, just pass the data
  if (this->LookupTable == nullptr)
  {
    svtkDebugMacro("RequestData: LookupTable not set, "
                  "passing input to output.");

    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    this->DataWasPassed = 1;
  }
  else // normal behaviour
  {
    this->LookupTable->Build(); // make sure table is built

    if (this->DataWasPassed)
    {
      outData->GetPointData()->SetScalars(nullptr);
      this->DataWasPassed = 0;
    }

    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageMapToColors::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int numComponents = 4;

  switch (this->OutputFormat)
  {
    case SVTK_RGBA:
      numComponents = 4;
      break;
    case SVTK_RGB:
      numComponents = 3;
      break;
    case SVTK_LUMINANCE_ALPHA:
      numComponents = 2;
      break;
    case SVTK_LUMINANCE:
      numComponents = 1;
      break;
    default:
      svtkErrorMacro("RequestInformation: Unrecognized color format.");
      break;
  }

  if (this->LookupTable == nullptr)
  {
    svtkInformation* scalarInfo = svtkDataObject::GetActiveFieldInformation(
      inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
    if (scalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE()) != SVTK_UNSIGNED_CHAR)
    {
      svtkErrorMacro("RequestInformation: No LookupTable was set but input data is not "
                    "SVTK_UNSIGNED_CHAR, therefore input can't be passed through!");
      return 1;
    }
    else if (numComponents != scalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
    {
      svtkErrorMacro("RequestInformation: No LookupTable was set but number of components "
                    "in input doesn't match OutputFormat, therefore input can't be passed"
                    " through!");
      return 1;
    }
  }

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_UNSIGNED_CHAR, numComponents);
  return 1;
}

//----------------------------------------------------------------------------
// This non-templated function executes the filter for any type of data.
// All the data to process should be achieved outside this method as
// we can not always rely on the ActiveScalar information.

static void svtkImageMapToColorsExecute(svtkImageMapToColors* self, svtkImageData* inData,
  svtkDataArray* inArray, svtkCharArray* maskArray, svtkImageData* outData, svtkDataArray* outArray,
  int outExt[6], int id, unsigned char* nanColor)
{
  int idxY, idxZ;
  int extX, extY, extZ;
  svtkIdType inIncX, inIncY, inIncZ, inMaskIncX, inMaskIncY, inMaskIncZ;
  svtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int dataType = inArray->GetDataType();
  int scalarSize = inArray->GetDataTypeSize();

  int coordinate[3] = { outExt[0], outExt[2], outExt[4] };
  void* inPtr = inData->GetArrayPointer(inArray, coordinate);
  char* inMask =
    maskArray ? static_cast<char*>(inData->GetArrayPointer(maskArray, coordinate)) : nullptr;

  int numberOfComponents, numberOfOutputComponents, outputFormat;
  int rowLength;
  svtkScalarsToColors* lookupTable = self->GetLookupTable();
  unsigned char* outPtr =
    static_cast<unsigned char*>(outData->GetArrayPointer(outArray, coordinate));
  unsigned char* outPtr1;
  void* inPtr1;

  // find the region to loop over
  extX = outExt[1] - outExt[0] + 1;
  extY = outExt[3] - outExt[2] + 1;
  extZ = outExt[5] - outExt[4] + 1;

  target = static_cast<unsigned long>(extZ * extY / 50.0);
  target++;

  // Get increments to march through data
  inData->GetContinuousIncrements(inArray, outExt, inIncX, inIncY, inIncZ);
  inMaskIncX = inMaskIncY = inMaskIncZ = 0;
  if (maskArray)
  {
    inData->GetContinuousIncrements(maskArray, outExt, inMaskIncX, inMaskIncY, inMaskIncZ);
  }
  // because we are using void * and char * we must take care
  // of the scalar size in the increments
  inIncY *= scalarSize;
  inIncZ *= scalarSize;
  outData->GetContinuousIncrements(outArray, outExt, outIncX, outIncY, outIncZ);
  numberOfComponents = inData->GetNumberOfScalarComponents();
  numberOfOutputComponents = outData->GetNumberOfScalarComponents();
  outputFormat = self->GetOutputFormat();
  rowLength = extX * scalarSize * numberOfComponents;

  // Loop through output pixels
  outPtr1 = outPtr;
  inPtr1 = static_cast<void*>(static_cast<char*>(inPtr) + self->GetActiveComponent() * scalarSize);
  for (idxZ = 0; idxZ < extZ; idxZ++)
  {
    for (idxY = 0; !self->AbortExecute && idxY < extY; idxY++)
    {
      if (!id)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (50.0 * target));
        }
        count++;
      }
      lookupTable->MapScalarsThroughTable2(
        inPtr1, outPtr1, dataType, extX, numberOfComponents, outputFormat);
      // Handle NaN color when mask
      if (inMask != nullptr)
      {
        unsigned char* outPtr2 = outPtr1;
        for (svtkIdType idx = 0; idx < extX; ++idx, outPtr2 += outputFormat)
        {
          if (!inMask[idx])
          {
            switch (outputFormat)
            {
              case 4:
                outPtr2[3] = nanColor[3];
                SVTK_FALLTHROUGH;
              case 3:
                outPtr2[2] = nanColor[2];
                SVTK_FALLTHROUGH;
              case 2:
                outPtr2[1] = nanColor[1];
                SVTK_FALLTHROUGH;
              case 1:
                outPtr2[0] = nanColor[0];
            }
          }
        }
      }
      if (self->GetPassAlphaToOutput() && dataType == SVTK_UNSIGNED_CHAR && numberOfComponents > 1 &&
        (outputFormat == SVTK_RGBA || outputFormat == SVTK_LUMINANCE_ALPHA))
      {
        unsigned char* outPtr2 = outPtr1 + numberOfOutputComponents - 1;
        unsigned char* inPtr2 = static_cast<unsigned char*>(inPtr1) -
          self->GetActiveComponent() * scalarSize + numberOfComponents - 1;
        for (int i = 0; i < extX; i++)
        {
          *outPtr2 = (*outPtr2 * *inPtr2) / 255;
          outPtr2 += numberOfOutputComponents;
          inPtr2 += numberOfComponents;
        }
      }
      outPtr1 += outIncY + extX * numberOfOutputComponents;
      inPtr1 = static_cast<void*>(static_cast<char*>(inPtr1) + inIncY + rowLength);

      // Just move pointer if not nullptr
      inMask += inMask ? inMaskIncY + extX : 0;
    }
    outPtr1 += outIncZ;
    inPtr1 = static_cast<void*>(static_cast<char*>(inPtr1) + inIncZ);

    // Just move pointer if not nullptr
    inMask += inMask ? inMaskIncZ : 0;
  }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.

void svtkImageMapToColors::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int id)
{
  svtkDataArray* outArray = outData[0]->GetPointData()->GetScalars();
  svtkCharArray* maskArray =
    svtkArrayDownCast<svtkCharArray>(inData[0][0]->GetPointData()->GetArray("svtkValidPointMask"));
  svtkDataArray* inArray = this->GetInputArrayToProcess(0, inputVector);

  // Working method
  svtkImageMapToColorsExecute(
    this, inData[0][0], inArray, maskArray, outData[0], outArray, outExt, id, this->NaNColor);
}

//----------------------------------------------------------------------------
void svtkImageMapToColors::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputFormat: "
     << (this->OutputFormat == SVTK_RGBA
            ? "RGBA"
            : (this->OutputFormat == SVTK_RGB
                  ? "RGB"
                  : (this->OutputFormat == SVTK_LUMINANCE_ALPHA
                        ? "LuminanceAlpha"
                        : (this->OutputFormat == SVTK_LUMINANCE ? "Luminance" : "Unknown"))))
     << "\n";
  os << indent << "ActiveComponent: " << this->ActiveComponent << "\n";
  os << indent << "PassAlphaToOutput: " << this->PassAlphaToOutput << "\n";
  os << indent << "LookupTable: ";
  if (this->LookupTable)
  {
    this->LookupTable->PrintSelf(os << endl, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
