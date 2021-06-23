/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMetaImageReader.cxx

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

#include "svtkMetaImageReader.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtksys/FStream.hxx"

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
svtkStandardNewMacro(svtkMetaImageReader);

//----------------------------------------------------------------------------
svtkMetaImageReader::svtkMetaImageReader()
{
  GantryAngle = 0;
  strcpy(PatientName, "?");
  strcpy(PatientID, "?");
  strcpy(Date, "?");
  strcpy(Series, "?");
  strcpy(Study, "?");
  strcpy(ImageNumber, "?");
  strcpy(Modality, "?");
  strcpy(StudyID, "?");
  strcpy(StudyUID, "?");
  strcpy(TransferSyntaxUID, "?");

  RescaleSlope = 1;
  RescaleOffset = 0;
  BitsAllocated = 0;
  strcpy(DistanceUnits, "mm");
  strcpy(AnatomicalOrientation, "RAS");
  this->MetaImagePtr = new svtkmetaio::MetaImage;
  this->FileLowerLeft = 1;
}

//----------------------------------------------------------------------------
svtkMetaImageReader::~svtkMetaImageReader()
{
  delete this->MetaImagePtr;
}

//----------------------------------------------------------------------------
void svtkMetaImageReader::ExecuteInformation()
{
  if (!this->FileName)
  {
    svtkErrorMacro(<< "A filename was not specified.");
    return;
  }

  if (!this->MetaImagePtr->Read(this->FileName, false))
  {
    svtkErrorMacro(<< "MetaImage cannot parse file.");
    return;
  }

  this->SetFileDimensionality(this->MetaImagePtr->NDims());
  if (FileDimensionality <= 0 || FileDimensionality >= 4)
  {
    svtkErrorMacro(<< "Only understands image data of 1, 2, and 3 dimensions. "
                  << "This image has " << FileDimensionality << " dimensions");
    return;
  }
  svtkDebugMacro(<< "* This image has " << FileDimensionality << " dimensions");

  int i;

  switch (this->MetaImagePtr->ElementType())
  {
    case svtkmetaio::MET_NONE:
    case svtkmetaio::MET_ASCII_CHAR:
    case svtkmetaio::MET_LONG_LONG:
    case svtkmetaio::MET_ULONG_LONG:
    case svtkmetaio::MET_STRING:
    case svtkmetaio::MET_LONG_LONG_ARRAY:
    case svtkmetaio::MET_ULONG_LONG_ARRAY:
    case svtkmetaio::MET_FLOAT_ARRAY:
    case svtkmetaio::MET_DOUBLE_ARRAY:
    case svtkmetaio::MET_FLOAT_MATRIX:
    case svtkmetaio::MET_OTHER:
    default:
      svtkErrorMacro(<< "Unknown data type: " << this->MetaImagePtr->ElementType());
      return;
    case svtkmetaio::MET_CHAR:
    case svtkmetaio::MET_CHAR_ARRAY:
      this->DataScalarType = SVTK_SIGNED_CHAR;
      break;
    case svtkmetaio::MET_UCHAR:
    case svtkmetaio::MET_UCHAR_ARRAY:
      this->DataScalarType = SVTK_UNSIGNED_CHAR;
      break;
    case svtkmetaio::MET_SHORT:
    case svtkmetaio::MET_SHORT_ARRAY:
      this->DataScalarType = SVTK_SHORT;
      break;
    case svtkmetaio::MET_USHORT:
    case svtkmetaio::MET_USHORT_ARRAY:
      this->DataScalarType = SVTK_UNSIGNED_SHORT;
      break;
    case svtkmetaio::MET_INT:
    case svtkmetaio::MET_INT_ARRAY:
      this->DataScalarType = SVTK_INT;
      break;
    case svtkmetaio::MET_UINT:
    case svtkmetaio::MET_UINT_ARRAY:
      this->DataScalarType = SVTK_UNSIGNED_INT;
      break;
    case svtkmetaio::MET_LONG:
    case svtkmetaio::MET_LONG_ARRAY:
      this->DataScalarType = SVTK_LONG;
      break;
    case svtkmetaio::MET_ULONG:
    case svtkmetaio::MET_ULONG_ARRAY:
      this->DataScalarType = SVTK_UNSIGNED_LONG;
      break;
    case svtkmetaio::MET_FLOAT:
      this->DataScalarType = SVTK_FLOAT;
      break;
    case svtkmetaio::MET_DOUBLE:
      this->DataScalarType = SVTK_DOUBLE;
      break;
  }

  int extent[6] = { 0, 0, 0, 0, 0, 0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  double origin[3] = { 0.0, 0.0, 0.0 };
  for (i = 0; i < FileDimensionality; i++)
  {
    extent[2 * i] = 0;
    extent[2 * i + 1] = this->MetaImagePtr->DimSize(i) - 1;
    spacing[i] = fabs(this->MetaImagePtr->ElementSpacing(i));
    origin[i] = this->MetaImagePtr->Position(i);
  }
  this->SetNumberOfScalarComponents(this->MetaImagePtr->ElementNumberOfChannels());
  this->SetDataExtent(extent);
  this->SetDataSpacing(spacing);
  this->SetDataOrigin(origin);
  this->SetHeaderSize(this->MetaImagePtr->HeaderSize());
  this->FileLowerLeftOn();

  switch (this->MetaImagePtr->DistanceUnits())
  {
    default:
    case svtkmetaio::MET_DISTANCE_UNITS_UNKNOWN:
    case svtkmetaio::MET_DISTANCE_UNITS_UM:
    {
      strcpy(DistanceUnits, "um");
      break;
    }
    case svtkmetaio::MET_DISTANCE_UNITS_MM:
    {
      strcpy(DistanceUnits, "mm");
      break;
    }
    case svtkmetaio::MET_DISTANCE_UNITS_CM:
    {
      strcpy(DistanceUnits, "cm");
      break;
    }
  }

  strcpy(AnatomicalOrientation, this->MetaImagePtr->AnatomicalOrientationAcronym());

  svtkmetaio::MET_SizeOfType(this->MetaImagePtr->ElementType(), &BitsAllocated);

  RescaleSlope = this->MetaImagePtr->ElementToIntensityFunctionSlope();
  RescaleOffset = this->MetaImagePtr->ElementToIntensityFunctionOffset();

  if (this->MetaImagePtr->Modality() == svtkmetaio::MET_MOD_CT)
  {
    strcpy(Modality, "CT");
  }
  else if (this->MetaImagePtr->Modality() == svtkmetaio::MET_MOD_MR)
  {
    strcpy(Modality, "MR");
  }
  else
  {
    strcpy(Modality, "?");
  }
}

void svtkMetaImageReader::ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo)
{

  svtkImageData* data = this->AllocateOutputData(output, outInfo);

  if (!this->FileName)
  {
    svtkErrorMacro(<< "A filename was not specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("MetaImage");

  this->ComputeDataIncrements();

  if (!this->MetaImagePtr->Read(this->FileName, true, data->GetScalarPointer()))
  {
    svtkErrorMacro(<< "MetaImage cannot read data from file.");
    return;
  }

  this->MetaImagePtr->ElementByteOrderFix();
}

int svtkMetaImageReader::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{

  this->ExecuteInformation();

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataExtent, 6);
  outInfo->Set(svtkDataObject::SPACING(), this->DataSpacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), this->DataOrigin, 3);

  svtkDataObject::SetPointDataActiveScalarInfo(
    outInfo, this->DataScalarType, this->NumberOfScalarComponents);

  return 1;
}

