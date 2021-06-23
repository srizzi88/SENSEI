/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMINCImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include "svtkMINCImageReader.h"

#include "svtkObjectFactory.h"
#include <svtksys/SystemTools.hxx>

#include "svtkCharArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkShortArray.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"

#include "svtkType.h"

#include "svtkMINC.h"
#include "svtkMINCImageAttributes.h"
#include "svtk_netcdf.h"

#include <cctype>
#include <cfloat>
#include <cstdlib>
#include <map>
#include <string>

#define SVTK_MINC_MAX_DIMS 8

//--------------------------------------------------------------------------
svtkStandardNewMacro(svtkMINCImageReader);

//-------------------------------------------------------------------------
svtkMINCImageReader::svtkMINCImageReader()
{
  this->NumberOfTimeSteps = 1;
  this->TimeStep = 0;
  this->DirectionCosines = svtkMatrix4x4::New();
  this->RescaleIntercept = 0.0;
  this->RescaleSlope = 1.0;
  this->RescaleRealValues = 0;

  this->MINCImageType = 0;
  this->MINCImageTypeSigned = 1;

  this->ValidRange[0] = 0.0;
  this->ValidRange[1] = 1.0;

  this->ImageRange[0] = 0.0;
  this->ImageRange[1] = 1.0;

  this->DataRange[0] = 0.0;
  this->DataRange[1] = 1.0;

  this->ImageAttributes = svtkMINCImageAttributes::New();
  this->ImageAttributes->ValidateAttributesOff();

  this->FileNameHasChanged = 0;
}

//-------------------------------------------------------------------------
svtkMINCImageReader::~svtkMINCImageReader()
{
  if (this->DirectionCosines)
  {
    this->DirectionCosines->Delete();
    this->DirectionCosines = nullptr;
  }
  if (this->ImageAttributes)
  {
    this->ImageAttributes->Delete();
    this->ImageAttributes = nullptr;
  }
}

