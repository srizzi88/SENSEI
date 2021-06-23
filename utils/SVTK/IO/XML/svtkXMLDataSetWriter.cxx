/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLDataSetWriter.h"

#include "svtkAlgorithmOutput.h"
#include "svtkCallbackCommand.h"
#include "svtkDataSet.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLImageDataWriter.h"
#include "svtkXMLPolyDataWriter.h"
#include "svtkXMLRectilinearGridWriter.h"
#include "svtkXMLStructuredGridWriter.h"
#include "svtkXMLUnstructuredGridWriter.h"

svtkStandardNewMacro(svtkXMLDataSetWriter);

//----------------------------------------------------------------------------
svtkXMLDataSetWriter::svtkXMLDataSetWriter() = default;

//----------------------------------------------------------------------------
svtkXMLDataSetWriter::~svtkXMLDataSetWriter() = default;

//----------------------------------------------------------------------------
int svtkXMLDataSetWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}