//----------------------------------------------------------------------------
int svtkMetaImageReader::CanReadFile(const char* fname)
{

  std::string filename = fname;
  if (filename.empty())
  {
    return false;
  }

  bool extensionFound = false;
  std::string::size_type mhaPos = filename.rfind(".mha");
  if ((mhaPos != std::string::npos) && (mhaPos == filename.length() - 4))
  {
    extensionFound = true;
  }
  std::string::size_type mhdPos = filename.rfind(".mhd");
  if ((mhdPos != std::string::npos) && (mhdPos == filename.length() - 4))
  {
    extensionFound = true;
  }
  if (!extensionFound)
  {
    return false;
  }

  // Now check the file content
  svtksys::ifstream inputStream;

  inputStream.open(fname, ios::in | ios::binary);

  if (inputStream.fail())
  {
    return false;
  }

  char key[8000];

  inputStream >> key;

  if (inputStream.eof())
  {
    inputStream.close();
    return false;
  }

  if (strcmp(key, "NDims") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "ObjectType") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "TransformType") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "ID") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "ParentID") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "BinaryData") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "Comment") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "AcquisitionDate") == 0)
  {
    inputStream.close();
    return 3;
  }
  if (strcmp(key, "Modality") == 0)
  {
    inputStream.close();
    return 3;
  }

  inputStream.close();
  return false;
}

//----------------------------------------------------------------------------
int svtkMetaImageReader::GetDataByteOrder()
{
  return svtkmetaio::MET_SystemByteOrderMSB();
}

//----------------------------------------------------------------------------
void svtkMetaImageReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RescaleSlope: " << this->RescaleSlope << endl;
  os << indent << "RescaleOffset: " << this->RescaleOffset << endl;

  os << indent << "GantryAngle: " << this->GantryAngle << endl;
  os << indent << "PatientName: " << this->PatientName << endl;
  os << indent << "PatientID: " << this->PatientID << endl;
  os << indent << "Date: " << this->Date << endl;
  os << indent << "Series: " << this->Series << endl;
  os << indent << "Study: " << this->Study << endl;
  os << indent << "ImageNumber: " << this->ImageNumber << endl;
  os << indent << "Modality: " << this->Modality << endl;
  os << indent << "StudyID: " << this->StudyID << endl;
  os << indent << "StudyUID: " << this->StudyUID << endl;
  os << indent << "TransferSyntaxUID: " << this->TransferSyntaxUID << endl;

  os << indent << "BitsAllocated: " << this->BitsAllocated << endl;
  os << indent << "DistanceUnits: " << this->DistanceUnits << endl;
  os << indent << "AnatomicalOrientation: " << this->AnatomicalOrientation << endl;
}
