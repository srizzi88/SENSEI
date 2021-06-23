/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPNMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPNMReader
 * @brief   read pnm (i.e., portable anymap) files
 *
 *
 * svtkPNMReader is a source object that reads pnm (portable anymap) files.
 * This includes .pbm (bitmap), .pgm (grayscale), and .ppm (pixmap) files.
 * (Currently this object only reads binary versions of these files.)
 *
 * PNMReader creates structured point datasets. The dimension of the
 * dataset depends upon the number of files read. Reading a single file
 * results in a 2D image, while reading more than one file results in a
 * 3D volume.
 *
 * To read a volume, files must be of the form "FileName.<number>" (e.g.,
 * foo.ppm.0, foo.ppm.1, ...). You must also specify the DataExtent.  The
 * fifth and sixth values of the DataExtent specify the beginning and ending
 * files to read.
 */

#ifndef svtkPNMReader_h
#define svtkPNMReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader.h"

class SVTKIOIMAGE_EXPORT svtkPNMReader : public svtkImageReader
{
public:
  static svtkPNMReader* New();
  svtkTypeMacro(svtkPNMReader, svtkImageReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  int CanReadFile(const char* fname) override;
  /**
   * .pnm .pgm .ppm
   */
  const char* GetFileExtensions() override { return ".pnm .pgm .ppm"; }

  /**
   * PNM
   */
  const char* GetDescriptiveName() override { return "PNM"; }

protected:
  svtkPNMReader() {}
  ~svtkPNMReader() override {}
  void ExecuteInformation() override;

private:
  svtkPNMReader(const svtkPNMReader&) = delete;
  void operator=(const svtkPNMReader&) = delete;
};

#endif
