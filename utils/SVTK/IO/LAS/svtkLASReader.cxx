/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGDALRasterReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkLASReader.h"

#include <svtkCellArray.h>
#include <svtkInformation.h>
#include <svtkInformationVector.h>
#include <svtkNew.h>
#include <svtkObjectFactory.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkUnsignedShortArray.h>
#include <svtkVertexGlyphFilter.h>
#include <svtksys/FStream.hxx>

#include <liblas/liblas.hpp>

#include <fstream>
#include <iostream>
#include <valarray>

svtkStandardNewMacro(svtkLASReader);

//----------------------------------------------------------------------------
svtkLASReader::svtkLASReader()
{
  this->FileName = nullptr;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkLASReader::~svtkLASReader()
{
  delete[] this->FileName;
}

//----------------------------------------------------------------------------
int svtkLASReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(request), svtkInformationVector* outputVector)
{
  // Get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Open LAS File for reading
  svtksys::ifstream ifs;
  ifs.open(this->FileName, std::ios_base::binary | std::ios_base::in);

  if (!ifs.is_open())
  {
    svtkErrorMacro(<< "Unable to open file for reading: " << this->FileName);
    return SVTK_ERROR;
  }

  // Read header data
  liblas::ReaderFactory readerFactory;
  liblas::Reader reader = readerFactory.CreateWithStream(ifs);

  svtkNew<svtkPolyData> pointsPolyData;
  this->ReadPointRecordData(reader, pointsPolyData);
  ifs.close();

  // Convert points to verts in output polydata
  svtkNew<svtkVertexGlyphFilter> vertexFilter;
  vertexFilter->SetInputData(pointsPolyData);
  vertexFilter->Update();
  output->ShallowCopy(vertexFilter->GetOutput());

  return SVTK_OK;
}

//----------------------------------------------------------------------------
void svtkLASReader::ReadPointRecordData(liblas::Reader& reader, svtkPolyData* pointsPolyData)
{
  svtkNew<svtkPoints> points;
  // scalars associated with points
  svtkNew<svtkUnsignedShortArray> color;
  color->SetName("color");
  color->SetNumberOfComponents(3);
  svtkNew<svtkUnsignedShortArray> classification;
  classification->SetName("classification");
  classification->SetNumberOfComponents(1);
  svtkNew<svtkUnsignedShortArray> intensity;
  intensity->SetName("intensity");
  intensity->SetNumberOfComponents(1);

  liblas::Header header = liblas::Header(reader.GetHeader());
  std::valarray<double> scale = { header.GetScaleX(), header.GetScaleY(), header.GetScaleZ() };
  std::valarray<double> offset = { header.GetOffsetX(), header.GetOffsetY(), header.GetOffsetZ() };
  liblas::PointFormatName pointFormat = header.GetDataFormatId();
  int pointRecordsCount = header.GetPointRecordsCount();

  for (int i = 0; i < pointRecordsCount && reader.ReadNextPoint(); i++)
  {
    liblas::Point const& p = reader.GetPoint();
    std::valarray<double> lasPoint = { p.GetX(), p.GetY(), p.GetZ() };
    points->InsertNextPoint(&lasPoint[0]);
    // std::valarray<double> point = lasPoint * scale + offset;
    // We have seen a file where the scaled points were much smaller than the offset
    // So, all points ended up in the same place.
    std::valarray<double> point = lasPoint * scale;
    switch (pointFormat)
    {
      case liblas::ePointFormat2:
      case liblas::ePointFormat3:
      case liblas::ePointFormat5:
      {
        unsigned short c[3];
        c[0] = p.GetColor().GetRed();
        c[1] = p.GetColor().GetGreen();
        c[2] = p.GetColor().GetBlue();
        color->InsertNextTypedTuple(c);
        intensity->InsertNextValue(p.GetIntensity());
      }
      break;

      case liblas::ePointFormat0:
      case liblas::ePointFormat1:
        classification->InsertNextValue(p.GetClassification().GetClass());
        intensity->InsertNextValue(p.GetIntensity());
        break;

      case liblas::ePointFormatUnknown:
      default:
        intensity->InsertNextValue(p.GetIntensity());
        break;
    }
  }

  pointsPolyData->SetPoints(points);
  pointsPolyData->GetPointData()->AddArray(intensity);
  switch (pointFormat)
  {
    case liblas::ePointFormat2:
    case liblas::ePointFormat3:
    case liblas::ePointFormat5:
      pointsPolyData->GetPointData()->AddArray(color);
      break;

    case liblas::ePointFormat0:
    case liblas::ePointFormat1:
      pointsPolyData->GetPointData()->AddArray(classification);
      break;

    case liblas::ePointFormatUnknown:
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void svtkLASReader::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "svtkLASReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
