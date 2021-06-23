/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimplePointsWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimplePointsWriter.h"

#include "svtkErrorCode.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtksys/FStream.hxx"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

#include <iomanip>

svtkStandardNewMacro(svtkSimplePointsWriter);

svtkSimplePointsWriter::svtkSimplePointsWriter()
{
  svtksys::ofstream fout; // only used to extract the default precision
  this->DecimalPrecision = fout.precision();
}

void svtkSimplePointsWriter::WriteData()
{
  svtkPointSet* input = svtkPointSet::SafeDownCast(this->GetInput());
  svtkIdType numberOfPoints = 0;

  if (input)
  {
    numberOfPoints = input->GetNumberOfPoints();
  }

  // OpenSVTKFile() will report any errors that happen
  ostream* outfilep = this->OpenSVTKFile();
  if (!outfilep)
  {
    return;
  }

  ostream& outfile = *outfilep;

  for (svtkIdType i = 0; i < numberOfPoints; i++)
  {
    double p[3];
    input->GetPoint(i, p);
    outfile << std::setprecision(this->DecimalPrecision) << p[0] << " " << p[1] << " " << p[2]
            << std::endl;
  }

  // Close the file
  this->CloseSVTKFile(outfilep);

  // Delete the file if an error occurred
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    unlink(this->FileName);
  }
}

void svtkSimplePointsWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DecimalPrecision: " << this->DecimalPrecision << "\n";
}
