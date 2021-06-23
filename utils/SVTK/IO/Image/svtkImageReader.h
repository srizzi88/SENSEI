/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageReader
 * @brief   Superclass of transformable binary file readers.
 *
 * svtkImageReader provides methods needed to read a region from a file.
 * It supports both transforms and masks on the input data, but as a result
 * is more complicated and slower than its parent class svtkImageReader2.
 *
 * @sa
 * svtkBMPReader svtkPNMReader svtkTIFFReader
 */

#ifndef svtkImageReader_h
#define svtkImageReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader2.h"

class svtkTransform;

#define SVTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define SVTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class SVTKIOIMAGE_EXPORT svtkImageReader : public svtkImageReader2
{
public:
  static svtkImageReader* New();
  svtkTypeMacro(svtkImageReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the data VOI. You can limit the reader to only
   * read a subset of the data.
   */
  svtkSetVector6Macro(DataVOI, int);
  svtkGetVector6Macro(DataVOI, int);
  //@}

  //@{
  /**
   * Set/Get the Data mask.  The data mask is a simply integer whose bits are
   * treated as a mask to the bits read from disk.  That is, the data mask is
   * bitwise-and'ed to the numbers read from disk.  This ivar is stored as 64
   * bits, the largest mask you will need.  The mask will be truncated to the
   * data size required to be read (using the least significant bits).
   */
  svtkGetMacro(DataMask, svtkTypeUInt64);
  svtkSetMacro(DataMask, svtkTypeUInt64);
  //@}

  //@{
  /**
   * Set/Get transformation matrix to transform the data from slice space
   * into world space. This matrix must be a permutation matrix. To qualify,
   * the sums of the rows must be + or - 1.
   */
  virtual void SetTransform(svtkTransform*);
  svtkGetObjectMacro(Transform, svtkTransform);
  //@}

  // Warning !!!
  // following should only be used by methods or template helpers, not users
  void ComputeInverseTransformedExtent(int inExtent[6], int outExtent[6]);
  void ComputeInverseTransformedIncrements(svtkIdType inIncr[3], svtkIdType outIncr[3]);

  int OpenAndSeekFile(int extent[6], int slice);

  //@{
  /**
   * Set/get the scalar array name for this data set.
   */
  svtkSetStringMacro(ScalarArrayName);
  svtkGetStringMacro(ScalarArrayName);
  //@}

  /**
   * svtkImageReader itself can read raw binary files. That being the case,
   * we need to implement `CanReadFile` to return success for any file.
   * Subclasses that read specific file format should override and implement
   * appropriate checks for file format.
   */
  int CanReadFile(const char*) override
  {
    return 1; // I think I can read the file but I cannot prove it
  }

protected:
  svtkImageReader();
  ~svtkImageReader() override;

  svtkTypeUInt64 DataMask;

  svtkTransform* Transform;

  void ComputeTransformedSpacing(double Spacing[3]);
  void ComputeTransformedOrigin(double origin[3]);
  void ComputeTransformedExtent(int inExtent[6], int outExtent[6]);
  void ComputeTransformedIncrements(svtkIdType inIncr[3], svtkIdType outIncr[3]);

  int DataVOI[6];

  char* ScalarArrayName;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;

private:
  svtkImageReader(const svtkImageReader&) = delete;
  void operator=(const svtkImageReader&) = delete;
};

#endif