//-------------------------------------------------------------------------
void svtkMINCImageReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ImageAttributes: " << this->ImageAttributes << "\n";
  if (this->ImageAttributes)
  {
    this->ImageAttributes->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "DirectionCosines: " << this->DirectionCosines << "\n";
  if (this->DirectionCosines)
  {
    this->DirectionCosines->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "RescaleSlope: " << this->RescaleSlope << "\n";
  os << indent << "RescaleIntercept: " << this->RescaleIntercept << "\n";
  os << indent << "RescaleRealValues: " << (this->RescaleRealValues ? "On" : "Off") << "\n";
  os << indent << "DataRange: (" << this->DataRange[0] << ", " << this->DataRange[1] << ")\n";

  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << "\n";
  os << indent << "TimeStep: " << this->TimeStep << "\n";
}

//-------------------------------------------------------------------------
void svtkMINCImageReader::SetFileName(const char* name)
{
  // Set FileNameHasChanged even if the file name hasn't changed,
  // because it is possible that the user is re-reading a file after
  // changing it.
  if (!(name == nullptr && this->GetFileName() == nullptr))
  {
    this->FileNameHasChanged = 1;
  }

  this->Superclass::SetFileName(name);
}

//-------------------------------------------------------------------------
int svtkMINCImageReader::CanReadFile(const char* fname)
{
  // First do a very rapid check of the magic number
  FILE* fp = svtksys::SystemTools::Fopen(fname, "rb");
  if (!fp)
  {
    return 0;
  }

  char magic[4];
  size_t count = fread(magic, 4, 1, fp);
  fclose(fp);

  if (count != 1 || magic[0] != 'C' || magic[1] != 'D' || magic[2] != 'F' || magic[3] != '\001')
  {
    return 0;
  }

  // Do a more thorough check of the image:version attribute, since
  // there are lots of NetCDF files out there that aren't minc files.
  int ncid = 0;
  int status = nc_open(fname, 0, &ncid);
  if (status != NC_NOERR)
  {
    return 0;
  }

  int ndims = 0;
  int nvars = 0;
  int ngatts = 0;
  int unlimdimid = 0;
  status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
  if (status != NC_NOERR)
  {
    return 0;
  }

  int varid = 0;
  char varname[NC_MAX_NAME + 1];
  nc_type vartype = NC_INT;
  int nvardims;
  int dimids[SVTK_MINC_MAX_DIMS];
  int nvaratts = 0;
  for (varid = 0; varid < nvars && status == NC_NOERR; varid++)
  {
    status = nc_inq_var(ncid, varid, varname, &vartype, &nvardims, dimids, &nvaratts);
    if (status == NC_NOERR && strcmp(varname, MIimage) == 0)
    {
      nc_type atttype = NC_INT;
      size_t attlength = 0;
      status = nc_inq_att(ncid, varid, MIversion, &atttype, &attlength);
      if (status == NC_NOERR && atttype == NC_CHAR && attlength < 32)
      {
        char verstring[32];
        status = nc_get_att_text(ncid, varid, MIversion, verstring);
        if (status == NC_NOERR && strncmp(verstring, "MINC ", 5) == 0)
        {
          nc_close(ncid);
          return 1;
        }
      }
      break;
    }
  }

  nc_close(ncid);

  return 0;
}

//-------------------------------------------------------------------------
svtkMatrix4x4* svtkMINCImageReader::GetDirectionCosines()
{
  this->ReadMINCFileAttributes();
  return this->DirectionCosines;
}

//-------------------------------------------------------------------------
double svtkMINCImageReader::GetRescaleSlope()
{
  this->ReadMINCFileAttributes();
  this->FindRangeAndRescaleValues();
  return this->RescaleSlope;
}

//-------------------------------------------------------------------------
double svtkMINCImageReader::GetRescaleIntercept()
{
  this->ReadMINCFileAttributes();
  this->FindRangeAndRescaleValues();
  return this->RescaleIntercept;
}

//-------------------------------------------------------------------------
double* svtkMINCImageReader::GetDataRange()
{
  this->ReadMINCFileAttributes();
  this->FindRangeAndRescaleValues();
  return this->DataRange;
}

//-------------------------------------------------------------------------
int svtkMINCImageReader::GetNumberOfTimeSteps()
{
  this->ReadMINCFileAttributes();
  return this->NumberOfTimeSteps;
}

//-------------------------------------------------------------------------
svtkMINCImageAttributes* svtkMINCImageReader::GetImageAttributes()
{
  this->ReadMINCFileAttributes();
  return this->ImageAttributes;
}

//-------------------------------------------------------------------------
int svtkMINCImageReader::OpenNetCDFFile(const char* filename, int& ncid)
{
  int status = 0;

  if (filename == nullptr)
  {
    svtkErrorMacro("No filename was set");
    return 0;
  }

  status = nc_open(filename, 0, &ncid);
  if (status != NC_NOERR)
  {
    svtkErrorMacro("Could not open the MINC file:\n" << nc_strerror(status));
    return 0;
  }

  return 1;
}

//-------------------------------------------------------------------------
int svtkMINCImageReader::CloseNetCDFFile(int ncid)
{
  int status = 0;
  status = nc_close(ncid);
  if (status != NC_NOERR)
  {
    svtkErrorMacro("Could not close the MINC file:\n" << nc_strerror(status));
    return 0;
  }

  return 1;
}

//-------------------------------------------------------------------------
// this is a macro so the svtkErrorMacro will report a useful line number
#define svtkMINCImageReaderFailAndClose(ncid, status)                                               \
  {                                                                                                \
    if ((status) != NC_NOERR)                                                                      \
    {                                                                                              \
      svtkErrorMacro("There was an error with the MINC file:\n"                                     \
        << this->GetFileName() << "\n"                                                             \
        << nc_strerror(status));                                                                   \
    }                                                                                              \
    nc_close(ncid);                                                                                \
  }

//-------------------------------------------------------------------------
// Function for getting SVTK dimension index from the dimension name.
int svtkMINCImageReader::IndexFromDimensionName(const char* dimName)
{
  switch (dimName[0])
  {
    case 'x':
      return 0;
    case 'y':
      return 1;
    case 'z':
      return 2;
    default:
      if (strcmp(dimName, MIvector_dimension) == 0)
      {
        return -1;
      }
      break;
  }

  // Any unrecognized dimensions are returned as index 3
  return 3;
}

//-------------------------------------------------------------------------
int svtkMINCImageReader::ReadMINCFileAttributes()
{
  // If the filename hasn't changed since the last time the attributes
  // were read, don't read them again.
  if (!this->FileNameHasChanged)
  {
    return 1;
  }

  // Reset the MINC information for the file.
  this->MINCImageType = 0;
  this->MINCImageTypeSigned = 1;

  this->NumberOfTimeSteps = 1;
  this->DirectionCosines->Identity();

  // Orientation set tells us which direction cosines were found
  int orientationSet[3] = { 0, 0, 0 };

  this->ImageAttributes->Reset();

  // Miscellaneous NetCDF variables
  int status = 0;
  int ncid = 0;
  int dimid = 0;
  int varid = 0;
  int ndims = 0;
  int nvars = 0;
  int ngatts = 0;
  int unlimdimid = 0;

  if (this->OpenNetCDFFile(this->GetFileName(), ncid) == 0)
  {
    return 0;
  }

  // Get the basic information for the file.  The ndims are
  // ignored here, because we only want the dimensions that
  // belong to the image variable.
  status = nc_inq(ncid, &ndims, &nvars, &ngatts, &unlimdimid);
  if (status != NC_NOERR)
  {
    svtkMINCImageReaderFailAndClose(ncid, status);
    return 0;
  }
  if (ndims > SVTK_MINC_MAX_DIMS)
  {
    svtkErrorMacro("MINC file has " << ndims
                                   << ", but this reader"
                                      " only supports "
                                   << SVTK_MINC_MAX_DIMS << ".");
    return 0;
  }

  // Go through all the variables in the MINC file.  A varid of -1
  // is used to signal global attributes.
  for (varid = -1; varid < nvars; varid++)
  {
    char varname[NC_MAX_NAME + 1];
    int dimids[SVTK_MINC_MAX_DIMS];
    nc_type vartype = NC_SHORT;
    int nvardims = 0;
    int nvaratts = 0;

    if (varid == -1) // for global attributes
    {
      nvaratts = ngatts;
      varname[0] = '\0';
    }
    else
    {
      status = nc_inq_var(ncid, varid, varname, &vartype, &nvardims, dimids, &nvaratts);
      if (status != NC_NOERR)
      {
        svtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
      }
    }

    // Get all the variable attributes
    for (int j = 0; j < nvaratts; j++)
    {
      char attname[NC_MAX_NAME + 1];
      nc_type atttype;
      size_t attlength = 0;

      status = nc_inq_attname(ncid, varid, j, attname);
      if (status != NC_NOERR)
      {
        svtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
      }
      status = nc_inq_att(ncid, varid, attname, &atttype, &attlength);
      if (status != NC_NOERR)
      {
        svtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
      }

      // Get the attribute values as a svtkDataArray.
      svtkDataArray* dataArray = nullptr;
      switch (atttype)
      {
        case NC_BYTE:
        {
          // NetCDF leaves it up to us to decide whether NC_BYTE
          // should be signed.
          svtkUnsignedCharArray* ucharArray = svtkUnsignedCharArray::New();
          ucharArray->SetNumberOfValues(static_cast<svtkIdType>(attlength));
          nc_get_att_uchar(ncid, varid, attname, ucharArray->GetPointer(0));
          dataArray = ucharArray;
        }
        break;
        case NC_CHAR:
        {
          // The NC_CHAR type is for text.
          svtkCharArray* charArray = svtkCharArray::New();
          // The netcdf standard doesn't enforce null-termination
          // of string attributes, so we add a null here.
          charArray->Resize(static_cast<svtkIdType>(attlength + 1));
          char* dest = charArray->WritePointer(0, static_cast<svtkIdType>(attlength));
          nc_get_att_text(ncid, varid, attname, dest);
          dest[attlength] = '\0';
          dataArray = charArray;
        }
        break;
        case NC_SHORT:
        {
          svtkShortArray* shortArray = svtkShortArray::New();
          shortArray->SetNumberOfValues(static_cast<svtkIdType>(attlength));
          nc_get_att_short(ncid, varid, attname, shortArray->GetPointer(0));
          dataArray = shortArray;
        }
        break;
        case NC_INT:
        {
          svtkIntArray* intArray = svtkIntArray::New();
          intArray->SetNumberOfValues(static_cast<svtkIdType>(attlength));
          nc_get_att_int(ncid, varid, attname, intArray->GetPointer(0));
          dataArray = intArray;
        }
        break;
        case NC_FLOAT:
        {
          svtkFloatArray* floatArray = svtkFloatArray::New();
          floatArray->SetNumberOfValues(static_cast<svtkIdType>(attlength));
          nc_get_att_float(ncid, varid, attname, floatArray->GetPointer(0));
          dataArray = floatArray;
        }
        break;
        case NC_DOUBLE:
        {
          svtkDoubleArray* doubleArray = svtkDoubleArray::New();
          doubleArray->SetNumberOfValues(static_cast<svtkIdType>(attlength));
          nc_get_att_double(ncid, varid, attname, doubleArray->GetPointer(0));
          dataArray = doubleArray;
        }
        break;
        default:
          break;
      }
      if (dataArray)
      {
        this->ImageAttributes->SetAttributeValueAsArray(varname, attname, dataArray);
        dataArray->Delete();
      }
    }

    // Special treatment of image variable.
    if (strcmp(varname, MIimage) == 0)
    {
      // Set the type of the data.
      this->MINCImageType = vartype;

      // Find the sign of the data, default to "signed"
      int signedType = 1;
      // Except for bytes, where default is "unsigned"
      if (vartype == NC_BYTE)
      {
        signedType = 0;
      }
      const char* signtype = this->ImageAttributes->GetAttributeValueAsString(MIimage, MIsigntype);
      if (signtype)
      {
        if (strcmp(signtype, MI_UNSIGNED) == 0)
        {
          signedType = 0;
        }
      }
      this->MINCImageTypeSigned = signedType;

      for (int i = 0; i < nvardims; i++)
      {
        char dimname[NC_MAX_NAME + 1];
        size_t dimlength = 0;

        dimid = dimids[i];

        status = nc_inq_dim(ncid, dimid, dimname, &dimlength);
        if (status != NC_NOERR)
        {
          svtkMINCImageReaderFailAndClose(ncid, status);
          return 0;
        }

        this->ImageAttributes->AddDimension(dimname, static_cast<svtkIdType>(dimlength));

        int dimIndex = this->IndexFromDimensionName(dimname);

        if (dimIndex >= 0 && dimIndex < 3)
        {
          // Set the orientation matrix from the direction_cosines
          svtkDoubleArray* doubleArray = svtkArrayDownCast<svtkDoubleArray>(
            this->ImageAttributes->GetAttributeValueAsArray(dimname, MIdirection_cosines));
          if (doubleArray && doubleArray->GetNumberOfTuples() == 3)
          {
            double* dimDirCos = doubleArray->GetPointer(0);
            this->DirectionCosines->SetElement(0, dimIndex, dimDirCos[0]);
            this->DirectionCosines->SetElement(1, dimIndex, dimDirCos[1]);
            this->DirectionCosines->SetElement(2, dimIndex, dimDirCos[2]);
            orientationSet[dimIndex] = 1;
          }
        }
        else if (strcmp(dimname, MIvector_dimension) != 0)
        {
          // Set the NumberOfTimeSteps to the product of all dimensions
          // that are neither spatial dimensions nor vector dimensions.
          this->NumberOfTimeSteps *= static_cast<int>(dimlength);
        }
      }
    }
    else if (strcmp(varname, MIimagemin) == 0 || strcmp(varname, MIimagemax) == 0)
    {
      // Read the image-min and image-max.
      this->ImageAttributes->SetNumberOfImageMinMaxDimensions(nvardims);

      svtkDoubleArray* doubleArray = svtkDoubleArray::New();
      if (strcmp(varname, MIimagemin) == 0)
      {
        this->ImageAttributes->SetImageMin(doubleArray);
      }
      else
      {
        this->ImageAttributes->SetImageMax(doubleArray);
      }
      doubleArray->Delete();

      svtkIdType size = 1;
      size_t start[SVTK_MINC_MAX_DIMS];
      size_t count[SVTK_MINC_MAX_DIMS];

      for (int i = 0; i < nvardims; i++)
      {
        char dimname[NC_MAX_NAME + 1];
        size_t dimlength = 0;

        dimid = dimids[i];

        status = nc_inq_dim(ncid, dimid, dimname, &dimlength);
        if (status != NC_NOERR)
        {
          svtkMINCImageReaderFailAndClose(ncid, status);
          return 0;
        }

        start[i] = 0;
        count[i] = dimlength;

        size *= static_cast<svtkIdType>(dimlength);
      }

      doubleArray->SetNumberOfValues(size);
      status = nc_get_vara_double(ncid, varid, start, count, doubleArray->GetPointer(0));
      if (status != NC_NOERR)
      {
        svtkMINCImageReaderFailAndClose(ncid, status);
        return 0;
      }
    }
  }

  // Check to see if only 2 spatial dimensions were included,
  // since we'll have to make up the third dircos if that is the case
  int numDirCos = 0;
  int notSetIndex = 0;
  for (int dcount = 0; dcount < 3; dcount++)
  {
    if (orientationSet[dcount])
    {
      numDirCos++;
    }
    else
    {
      notSetIndex = dcount;
    }
  }
  // If only two were set, use cross product to get the third
  if (numDirCos == 2)
  {
    int idx1 = (notSetIndex + 1) % 3;
    int idx2 = (notSetIndex + 2) % 3;
    double v1[4];
    double v2[4];
    double v3[3];
    for (int tmpi = 0; tmpi < 4; tmpi++)
    {
      v1[tmpi] = v2[tmpi] = 0.0;
    }
    v1[idx1] = 1.0;
    v2[idx2] = 1.0;
    this->DirectionCosines->MultiplyPoint(v1, v1);
    this->DirectionCosines->MultiplyPoint(v2, v2);
    svtkMath::Cross(v1, v2, v3);
    this->DirectionCosines->SetElement(0, notSetIndex, v3[0]);
    this->DirectionCosines->SetElement(1, notSetIndex, v3[1]);
    this->DirectionCosines->SetElement(2, notSetIndex, v3[2]);
  }

  // Get the data type
  int dataType = this->ConvertMINCTypeToSVTKType(this->MINCImageType, this->MINCImageTypeSigned);
  this->ImageAttributes->SetDataType(dataType);

  // Get the name from the file name by removing the path and
  // the extension.
  const char* fileName = this->FileName;
  char name[4096];
  name[0] = '\0';
  int startChar = 0;
  int endChar = static_cast<int>(strlen(fileName));

  for (startChar = endChar - 1; startChar > 0; startChar--)
  {
    if (fileName[startChar] == '.')
    {
      endChar = startChar;
    }
    if (fileName[startChar - 1] == '/'
#ifdef _WIN32
      || fileName[startChar - 1] == '\\'
#endif
    )
    {
      break;
    }
  }
  if (endChar - startChar > 127)
  {
    endChar = startChar + 128;
  }
  if (endChar > startChar)
  {
    strncpy(name, &fileName[startChar], endChar - startChar);
    name[endChar - startChar] = '\0';
  }

  this->ImageAttributes->SetName(name);

  // We're done reading the attributes, so close the file.
  if (this->CloseNetCDFFile(ncid) == 0)
  {
    return 0;
  }

  // Get the ValidRange and ImageRange.
  this->ImageAttributes->FindValidRange(this->ValidRange);
  this->ImageAttributes->FindImageRange(this->ImageRange);

  // Don't have to do this again until the file name changes.
  this->FileNameHasChanged = 0;

  return 1;
}

//-------------------------------------------------------------------------
int svtkMINCImageReader::ConvertMINCTypeToSVTKType(int minctype, int mincsigned)
{
  int dataType = 0;

  // Get the svtk type of the data.
  switch (minctype)
  {
    case NC_BYTE:
      dataType = SVTK_UNSIGNED_CHAR;
      if (mincsigned)
      {
        dataType = SVTK_SIGNED_CHAR;
      }
      break;
    case NC_SHORT:
      dataType = SVTK_UNSIGNED_SHORT;
      if (mincsigned)
      {
        dataType = SVTK_SHORT;
      }
      break;
    case NC_INT:
      dataType = SVTK_UNSIGNED_INT;
      if (mincsigned)
      {
        dataType = SVTK_INT;
      }
      break;
    case NC_FLOAT:
      dataType = SVTK_FLOAT;
      break;
    case NC_DOUBLE:
      dataType = SVTK_DOUBLE;
      break;
    default:
      break;
  }

  return dataType;
}

//-------------------------------------------------------------------------
void svtkMINCImageReader::FindRangeAndRescaleValues()
{
  // Set DataRange and Rescale values according to whether
  // RescaleRealValues is set
  if (this->RescaleRealValues)
  {
    // Set DataRange to ImageRange
    this->DataRange[0] = this->ImageRange[0];
    this->DataRange[1] = this->ImageRange[1];

    // The output data values will be the real data values.
    this->RescaleSlope = 1.0;
    this->RescaleIntercept = 0.0;
  }
  else
  {
    // Set DataRange to ValidRange
    this->DataRange[0] = this->ValidRange[0];
    this->DataRange[1] = this->ValidRange[1];

    // Set rescale parameters
    this->RescaleSlope =
      ((this->ImageRange[1] - this->ImageRange[0]) / (this->ValidRange[1] - this->ValidRange[0]));

    this->RescaleIntercept = (this->ImageRange[0] - this->RescaleSlope * this->ValidRange[0]);
  }
}

//-------------------------------------------------------------------------
void svtkMINCImageReader::ExecuteInformation()
{
  // Read the MINC attributes from the file.
  if (this->ReadMINCFileAttributes() == 0)
  {
    return;
  }

  // Set the SVTK information from the MINC information.
  int dataExtent[6] = { 0, 0, 0, 0, 0, 0 };

  double dataSpacing[3] = { 1.0, 1.0, 1.0 };

  double dataOrigin[3] = { 0.0, 0.0, 0.0 };

  int numberOfComponents = 1;

  int fileType = this->ConvertMINCTypeToSVTKType(this->MINCImageType, this->MINCImageTypeSigned);

  if (fileType == 0)
  {
    svtkErrorMacro("Couldn't convert NetCDF data type "
      << this->MINCImageType << (this->MINCImageTypeSigned ? " signed" : " unsigned")
      << " to a SVTK data type.");
    return;
  }

  // Compute the DataRange, RescaleSlope, and RescaleIntercept
  this->FindRangeAndRescaleValues();

  // If we are rescaling the data, find the appropriate
  // output data type.  The data is only rescaled if the
  // data has an ImageMin and ImageMax.
  int dataType = fileType;
  if (this->RescaleRealValues && this->ImageAttributes->GetImageMin() &&
    this->ImageAttributes->GetImageMax())
  {
    switch (fileType)
    {
      case SVTK_SIGNED_CHAR:
      case SVTK_UNSIGNED_CHAR:
      case SVTK_CHAR:
      case SVTK_SHORT:
      case SVTK_UNSIGNED_SHORT:
        dataType = SVTK_FLOAT;
        break;
      case SVTK_INT:
      case SVTK_UNSIGNED_INT:
        dataType = SVTK_DOUBLE;
        break;
      default:
        dataType = fileType;
        break;
    }
  }

  // Go through the image dimensions to discover data information.
  svtkStringArray* dimensionNames = this->ImageAttributes->GetDimensionNames();
  svtkIdTypeArray* dimensionLengths = this->ImageAttributes->GetDimensionLengths();

  unsigned int numberOfDimensions = dimensionNames->GetNumberOfValues();
  for (unsigned int i = 0; i < numberOfDimensions; i++)
  {
    const char* dimName = dimensionNames->GetValue(i);
    svtkIdType dimLength = dimensionLengths->GetValue(i);

    // Set the SVTK dimension index.
    int dimIndex = this->IndexFromDimensionName(dimName);

    // Do special things with the spatial dimensions.
    if (dimIndex >= 0 && dimIndex < 3)
    {
      // Set the spacing from the 'step' attribute.
      double step = this->ImageAttributes->GetAttributeValueAsDouble(dimName, MIstep);
      if (step)
      {
        dataSpacing[dimIndex] = step;
      }

      // Set the origin from the 'start' attribute.
      double start = this->ImageAttributes->GetAttributeValueAsDouble(dimName, MIstart);
      if (start)
      {
        dataOrigin[dimIndex] = start;
      }

      // Set the extent from the dimension length.
      dataExtent[2 * dimIndex + 1] = static_cast<int>(dimLength - 1);
    }

    // Check for vector_dimension.
    else if (strcmp(dimName, MIvector_dimension) == 0)
    {
      numberOfComponents = dimLength;
    }
  }

  this->SetDataExtent(dataExtent);
  this->SetDataSpacing(dataSpacing[0], dataSpacing[1], dataSpacing[2]);
  this->SetDataOrigin(dataOrigin[0], dataOrigin[1], dataOrigin[2]);
  this->SetDataScalarType(dataType);
  this->SetNumberOfScalarComponents(numberOfComponents);
}

//-------------------------------------------------------------------------
// Data conversion functions.  The rounding is done using the same
// method as in the MINC libraries.
#define svtkMINCImageReaderConvertMacro(F, T, MIN, MAX)                                             \
  inline void svtkMINCImageReaderConvert(const F& inVal, T& outVal)                                 \
  {                                                                                                \
    double val = inVal;                                                                            \
    if (val >= static_cast<double>(MIN))                                                           \
    {                                                                                              \
      if (val <= static_cast<double>(MAX))                                                         \
      {                                                                                            \
        outVal = static_cast<T>((val < 0) ? (val - 0.5) : (val + 0.5));                            \
        return;                                                                                    \
      }                                                                                            \
      outVal = static_cast<T>(MAX);                                                                \
      return;                                                                                      \
    }                                                                                              \
    outVal = static_cast<T>(MIN);                                                                  \
  }

#define svtkMINCImageReaderConvertMacroFloat(F, T)                                                  \
  inline void svtkMINCImageReaderConvert(const F& inVal, T& outVal)                                 \
  {                                                                                                \
    outVal = static_cast<T>(inVal);                                                                \
  }

svtkMINCImageReaderConvertMacro(double, signed char, SVTK_SIGNED_CHAR_MIN, SVTK_SIGNED_CHAR_MAX);
svtkMINCImageReaderConvertMacro(double, unsigned char, 0, SVTK_UNSIGNED_CHAR_MAX);
svtkMINCImageReaderConvertMacro(double, short, SVTK_SHORT_MIN, SVTK_SHORT_MAX);
svtkMINCImageReaderConvertMacro(double, unsigned short, 0, SVTK_UNSIGNED_SHORT_MAX);
svtkMINCImageReaderConvertMacro(double, int, SVTK_INT_MIN, SVTK_INT_MAX);
svtkMINCImageReaderConvertMacro(double, unsigned int, 0, SVTK_UNSIGNED_INT_MAX);
svtkMINCImageReaderConvertMacroFloat(double, float);
svtkMINCImageReaderConvertMacroFloat(double, double);

//-------------------------------------------------------------------------
// Overloaded functions for reading various data types.

// Handle most with a macro.
#define svtkMINCImageReaderReadChunkMacro(ncFunction, T)                                            \
  inline int svtkMINCImageReaderReadChunk(                                                          \
    int ncid, int varid, size_t* start, size_t* count, T* buffer)                                  \
  {                                                                                                \
    return ncFunction(ncid, varid, start, count, buffer);                                          \
  }

#define svtkMINCImageReaderReadChunkMacro2(ncFunction, T1, T2)                                      \
  inline int svtkMINCImageReaderReadChunk(                                                          \
    int ncid, int varid, size_t* start, size_t* count, T1* buffer)                                 \
  {                                                                                                \
    return ncFunction(ncid, varid, start, count, (T2*)buffer);                                     \
  }

svtkMINCImageReaderReadChunkMacro(nc_get_vara_schar, signed char);
svtkMINCImageReaderReadChunkMacro(nc_get_vara_uchar, unsigned char);
svtkMINCImageReaderReadChunkMacro(nc_get_vara_short, short);
svtkMINCImageReaderReadChunkMacro2(nc_get_vara_short, unsigned short, short);
svtkMINCImageReaderReadChunkMacro(nc_get_vara_int, int);
svtkMINCImageReaderReadChunkMacro2(nc_get_vara_int, unsigned int, int);
svtkMINCImageReaderReadChunkMacro(nc_get_vara_float, float);
svtkMINCImageReaderReadChunkMacro(nc_get_vara_double, double);

//-------------------------------------------------------------------------
template <class T1, class T2>
void svtkMINCImageReaderExecuteChunk(T1* outPtr, T2* buffer, double slope, double intercept,
  int ncid, int varid, int ndims, size_t* start, size_t* count, svtkIdType* permutedInc)
{
  // Read the chunk of data from the MINC file.
  svtkMINCImageReaderReadChunk(ncid, varid, start, count, buffer);

  // Create space to save values during the copy loop.
  T1* saveOutPtr[SVTK_MINC_MAX_DIMS];
  size_t index[SVTK_MINC_MAX_DIMS];
  int idim = 0;
  for (idim = 0; idim < ndims; idim++)
  {
    index[idim] = 0;
    saveOutPtr[idim] = outPtr;
  }

  // See if there is a range of dimensions over which the
  // the MINC data and SVTK data will be contiguous.  The
  // lastdim is the dimension after which all dimensions
  // are contiguous between the MINC file and the output.
  int lastdim = ndims - 1;
  int ncontiguous = 1;
  svtkIdType dimprod = 1;
  for (idim = ndims; idim > 0;)
  {
    idim--;

    lastdim = idim;
    ncontiguous = dimprod;

    if (dimprod != permutedInc[idim])
    {
      break;
    }

    dimprod *= static_cast<svtkIdType>(count[idim]);
  }

  // Save the count and permuted increment of this dimension.
  size_t lastdimcount = count[lastdim];
  size_t lastdimindex = 0;
  svtkIdType lastdimInc = permutedInc[lastdim];
  T1* lastdimOutPtr = saveOutPtr[lastdim];

  // Loop over all contiguous sections of the image.
  for (;;)
  {
    // Loop through one contiguous section
    svtkIdType k = ncontiguous;
    do
    {
      // Use special function for type conversion.
      svtkMINCImageReaderConvert((*buffer++) * slope + intercept, *outPtr++);
    } while (--k);

    lastdimindex++;
    lastdimOutPtr += lastdimInc;
    outPtr = lastdimOutPtr;

    // Continue until done lastdim.
    if (lastdimindex < lastdimcount)
    {
      continue;
    }

    // Handle all dimensions that are lower than lastdim.  Go down
    // the dimensions one at a time until we find one for which
    // the index is still less than the count.
    idim = lastdim;
    do
    {
      // We're done if the lowest dim's index has reached its count.
      if (idim == 0)
      {
        return;
      }
      // Reset the index to zero if it previously reached its count.
      index[idim--] = 0;

      // Now increase the index for the next lower dimension;
      index[idim]++;
      saveOutPtr[idim] += permutedInc[idim];

      // Continue the loop if this dim's index has reached its count.
    } while (index[idim] >= count[idim]);

    // Increment back up to the lastdim, resetting the pointers.
    outPtr = saveOutPtr[idim];
    do
    {
      saveOutPtr[++idim] = outPtr;
    } while (idim < lastdim);

    lastdimOutPtr = outPtr;
    lastdimindex = 0;
  }
}

//-------------------------------------------------------------------------
// Our own template that only includes MINC data types.

#define svtkMINCImageReaderTemplateMacro(call)                                                      \
  case SVTK_DOUBLE:                                                                                 \
  {                                                                                                \
    typedef double SVTK_TT;                                                                         \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case SVTK_FLOAT:                                                                                  \
  {                                                                                                \
    typedef float SVTK_TT;                                                                          \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case SVTK_INT:                                                                                    \
  {                                                                                                \
    typedef int SVTK_TT;                                                                            \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case SVTK_UNSIGNED_INT:                                                                           \
  {                                                                                                \
    typedef unsigned int SVTK_TT;                                                                   \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case SVTK_SHORT:                                                                                  \
  {                                                                                                \
    typedef short SVTK_TT;                                                                          \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case SVTK_UNSIGNED_SHORT:                                                                         \
  {                                                                                                \
    typedef unsigned short SVTK_TT;                                                                 \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case SVTK_SIGNED_CHAR:                                                                            \
  {                                                                                                \
    typedef signed char SVTK_TT;                                                                    \
    call;                                                                                          \
  }                                                                                                \
  break;                                                                                           \
  case SVTK_UNSIGNED_CHAR:                                                                          \
  {                                                                                                \
    typedef unsigned char SVTK_TT;                                                                  \
    call;                                                                                          \
  }                                                                                                \
  break

//-------------------------------------------------------------------------
void svtkMINCImageReader::ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo)
{
  svtkImageData* data = this->AllocateOutputData(output, outInfo);
  int scalarType = data->GetScalarType();
  int scalarSize = data->GetScalarSize();
  int numComponents = data->GetNumberOfScalarComponents();
  int outExt[6];
  this->GetOutputInformation(0)->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);
  svtkIdType outInc[3];
  data->GetIncrements(outInc);
  int outSize[3];
  data->GetDimensions(outSize);

  void* outPtr = data->GetScalarPointerForExtent(outExt);

  int timeStep = this->TimeStep;
  if (timeStep < 0 || timeStep >= this->NumberOfTimeSteps)
  {
    svtkWarningMacro("TimeStep is set to " << this->TimeStep << " but there are only "
                                          << this->NumberOfTimeSteps << " time steps.");
    timeStep = timeStep % this->NumberOfTimeSteps;
  }

  int status = 0;
  int ncid = 0;
  int varid = 0;

  if (this->OpenNetCDFFile(this->GetFileName(), ncid) == 0)
  {
    return;
  }

  // Get the image variable.
  status = nc_inq_varid(ncid, MIimage, &varid);
  if (status != NC_NOERR)
  {
    svtkMINCImageReaderFailAndClose(ncid, status);
    return;
  }

  // Get the dimensions.
  svtkStringArray* dimensionNames = this->ImageAttributes->GetDimensionNames();
  svtkIdTypeArray* dimensionLengths = this->ImageAttributes->GetDimensionLengths();
  int ndims = dimensionNames->GetNumberOfValues();
  int idim = 0;
  int nminmaxdims = this->ImageAttributes->GetNumberOfImageMinMaxDimensions();
  svtkIdType minmaxSize = 0;
  if (this->ImageAttributes->GetImageMin())
  {
    minmaxSize = this->ImageAttributes->GetImageMin()->GetNumberOfTuples();
  }

  // The default dimensionality of the chunks that are used.
  int nchunkdims = ndims - nminmaxdims;

  // All of these values will be changed in the following loop
  svtkIdType nchunks = 1;
  svtkIdType numTimeSteps = 1;
  svtkIdType chunkSize = 1;
  int hitChunkSizeLimit = 0;
  int nchunkdimsIsSet = 0;

  // These arrays will be filled in by the following loop
  svtkIdType permutedInc[SVTK_MINC_MAX_DIMS];
  size_t start[SVTK_MINC_MAX_DIMS];
  size_t count[SVTK_MINC_MAX_DIMS];
  size_t length[SVTK_MINC_MAX_DIMS];

  // Loop over the dimensions starting with the fastest-varying.
  for (idim = ndims; idim > 0;)
  {
    idim--;

    const char* dimName = dimensionNames->GetValue(idim);
    svtkIdType dimLength = dimensionLengths->GetValue(idim);
    length[idim] = dimLength;

    // Find the SVTK dimension index.
    int dimIndex = this->IndexFromDimensionName(dimName);

    if (dimIndex >= 0 && dimIndex < 3)
    {
      // Set start and count according to the update extent.
      start[idim] = outExt[2 * dimIndex];
      count[idim] = outExt[2 * dimIndex + 1] - outExt[2 * dimIndex] + 1;
      permutedInc[idim] = outInc[dimIndex];
    }
    else if (strcmp(dimName, MIvector_dimension) == 0)
    {
      // Vector dimension size is also stored in numComponents.
      start[idim] = 0;
      count[idim] = numComponents;
      permutedInc[idim] = 1;
    }
    else
    {
      // Use TimeStep to compute the index into the remaining dimensions.
      start[idim] = (timeStep / numTimeSteps) % dimLength;
      count[idim] = 1;
      numTimeSteps *= dimLength;
      permutedInc[idim] = 0;
    }

    // For scalar minmax, use chunk sizes of 65536 or less,
    // unless this would force the chunk size to be 1
    if (nminmaxdims == 0 && chunkSize != 1 && chunkSize * count[idim] > 65536)
    {
      hitChunkSizeLimit = 1;
    }

    // If idim is one of the image-min/image-max dimensions, or if
    // we have reached the maximum chunk size, then increase the
    // number of chunks instead of increasing the chunk size
    if (idim < nminmaxdims || hitChunkSizeLimit)
    {
      // Number of chunks is product of dimensions in minmax.
      nchunks *= static_cast<svtkIdType>(count[idim]);

      // Only set nchunkdims once
      if (nchunkdimsIsSet == 0)
      {
        nchunkdims = ndims - idim - 1;
        nchunkdimsIsSet = 1;
      }
    }
    else
    {
      chunkSize *= static_cast<svtkIdType>(count[idim]);
    }
  }

  // Create a buffer for intermediate results.
  int fileType = this->ImageAttributes->GetDataType();
  void* buffer = nullptr;
  switch (fileType)
  {
    svtkMINCImageReaderTemplateMacro(buffer = (void*)(new SVTK_TT[chunkSize]));
  }

  // Initialize the min and max to the global min max.
  double* minPtr = &this->ImageRange[0];
  double* maxPtr = &this->ImageRange[1];

  // If min and max arrays are not empty, use them instead.
  if (minmaxSize > 0)
  {
    minPtr = this->ImageAttributes->GetImageMin()->GetPointer(0);
    maxPtr = this->ImageAttributes->GetImageMax()->GetPointer(0);
  }

  // Initialize the start and count to use for each chunk.
  size_t start2[SVTK_MINC_MAX_DIMS];
  size_t count2[SVTK_MINC_MAX_DIMS];
  for (idim = 0; idim < ndims; idim++)
  {
    start2[idim] = start[idim];
    count2[idim] = count[idim];
  }

  // Go through all the chunks
  for (svtkIdType ichunk = 0; ichunk < nchunks; ichunk++)
  {
    // Find the start and count to use for each chunk.
    svtkIdType minmaxIdx = 0;
    svtkIdType minmaxInc = 1;
    svtkIdType chunkProd = 1;
    svtkIdType chunkOffset = 0;
    for (idim = ndims - nchunkdims; idim > 0;)
    {
      idim--;
      start2[idim] = start[idim] + (ichunk / chunkProd) % count[idim];
      count2[idim] = 1;
      if (idim < nminmaxdims)
      {
        minmaxIdx += static_cast<svtkIdType>(start2[idim] * minmaxInc);
        minmaxInc *= static_cast<svtkIdType>(length[idim]);
      }
      chunkOffset += static_cast<svtkIdType>((start2[idim] - start[idim]) * permutedInc[idim]);
      chunkProd *= static_cast<svtkIdType>(count[idim]);
    }

    // Get the min and max values to apply to this chunk
    double chunkRange[2];
    if (fileType == SVTK_FLOAT || fileType == SVTK_DOUBLE)
    {
      // minc files that are float or double use global scaling
      chunkRange[0] = this->ImageRange[0];
      chunkRange[1] = this->ImageRange[1];
    }
    else
    {
      // minc files of other types use slice-by-slice scaling
      chunkRange[0] = minPtr[minmaxIdx];
      chunkRange[1] = maxPtr[minmaxIdx];
    }

    // Use the range to calculate a linear transformation
    // to apply to the data values of this chunk.
    double slope = ((chunkRange[1] - chunkRange[0]) /
      ((this->ValidRange[1] - this->ValidRange[0]) * this->RescaleSlope));
    double intercept =
      ((chunkRange[0] - this->RescaleIntercept) / this->RescaleSlope) - slope * this->ValidRange[0];

    // set the output pointer to use for this chunk
    void* outPtr1 = (void*)(((char*)outPtr) + chunkOffset * scalarSize);

    // Read in the chunks and permute them.
    if (scalarType == fileType)
    {
      switch (scalarType)
      {
        svtkMINCImageReaderTemplateMacro(svtkMINCImageReaderExecuteChunk((SVTK_TT*)outPtr1,
          (SVTK_TT*)buffer, slope, intercept, ncid, varid, ndims, start2, count2, permutedInc));
      }
    }
    else if (scalarType == SVTK_FLOAT)
    {
      switch (fileType)
      {
        svtkMINCImageReaderTemplateMacro(svtkMINCImageReaderExecuteChunk((float*)outPtr1,
          (SVTK_TT*)buffer, slope, intercept, ncid, varid, ndims, start2, count2, permutedInc));
      }
    }
    else if (scalarType == SVTK_DOUBLE)
    {
      switch (fileType)
      {
        svtkMINCImageReaderTemplateMacro(svtkMINCImageReaderExecuteChunk((double*)outPtr1,
          (SVTK_TT*)buffer, slope, intercept, ncid, varid, ndims, start2, count2, permutedInc));
      }
    }
  }

  switch (fileType)
  {
    svtkMINCImageReaderTemplateMacro(delete[]((SVTK_TT*)buffer));
  }

  this->CloseNetCDFFile(ncid);
}
