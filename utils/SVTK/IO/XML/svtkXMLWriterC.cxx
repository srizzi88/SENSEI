/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLWriterC.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLWriterC.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
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

// Function to allocate a svtkDataArray and point it at the given data.
// The data are not copied.
static svtkSmartPointer<svtkDataArray> svtkXMLWriterC_NewDataArray(const char* method,
  const char* name, int dataType, void* data, svtkIdType numTuples, int numComponents);

// Function to allocate a svtkCellArray and point it at the given
// cells.  The cells are not copied.
static svtkSmartPointer<svtkCellArray> svtkXMLWriterC_NewCellArray(
  const char* method, svtkIdType ncells, svtkIdType* cells, svtkIdType cellsSize);

// Function to implement svtkXMLWriterC_SetPointData and
// svtkXMLWriterC_SetCellData without duplicate code.
static void svtkXMLWriterC_SetDataInternal(svtkXMLWriterC* self, const char* name, int dataType,
  void* data, svtkIdType numTuples, int numComponents, const char* role, const char* method,
  int isPoints);

extern "C"
{

  //----------------------------------------------------------------------------
  // Define the interface structure.  Note this is a C struct so it has no
  // real constructor or destructor.
  struct svtkXMLWriterC_s
  {
    svtkSmartPointer<svtkXMLWriter> Writer;
    svtkSmartPointer<svtkDataObject> DataObject;
    int Writing;
  };

  //----------------------------------------------------------------------------
  svtkXMLWriterC* svtkXMLWriterC_New(void)
  {
    if (svtkXMLWriterC* self = new svtkXMLWriterC)
    {
      // Initialize the object.
      self->Writer = nullptr;
      self->DataObject = nullptr;
      self->Writing = 0;
      return self;
    }
    else
    {
      svtkGenericWarningMacro("Failed to allocate a svtkXMLWriterC object.");
      return nullptr;
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_Delete(svtkXMLWriterC* self)
  {
    if (self)
    {
      // Finalize the object.
      self->Writer = nullptr;
      self->DataObject = nullptr;
      delete self;
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetDataObjectType(svtkXMLWriterC* self, int objType)
  {
    if (!self)
    {
      return;
    }
    if (!self->DataObject)
    {
      // Create the writer and data object.
      switch (objType)
      {
        case SVTK_POLY_DATA:
        {
          self->DataObject = svtkSmartPointer<svtkPolyData>::New();
          self->Writer = svtkSmartPointer<svtkXMLPolyDataWriter>::New();
        };
        break;
        case SVTK_UNSTRUCTURED_GRID:
        {
          self->DataObject = svtkSmartPointer<svtkUnstructuredGrid>::New();
          self->Writer = svtkSmartPointer<svtkXMLUnstructuredGridWriter>::New();
        };
        break;
        case SVTK_STRUCTURED_GRID:
        {
          self->DataObject = svtkSmartPointer<svtkStructuredGrid>::New();
          self->Writer = svtkSmartPointer<svtkXMLStructuredGridWriter>::New();
        };
        break;
        case SVTK_RECTILINEAR_GRID:
        {
          self->DataObject = svtkSmartPointer<svtkRectilinearGrid>::New();
          self->Writer = svtkSmartPointer<svtkXMLRectilinearGridWriter>::New();
        };
        break;
        case SVTK_IMAGE_DATA:
        {
          self->DataObject = svtkSmartPointer<svtkImageData>::New();
          self->Writer = svtkSmartPointer<svtkXMLImageDataWriter>::New();
        };
        break;
      }

      // Set the data object as input to the writer.
      if (self->Writer && self->DataObject)
      {
        self->Writer->SetInputData(self->DataObject);
      }
      else
      {
        svtkGenericWarningMacro(
          "Failed to allocate data object and writer for type " << objType << ".");
      }
    }
    else
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetDataObjectType called twice.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetDataModeType(svtkXMLWriterC* self, int datamodetype)
  {
    if (!self)
    {
      return;
    }
    if (self->Writer)
    {
      // Invoke the writer.
      switch (datamodetype)
      {
        case svtkXMLWriter::Ascii:
        case svtkXMLWriter::Binary:
        case svtkXMLWriter::Appended:
          self->Writer->SetDataMode(datamodetype);
          break;
        default:
          svtkGenericWarningMacro(
            "svtkXMLWriterC_SetDataModeType : unknown DataMode: " << datamodetype);
      }
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetDataModeType called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetExtent(svtkXMLWriterC* self, int extent[6])
  {
    if (!self)
    {
      return;
    }
    if (svtkImageData* imData = svtkImageData::SafeDownCast(self->DataObject))
    {
      imData->SetExtent(extent);
    }
    else if (svtkStructuredGrid* sGrid = svtkStructuredGrid::SafeDownCast(self->DataObject))
    {
      sGrid->SetExtent(extent);
    }
    else if (svtkRectilinearGrid* rGrid = svtkRectilinearGrid::SafeDownCast(self->DataObject))
    {
      rGrid->SetExtent(extent);
    }
    else if (self->DataObject)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetExtent called for "
        << self->DataObject->GetClassName() << " data object.");
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetExtent called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetPoints(svtkXMLWriterC* self, int dataType, void* data, svtkIdType numPoints)
  {
    if (!self)
    {
      return;
    }
    if (svtkPointSet* dataObject = svtkPointSet::SafeDownCast(self->DataObject))
    {
      // Create the svtkDataArray that will reference the points.
      if (svtkSmartPointer<svtkDataArray> array =
            svtkXMLWriterC_NewDataArray("SetPoints", nullptr, dataType, data, numPoints, 3))
      {
        // Store the point array in the data object's points.
        if (svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New())
        {
          points->SetNumberOfPoints(numPoints);
          points->SetData(array);
          dataObject->SetPoints(points);
        }
        else
        {
          svtkGenericWarningMacro("svtkXMLWriterC_SetPoints failed to create a svtkPoints object.");
        }
      }
    }
    else if (self->DataObject)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetPoints called for "
        << self->DataObject->GetClassName() << " data object.");
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetPoints called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetOrigin(svtkXMLWriterC* self, double origin[3])
  {
    if (!self)
    {
      return;
    }
    if (svtkImageData* dataObject = svtkImageData::SafeDownCast(self->DataObject))
    {
      dataObject->SetOrigin(origin);
    }
    else if (self->DataObject)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetOrigin called for "
        << self->DataObject->GetClassName() << " data object.");
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetOrigin called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetSpacing(svtkXMLWriterC* self, double spacing[3])
  {
    if (!self)
    {
      return;
    }
    if (svtkImageData* dataObject = svtkImageData::SafeDownCast(self->DataObject))
    {
      dataObject->SetSpacing(spacing);
    }
    else if (self->DataObject)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetSpacing called for "
        << self->DataObject->GetClassName() << " data object.");
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetSpacing called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetCoordinates(
    svtkXMLWriterC* self, int axis, int dataType, void* data, svtkIdType numCoordinates)
  {
    if (!self)
    {
      return;
    }
    if (svtkRectilinearGrid* dataObject = svtkRectilinearGrid::SafeDownCast(self->DataObject))
    {
      // Check the axis number.
      if (axis < 0 || axis > 2)
      {
        svtkGenericWarningMacro("svtkXMLWriterC_SetCoordinates called with invalid axis "
          << axis << ".  Use 0 for X, 1 for Y, and 2 for Z.");
      }

      // Create the svtkDataArray that will reference the coordinates.
      if (svtkSmartPointer<svtkDataArray> array = svtkXMLWriterC_NewDataArray(
            "SetCoordinates", nullptr, dataType, data, numCoordinates, 1))
      {
        switch (axis)
        {
          case 0:
            dataObject->SetXCoordinates(array);
            break;
          case 1:
            dataObject->SetYCoordinates(array);
            break;
          case 2:
            dataObject->SetZCoordinates(array);
            break;
        }
      }
    }
    else if (self->DataObject)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetCoordinates called for "
        << self->DataObject->GetClassName() << " data object.");
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetCoordinates called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetCellsWithType(
    svtkXMLWriterC* self, int cellType, svtkIdType ncells, svtkIdType* cells, svtkIdType cellsSize)
  {
    if (!self)
    {
      return;
    }
    if (svtkPolyData* dataObject = svtkPolyData::SafeDownCast(self->DataObject))
    {
      // Create a cell array to reference the cells.
      if (svtkSmartPointer<svtkCellArray> cellArray =
            svtkXMLWriterC_NewCellArray("SetCellsWithType", ncells, cells, cellsSize))
      {
        // Store the cell array in the data object.
        if (cellType == SVTK_VERTEX || cellType == SVTK_POLY_VERTEX)
        {
          dataObject->SetVerts(cellArray);
        }
        else if (cellType == SVTK_LINE || cellType == SVTK_POLY_LINE)
        {
          dataObject->SetLines(cellArray);
        }
        else if (cellType == SVTK_TRIANGLE || cellType == SVTK_TRIANGLE_STRIP)
        {
          dataObject->SetStrips(cellArray);
        }
        else // if(cellType == SVTK_POLYGON || cellType == SVTK_QUAD)
        {
          dataObject->SetPolys(cellArray);
        }
      }
    }
    else if (svtkUnstructuredGrid* uGrid = svtkUnstructuredGrid::SafeDownCast(self->DataObject))
    {
      // Create a cell array to reference the cells.
      if (svtkSmartPointer<svtkCellArray> cellArray =
            svtkXMLWriterC_NewCellArray("SetCellsWithType", ncells, cells, cellsSize))
      {
        // Store the cell array in the data object.
        uGrid->SetCells(cellType, cellArray);
      }
    }
    else if (self->DataObject)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetCellsWithType called for "
        << self->DataObject->GetClassName() << " data object.");
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetCellsWithType called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetCellsWithTypes(
    svtkXMLWriterC* self, int* cellTypes, svtkIdType ncells, svtkIdType* cells, svtkIdType cellsSize)
  {
    if (!self)
    {
      return;
    }
    if (svtkUnstructuredGrid* dataObject = svtkUnstructuredGrid::SafeDownCast(self->DataObject))
    {
      // Create a cell array to reference the cells.
      if (svtkSmartPointer<svtkCellArray> cellArray =
            svtkXMLWriterC_NewCellArray("SetCellsWithType", ncells, cells, cellsSize))
      {
        // Store the cell array in the data object.
        dataObject->SetCells(cellTypes, cellArray);
      }
    }
    else if (self->DataObject)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_SetCellsWithTypes called for "
        << self->DataObject->GetClassName() << " data object.");
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetCellsWithTypes called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetPointData(svtkXMLWriterC* self, const char* name, int dataType, void* data,
    svtkIdType numTuples, int numComponents, const char* role)
  {
    svtkXMLWriterC_SetDataInternal(
      self, name, dataType, data, numTuples, numComponents, role, "SetPointData", 1);
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetCellData(svtkXMLWriterC* self, const char* name, int dataType, void* data,
    svtkIdType numTuples, int numComponents, const char* role)
  {
    svtkXMLWriterC_SetDataInternal(
      self, name, dataType, data, numTuples, numComponents, role, "SetCellData", 0);
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetFileName(svtkXMLWriterC* self, const char* fileName)
  {
    if (!self)
    {
      return;
    }
    if (self->Writer)
    {
      // Store the file name.
      self->Writer->SetFileName(fileName);
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetFileName called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  int svtkXMLWriterC_Write(svtkXMLWriterC* self)
  {
    if (!self)
    {
      return 0;
    }
    if (self->Writer)
    {
      // Invoke the writer.
      return self->Writer->Write();
    }
    else
    {
      svtkGenericWarningMacro("svtkXMLWriterC_Write called before svtkXMLWriterC_SetDataObjectType.");
      return 0;
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_SetNumberOfTimeSteps(svtkXMLWriterC* self, int numTimeSteps)
  {
    if (!self)
    {
      return;
    }
    if (self->Writer)
    {
      // Set the number of time steps on the writer.
      self->Writer->SetNumberOfTimeSteps(numTimeSteps);
    }
    else
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_SetNumberOfTimeSteps called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_Start(svtkXMLWriterC* self)
  {
    if (!self)
    {
      return;
    }
    if (self->Writing)
    {
      svtkGenericWarningMacro(
        "svtkXMLWriterC_Start called multiple times without svtkXMLWriterC_Stop.");
    }
    else if (self->Writer)
    {
      // Check the conditions.
      if (self->Writer->GetNumberOfTimeSteps() == 0)
      {
        svtkGenericWarningMacro("svtkXMLWriterC_Start called with no time steps.");
      }
      else if (self->Writer->GetFileName() == nullptr)
      {
        svtkGenericWarningMacro("svtkXMLWriterC_Start called before svtkXMLWriterC_SetFileName.");
      }
      else
      {
        // Tell the writer to start writing.
        self->Writer->Start();
        self->Writing = 1;
      }
    }
    else
    {
      svtkGenericWarningMacro("svtkXMLWriterC_Start called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_WriteNextTimeStep(svtkXMLWriterC* self, double timeValue)
  {
    if (!self)
    {
      return;
    }
    if (!self->Writing)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_WriteNextTimeStep called before svtkXMLWriterC_Start.");
    }
    else if (self->Writer)
    {
      // Tell the writer to write this time step.
      self->Writer->WriteNextTime(timeValue);
    }
    else
    {
      svtkGenericWarningMacro("svtkXMLWriterC_Stop called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

  //----------------------------------------------------------------------------
  void svtkXMLWriterC_Stop(svtkXMLWriterC* self)
  {
    if (!self)
    {
      return;
    }
    if (!self->Writing)
    {
      svtkGenericWarningMacro("svtkXMLWriterC_Stop called before svtkXMLWriterC_Start.");
    }
    else if (self->Writer)
    {
      // Tell the writer to stop writing.
      self->Writer->Stop();
      self->Writing = 0;
    }
    else
    {
      svtkGenericWarningMacro("svtkXMLWriterC_Stop called before svtkXMLWriterC_SetDataObjectType.");
    }
  }

} /* extern "C" */

//----------------------------------------------------------------------------
static svtkSmartPointer<svtkDataArray> svtkXMLWriterC_NewDataArray(const char* method,
  const char* name, int dataType, void* data, svtkIdType numTuples, int numComponents)
{
  // Create the svtkDataArray that will reference the data.
  svtkSmartPointer<svtkDataArray> array = svtkDataArray::CreateDataArray(dataType);
  if (array)
  {
    array->Delete();
  }
  if (!array || array->GetDataType() != dataType)
  {
    svtkGenericWarningMacro(
      "svtkXMLWriterC_" << method << " could not allocate array of type " << dataType << ".");
    return nullptr;
  }

  // Set the number of components.
  array->SetNumberOfComponents(numComponents);

  // Set the name if one was given.
  array->SetName(name);

  // Point the array at the given data.  It is not copied.
  array->SetVoidArray(data, numTuples * numComponents, 1);

  // Return the array.
  return array;
}

//----------------------------------------------------------------------------
static svtkSmartPointer<svtkCellArray> svtkXMLWriterC_NewCellArray(
  const char* method, svtkIdType ncells, svtkIdType* cells, svtkIdType cellsSize)
{
  // Create a svtkIdTypeArray to reference the cells.
  svtkSmartPointer<svtkIdTypeArray> array = svtkSmartPointer<svtkIdTypeArray>::New();
  if (!array)
  {
    svtkGenericWarningMacro("svtkXMLWriterC_" << method << " failed to allocate a svtkIdTypeArray.");
    return nullptr;
  }
  array->SetArray(cells, ncells * cellsSize, 1);

  // Create the cell array.
  svtkSmartPointer<svtkCellArray> cellArray = svtkSmartPointer<svtkCellArray>::New();
  if (!cellArray)
  {
    svtkGenericWarningMacro("svtkXMLWriterC_" << method << " failed to allocate a svtkCellArray.");
    return nullptr;
  }
  cellArray->AllocateExact(ncells, array->GetNumberOfValues() - ncells);
  cellArray->ImportLegacyFormat(array);
  return cellArray;
}

//----------------------------------------------------------------------------
static void svtkXMLWriterC_SetDataInternal(svtkXMLWriterC* self, const char* name, int dataType,
  void* data, svtkIdType numTuples, int numComponents, const char* role, const char* method,
  int isPoints)
{
  if (!self)
  {
    return;
  }
  if (svtkDataSet* dataObject = svtkDataSet::SafeDownCast(self->DataObject))
  {
    if (svtkSmartPointer<svtkDataArray> array =
          svtkXMLWriterC_NewDataArray(method, name, dataType, data, numTuples, numComponents))
    {
      // Store either in point data or cell data.
      svtkDataSetAttributes* dsa;
      if (isPoints)
      {
        dsa = dataObject->GetPointData();
      }
      else
      {
        dsa = dataObject->GetCellData();
      }

      // Store the data array with the requested role.
      if (role && strcmp(role, "SCALARS") == 0)
      {
        dsa->SetScalars(array);
      }
      else if (role && strcmp(role, "VECTORS") == 0)
      {
        dsa->SetVectors(array);
      }
      else if (role && strcmp(role, "NORMALS") == 0)
      {
        dsa->SetNormals(array);
      }
      else if (role && strcmp(role, "TENSORS") == 0)
      {
        dsa->SetTensors(array);
      }
      else if (role && strcmp(role, "TCOORDS") == 0)
      {
        dsa->SetTCoords(array);
      }
      else
      {
        dsa->AddArray(array);
      }
    }
  }
  else if (self->DataObject)
  {
    svtkGenericWarningMacro("svtkXMLWriterC_" << method << " called for "
                                            << self->DataObject->GetClassName() << " data object.");
  }
  else
  {
    svtkGenericWarningMacro(
      "svtkXMLWriterC_" << method << " called before svtkXMLWriterC_SetDataObjectType.");
  }
}
