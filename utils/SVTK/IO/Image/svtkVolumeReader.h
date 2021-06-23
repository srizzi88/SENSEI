/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumeReader
 * @brief   read image files
 *
 * svtkVolumeReader is a source object that reads image files.
 *
 * VolumeReader creates structured point datasets. The dimension of the
 * dataset depends upon the number of files read. Reading a single file
 * results in a 2D image, while reading more than one file results in a
 * 3D volume.
 *
 * File names are created using FilePattern and FilePrefix as follows:
 * snprintf (filename, sizeof(filename), FilePattern, FilePrefix, number);
 * where number is in the range ImageRange[0] to ImageRange[1]. If
 * ImageRange[1] <= ImageRange[0], then slice number ImageRange[0] is
 * read. Thus to read an image set ImageRange[0] = ImageRange[1] = slice
 * number. The default behavior is to read a single file (i.e., image slice 1).
 *
 * The DataMask instance variable is used to read data files with embedded
 * connectivity or segmentation information. For example, some data has
 * the high order bit set to indicate connected surface. The DataMask allows
 * you to select this data. Other important ivars include HeaderSize, which
 * allows you to skip over initial info, and SwapBytes, which turns on/off
 * byte swapping. Consider using svtkImageReader as a replacement.
 *
 * @sa
 * svtkSliceCubes svtkMarchingCubes svtkPNMReader svtkVolume16Reader
 * svtkImageReader
 */

#ifndef svtkVolumeReader_h
#define svtkVolumeReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKIOIMAGE_EXPORT svtkVolumeReader : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkVolumeReader, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file prefix for the image file(s).
   */
  svtkSetStringMacro(FilePrefix);
  svtkGetStringMacro(FilePrefix);
  //@}

  //@{
  /**
   * The snprintf format used to build filename from FilePrefix and number.
   */
  svtkSetStringMacro(FilePattern);
  svtkGetStringMacro(FilePattern);
  //@}

  //@{
  /**
   * Set the range of files to read.
   */
  svtkSetVector2Macro(ImageRange, int);
  svtkGetVectorMacro(ImageRange, int, 2);
  //@}

  //@{
  /**
   * Specify the spacing for the data.
   */
  svtkSetVector3Macro(DataSpacing, double);
  svtkGetVectorMacro(DataSpacing, double, 3);
  //@}

  //@{
  /**
   * Specify the origin for the data.
   */
  svtkSetVector3Macro(DataOrigin, double);
  svtkGetVectorMacro(DataOrigin, double, 3);
  //@}

  /**
   * Other objects make use of this method.
   */
  virtual svtkImageData* GetImage(int ImageNumber) = 0;

protected:
  svtkVolumeReader();
  ~svtkVolumeReader() override;

  char* FilePrefix;
  char* FilePattern;
  int ImageRange[2];
  double DataSpacing[3];
  double DataOrigin[3];

private:
  svtkVolumeReader(const svtkVolumeReader&) = delete;
  void operator=(const svtkVolumeReader&) = delete;
};

#endif
