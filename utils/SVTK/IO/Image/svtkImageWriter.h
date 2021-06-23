/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageWriter
 * @brief   Writes images to files.
 *
 * svtkImageWriter writes images to files with any data type. The data type of
 * the file is the same scalar type as the input.  The dimensionality
 * determines whether the data will be written in one or multiple files.
 * This class is used as the superclass of most image writing classes
 * such as svtkBMPWriter etc. It supports streaming.
 */

#ifndef svtkImageWriter_h
#define svtkImageWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKIOIMAGE_EXPORT svtkImageWriter : public svtkImageAlgorithm
{
public:
  static svtkImageWriter* New();
  svtkTypeMacro(svtkImageWriter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name for the image file. You should specify either
   * a FileName or a FilePrefix. Use FilePrefix if the data is stored
   * in multiple files.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify file prefix for the image file(s).You should specify either
   * a FileName or FilePrefix. Use FilePrefix if the data is stored
   * in multiple files.
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
   * What dimension are the files to be written. Usually this is 2, or 3.
   * If it is 2 and the input is a volume then the volume will be
   * written as a series of 2d slices.
   */
  svtkSetMacro(FileDimensionality, int);
  svtkGetMacro(FileDimensionality, int);
  //@}

  /**
   * Set/Get the input object from the image pipeline.
   */
  svtkImageData* GetInput();

  /**
   * The main interface which triggers the writer to start.
   */
  virtual void Write();

  void DeleteFiles();

protected:
  svtkImageWriter();
  ~svtkImageWriter() override;

  int FileDimensionality;
  char* FilePrefix;
  char* FilePattern;
  char* FileName;
  int FileNumber;
  int FileLowerLeft;
  char* InternalFileName;
  size_t InternalFileNameSize;

  virtual void RecursiveWrite(int dim, svtkImageData* region, svtkInformation* inInfo, ostream* file);
  virtual void RecursiveWrite(
    int dim, svtkImageData* cache, svtkImageData* data, svtkInformation* inInfo, ostream* file);
  virtual void WriteFile(ostream* file, svtkImageData* data, int extent[6], int wExtent[6]);
  virtual void WriteFileHeader(ostream*, svtkImageData*, int[6]) {}
  virtual void WriteFileTrailer(ostream*, svtkImageData*) {}

  // Required for subclasses that need to prevent the writer
  // from touching the file system. The getter/setter are only
  // available in these subclasses.
  svtkTypeUBool WriteToMemory;

  // subclasses that do write to memory can override this
  // to implement the simple case
  virtual void MemoryWrite(int, svtkImageData*, int[6], svtkInformation*) {}

  // This is called by the superclass.
  // This is the method you should override.
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int MinimumFileNumber;
  int MaximumFileNumber;
  int FilesDeleted;

private:
  svtkImageWriter(const svtkImageWriter&) = delete;
  void operator=(const svtkImageWriter&) = delete;
};

#endif
