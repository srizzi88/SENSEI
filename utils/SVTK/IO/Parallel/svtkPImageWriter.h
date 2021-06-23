/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPImageWriter
 * @brief   Writes images to files.
 *
 * svtkPImageWriter writes images to files with any data type. The data type of
 * the file is the same scalar type as the input.  The dimensionality
 * determines whether the data will be written in one or multiple files.
 * This class is used as the superclass of most image writing classes
 * such as svtkBMPWriter etc. It supports streaming.
 */

#ifndef svtkPImageWriter_h
#define svtkPImageWriter_h

#include "svtkIOParallelModule.h" // For export macro
#include "svtkImageWriter.h"
class svtkPipelineSize;

class SVTKIOPARALLEL_EXPORT svtkPImageWriter : public svtkImageWriter
{
public:
  static svtkPImageWriter* New();
  svtkTypeMacro(svtkPImageWriter, svtkImageWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / Get the memory limit in kibibytes (1024 bytes). The writer will
   * stream to attempt to keep the pipeline size within this limit
   */
  svtkSetMacro(MemoryLimit, unsigned long);
  svtkGetMacro(MemoryLimit, unsigned long);
  //@}

protected:
  svtkPImageWriter();
  ~svtkPImageWriter() override;

  unsigned long MemoryLimit;

  void RecursiveWrite(
    int dim, svtkImageData* region, svtkInformation* inInfo, ostream* file) override;
  void RecursiveWrite(int dim, svtkImageData* cache, svtkImageData* data, svtkInformation* inInfo,
    ostream* file) override
  {
    this->svtkImageWriter::RecursiveWrite(dim, cache, data, inInfo, file);
  }

  svtkPipelineSize* SizeEstimator;

private:
  svtkPImageWriter(const svtkPImageWriter&) = delete;
  void operator=(const svtkPImageWriter&) = delete;
};

#endif
