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

#include "svtkPDALReader.h"

#include <svtkDoubleArray.h>
#include <svtkFloatArray.h>
#include <svtkInformation.h>
#include <svtkInformationVector.h>
#include <svtkNew.h>
#include <svtkObjectFactory.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkTypeInt16Array.h>
#include <svtkTypeInt32Array.h>
#include <svtkTypeInt64Array.h>
#include <svtkTypeInt8Array.h>
#include <svtkTypeUInt16Array.h>
#include <svtkTypeUInt32Array.h>
#include <svtkTypeUInt64Array.h>
#include <svtkTypeUInt8Array.h>
#include <svtkVertexGlyphFilter.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if __GNUC__ > 6
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif
#endif
#include <pdal/Reader.hpp>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <pdal/Options.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>

svtkStandardNewMacro(svtkPDALReader);

//----------------------------------------------------------------------------
svtkPDALReader::svtkPDALReader()
{
  this->FileName = nullptr;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkPDALReader::~svtkPDALReader()
{
  if (this->FileName)
  {
    delete[] this->FileName;
  }
}

//----------------------------------------------------------------------------
int svtkPDALReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(request), svtkInformationVector* outputVector)
{
  try
  {
    // Get the info object
    svtkInformation* outInfo = outputVector->GetInformationObject(0);

    // Get the output
    svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

    pdal::StageFactory factory;
    std::string driverName = factory.inferReaderDriver(this->FileName);
    if (driverName.empty())
    {
      svtkErrorMacro("Cannot infer the reader driver for " << this->FileName);
      return 0;
    }

    pdal::Option opt_filename("filename", this->FileName);
    pdal::Options opts;
    opts.add(opt_filename);
    pdal::Stage* reader = factory.createStage(driverName);
    if (!reader)
    {
      svtkErrorMacro("Cannot open file " << this->FileName);
      return 0;
    }
    reader->setOptions(opts);

    svtkNew<svtkPolyData> pointsPolyData;
    this->ReadPointRecordData(*reader, pointsPolyData);

    // Convert points to verts in output polydata
    svtkNew<svtkVertexGlyphFilter> vertexFilter;
    vertexFilter->SetInputData(pointsPolyData);
    vertexFilter->Update();
    output->ShallowCopy(vertexFilter->GetOutput());
    return SVTK_OK;
  }
  catch (const std::exception& e)
  {
    svtkErrorMacro("exception: " << e.what());
  }
  catch (...)
  {
    svtkErrorMacro("Unknown exception");
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkPDALReader::ReadPointRecordData(pdal::Stage& reader, svtkPolyData* pointsPolyData)
{
  svtkNew<svtkPoints> points;
  points->SetDataTypeToDouble();
  pointsPolyData->SetPoints(points);
  pdal::PointTable table;
  reader.prepare(table);
  pdal::PointViewSet pointViewSet = reader.execute(table);
  pdal::PointViewPtr pointView = *pointViewSet.begin();
  points->SetNumberOfPoints(pointView->size());
  pdal::Dimension::IdList dims = pointView->dims();
  std::vector<svtkDoubleArray*> doubleArray(dims.size(), nullptr);
  std::vector<svtkFloatArray*> floatArray(dims.size(), nullptr);
  std::vector<svtkTypeUInt8Array*> uInt8Array(dims.size(), nullptr);
  std::vector<svtkTypeUInt16Array*> uInt16Array(dims.size(), nullptr);
  std::vector<svtkTypeUInt32Array*> uInt32Array(dims.size(), nullptr);
  std::vector<svtkTypeUInt64Array*> uInt64Array(dims.size(), nullptr);
  std::vector<svtkTypeInt8Array*> int8Array(dims.size(), nullptr);
  std::vector<svtkTypeInt16Array*> int16Array(dims.size(), nullptr);
  std::vector<svtkTypeInt32Array*> int32Array(dims.size(), nullptr);
  std::vector<svtkTypeInt64Array*> int64Array(dims.size(), nullptr);
  svtkTypeUInt16Array* colorArray = nullptr;
  // check if we have a color field, and create the required array
  bool hasColor = false, hasRed = false, hasGreen = false, hasBlue = false;
  for (size_t i = 0; i < dims.size(); ++i)
  {
    pdal::Dimension::Id dimensionId = dims[i];
    switch (dimensionId)
    {
      case pdal::Dimension::Id::Red:
        hasRed = true;
        break;
      case pdal::Dimension::Id::Green:
        hasGreen = true;
        break;
      case pdal::Dimension::Id::Blue:
        hasBlue = true;
        break;
      default:
        continue;
    }
  }
  if (hasRed && hasGreen && hasBlue)
  {
    hasColor = true;
    svtkNew<svtkTypeUInt16Array> a;
    a->SetNumberOfComponents(3);
    a->SetNumberOfTuples(pointView->size());
    a->SetName("Color");
    pointsPolyData->GetPointData()->AddArray(a);
    colorArray = a;
  }
  // create arrays for fields
  for (size_t i = 0; i < dims.size(); ++i)
  {
    pdal::Dimension::Id dimensionId = dims[i];
    if (dimensionId == pdal::Dimension::Id::X || dimensionId == pdal::Dimension::Id::Y ||
      dimensionId == pdal::Dimension::Id::Z)
    {
      continue;
    }
    if (hasColor &&
      (dimensionId == pdal::Dimension::Id::Red || dimensionId == pdal::Dimension::Id::Green ||
        dimensionId == pdal::Dimension::Id::Blue))
    {
      continue;
    }
    switch (pointView->dimType(dimensionId))
    {
      case pdal::Dimension::Type::Double:
      {
        svtkNew<svtkDoubleArray> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        doubleArray[i] = a;
        break;
      }
      case pdal::Dimension::Type::Float:
      {
        svtkNew<svtkFloatArray> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        floatArray[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned8:
      {
        svtkNew<svtkTypeUInt8Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt8Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned16:
      {
        svtkNew<svtkTypeUInt16Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt16Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned32:
      {
        svtkNew<svtkTypeUInt32Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt32Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned64:
      {
        svtkNew<svtkTypeUInt64Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt64Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed8:
      {
        svtkNew<svtkTypeInt8Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int8Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed16:
      {
        svtkNew<svtkTypeInt16Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int16Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed32:
      {
        svtkNew<svtkTypeInt32Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int32Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed64:
      {
        svtkNew<svtkTypeInt64Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int64Array[i] = a;
        break;
      }
      default:
        throw std::runtime_error("Invalid pdal::Dimension::Type");
    }
  }
  for (pdal::PointId pointId = 0; pointId < pointView->size(); ++pointId)
  {
    double point[3] = { pointView->getFieldAs<double>(pdal::Dimension::Id::X, pointId),
      pointView->getFieldAs<double>(pdal::Dimension::Id::Y, pointId),
      pointView->getFieldAs<double>(pdal::Dimension::Id::Z, pointId) };
    points->SetPoint(pointId, point);
    if (hasColor)
    {
      uint16_t color[3] = {
        pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Red, pointId),
        pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Green, pointId),
        pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Blue, pointId),
      };
      colorArray->SetTypedTuple(pointId, color);
    }
    for (size_t i = 0; i < dims.size(); ++i)
    {
      pdal::Dimension::Id dimensionId = dims[i];
      if (dimensionId == pdal::Dimension::Id::X || dimensionId == pdal::Dimension::Id::Y ||
        dimensionId == pdal::Dimension::Id::Z)
      {
        continue;
      }
      if (hasColor &&
        (dimensionId == pdal::Dimension::Id::Red || dimensionId == pdal::Dimension::Id::Green ||
          dimensionId == pdal::Dimension::Id::Blue))
      {
        continue;
      }
      switch (pointView->dimType(dimensionId))
      {
        case pdal::Dimension::Type::Double:
        {
          svtkDoubleArray* a = doubleArray[i];
          double value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Float:
        {
          svtkFloatArray* a = floatArray[i];
          float value = pointView->getFieldAs<float>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned8:
        {
          svtkTypeUInt8Array* a = uInt8Array[i];
          uint8_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned16:
        {
          svtkTypeUInt16Array* a = uInt16Array[i];
          uint16_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned32:
        {
          svtkTypeUInt32Array* a = uInt32Array[i];
          uint32_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned64:
        {
          svtkTypeUInt64Array* a = uInt64Array[i];
          uint64_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed8:
        {
          svtkTypeInt8Array* a = int8Array[i];
          int8_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed16:
        {
          svtkTypeInt16Array* a = int16Array[i];
          int16_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed32:
        {
          svtkTypeInt32Array* a = int32Array[i];
          int32_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed64:
        {
          svtkTypeInt64Array* a = int64Array[i];
          int64_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        default:
          throw std::runtime_error("Invalid pdal::Dimension::Type");
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkPDALReader::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "svtkPDALReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
