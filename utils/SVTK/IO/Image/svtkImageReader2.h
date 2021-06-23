/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageReader2
 * @brief   Superclass of binary file readers.
 *
 * svtkImageReader2 is a parent class for many SVTK image readers.
 * It was written to simplify the interface of svtkImageReader.
 * It can also be used directly to read data without headers (raw).
 * It is a good super class for streaming readers that do not require
 * a mask or transform on the data. An example of reading a raw file is
 * shown below:
 * \code
 *  svtkSmartPointer<svtkImageReader2> reader =
 *   svtkSmartPointer<svtkImageReader2>::New();
 * reader->SetFilePrefix(argv[1]);
 * reader->SetDataExtent(0, 63, 0, 63, 1, 93);
 * reader->SetDataSpacing(3.2, 3.2, 1.5);
 * reader->SetDataOrigin(0.0, 0.0, 0.0);
 * reader->SetDataScalarTypeToUnsignedShort();
 * reader->SetDataByteOrderToLittleEndian();
 * reader->UpdateWholeExtent();
 * \endcode
 *
 * @sa
 * svtkJPEGReader svtkPNGReader svtkImageReader svtkGESignaReader
 */

#ifndef svtkImageReader2_h
#define svtkImageReader2_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkStringArray;

#define SVTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define SVTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class SVTKIOIMAGE_EXPORT svtkImageReader2 : public svtkImageAlgorithm
{
public:
  static svtkImageReader2* New();
  svtkTypeMacro(svtkImageReader2, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name for the image file. If the data is stored in
   * multiple files, then use SetFileNames or SetFilePrefix instead.
   */
  virtual void SetFileName(const char*);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify a list of file names.  Each file must be a single slice,
   * and each slice must be of the same size. The files must be in the
   * correct order.
   * Use SetFileName when reading a volume (multiple slice), since
   * DataExtent will be modified after a SetFileNames call.
   */
  virtual void SetFileNames(svtkStringArray*);
  svtkGetObjectMacro(FileNames, svtkStringArray);
  //@}

  //@{
  /**
   * Specify file prefix for the image file or files.  This can be
   * used in place of SetFileName or SetFileNames if the filenames
   * follow a specific naming pattern, but you must explicitly set
   * the DataExtent so that the reader will know what range of slices
   * to load.
   */
  virtual void SetFilePrefix(const char*);
  svtkGetStringMacro(FilePrefix);
  //@}

  //@{
  /**
   * The snprintf-style format string used to build filename from
   * FilePrefix and slice number.
   */
  virtual void SetFilePattern(const char*);
  svtkGetStringMacro(FilePattern);
  //@}

  /**
   * Specify the in memory image buffer.
   * May be used by a reader to allow the reading
   * of an image from memory instead of from file.
   */
  virtual void SetMemoryBuffer(const void*);
  virtual const void* GetMemoryBuffer() { return this->MemoryBuffer; }

  /**
   * Specify the in memory image buffer length.
   */
  virtual void SetMemoryBufferLength(svtkIdType buflen);
  svtkIdType GetMemoryBufferLength() { return this->MemoryBufferLength; }

  /**
   * Set the data type of pixels in the file.
   * If you want the output scalar type to have a different value, set it
   * after this method is called.
   */
  virtual void SetDataScalarType(int type);
  virtual void SetDataScalarTypeToFloat() { this->SetDataScalarType(SVTK_FLOAT); }
  virtual void SetDataScalarTypeToDouble() { this->SetDataScalarType(SVTK_DOUBLE); }
  virtual void SetDataScalarTypeToInt() { this->SetDataScalarType(SVTK_INT); }
  virtual void SetDataScalarTypeToUnsignedInt() { this->SetDataScalarType(SVTK_UNSIGNED_INT); }
  virtual void SetDataScalarTypeToShort() { this->SetDataScalarType(SVTK_SHORT); }
  virtual void SetDataScalarTypeToUnsignedShort() { this->SetDataScalarType(SVTK_UNSIGNED_SHORT); }
  virtual void SetDataScalarTypeToChar() { this->SetDataScalarType(SVTK_CHAR); }
  virtual void SetDataScalarTypeToSignedChar() { this->SetDataScalarType(SVTK_SIGNED_CHAR); }
  virtual void SetDataScalarTypeToUnsignedChar() { this->SetDataScalarType(SVTK_UNSIGNED_CHAR); }

  //@{
  /**
   * Get the file format.  Pixels are this type in the file.
   */
  svtkGetMacro(DataScalarType, int);
  //@}

  //@{
  /**
   * Set/Get the number of scalar components
   */
  svtkSetMacro(NumberOfScalarComponents, int);
  svtkGetMacro(NumberOfScalarComponents, int);
  //@}

  //@{
  /**
   * Get/Set the extent of the data on disk.
   */
  svtkSetVector6Macro(DataExtent, int);
  svtkGetVector6Macro(DataExtent, int);
  //@}

  //@{
  /**
   * The number of dimensions stored in a file. This defaults to two.
   */
  svtkSetMacro(FileDimensionality, int);
  int GetFileDimensionality() { return this->FileDimensionality; }
  //@}

  //@{
  /**
   * Set/Get the spacing of the data in the file.
   */
  svtkSetVector3Macro(DataSpacing, double);
  svtkGetVector3Macro(DataSpacing, double);
  //@}

  //@{
  /**
   * Set/Get the origin of the data (location of first pixel in the file).
   */
  svtkSetVector3Macro(DataOrigin, double);
  svtkGetVector3Macro(DataOrigin, double);
  //@}

  //@{
  /**
   * Set/Get the direction of the data (9 elements: 3x3 matrix).
   */
  svtkSetVectorMacro(DataDirection, double, 9);
  svtkGetVectorMacro(DataDirection, double, 9);
  //@}

  //@{
  /**
   * Get the size of the header computed by this object.
   */
  unsigned long GetHeaderSize();
  unsigned long GetHeaderSize(unsigned long slice);
  //@}

  /**
   * If there is a tail on the file, you want to explicitly set the
   * header size.
   */
  virtual void SetHeaderSize(unsigned long size);

  //@{
  /**
   * These methods should be used instead of the SwapBytes methods.
   * They indicate the byte ordering of the file you are trying
   * to read in. These methods will then either swap or not swap
   * the bytes depending on the byte ordering of the machine it is
   * being run on. For example, reading in a BigEndian file on a
   * BigEndian machine will result in no swapping. Trying to read
   * the same file on a LittleEndian machine will result in swapping.
   * As a quick note most UNIX machines are BigEndian while PC's
   * and VAX tend to be LittleEndian. So if the file you are reading
   * in was generated on a VAX or PC, SetDataByteOrderToLittleEndian
   * otherwise SetDataByteOrderToBigEndian.
   */
  virtual void SetDataByteOrderToBigEndian();
  virtual void SetDataByteOrderToLittleEndian();
  virtual int GetDataByteOrder();
  virtual void SetDataByteOrder(int);
  virtual const char* GetDataByteOrderAsString();
  //@}

  //@{
  /**
   * When reading files which start at an unusual index, this can be added
   * to the slice number when generating the file name (default = 0)
   */
  svtkSetMacro(FileNameSliceOffset, int);
  svtkGetMacro(FileNameSliceOffset, int);
  //@}

  //@{
  /**
   * When reading files which have regular, but non contiguous slices
   * (eg filename.1,filename.3,filename.5)
   * a spacing can be specified to skip missing files (default = 1)
   */
  svtkSetMacro(FileNameSliceSpacing, int);
  svtkGetMacro(FileNameSliceSpacing, int);
  //@}

  //@{
  /**
   * Set/Get the byte swapping to explicitly swap the bytes of a file.
   */
  svtkSetMacro(SwapBytes, svtkTypeBool);
  virtual svtkTypeBool GetSwapBytes() { return this->SwapBytes; }
  svtkBooleanMacro(SwapBytes, svtkTypeBool);
  //@}

  istream* GetFile() { return this->File; }
  svtkGetVectorMacro(DataIncrements, unsigned long, 4);

  virtual int OpenFile();
  void CloseFile();
  virtual void SeekFile(int i, int j, int k);

  //@{
  /**
   * Set/Get whether the data comes from the file starting in the lower left
   * corner or upper left corner.
   */
  svtkBooleanMacro(FileLowerLeft, svtkTypeBool);
  svtkGetMacro(FileLowerLeft, svtkTypeBool);
  svtkSetMacro(FileLowerLeft, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the internal file name
   */
  virtual void ComputeInternalFileName(int slice);
  svtkGetStringMacro(InternalFileName);
  //@}

  /**
   * Return non zero if the reader can read the given file name.
   * Should be implemented by all sub-classes of svtkImageReader2.
   * For non zero return values the following values are to be used
   * 1 - I think I can read the file but I cannot prove it
   * 2 - I definitely can read the file
   * 3 - I can read the file and I have validated that I am the
   * correct reader for this file
   */
  virtual int CanReadFile(const char* svtkNotUsed(fname)) { return 0; }

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  virtual const char* GetFileExtensions() { return nullptr; }

  //@{
  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  virtual const char* GetDescriptiveName() { return nullptr; }

protected:
  svtkImageReader2();
  ~svtkImageReader2() override;
  //@}

  svtkStringArray* FileNames;

  char* InternalFileName;
  char* FileName;
  char* FilePrefix;
  char* FilePattern;
  int NumberOfScalarComponents;
  svtkTypeBool FileLowerLeft;

  const void* MemoryBuffer;
  svtkIdType MemoryBufferLength;

  istream* File;
  unsigned long DataIncrements[4];
  int DataExtent[6];
  svtkTypeBool SwapBytes;

  int FileDimensionality;
  unsigned long HeaderSize;
  int DataScalarType;
  unsigned long ManualHeaderSize;

  double DataSpacing[3];
  double DataOrigin[3];
  double DataDirection[9];

  int FileNameSliceOffset;
  int FileNameSliceSpacing;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  virtual void ExecuteInformation();
  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;
  virtual void ComputeDataIncrements();

private:
  svtkImageReader2(const svtkImageReader2&) = delete;
  void operator=(const svtkImageReader2&) = delete;
};

#endif
