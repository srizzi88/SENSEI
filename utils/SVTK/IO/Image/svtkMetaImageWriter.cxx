/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMetaImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifdef _MSC_VER
#pragma warning(disable : 4018)
#endif

#include "svtkMetaImageWriter.h"

#include "svtkAlgorithmOutput.h"
#include "svtkCommand.h"
#include "svtkDataSetAttributes.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkmetaio/metaEvent.h"
#include "svtkmetaio/metaImage.h"
#include "svtkmetaio/metaImageTypes.h"
#include "svtkmetaio/metaImageUtils.h"
#include "svtkmetaio/metaObject.h"
#include "svtkmetaio/metaTypes.h"
#include "svtkmetaio/metaUtils.h"
#include <string>

#include <sys/stat.h>

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkMetaImageWriter);

//----------------------------------------------------------------------------
svtkMetaImageWriter::svtkMetaImageWriter()
{
  this->MHDFileName = nullptr;
  this->FileLowerLeft = 1;

  this->MetaImagePtr = new svtkmetaio::MetaImage;
  this->Compress = true;
}

//----------------------------------------------------------------------------
svtkMetaImageWriter::~svtkMetaImageWriter()
{
  this->SetFileName(nullptr);
  delete this->MetaImagePtr;
}

//----------------------------------------------------------------------------
void svtkMetaImageWriter::SetFileName(const char* fname)
{
  this->SetMHDFileName(fname);
  this->Superclass::SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void svtkMetaImageWriter::SetRAWFileName(const char* fname)
{
  this->Superclass::SetFileName(fname);
}

//----------------------------------------------------------------------------
char* svtkMetaImageWriter::GetRAWFileName()
{
  return this->Superclass::GetFileName();
}

//----------------------------------------------------------------------------
void svtkMetaImageWriter::Write()
{
  this->SetErrorCode(svtkErrorCode::NoError);

  svtkDemandDrivenPipeline::SafeDownCast(this->GetInputExecutive(0, 0))->UpdateInformation();

  // Error checking
  if (this->GetInput() == nullptr)
  {
    svtkErrorMacro(<< "Write:Please specify an input!");
    return;
  }

  if (!this->MHDFileName)
  {
    svtkErrorMacro("Output file name not specified");
    return;
  }

  int nDims = 3;
  int* ext = this->GetInputInformation(0, 0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  if (ext[4] == ext[5])
  {
    nDims = 2;
    if (ext[2] == ext[3])
    {
      nDims = 1;
    }
  }

  this->GetInputAlgorithm()->UpdateExtent(ext);

  double origin[3];
  double spacingDouble[3];
  this->GetInput()->GetOrigin(origin);
  this->GetInput()->GetSpacing(spacingDouble);

  float spacing[3];
  spacing[0] = spacingDouble[0];
  spacing[1] = spacingDouble[1];
  spacing[2] = spacingDouble[2];

  int dimSize[3];
  dimSize[0] = ext[1] - ext[0] + 1;
  dimSize[1] = ext[3] - ext[2] + 1;
  dimSize[2] = ext[5] - ext[4] + 1;

  svtkmetaio::MET_ValueEnumType elementType;

  int scalarType = this->GetInput()->GetScalarType();
  switch (scalarType)
  {
    case SVTK_CHAR:
      elementType = svtkmetaio::MET_CHAR;
      break;
    case SVTK_SIGNED_CHAR:
      elementType = svtkmetaio::MET_CHAR;
      break;
    case SVTK_UNSIGNED_CHAR:
      elementType = svtkmetaio::MET_UCHAR;
      break;
    case SVTK_SHORT:
      elementType = svtkmetaio::MET_SHORT;
      break;
    case SVTK_UNSIGNED_SHORT:
      elementType = svtkmetaio::MET_USHORT;
      break;
    case SVTK_INT:
      elementType = svtkmetaio::MET_INT;
      break;
    case SVTK_UNSIGNED_INT:
      elementType = svtkmetaio::MET_UINT;
      break;
    case SVTK_LONG:
      elementType = svtkmetaio::MET_LONG;
      break;
    case SVTK_UNSIGNED_LONG:
      elementType = svtkmetaio::MET_ULONG;
      break;
    case SVTK_FLOAT:
      elementType = svtkmetaio::MET_FLOAT;
      break;
    case SVTK_DOUBLE:
      elementType = svtkmetaio::MET_DOUBLE;
      break;
    default:
      svtkErrorMacro("Unknown scalar type.");
      return;
  }

  origin[0] += ext[0] * spacing[0];
  origin[1] += ext[2] * spacing[1];
  origin[2] += ext[4] * spacing[2];

  int numberOfElements = this->GetInput()->GetNumberOfScalarComponents();

  this->MetaImagePtr->InitializeEssential(nDims, dimSize, spacing, elementType, numberOfElements,
    this->GetInput()->GetScalarPointer(ext[0], ext[2], ext[4]), false);
  this->MetaImagePtr->Position(origin);

  if (this->GetRAWFileName())
  {
    this->MetaImagePtr->ElementDataFileName(this->GetRAWFileName());
  }

  this->SetFileDimensionality(nDims);
  this->MetaImagePtr->CompressedData(Compress);

  this->InvokeEvent(svtkCommand::StartEvent);
  this->UpdateProgress(0.0);
  this->MetaImagePtr->Write(this->MHDFileName);
  this->UpdateProgress(1.0);
  this->InvokeEvent(svtkCommand::EndEvent);
}

//----------------------------------------------------------------------------
void svtkMetaImageWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MHDFileName: " << (this->MHDFileName ? this->MHDFileName : "(none)") << endl;
}
