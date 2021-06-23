/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimplePointsWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSimplePointsWriter
 * @brief   write a file of xyz coordinates
 *
 * svtkSimplePointsWriter writes a simple file of xyz coordinates
 *
 * @sa
 * svtkSimplePointsReader
 */

#ifndef svtkSimplePointsWriter_h
#define svtkSimplePointsWriter_h

#include "svtkDataSetWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class SVTKIOLEGACY_EXPORT svtkSimplePointsWriter : public svtkDataSetWriter
{
public:
  static svtkSimplePointsWriter* New();
  svtkTypeMacro(svtkSimplePointsWriter, svtkDataSetWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkGetMacro(DecimalPrecision, int);
  svtkSetMacro(DecimalPrecision, int);

protected:
  svtkSimplePointsWriter();
  ~svtkSimplePointsWriter() override {}

  void WriteData() override;

  int DecimalPrecision;

private:
  svtkSimplePointsWriter(const svtkSimplePointsWriter&) = delete;
  void operator=(const svtkSimplePointsWriter&) = delete;
};

#endif
