/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJSONImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkJSONImageWriter
 * @brief   Writes svtkImageData to a JSON file.
 *
 * svtkJSONImageWriter writes a JSON file which will describe the
 * data inside a svtkImageData.
 */

#ifndef svtkJSONImageWriter_h
#define svtkJSONImageWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKIOIMAGE_EXPORT svtkJSONImageWriter : public svtkImageAlgorithm
{
public:
  static svtkJSONImageWriter* New();
  svtkTypeMacro(svtkJSONImageWriter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name for the image file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify ArrayName to export. By default nullptr which will dump ALL arrays.
   */
  svtkSetStringMacro(ArrayName);
  svtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Specify Slice in Z to export. By default -1 which will dump the full 3D domain.
   */
  svtkSetMacro(Slice, int);
  svtkGetMacro(Slice, int);
  //@}

  /**
   * The main interface which triggers the writer to start.
   */
  virtual void Write();

protected:
  svtkJSONImageWriter();
  ~svtkJSONImageWriter() override;

  char* FileName;
  char* ArrayName;
  int Slice;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkJSONImageWriter(const svtkJSONImageWriter&) = delete;
  void operator=(const svtkJSONImageWriter&) = delete;
};

#endif
