/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWriter
 * @brief   abstract class to write data to file(s)
 *
 * svtkWriter is an abstract class for mapper objects that write their data
 * to disk (or into a communications port). All writers respond to Write()
 * method. This method insures that there is input and input is up to date.
 *
 * @warning
 * Every subclass of svtkWriter must implement a WriteData() method. Most likely
 * will have to create SetInput() method as well.
 *
 * @sa
 * svtkXMLDataSetWriter svtkDataSetWriter svtkImageWriter svtkMCubesWriter
 */

#ifndef svtkWriter_h
#define svtkWriter_h

#include "svtkAlgorithm.h"
#include "svtkIOCoreModule.h" // For export macro

class svtkDataObject;

#define SVTK_ASCII 1
#define SVTK_BINARY 2

class SVTKIOCORE_EXPORT svtkWriter : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkWriter, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartMethod() and EndMethod() methods.
   * Returns 1 on success and 0 on failure.
   */
  virtual int Write();

  /**
   * Encode the string so that the reader will not have problems.
   * The resulting string is up to three times the size of the input
   * string.  doublePercent indicates whether to output a double '%' before
   * escaped characters so the string may be used as a printf format string.
   */
  void EncodeString(char* resname, const char* name, bool doublePercent);

  /**
   * Encode the string so that the reader will not have problems.
   * The resulting string is up to three times the size of the input
   * string.  Write the string to the output stream.
   * doublePercent indicates whether to output a double '%' before
   * escaped characters so the string may be used as a printf format string.
   */
  void EncodeWriteString(ostream* out, const char* name, bool doublePercent);

  //@{
  /**
   * Set/get the input to this writer.
   */
  void SetInputData(svtkDataObject* input);
  void SetInputData(int index, svtkDataObject* input);
  //@}

  svtkDataObject* GetInput();
  svtkDataObject* GetInput(int port);

protected:
  svtkWriter();
  ~svtkWriter() override;

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  virtual void WriteData() = 0; // internal method subclasses must respond to
  svtkTimeStamp WriteTime;

private:
  svtkWriter(const svtkWriter&) = delete;
  void operator=(const svtkWriter&) = delete;
};

#endif
