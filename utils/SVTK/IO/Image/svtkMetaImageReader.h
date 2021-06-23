/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMetaImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMetaImageReader
 * @brief   read binary UNC meta image data
 *
 * One of the formats for which a reader is already available in the toolkit is
 * the MetaImage file format. This is a fairly simple yet powerful format
 * consisting of a text header and a binary data section. The following
 * instructions describe how you can write a MetaImage header for the data that
 * you download from the BrainWeb page.
 *
 * The minimal structure of the MetaImage header is the following:
 *
 *    NDims = 3
 *    DimSize = 181 217 181
 *    ElementType = MET_UCHAR
 *    ElementSpacing = 1.0 1.0 1.0
 *    ElementByteOrderMSB = False
 *    ElementDataFile = brainweb1.raw
 *
 *    * NDims indicate that this is a 3D image. ITK can handle images of
 *      arbitrary dimension.
 *    * DimSize indicates the size of the volume in pixels along each
 *      direction.
 *    * ElementType indicate the primitive type used for pixels. In this case
 *      is "unsigned char", implying that the data is digitized in 8 bits /
 *      pixel.
 *    * ElementSpacing indicates the physical separation between the center of
 *      one pixel and the center of the next pixel along each direction in space.
 *      The units used are millimeters.
 *    * ElementByteOrderMSB indicates is the data is encoded in little or big
 *      endian order. You might want to play with this value when moving data
 *      between different computer platforms.
 *    * ElementDataFile is the name of the file containing the raw binary data
 *      of the image. This file must be in the same directory as the header.
 *
 * MetaImage headers are expected to have extension: ".mha" or ".mhd"
 *
 * Once you write this header text file, it should be possible to read the
 * image into your ITK based application using the itk::FileIOToImageFilter
 * class.
 *
 *
 *
 */

#ifndef svtkMetaImageReader_h
#define svtkMetaImageReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader2.h"

namespace svtkmetaio
{
class MetaImage;
} // forward declaration

class SVTKIOIMAGE_EXPORT svtkMetaImageReader : public svtkImageReader2
{
public:
  svtkTypeMacro(svtkMetaImageReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with FlipNormals turned off and Normals set to true.
   */
  static svtkMetaImageReader* New();

  const char* GetFileExtensions() override { return ".mhd .mha"; }

  const char* GetDescriptiveName() override { return "MetaIO Library: MetaImage"; }

  // These duplicate functions in svtkImageReader2, svtkMedicalImageReader.
  double* GetPixelSpacing() { return this->GetDataSpacing(); }
  int GetWidth() { return (this->GetDataExtent()[1] - this->GetDataExtent()[0] + 1); }
  int GetHeight() { return (this->GetDataExtent()[3] - this->GetDataExtent()[2] + 1); }
  double* GetImagePositionPatient() { return this->GetDataOrigin(); }
  int GetNumberOfComponents() { return this->GetNumberOfScalarComponents(); }
  int GetPixelRepresentation() { return this->GetDataScalarType(); }
  int GetDataByteOrder(void) override;

  svtkGetMacro(RescaleSlope, double);
  svtkGetMacro(RescaleOffset, double);
  svtkGetMacro(BitsAllocated, int);
  svtkGetStringMacro(DistanceUnits);
  svtkGetStringMacro(AnatomicalOrientation);
  svtkGetMacro(GantryAngle, double);
  svtkGetStringMacro(PatientName);
  svtkGetStringMacro(PatientID);
  svtkGetStringMacro(Date);
  svtkGetStringMacro(Series);
  svtkGetStringMacro(ImageNumber);
  svtkGetStringMacro(Modality);
  svtkGetStringMacro(StudyID);
  svtkGetStringMacro(StudyUID);
  svtkGetStringMacro(TransferSyntaxUID);

  /**
   * Test whether the file with the given name can be read by this
   * reader.
   */
  int CanReadFile(const char* name) override;

protected:
  svtkMetaImageReader();
  ~svtkMetaImageReader() override;

  // These functions make no sense for this (or most) file readers
  // and should be hidden from the user...but then the getsettest fails.
  /*virtual void SetFilePrefix(const char * arg)
    { svtkImageReader2::SetFilePrefix(arg); }
  virtual void SetFilePattern(const char * arg)
    { svtkImageReader2::SetFilePattern(arg); }
  virtual void SetDataScalarType(int type)
    { svtkImageReader2::SetDataScalarType(type); }
  virtual void SetDataScalarTypeToFloat()
    { this->SetDataScalarType(SVTK_FLOAT); }
  virtual void SetDataScalarTypeToDouble()
    { this->SetDataScalarType(SVTK_DOUBLE); }
  virtual void SetDataScalarTypeToInt()
    { this->SetDataScalarType(SVTK_INT); }
  virtual void SetDataScalarTypeToShort()
    { this->SetDataScalarType(SVTK_SHORT); }
  virtual void SetDataScalarTypeToUnsignedShort()
    {this->SetDataScalarType(SVTK_UNSIGNED_SHORT);}
  virtual void SetDataScalarTypeToUnsignedChar()
    {this->SetDataScalarType(SVTK_UNSIGNED_CHAR);}
  svtkSetMacro(NumberOfScalarComponents, int);
  svtkSetVector6Macro(DataExtent, int);
  svtkSetMacro(FileDimensionality, int);
  svtkSetVector3Macro(DataSpacing, double);
  svtkSetVector3Macro(DataOrigin, double);
  svtkSetMacro(HeaderSize, unsigned long);
  unsigned long GetHeaderSize(unsigned long)
    { return 0; }
  virtual void SetDataByteOrderToBigEndian()
    { this->SetDataByteOrderToBigEndian(); }
  virtual void SetDataByteOrderToLittleEndian()
    { this->SetDataByteOrderToBigEndian(); }
  virtual void SetDataByteOrder(int order)
    { this->SetDataByteOrder(order); }
  svtkSetMacro(FileNameSliceOffset,int);
  svtkSetMacro(FileNameSliceSpacing,int);
  svtkSetMacro(SwapBytes, int);
  virtual int OpenFile()
    { return svtkImageReader2::OpenFile(); }
  virtual void SeekFile(int i, int j, int k)
    { svtkImageReader2::SeekFile(i, j, k); }
  svtkSetMacro(FileLowerLeft, int);
  virtual void ComputeInternalFileName(int slice)
    { svtkImageReader2::ComputeInternalFileName(slice); }
  svtkGetStringMacro(InternalFileName);
  const char * GetDataByteOrderAsString(void)
    { return svtkImageReader2::GetDataByteOrderAsString(); }
  unsigned long GetHeaderSize(void)
    { return svtkImageReader2::GetHeaderSize(); }*/

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkMetaImageReader(const svtkMetaImageReader&) = delete;
  void operator=(const svtkMetaImageReader&) = delete;

  svtkmetaio::MetaImage* MetaImagePtr;

  double GantryAngle;
  char PatientName[255];
  char PatientID[255];
  char Date[255];
  char Series[255];
  char Study[255];
  char ImageNumber[255];
  char Modality[255];
  char StudyID[255];
  char StudyUID[255];
  char TransferSyntaxUID[255];

  double RescaleSlope;
  double RescaleOffset;
  int BitsAllocated;
  char DistanceUnits[255];
  char AnatomicalOrientation[255];
};

#endif
