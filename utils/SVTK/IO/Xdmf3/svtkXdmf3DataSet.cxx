/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3DataSet.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXdmf3DataSet.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkExtractSelection.h"
#include "svtkImageData.h"
#include "svtkMergePoints.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkOutEdgeIterator.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkType.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVertexListIterator.h"
#include "svtkXdmf3ArrayKeeper.h"
#include "svtkXdmf3ArraySelection.h"

// clang-format off
#include "svtk_xdmf3.h"
#include SVTKXDMF3_HEADER(core/XdmfArrayType.hpp)
#include SVTKXDMF3_HEADER(XdmfAttribute.hpp)
#include SVTKXDMF3_HEADER(XdmfAttributeCenter.hpp)
#include SVTKXDMF3_HEADER(XdmfAttributeType.hpp)
#include SVTKXDMF3_HEADER(XdmfCurvilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfDomain.hpp)
#include SVTKXDMF3_HEADER(XdmfGeometry.hpp)
#include SVTKXDMF3_HEADER(XdmfGeometryType.hpp)
#include SVTKXDMF3_HEADER(XdmfGraph.hpp)
#include SVTKXDMF3_HEADER(XdmfRectilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfRegularGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfSet.hpp)
#include SVTKXDMF3_HEADER(XdmfSetType.hpp)
#include SVTKXDMF3_HEADER(XdmfUnstructuredGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfTime.hpp)
#include SVTKXDMF3_HEADER(XdmfTopology.hpp)
#include SVTKXDMF3_HEADER(XdmfTopologyType.hpp)
// clang-format on

//==============================================================================
bool svtkXdmf3DataSet_ReadIfNeeded(XdmfArray* array, bool dbg = false)
{
  if (!array->isInitialized())
  {
    if (dbg)
    {
      cerr << "READ " << array << endl;
    }
    array->read();
    return true;
  }
  return false;
}

void svtkXdmf3DataSet_ReleaseIfNeeded(XdmfArray* array, bool MyInit, bool dbg = false)
{
  if (MyInit)
  {
    if (dbg)
    {
      cerr << "RELEASE " << array << endl;
    }
    // array->release(); //reader level uses svtkXdmfArrayKeeper to aggregate now
  }
}

//==============================================================================
svtkDataArray* svtkXdmf3DataSet::XdmfToSVTKArray(XdmfArray* xArray,
  std::string attrName, // TODO: passing in attrName here, because
                        // XdmfArray::getName() is oddly not virtual
  unsigned int preferredComponents, svtkXdmf3ArrayKeeper* keeper)
{
  // TODO: verify the 32/64 choices are correct in all configurations
  shared_ptr<const XdmfArrayType> arrayType = xArray->getArrayType();
  svtkDataArray* vArray = nullptr;
  int svtk_type = -1;
  if (arrayType == XdmfArrayType::Int8())
  {
    svtk_type = SVTK_CHAR;
  }
  else if (arrayType == XdmfArrayType::Int16())
  {
    svtk_type = SVTK_SHORT;
  }
  else if (arrayType == XdmfArrayType::Int32())
  {
    svtk_type = SVTK_INT;
  }
  else if (arrayType == XdmfArrayType::Int64())
  {
    svtk_type = SVTK_LONG;
  }
  // else if (arrayType == XdmfArrayType::UInt64()) UInt64 does not exist
  //{
  // svtk_type = SVTK_LONG;
  //}
  else if (arrayType == XdmfArrayType::Float32())
  {
    svtk_type = SVTK_FLOAT;
  }
  else if (arrayType == XdmfArrayType::Float64())
  {
    svtk_type = SVTK_DOUBLE;
  }
  else if (arrayType == XdmfArrayType::UInt8())
  {
    svtk_type = SVTK_UNSIGNED_CHAR;
  }
  else if (arrayType == XdmfArrayType::UInt16())
  {
    svtk_type = SVTK_UNSIGNED_SHORT;
  }
  else if (arrayType == XdmfArrayType::UInt32())
  {
    svtk_type = SVTK_UNSIGNED_INT;
  }
  else if (arrayType == XdmfArrayType::String())
  {
    svtk_type = SVTK_STRING;
  }
  else
  {
    cerr << "Skipping unrecognized array type [" << arrayType->getName() << "]" << endl;
    return nullptr;
  }
  vArray = svtkDataArray::CreateDataArray(svtk_type);
  if (vArray)
  {
    vArray->SetName(attrName.c_str());

    std::vector<unsigned int> dims = xArray->getDimensions();
    unsigned int ndims = static_cast<unsigned int>(dims.size());
    unsigned int ncomp = preferredComponents;
    if (preferredComponents == 0) // caller doesn't know what to expect,
    {
      ncomp = 1; // 1 is a safe bet
      if (ndims > 1)
      {
        // use last xdmf dim
        ncomp = dims[ndims - 1];
      }
    }
    unsigned int ntuples = xArray->getSize() / ncomp;

    vArray->SetNumberOfComponents(static_cast<int>(ncomp));
    vArray->SetNumberOfTuples(ntuples);
    bool freeMe = svtkXdmf3DataSet_ReadIfNeeded(xArray);
#define DO_DEEPREAD 0
#if DO_DEEPREAD
    // deepcopy
    switch (vArray->GetDataType())
    {
      svtkTemplateMacro(
        xArray->getValues(0, static_cast<SVTK_TT*>(vArray->GetVoidPointer(0)), ntuples * ncomp););
      default:
        cerr << "UNKNOWN" << endl;
    }
#else
    // shallowcopy
    vArray->SetVoidArray(xArray->getValuesInternal(), ntuples * ncomp, 1);
    if (keeper && freeMe)
    {
      keeper->Insert(xArray);
    }
#endif

    /*
        cerr
          << xArray << " " << xArray->getValuesInternal() << " "
          << vArray->GetVoidPointer(0) << " " << ntuples << " "
          << vArray << " " << vArray->GetName() << endl;
    */
    svtkXdmf3DataSet_ReleaseIfNeeded(xArray, freeMe);
  }
  return vArray;
}

//--------------------------------------------------------------------------
bool svtkXdmf3DataSet::SVTKToXdmfArray(
  svtkDataArray* vArray, XdmfArray* xArray, unsigned int rank, unsigned int* dims)
{
  std::vector<unsigned int> xdims;
  if (rank == 0)
  {
    xdims.push_back(vArray->GetNumberOfTuples());
  }
  else
  {
    for (unsigned int i = 0; i < rank; i++)
    {
      xdims.push_back(dims[i]);
    }
  }
  // add additional dimension to the xdmf array to match the svtk array's width,
  // ex coordinate arrays have xyz, so add [3]
  unsigned int ncomp = static_cast<unsigned int>(vArray->GetNumberOfComponents());
  if (ncomp != 1)
  {
    xdims.push_back(ncomp);
  }

  if (vArray->GetName())
  {
    xArray->setName(vArray->GetName());
  }

#define DO_DEEPWRITE 1
  // TODO: verify the 32/64 choices are correct in all configurations
  switch (vArray->GetDataType())
  {
    case SVTK_VOID:
      return false;
    case SVTK_BIT:
      return false;
    case SVTK_CHAR:
    case SVTK_SIGNED_CHAR:
      xArray->initialize(XdmfArrayType::Int8(), xdims);
#if DO_DEEPWRITE
      // deepcopy
      xArray->insert(0, static_cast<char*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      // shallowcopy
      xArray->setValuesInternal(
        static_cast<char*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_UNSIGNED_CHAR:
      xArray->initialize(XdmfArrayType::UInt8(), xdims);
#if DO_DEEPWRITE
      xArray->insert(
        0, static_cast<unsigned char*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<unsigned char*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_SHORT:
      xArray->initialize(XdmfArrayType::Int16(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0, static_cast<short*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<short*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_UNSIGNED_SHORT:
      xArray->initialize(XdmfArrayType::UInt16(), xdims);
#if DO_DEEPWRITE
      xArray->insert(
        0, static_cast<unsigned short*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<unsigned short*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_INT:
      xArray->initialize(XdmfArrayType::Int32(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0, static_cast<int*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<int*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_UNSIGNED_INT:
      xArray->initialize(XdmfArrayType::UInt32(), xdims);
#if DO_DEEPWRITE
      xArray->insert(
        0, static_cast<unsigned int*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<unsigned int*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_LONG:
      xArray->initialize(XdmfArrayType::Int64(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0, static_cast<long*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<long*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_UNSIGNED_LONG:
      //  arrayType = XdmfArrayType::UInt64(); UInt64 does not exist
      return false;
    case SVTK_FLOAT:
      xArray->initialize(XdmfArrayType::Float32(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0, static_cast<float*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<float*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_DOUBLE:
      xArray->initialize(XdmfArrayType::Float64(), xdims);
#if DO_DEEPWRITE
      xArray->insert(0, static_cast<double*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
      xArray->setValuesInternal(
        static_cast<double*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      break;
    case SVTK_ID_TYPE:
      if (SVTK_SIZEOF_ID_TYPE == XdmfArrayType::Int64()->getElementSize())
      {
        xArray->initialize(XdmfArrayType::Int64(), xdims);
#if DO_DEEPWRITE
        xArray->insert(0, static_cast<long*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
        xArray->setValuesInternal(
          static_cast<long*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      }
      else
      {
        xArray->initialize(XdmfArrayType::Int32(), xdims);
#if DO_DEEPWRITE
        xArray->insert(0, static_cast<int*>(vArray->GetVoidPointer(0)), vArray->GetDataSize());
#else
        xArray->setValuesInternal(
          static_cast<int*>(vArray->GetVoidPointer(0)), vArray->GetDataSize(), false);
#endif
      }
      break;
    case SVTK_STRING:
      return false;
      // TODO: what is correct syntax here?
      // xArray->initialize(XdmfArrayType::String(), xdims);
      // xArray->setValuesInternal(
      //  static_cast<std::string>(vArray->GetVoidPointer(0)),
      //  vArray->GetDataSize(),
      //  false);
      // break;
    case SVTK_OPAQUE:
    case SVTK_LONG_LONG:
    case SVTK_UNSIGNED_LONG_LONG:
#if !defined(SVTK_LEGACY_REMOVE)
    case SVTK___INT64:
    case SVTK_UNSIGNED___INT64:
#endif
    case SVTK_VARIANT:
    case SVTK_OBJECT:
    case SVTK_UNICODE_STRING:
      return false;
    default:
      cerr << "Unrecognized svtk_type";
      return false;
  }

  return true;
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::XdmfToSVTKAttributes(svtkXdmf3ArraySelection* fselection,
  svtkXdmf3ArraySelection* cselection, svtkXdmf3ArraySelection* pselection, XdmfGrid* grid,
  svtkDataObject* dObject, svtkXdmf3ArrayKeeper* keeper)
{
  svtkDataSet* dataSet = svtkDataSet::SafeDownCast(dObject);
  if (!dataSet)
  {
    return;
  }
  unsigned int numCells = dataSet->GetNumberOfCells();
  unsigned int numPoints = dataSet->GetNumberOfPoints();
  unsigned int numAttributes = grid->getNumberAttributes();
  for (unsigned int cc = 0; cc < numAttributes; cc++)
  {
    shared_ptr<XdmfAttribute> xmfAttribute = grid->getAttribute(cc);
    std::string attrName = xmfAttribute->getName();
    if (attrName.length() == 0)
    {
      cerr << "Skipping unnamed array." << endl;
      continue;
    }

    // figure out how many components in this array
    std::vector<unsigned int> dims = xmfAttribute->getDimensions();
    unsigned int ndims = static_cast<unsigned int>(dims.size());
    unsigned int nvals = 1;
    for (unsigned int i = 0; i < dims.size(); i++)
    {
      nvals = nvals * dims[i];
    }

    unsigned int ncomp = 1;

    svtkFieldData* fieldData = 0;

    shared_ptr<const XdmfAttributeCenter> attrCenter = xmfAttribute->getCenter();
    if (attrCenter == XdmfAttributeCenter::Grid())
    {
      if (!fselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetFieldData();
      ncomp = dims[ndims - 1];
    }
    else if (attrCenter == XdmfAttributeCenter::Cell())
    {
      if (!cselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      if (numCells == 0)
      {
        continue;
      }
      fieldData = dataSet->GetCellData();
      ncomp = nvals / numCells;
    }
    else if (attrCenter == XdmfAttributeCenter::Node())
    {
      if (!pselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      if (numPoints == 0)
      {
        continue;
      }
      fieldData = dataSet->GetPointData();
      ncomp = nvals / numPoints;
    }
    else if (attrCenter == XdmfAttributeCenter::Other() &&
      xmfAttribute->getItemType() == "FiniteElementFunction")
    {
      if (!pselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      if (numPoints == 0)
      {
        continue;
      }
    }
    else
    {
      cerr << "skipping " << attrName << " unrecognized association" << endl;
      continue; // unhandled.
    }
    svtkDataSetAttributes* fdAsDSA = svtkDataSetAttributes::SafeDownCast(fieldData);

    shared_ptr<const XdmfAttributeType> attrType = xmfAttribute->getType();
    enum vAttType
    {
      NONE,
      SCALAR,
      VECTOR,
      TENSOR,
      MATRIX,
      TENSOR6,
      GLOBALID
    };
    int atype = NONE;
    if (attrType == XdmfAttributeType::Scalar() && ncomp == 1)
    {
      atype = SCALAR;
    }
    else if (attrType == XdmfAttributeType::Vector() && ncomp == 3)
    {
      atype = VECTOR;
    }
    else if (attrType == XdmfAttributeType::Tensor() && ncomp == 9)
    {
      atype = TENSOR;
    }
    else if (attrType == XdmfAttributeType::Matrix())
    {
      atype = MATRIX;
    }
    else if (attrType == XdmfAttributeType::Tensor6())
    {
      atype = TENSOR6;
    }
    else if (attrType == XdmfAttributeType::GlobalId() && ncomp == 1)
    {
      atype = GLOBALID;
    }

    svtkDataArray* array = XdmfToSVTKArray(xmfAttribute.get(), attrName, ncomp, keeper);

    if (xmfAttribute->getItemType() == "FiniteElementFunction")
    {
      ParseFiniteElementFunction(dObject, xmfAttribute, array, grid);
    }
    else if (array)
    {

      fieldData->AddArray(array);

      if (fdAsDSA)
      {
        switch (atype)
        {
          case SCALAR:
            if (!fdAsDSA->GetScalars())
            {
              fdAsDSA->SetScalars(array);
            }
            break;
          case VECTOR:
            if (!fdAsDSA->GetVectors())
            {
              fdAsDSA->SetVectors(array);
            }
            break;
          case TENSOR:
            if (!fdAsDSA->GetTensors())
            {
              fdAsDSA->SetTensors(array);
            }
            break;
          case GLOBALID:
            if (!fdAsDSA->GetGlobalIds())
            {
              fdAsDSA->SetGlobalIds(array);
            }
            break;
        }
      }
      array->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::SVTKToXdmfAttributes(svtkDataObject* dObject, XdmfGrid* grid)
{
  svtkDataSet* dataSet = svtkDataSet::SafeDownCast(dObject);
  if (!dataSet)
  {
    return;
  }

  unsigned int FDims[1];
  FDims[0] = dataSet->GetFieldData()->GetNumberOfTuples();
  unsigned int CRank = 3;
  unsigned int CDims[3];
  unsigned int PRank = 3;
  unsigned int PDims[3];
  unsigned int Dims[3];
  int wExtent[6];
  wExtent[0] = 0;
  wExtent[1] = -1;
  svtkImageData* dsAsID = svtkImageData::SafeDownCast(dataSet);
  if (dsAsID)
  {
    dsAsID->GetExtent(wExtent);
  }
  else
  {
    svtkRectilinearGrid* dsAsRG = svtkRectilinearGrid::SafeDownCast(dataSet);
    if (dsAsRG)
    {
      dsAsRG->GetExtent(wExtent);
    }
    else
    {
      svtkStructuredGrid* dsAsSG = svtkStructuredGrid::SafeDownCast(dataSet);
      if (dsAsSG)
      {
        dsAsSG->GetExtent(wExtent);
      }
    }
  }
  if (wExtent[1] > wExtent[0])
  {
    Dims[2] = static_cast<unsigned int>(wExtent[1] - wExtent[0] + 1);
    Dims[1] = static_cast<unsigned int>(wExtent[3] - wExtent[2] + 1);
    Dims[0] = static_cast<unsigned int>(wExtent[5] - wExtent[4] + 1);
    PDims[0] = Dims[0];
    PDims[1] = Dims[1];
    PDims[2] = Dims[2];
    CDims[0] = Dims[0] - 1;
    CDims[1] = Dims[1] - 1;
    CDims[2] = Dims[2] - 1;
  }
  else
  {
    PRank = 1;
    PDims[0] = dataSet->GetNumberOfPoints();
    CRank = 1;
    CDims[0] = dataSet->GetNumberOfCells();
  }

  shared_ptr<const XdmfAttributeCenter> center;
  svtkFieldData* fieldData;
  for (int fa = 0; fa < 3; fa++)
  {
    switch (fa)
    {
      case 0:
        fieldData = dataSet->GetFieldData();
        center = XdmfAttributeCenter::Grid();
        break;
      case 1:
        fieldData = dataSet->GetPointData();
        center = XdmfAttributeCenter::Node();
        break;
      default:
        fieldData = dataSet->GetCellData();
        center = XdmfAttributeCenter::Cell();
    }

    svtkDataSetAttributes* fdAsDSA = svtkDataSetAttributes::SafeDownCast(fieldData);
    int numArrays = fieldData->GetNumberOfArrays();
    for (int cc = 0; cc < numArrays; cc++)
    {
      svtkDataArray* vArray = fieldData->GetArray(cc);
      if (!vArray)
      {
        // We're skipping non-numerical arrays for now because
        // we do not support their serialization in the heavy data file.
        continue;
      }
      std::string attrName = vArray->GetName();
      if (attrName.length() == 0)
      {
        cerr << "Skipping unnamed array." << endl;
        continue;
      }
      shared_ptr<XdmfAttribute> xmfAttribute = XdmfAttribute::New();
      xmfAttribute->setName(attrName);
      xmfAttribute->setCenter(center);
      // TODO: Also use ncomponents to tell xdmf about the other vectors etc
      if (fdAsDSA)
      {
        if (vArray == fdAsDSA->GetScalars())
        {
          xmfAttribute->setType(XdmfAttributeType::Scalar());
        }
        else if (vArray == fdAsDSA->GetVectors())
        {
          xmfAttribute->setType(XdmfAttributeType::Vector());
        }
        else if (vArray == fdAsDSA->GetTensors())
        {
          xmfAttribute->setType(XdmfAttributeType::Tensor());
        }
        else if (vArray == fdAsDSA->GetGlobalIds())
        {
          xmfAttribute->setType(XdmfAttributeType::GlobalId());
        }
      }

      unsigned int rank = 1;
      unsigned int* dims = FDims;
      if (fa == 1)
      {
        rank = PRank;
        dims = PDims;
      }
      else if (fa == 2)
      {
        rank = CRank;
        dims = CDims;
      }
      bool OK = SVTKToXdmfArray(vArray, xmfAttribute.get(), rank, dims);
      if (OK)
      {
        grid->insert(xmfAttribute);
      }
    }
  }
}

//----------------------------------------------------------------------------
unsigned int svtkXdmf3DataSet::GetNumberOfPointsPerCell(int svtk_cell_type, bool& fail)
{
  fail = false;
  switch (svtk_cell_type)
  {
    case SVTK_POLY_VERTEX:
      return 0;
    case SVTK_POLY_LINE:
      return 0;
    case SVTK_POLYGON:
      return 0;
    case SVTK_TRIANGLE:
      return 3;
    case SVTK_QUAD:
      return 4;
    case SVTK_TETRA:
      return 4;
    case SVTK_PYRAMID:
      return 5;
    case SVTK_WEDGE:
      return 6;
    case SVTK_HEXAHEDRON:
      return 8;
    case SVTK_QUADRATIC_EDGE:
      return 3;
    case SVTK_QUADRATIC_TRIANGLE:
      return 6;
    case SVTK_QUADRATIC_QUAD:
      return 8;
    case SVTK_BIQUADRATIC_QUAD:
      return 9;
    case SVTK_QUADRATIC_TETRA:
      return 10;
    case SVTK_QUADRATIC_PYRAMID:
      return 13;
    case SVTK_QUADRATIC_WEDGE:
      return 15;
    case SVTK_BIQUADRATIC_QUADRATIC_WEDGE:
      return 18;
    case SVTK_QUADRATIC_HEXAHEDRON:
      return 20;
    case SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      return 24;
    case SVTK_TRIQUADRATIC_HEXAHEDRON:
      return 24;
  }
  fail = true;
  return 0;
}

//----------------------------------------------------------------------------
int svtkXdmf3DataSet::GetXdmfCellType(int svtkType)
{
  switch (svtkType)
  {
    case SVTK_EMPTY_CELL:
      return 0x0; // XdmfTopologyType::NoTopologyType()
    case SVTK_VERTEX:
    case SVTK_POLY_VERTEX:
      return 0x1; // XdmfTopologyType::Polyvertex()
    case SVTK_LINE:
    case SVTK_POLY_LINE:
      return 0x2; // XdmfTopologyType::Polyline()
    case SVTK_TRIANGLE:
    case SVTK_TRIANGLE_STRIP:
      return 0x4; // XdmfTopologyType::Triangle()
    case SVTK_POLYGON:
      return 0x3; // XdmfTopologyType::Polygon()
    case SVTK_PIXEL:
    case SVTK_QUAD:
      return 0x5; // XdmfTopologyType::Quadrilateral()
    case SVTK_TETRA:
      return 0x6; // XdmfTopologyType::Tetrahedron()
    case SVTK_VOXEL:
    case SVTK_HEXAHEDRON:
      return 0x9; // XdmfTopologyType::Hexahedron()
    case SVTK_WEDGE:
      return 0x8; // XdmfTopologyType::Wedge()
    case SVTK_PYRAMID:
      return 0x7; // XdmfTopologyType::Pyramid()
    case SVTK_POLYHEDRON:
      return 0x10; // XdmfTopologyType::Polyhedron()
    case SVTK_PENTAGONAL_PRISM:
    case SVTK_HEXAGONAL_PRISM:
    case SVTK_QUADRATIC_EDGE:
    case SVTK_QUADRATIC_TRIANGLE:
    case SVTK_QUADRATIC_QUAD:
    case SVTK_QUADRATIC_TETRA:
    case SVTK_QUADRATIC_HEXAHEDRON:
    case SVTK_QUADRATIC_WEDGE:
    case SVTK_QUADRATIC_PYRAMID:
    case SVTK_BIQUADRATIC_QUAD:
    case SVTK_TRIQUADRATIC_HEXAHEDRON:
    case SVTK_QUADRATIC_LINEAR_QUAD:
    case SVTK_QUADRATIC_LINEAR_WEDGE:
    case SVTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    case SVTK_BIQUADRATIC_TRIANGLE:
    case SVTK_CUBIC_LINE:
    case SVTK_CONVEX_POINT_SET:
    case SVTK_PARAMETRIC_CURVE:
    case SVTK_PARAMETRIC_SURFACE:
    case SVTK_PARAMETRIC_TRI_SURFACE:
    case SVTK_PARAMETRIC_QUAD_SURFACE:
    case SVTK_PARAMETRIC_TETRA_REGION:
    case SVTK_PARAMETRIC_HEX_REGION:
    case SVTK_HIGHER_ORDER_EDGE:
    case SVTK_HIGHER_ORDER_TRIANGLE:
    case SVTK_HIGHER_ORDER_QUAD:
    case SVTK_HIGHER_ORDER_POLYGON:
    case SVTK_HIGHER_ORDER_TETRAHEDRON:
    case SVTK_HIGHER_ORDER_WEDGE:
    case SVTK_HIGHER_ORDER_PYRAMID:
    case SVTK_HIGHER_ORDER_HEXAHEDRON:
      cerr << "I do not know how to make that xdmf cell type" << endl;
      // TODO: see list below
      return -1;
    default:
      cerr << "Unknown svtk cell type" << endl;
      return -1;
  }

  /*
  0x22; //XdmfTopologyType::Edge_3() ));
  0x24; //XdmfTopologyType::Triangle_6() ));
  0x25; //XdmfTopologyType::Quadrilateral_8() 0x25));
  0x23; //XdmfTopologyType::Quadrilateral_9() 0x23));
  0x25; //XdmfTopologyType::Tetrahedron_10() 0x26));
  0x27; //XdmfTopologyType::Pyramid_13() 0x27));
  0x28; //XdmfTopologyType::Wedge_15() ));
  0x29; //XdmfTopologyType::Wedge_18() 0x29));
  0x30; //XdmfTopologyType::Hexahedron_20() 0x30));
  0x31; //XdmfTopologyType::Hexahedron_24() 0x31));
  0x32; //XdmfTopologyType::Hexahedron_27() 0x32));
  0x33; //XdmfTopologyType::Hexahedron_64() 0x33));
  0x34; //XdmfTopologyType::Hexahedron_125() 0x34));
  0x35; //XdmfTopologyType::Hexahedron_216()
  0x36; //XdmfTopologyType::Hexahedron_343()
  0x37; //XdmfTopologyType::Hexahedron_512()
  0x38; //XdmfTopologyType::Hexahedron_729()
  0x39; //XdmfTopologyType::Hexahedron_1000()
  0x40; //XdmfTopologyType::Hexahedron_1331()
  0x41; //XdmfTopologyType::Hexahedron_Spectral_64()
  0x42; //XdmfTopologyType::Hexahedron_Spectral_125()
  0x43; //XdmfTopologyType::Hexahedron_Spectral_216()
  0x44; //XdmfTopologyType::Hexahedron_Spectral_343()
  0x45; //XdmfTopologyType::Hexahedron_Spectral_512()
  0x46; //XdmfTopologyType::Hexahedron_Spectral_729()
  0x47; //XdmfTopologyType::Hexahedron_Spectral_1000()
  0x48; //XdmfTopologyType::Hexahedron_Spectral_1331()
  0x70; //XdmfTopologyType::Mixed()
  */
}

//----------------------------------------------------------------------------
int svtkXdmf3DataSet::GetSVTKCellType(shared_ptr<const XdmfTopologyType> topologyType)
{
  // TODO: examples to test/demonstrate each of these
  if (topologyType == XdmfTopologyType::Polyvertex())
  {
    return SVTK_POLY_VERTEX;
  }
  if (topologyType->getName() == XdmfTopologyType::Polyline(0)->getName())
  {
    return SVTK_POLY_LINE;
  }
  if (topologyType->getName() == XdmfTopologyType::Polygon(0)->getName())
  {
    return SVTK_POLYGON; // FIXME: should this not be treated as mixed?
  }
  if (topologyType == XdmfTopologyType::Triangle())
  {
    return SVTK_TRIANGLE;
  }
  if (topologyType == XdmfTopologyType::Quadrilateral())
  {
    return SVTK_QUAD;
  }
  if (topologyType == XdmfTopologyType::Tetrahedron())
  {
    return SVTK_TETRA;
  }
  if (topologyType == XdmfTopologyType::Pyramid())
  {
    return SVTK_PYRAMID;
  }
  if (topologyType == XdmfTopologyType::Wedge())
  {
    return SVTK_WEDGE;
  }
  if (topologyType == XdmfTopologyType::Hexahedron())
  {
    return SVTK_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Edge_3())
  {
    return SVTK_QUADRATIC_EDGE;
  }
  if (topologyType == XdmfTopologyType::Triangle_6())
  {
    return SVTK_QUADRATIC_TRIANGLE;
  }
  if (topologyType == XdmfTopologyType::Quadrilateral_8())
  {
    return SVTK_QUADRATIC_QUAD;
  }
  if (topologyType == XdmfTopologyType::Quadrilateral_9())
  {
    return SVTK_BIQUADRATIC_QUAD;
  }
  if (topologyType == XdmfTopologyType::Tetrahedron_10())
  {
    return SVTK_QUADRATIC_TETRA;
  }
  if (topologyType == XdmfTopologyType::Pyramid_13())
  {
    return SVTK_QUADRATIC_PYRAMID;
  }
  if (topologyType == XdmfTopologyType::Wedge_15())
  {
    return SVTK_QUADRATIC_WEDGE;
  }
  if (topologyType == XdmfTopologyType::Wedge_18())
  {
    return SVTK_BIQUADRATIC_QUADRATIC_WEDGE;
  }
  if (topologyType == XdmfTopologyType::Hexahedron_20())
  {
    return SVTK_QUADRATIC_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Hexahedron_24())
  {
    return SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Hexahedron_27())
  {
    return SVTK_TRIQUADRATIC_HEXAHEDRON;
  }
  if (topologyType == XdmfTopologyType::Polyhedron())
  {
    return SVTK_POLYHEDRON;
  }
  /* TODO: Translate these new XDMF cell types
  static shared_ptr<const XdmfTopologyType> Hexahedron_64();
  static shared_ptr<const XdmfTopologyType> Hexahedron_125();
  static shared_ptr<const XdmfTopologyType> Hexahedron_216();
  static shared_ptr<const XdmfTopologyType> Hexahedron_343();
  static shared_ptr<const XdmfTopologyType> Hexahedron_512();
  static shared_ptr<const XdmfTopologyType> Hexahedron_729();
  static shared_ptr<const XdmfTopologyType> Hexahedron_1000();
  static shared_ptr<const XdmfTopologyType> Hexahedron_1331();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_64();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_125();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_216();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_343();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_512();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_729();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_1000();
  static shared_ptr<const XdmfTopologyType> Hexahedron_Spectral_1331();
  */
  if (topologyType == XdmfTopologyType::Mixed())
  {
    return SVTK_NUMBER_OF_CELL_TYPES;
  }

  return SVTK_EMPTY_CELL;
}
//==========================================================================
int svtkXdmf3DataSet::GetSVTKFiniteElementCellType(unsigned int element_degree,
  const std::string& element_family, shared_ptr<const XdmfTopologyType> topologyType)
{
  // Linear geometry and linear or constant function
  // isoparametric element
  if (topologyType == XdmfTopologyType::Triangle() &&
    (element_degree == 1 || element_degree == 0) &&
    (element_family == "CG" || element_family == "DG"))
  {
    return SVTK_TRIANGLE;
  }

  // Linear or quadratic geometry and quadratic function
  // isoparametric and superparametric element
  if ((topologyType == XdmfTopologyType::Triangle() ||
        topologyType == XdmfTopologyType::Triangle_6()) &&
    element_degree == 2 && (element_family == "CG" || element_family == "DG"))
  {
    return SVTK_QUADRATIC_TRIANGLE;
  }

  // Quadratic geometry and linear or const function
  // subparametric element
  if (topologyType == XdmfTopologyType::Triangle_6() &&
    (element_degree == 1 || element_degree == 0) &&
    (element_family == "CG" || element_family == "DG"))
  {
    return SVTK_QUADRATIC_TRIANGLE;
  }

  // Linear geometry and linear or constant function
  // isoparametric element
  if (topologyType == XdmfTopologyType::Tetrahedron() &&
    (element_degree == 1 || element_degree == 0) &&
    (element_family == "CG" || element_family == "DG"))
  {
    return SVTK_TETRA;
  }

  // Linear or quadratic geometry and quadratic function
  // isoparametric and superparametric element
  if ((topologyType == XdmfTopologyType::Tetrahedron() ||
        topologyType == XdmfTopologyType::Tetrahedron_10()) &&
    element_degree == 2 && (element_family == "CG" || element_family == "DG"))
  {
    return SVTK_QUADRATIC_TETRA;
  }

  // Linear geometry and linear or const function
  // isoparametric element
  if (topologyType == XdmfTopologyType::Quadrilateral() &&
    (element_degree == 1 || element_degree == 0) &&
    (element_family == "Q" || element_family == "DQ"))
  {
    return SVTK_QUAD;
  }

  // Linear geometry and quadratic function
  // superparametric element
  if (topologyType == XdmfTopologyType::Quadrilateral() && (element_degree == 2) &&
    (element_family == "Q" || element_family == "DQ"))
  {
    return SVTK_BIQUADRATIC_QUAD;
  }

  // Linear geometry and Raviart-Thomas
  if (topologyType == XdmfTopologyType::Triangle() && element_degree == 1 && element_family == "RT")
  {
    return SVTK_TRIANGLE;
  }

  // Linear geometry and higher order function
  if (topologyType == XdmfTopologyType::Triangle() && element_degree >= 3 &&
    (element_family == "CG" || element_family == "DG"))
  {
    return SVTK_TRIANGLE;
  }

  cerr << "Finite element function of family " << element_family
       << " and "
          "degree "
       << std::to_string(element_degree) << " on " << topologyType->getName()
       << " is not supported." << endl;
  return 0;
}
//==========================================================================
void svtkXdmf3DataSet::SetTime(XdmfGrid* grid, double hasTime, double time)
{
  if (hasTime)
  {
    grid->setTime(XdmfTime::New(time));
  }
}
//==========================================================================
void svtkXdmf3DataSet::SetTime(XdmfGraph* graph, double hasTime, double time)
{
  if (hasTime)
  {
    graph->setTime(XdmfTime::New(time));
  }
}

//==========================================================================

void svtkXdmf3DataSet::XdmfToSVTK(svtkXdmf3ArraySelection* fselection,
  svtkXdmf3ArraySelection* cselection, svtkXdmf3ArraySelection* pselection, XdmfRegularGrid* grid,
  svtkImageData* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  svtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  svtkXdmf3DataSet::XdmfToSVTKAttributes(fselection, cselection, pselection, grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::CopyShape(
  XdmfRegularGrid* grid, svtkImageData* dataSet, svtkXdmf3ArrayKeeper* svtkNotUsed(keeper))
{
  if (!dataSet)
  {
    return;
  }

  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;

  shared_ptr<XdmfArray> xdims = grid->getDimensions();
  if (xdims)
  {
    bool freeMe = svtkXdmf3DataSet_ReadIfNeeded(xdims.get());
    for (unsigned int i = 0; (i < 3 && i < xdims->getSize()); i++)
    {
      whole_extent[(2 - i) * 2 + 1] = xdims->getValue<int>(i) - 1;
    }
    if (xdims->getSize() == 2)
    {
      whole_extent[1] = whole_extent[0];
    }
    svtkXdmf3DataSet_ReleaseIfNeeded(xdims.get(), freeMe);
  }
  dataSet->SetExtent(whole_extent);

  double origin[3];
  origin[0] = 0.0;
  origin[1] = 0.0;
  origin[2] = 0.0;
  shared_ptr<XdmfArray> xorigin = grid->getOrigin();
  if (xorigin)
  {
    bool freeMe = svtkXdmf3DataSet_ReadIfNeeded(xorigin.get());
    for (unsigned int i = 0; (i < 3 && i < xorigin->getSize()); i++)
    {
      origin[2 - i] = xorigin->getValue<double>(i);
    }
    svtkXdmf3DataSet_ReleaseIfNeeded(xorigin.get(), freeMe);
  }
  dataSet->SetOrigin(origin);

  double spacing[3];
  spacing[0] = 1.0;
  spacing[1] = 1.0;
  spacing[2] = 1.0;
  shared_ptr<XdmfArray> xspacing;
  xspacing = grid->getBrickSize();
  if (xspacing)
  {
    bool freeMe = svtkXdmf3DataSet_ReadIfNeeded(xspacing.get());
    for (unsigned int i = 0; (i < 3 && i < xspacing->getSize()); i++)
    {
      spacing[2 - i] = xspacing->getValue<double>(i);
    }
    svtkXdmf3DataSet_ReleaseIfNeeded(xspacing.get(), freeMe);
  }

  dataSet->SetSpacing(spacing);
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::SVTKToXdmf(
  svtkImageData* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name)
{
  int whole_extent[6];
  dataSet->GetExtent(whole_extent);
  double bounds[6];
  dataSet->GetBounds(bounds);
  double origin[3];
  origin[0] = bounds[0];
  origin[1] = bounds[2];
  origin[2] = bounds[4];
  double spacing[3];
  dataSet->GetSpacing(spacing);
  unsigned int dims[3];
  dims[0] = static_cast<unsigned int>(whole_extent[1] - whole_extent[0] + 1);
  dims[1] = static_cast<unsigned int>(whole_extent[3] - whole_extent[2] + 1);
  dims[2] = static_cast<unsigned int>(whole_extent[5] - whole_extent[4] + 1);
  shared_ptr<XdmfRegularGrid> grid = XdmfRegularGrid::New(
    spacing[2], spacing[1], spacing[0], dims[2], dims[1], dims[0], origin[2], origin[1], origin[0]);
  if (name)
  {
    grid->setName(std::string(name));
  }

  svtkXdmf3DataSet::SVTKToXdmfAttributes(dataSet, grid.get());
  svtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================

void svtkXdmf3DataSet::XdmfToSVTK(svtkXdmf3ArraySelection* fselection,
  svtkXdmf3ArraySelection* cselection, svtkXdmf3ArraySelection* pselection, XdmfRectilinearGrid* grid,
  svtkRectilinearGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  svtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  svtkXdmf3DataSet::XdmfToSVTKAttributes(fselection, cselection, pselection, grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::CopyShape(
  XdmfRectilinearGrid* grid, svtkRectilinearGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (!dataSet)
  {
    return;
  }

  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;

  shared_ptr<XdmfArray> xdims;
  xdims = grid->getDimensions();
  // Note: XDMF standard for RECTMESH is inconsistent with SMESH and CORECTMESH
  // it is ijk in SVTK terms and they are kji.
  if (xdims)
  {
    bool freeMe = svtkXdmf3DataSet_ReadIfNeeded(xdims.get());
    for (unsigned int i = 0; (i < 3 && i < xdims->getSize()); i++)
    {
      whole_extent[i * 2 + 1] = xdims->getValue<int>(i) - 1;
    }
    if (xdims->getSize() == 2)
    {
      whole_extent[5] = whole_extent[4];
    }
    svtkXdmf3DataSet_ReleaseIfNeeded(xdims.get(), freeMe);
  }
  dataSet->SetExtent(whole_extent);

  shared_ptr<XdmfArray> xCoords;

  xCoords = grid->getCoordinates(0);
  svtkDataArray* vCoords =
    svtkXdmf3DataSet::XdmfToSVTKArray(xCoords.get(), xCoords->getName(), 1, keeper);
  dataSet->SetXCoordinates(vCoords);
  if (vCoords)
  {
    vCoords->Delete();
  }

  xCoords = grid->getCoordinates(1);
  vCoords = svtkXdmf3DataSet::XdmfToSVTKArray(xCoords.get(), xCoords->getName(), 1, keeper);
  dataSet->SetYCoordinates(vCoords);
  if (vCoords)
  {
    vCoords->Delete();
  }

  if (xdims->getSize() > 2)
  {
    xCoords = grid->getCoordinates(2);
    vCoords = svtkXdmf3DataSet::XdmfToSVTKArray(xCoords.get(), xCoords->getName(), 1, keeper);
    dataSet->SetZCoordinates(vCoords);
    if (vCoords)
    {
      vCoords->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::SVTKToXdmf(
  svtkRectilinearGrid* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name)
{
  shared_ptr<XdmfArray> xXCoords = XdmfArray::New();
  shared_ptr<XdmfArray> xYCoords = XdmfArray::New();
  shared_ptr<XdmfArray> xZCoords = XdmfArray::New();

  bool OK = true;
  svtkDataArray* vCoords = dataSet->GetXCoordinates();
  OK &= svtkXdmf3DataSet::SVTKToXdmfArray(vCoords, xXCoords.get());
  if (OK)
  {
    vCoords = dataSet->GetYCoordinates();
    OK &= svtkXdmf3DataSet::SVTKToXdmfArray(vCoords, xYCoords.get());
    if (OK)
    {
      vCoords = dataSet->GetZCoordinates();
      OK &= svtkXdmf3DataSet::SVTKToXdmfArray(vCoords, xZCoords.get());
    }
  }

  if (!OK)
  {
    return;
  }

  shared_ptr<XdmfRectilinearGrid> grid = XdmfRectilinearGrid::New(xXCoords, xYCoords, xZCoords);

  if (name)
  {
    grid->setName(std::string(name));
  }

  svtkXdmf3DataSet::SVTKToXdmfAttributes(dataSet, grid.get());
  svtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================

void svtkXdmf3DataSet::XdmfToSVTK(svtkXdmf3ArraySelection* fselection,
  svtkXdmf3ArraySelection* cselection, svtkXdmf3ArraySelection* pselection, XdmfCurvilinearGrid* grid,
  svtkStructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  svtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  svtkXdmf3DataSet::XdmfToSVTKAttributes(fselection, cselection, pselection, grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::CopyShape(
  XdmfCurvilinearGrid* grid, svtkStructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (!dataSet)
  {
    return;
  }

  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;
  shared_ptr<XdmfArray> xdims;
  xdims = grid->getDimensions();
  if (xdims)
  {
    for (unsigned int i = 0; (i < 3 && i < xdims->getSize()); i++)
    {
      whole_extent[(2 - i) * 2 + 1] = xdims->getValue<int>(i) - 1;
    }
  }
  if (xdims->getSize() == 2)
  {
    whole_extent[1] = whole_extent[0];
  }
  dataSet->SetExtent(whole_extent);

  svtkDataArray* vPoints = nullptr;
  shared_ptr<XdmfGeometry> geom = grid->getGeometry();
  if (geom->getType() == XdmfGeometryType::XY())
  {
    vPoints = svtkXdmf3DataSet::XdmfToSVTKArray(geom.get(), "", 2, keeper);
    svtkDataArray* vPoints3 = vPoints->NewInstance();
    vPoints3->SetNumberOfComponents(3);
    vPoints3->SetNumberOfTuples(vPoints->GetNumberOfTuples());
    vPoints3->SetName("");
    vPoints3->CopyComponent(0, vPoints, 0);
    vPoints3->CopyComponent(1, vPoints, 1);
    vPoints3->FillComponent(2, 0.0);
    vPoints->Delete();
    vPoints = vPoints3;
  }
  else if (geom->getType() == XdmfGeometryType::XYZ())
  {
    vPoints = svtkXdmf3DataSet::XdmfToSVTKArray(geom.get(), "", 3, keeper);
  }
  else
  {
    // TODO: No X_Y or X_Y_Z in xdmf anymore
    return;
  }
  svtkPoints* p = svtkPoints::New();
  p->SetData(vPoints);
  dataSet->SetPoints(p);
  p->Delete();
  if (vPoints)
  {
    vPoints->Delete();
  }
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::SVTKToXdmf(
  svtkStructuredGrid* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name)
{
  int whole_extent[6];
  whole_extent[0] = 0;
  whole_extent[1] = -1;
  whole_extent[2] = 0;
  whole_extent[3] = -1;
  whole_extent[4] = 0;
  whole_extent[5] = -1;
  dataSet->GetExtent(whole_extent);
  shared_ptr<XdmfArray> xdims = XdmfArray::New();
  xdims->initialize(XdmfArrayType::Int32());
  if (xdims)
  {
    for (unsigned int i = 0; i < 3; i++)
    {
      int extent = whole_extent[(2 - i) * 2 + 1] - whole_extent[(2 - i) * 2] + 1;
      xdims->pushBack<int>(extent);
    }
  }

  svtkDataArray* vCoords = dataSet->GetPoints()->GetData();
  shared_ptr<XdmfGeometry> xCoords = XdmfGeometry::New();
  bool OK = svtkXdmf3DataSet::SVTKToXdmfArray(vCoords, xCoords.get());
  if (!OK)
  {
    return;
  }
  xCoords->setType(XdmfGeometryType::XYZ());

  shared_ptr<XdmfCurvilinearGrid> grid = XdmfCurvilinearGrid::New(xdims);
  grid->setGeometry(xCoords);

  if (name)
  {
    grid->setName(std::string(name));
  }

  svtkXdmf3DataSet::SVTKToXdmfAttributes(dataSet, grid.get());
  svtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================

void svtkXdmf3DataSet::XdmfToSVTK(svtkXdmf3ArraySelection* fselection,
  svtkXdmf3ArraySelection* cselection, svtkXdmf3ArraySelection* pselection,
  XdmfUnstructuredGrid* grid, svtkUnstructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  svtkXdmf3DataSet::CopyShape(grid, dataSet, keeper);
  svtkXdmf3DataSet::XdmfToSVTKAttributes(fselection, cselection, pselection, grid, dataSet, keeper);
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::CopyShape(
  XdmfUnstructuredGrid* grid, svtkUnstructuredGrid* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (!dataSet)
  {
    return;
  }

  shared_ptr<XdmfTopology> xTopology = grid->getTopology();
  shared_ptr<const XdmfTopologyType> xCellType = xTopology->getType();
  int vCellType = svtkXdmf3DataSet::GetSVTKCellType(xCellType);
  if (vCellType == SVTK_EMPTY_CELL)
  {
    return;
  }

  bool freeMe = svtkXdmf3DataSet_ReadIfNeeded(xTopology.get());

  if (xCellType != XdmfTopologyType::Mixed())
  {
    // all cells are of the same type.
    unsigned int numPointsPerCell = xCellType->getNodesPerElement();

    // translate cell array
    unsigned int numCells = xTopology->getNumberElements();

    int* cell_types = new int[numCells];

    svtkCellArray* vCells = svtkCellArray::New();
    vCells->AllocateEstimate(numCells, numPointsPerCell);

    // xmfConnections: N p1 p2 ... pN
    // i.e. Triangles : 3 0 1 2    3 3 4 5   3 6 7 8
    svtkIdType index = 0;
    for (svtkIdType cc = 0; cc < static_cast<svtkIdType>(numCells); cc++)
    {
      cell_types[cc] = vCellType;
      vCells->InsertNextCell(static_cast<int>(numPointsPerCell));
      for (svtkIdType i = 0; i < static_cast<svtkIdType>(numPointsPerCell); i++)
      {
        vCells->InsertCellPoint(xTopology->getValue<svtkIdType>(index++));
      }
    }
    dataSet->SetCells(cell_types, vCells);
    vCells->Delete();
    svtkXdmf3DataSet_ReleaseIfNeeded(xTopology.get(), freeMe);
    delete[] cell_types;
  }
  else
  {
    // mixed cell types
    unsigned int conn_length = xTopology->getSize();
    svtkIdType numCells = xTopology->getNumberElements();

    int* cell_types = new int[numCells];

    /* Create Cell Array */
    svtkCellArray* vCells = svtkCellArray::New();
    vCells->AllocateExact(numCells, conn_length);

    /* xmfConnections : N p1 p2 ... pN */
    /* i.e. Triangles : 3 0 1 2    3 3 4 5   3 6 7 8 */
    svtkIdType index = 0;
    for (svtkIdType cc = 0; cc < numCells; cc++)
    {
      shared_ptr<const XdmfTopologyType> nextCellType =
        XdmfTopologyType::New(xTopology->getValue<svtkIdType>(index++));
      int svtk_cell_typeI = svtkXdmf3DataSet::GetSVTKCellType(nextCellType);

      if (svtk_cell_typeI != SVTK_POLYHEDRON)
      {
        bool unknownCell;
        unsigned int numPointsPerCell =
          svtkXdmf3DataSet::GetNumberOfPointsPerCell(svtk_cell_typeI, unknownCell);

        if (unknownCell)
        {
          // encountered an unknown cell.
          cerr << "Unknown cell type." << endl;
          vCells->Delete();
          delete[] cell_types;
          svtkXdmf3DataSet_ReleaseIfNeeded(xTopology.get(), freeMe);
          return;
        }

        if (numPointsPerCell == 0)
        {
          // cell type does not have a fixed number of points in which case the
          // next entry in xmfConnections tells us the number of points.
          numPointsPerCell = xTopology->getValue<unsigned int>(index++);
        }

        cell_types[cc] = svtk_cell_typeI;
        vCells->InsertNextCell(static_cast<int>(numPointsPerCell));
        for (svtkIdType i = 0; i < static_cast<svtkIdType>(numPointsPerCell); i++)
        {
          vCells->InsertCellPoint(xTopology->getValue<svtkIdType>(index++));
        }
      }
      else
      {
        // polyhedrons do not have a fixed number of faces in which case the
        // next entry in xmfConnections tells us the number of faces.
        const unsigned int numFacesPerCell = xTopology->getValue<unsigned int>(index++);

        // polyhedrons do not have a fixed number of points in which case the
        // the number of points needs to be obtained from the data.
        unsigned int numPointsPerCell = 0;
        for (svtkIdType i = 0; i < static_cast<svtkIdType>(numFacesPerCell); i++)
        {
          // faces do not have a fixed number of points in which case the next
          // entry in xmfConnections tells us the number of points.
          numPointsPerCell += xTopology->getValue<unsigned int>(index + numPointsPerCell + i);
        }

        // add cell entry to the array, which for polyhedrons is in the format:
        // [cellLength, nCellFaces, nFace0Pts, id0_0, id0_1, ...,
        //                          nFace1Pts, id1_0, id1_1, ...,
        //                          ...]
        cell_types[cc] = svtk_cell_typeI;
        vCells->InsertNextCell(static_cast<int>(numPointsPerCell + numFacesPerCell + 1));
        vCells->InsertCellPoint(static_cast<svtkIdType>(numFacesPerCell));
        for (svtkIdType i = 0; i < static_cast<svtkIdType>(numPointsPerCell + numFacesPerCell); i++)
        {
          vCells->InsertCellPoint(xTopology->getValue<svtkIdType>(index++));
        }
      }
    }

    dataSet->SetCells(cell_types, vCells);
    vCells->Delete();
    delete[] cell_types;
    svtkXdmf3DataSet_ReleaseIfNeeded(xTopology.get(), freeMe);
  }

  // copy geometry
  svtkDataArray* vPoints = nullptr;
  shared_ptr<XdmfGeometry> geom = grid->getGeometry();
  if (geom->getType() == XdmfGeometryType::XY())
  {
    vPoints = svtkXdmf3DataSet::XdmfToSVTKArray(geom.get(), "", 2, keeper);
    svtkDataArray* vPoints3 = vPoints->NewInstance();
    vPoints3->SetNumberOfComponents(3);
    vPoints3->SetNumberOfTuples(vPoints->GetNumberOfTuples());
    vPoints3->SetName("");
    vPoints3->CopyComponent(0, vPoints, 0);
    vPoints3->CopyComponent(1, vPoints, 1);
    vPoints3->FillComponent(2, 0.0);
    vPoints->Delete();
    vPoints = vPoints3;
  }
  else if (geom->getType() == XdmfGeometryType::XYZ())
  {
    vPoints = svtkXdmf3DataSet::XdmfToSVTKArray(geom.get(), "", 3, keeper);
  }
  else
  {
    // TODO: No X_Y or X_Y_Z in xdmf anymore
    return;
  }

  svtkPoints* p = svtkPoints::New();
  p->SetData(vPoints);
  dataSet->SetPoints(p);
  p->Delete();
  if (vPoints)
  {
    vPoints->Delete();
  }
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::SVTKToXdmf(
  svtkPointSet* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name)
{
  svtkPoints* pts = dataSet->GetPoints();
  if (!pts)
  {
    return;
  }
  svtkDataArray* vCoords = pts->GetData();
  shared_ptr<XdmfGeometry> xCoords = XdmfGeometry::New();
  bool OK = svtkXdmf3DataSet::SVTKToXdmfArray(vCoords, xCoords.get());
  if (!OK)
  {
    return;
  }
  xCoords->setType(XdmfGeometryType::XYZ());

  shared_ptr<XdmfUnstructuredGrid> grid = XdmfUnstructuredGrid::New();
  if (name)
  {
    grid->setName(std::string(name));
  }
  grid->setGeometry(xCoords);

  shared_ptr<XdmfTopology> xTopology = XdmfTopology::New();
  grid->setTopology(xTopology);

  // TODO: homogeneous case in old reader _might_ be faster
  // for simplicity I am treating all dataSets as having mixed cell types
  xTopology->setType(XdmfTopologyType::Mixed());
  svtkIdType numCells = dataSet->GetNumberOfCells();

  // reserve some space
  /*4 = celltype+numids+id0+id1 or celltype+id0+id1+id2*/
  const int PER_CELL_ESTIMATE = 4;
  unsigned int total_estimate = numCells * PER_CELL_ESTIMATE;
  if (SVTK_SIZEOF_ID_TYPE == XdmfArrayType::Int64()->getElementSize())
  {
    xTopology->initialize(XdmfArrayType::Int64(), total_estimate);
  }
  else
  {
    xTopology->initialize(XdmfArrayType::Int32(), total_estimate);
  }

  unsigned int tcount = 0;
  svtkIdType cntr = 0;
  for (svtkIdType cid = 0; cid < numCells; cid++)
  {
    svtkCell* cell = dataSet->GetCell(cid);
    svtkIdType cellType = dataSet->GetCellType(cid);
    svtkIdType numPts = cell->GetNumberOfPoints();
    int xType = svtkXdmf3DataSet::GetXdmfCellType(cellType);
    if (xType != -1)
    {
      xTopology->insert(cntr++, xType);
    }
    tcount += 1;
    switch (cellType)
    {
      case SVTK_VERTEX:
      case SVTK_POLY_VERTEX:
      case SVTK_LINE:
      case SVTK_POLY_LINE:
      case SVTK_POLYGON:
        xTopology->insert(cntr++, static_cast<long>(numPts));
        tcount += 1;
        break;
      default:
        break;
    }
    if (cellType == SVTK_VOXEL)
    {
      // Reinterpret to xdmf's order
      xTopology->insert(cntr++, (int)cell->GetPointId(0));
      xTopology->insert(cntr++, (int)cell->GetPointId(1));
      xTopology->insert(cntr++, (int)cell->GetPointId(3));
      xTopology->insert(cntr++, (int)cell->GetPointId(2));
      xTopology->insert(cntr++, (int)cell->GetPointId(4));
      xTopology->insert(cntr++, (int)cell->GetPointId(5));
      xTopology->insert(cntr++, (int)cell->GetPointId(7));
      xTopology->insert(cntr++, (int)cell->GetPointId(6));
      tcount += 8;
    }
    else if (cellType == SVTK_PIXEL)
    {
      // Reinterpret to xdmf's order
      xTopology->insert(cntr++, (int)cell->GetPointId(0));
      xTopology->insert(cntr++, (int)cell->GetPointId(1));
      xTopology->insert(cntr++, (int)cell->GetPointId(3));
      xTopology->insert(cntr++, (int)cell->GetPointId(2));
      tcount += 4;
    }
    else if (cellType == SVTK_POLYHEDRON)
    {
      // Convert polyhedron to format:
      // [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
      const svtkIdType numFaces = cell->GetNumberOfFaces();
      xTopology->insert(cntr++, static_cast<long>(numFaces));
      tcount += 1;

      svtkIdType fid, pid;
      svtkCell* face;
      for (fid = 0; fid < numFaces; fid++)
      {
        face = cell->GetFace(fid);
        numPts = face->GetNumberOfPoints();
        xTopology->insert(cntr++, static_cast<long>(numPts));
        tcount += 1;
        for (pid = 0; pid < numPts; pid++)
        {
          xTopology->insert(cntr++, (int)face->GetPointId(pid));
        }
        tcount += numPts;
      }
    }
    else
    {
      for (svtkIdType pid = 0; pid < numPts; pid++)
      {
        xTopology->insert(cntr++, (int)cell->GetPointId(pid));
      }
      tcount += numPts;
    }
  }
  xTopology->resize(tcount, 0); // release unused reserved space

  svtkXdmf3DataSet::SVTKToXdmfAttributes(dataSet, grid.get());
  svtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================

void svtkXdmf3DataSet::XdmfToSVTK(svtkXdmf3ArraySelection* fselection,
  svtkXdmf3ArraySelection* cselection, svtkXdmf3ArraySelection* pselection, XdmfGraph* grid,
  svtkMutableDirectedGraph* dataSet, svtkXdmf3ArrayKeeper* keeper)
{
  if (!dataSet)
  {
    return;
  }

  unsigned int numNodes = grid->getNumberNodes();
  shared_ptr<XdmfArray> mRowPointer = grid->getRowPointer();
  shared_ptr<XdmfArray> mColumnIndex = grid->getColumnIndex();
  shared_ptr<XdmfArray> mValues = grid->getValues();
  bool freeRow = svtkXdmf3DataSet_ReadIfNeeded(mRowPointer.get());
  bool freeColumn = svtkXdmf3DataSet_ReadIfNeeded(mColumnIndex.get());
  bool freeValues = svtkXdmf3DataSet_ReadIfNeeded(mValues.get());
  // unpack the compressed row storage format graph into nodes and edges

  svtkSmartPointer<svtkDoubleArray> wA = svtkSmartPointer<svtkDoubleArray>::New();
  wA->SetName("Edge Weights");
  wA->SetNumberOfComponents(1);

  // Nodes
  for (unsigned int i = 0; i < numNodes; ++i)
  {
    dataSet->AddVertex();
  }

  // Edges
  unsigned int index = 0;
  for (unsigned int i = 0; i < numNodes; ++i)
  {
    for (unsigned int j = mRowPointer->getValue<unsigned int>(i);
         j < mRowPointer->getValue<unsigned int>(i + 1); ++j)
    {
      const unsigned int k = mColumnIndex->getValue<unsigned int>(j);
      dataSet->AddEdge(i, k);

      double value = mValues->getValue<double>(index++);
      wA->InsertNextValue(value);
    }
  }

  svtkXdmf3DataSet_ReleaseIfNeeded(mRowPointer.get(), freeRow);
  svtkXdmf3DataSet_ReleaseIfNeeded(mColumnIndex.get(), freeColumn);
  svtkXdmf3DataSet_ReleaseIfNeeded(mValues.get(), freeValues);

  // Copy over arrays
  svtkDataSetAttributes* edgeData = dataSet->GetEdgeData();
  edgeData->AddArray(wA);

  // Next the optional arrays
  unsigned int numAttributes = grid->getNumberAttributes();
  for (unsigned int cc = 0; cc < numAttributes; cc++)
  {
    shared_ptr<XdmfAttribute> xmfAttribute = grid->getAttribute(cc);
    std::string attrName = xmfAttribute->getName();
    if (attrName.length() == 0)
    {
      cerr << "Skipping unnamed array." << endl;
      continue;
    }

    svtkFieldData* fieldData = 0;
    shared_ptr<const XdmfAttributeCenter> attrCenter = xmfAttribute->getCenter();
    if (attrCenter == XdmfAttributeCenter::Grid())
    {
      if (!fselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetFieldData();
    }
    else if (attrCenter == XdmfAttributeCenter::Edge())
    {
      if (!cselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetEdgeData();
    }
    else if (attrCenter == XdmfAttributeCenter::Node())
    {
      if (!pselection->ArrayIsEnabled(attrName.c_str()))
      {
        continue;
      }
      fieldData = dataSet->GetVertexData();
    }
    else
    {
      cerr << "Skipping " << attrName << " unrecognized association" << endl;
      continue; // unhandled.
    }

    svtkDataArray* array = svtkXdmf3DataSet::XdmfToSVTKArray(xmfAttribute.get(), attrName, 0, keeper);
    if (array)
    {
      fieldData->AddArray(array);
      array->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::SVTKToXdmf(
  svtkDirectedGraph* dataSet, XdmfDomain* domain, bool hasTime, double time, const char* name)
{
  // get list of vertices
  svtkSmartPointer<svtkVertexListIterator> vit = svtkSmartPointer<svtkVertexListIterator>::New();
  dataSet->GetVertices(vit);

  svtkIdType numNodes = dataSet->GetNumberOfVertices();
  shared_ptr<XdmfArray> mRowPointer = XdmfArray::New();
  shared_ptr<XdmfArray> mColumnIndex = XdmfArray::New();
  shared_ptr<XdmfArray> mValues = XdmfArray::New();
  mValues->initialize(XdmfArrayType::Float32());
  if (SVTK_SIZEOF_ID_TYPE == XdmfArrayType::Int64()->getElementSize())
  {
    mRowPointer->initialize(XdmfArrayType::Int64());
    mColumnIndex->initialize(XdmfArrayType::Int64());
  }
  else
  {
    mRowPointer->initialize(XdmfArrayType::Int32());
    mColumnIndex->initialize(XdmfArrayType::Int32());
  }

  svtkDoubleArray* wA =
    svtkArrayDownCast<svtkDoubleArray>(dataSet->GetEdgeData()->GetArray("Edge Weights"));

  while (vit->HasNext())
  {
    svtkIdType sVertex = vit->Next();

    mRowPointer->pushBack(mColumnIndex->getSize());

    svtkSmartPointer<svtkOutEdgeIterator> eit = svtkSmartPointer<svtkOutEdgeIterator>::New();
    dataSet->GetOutEdges(sVertex, eit);

    while (eit->HasNext())
    {
      svtkOutEdgeType e = eit->Next();
      svtkIdType dVertex = e.Target;
      mColumnIndex->pushBack((int)dVertex);
      double eW = 1.0;
      if (wA)
      {
        eW = wA->GetValue(e.Id);
      }
      mValues->pushBack(eW);
    }
  }
  mRowPointer->pushBack(mValues->getSize());

  shared_ptr<XdmfGraph> grid = XdmfGraph::New(numNodes);
  grid->setValues(mValues);
  grid->setColumnIndex(mColumnIndex);
  grid->setRowPointer(mRowPointer);
  if (name)
  {
    grid->setName(std::string(name));
  }

  svtkFieldData* fd;
  shared_ptr<const XdmfAttributeCenter> center;
  for (int i = 0; i < 3; i++)
  {
    switch (i)
    {
      case 0:
        fd = dataSet->GetFieldData();
        center = XdmfAttributeCenter::Grid();
        break;
      case 1:
        fd = dataSet->GetVertexData();
        center = XdmfAttributeCenter::Node();
        break;
      default:
        fd = dataSet->GetEdgeData();
        center = XdmfAttributeCenter::Edge();
    }

    for (svtkIdType j = 0; j < fd->GetNumberOfArrays(); j++)
    {
      svtkDataArray* vArray = fd->GetArray(j);
      if (vArray == wA)
      {
        continue;
      }
      shared_ptr<XdmfAttribute> xmfAttribute = XdmfAttribute::New();
      if (!vArray->GetName())
      {
        continue;
      }
      xmfAttribute->setName(vArray->GetName());
      xmfAttribute->setCenter(center);
      bool OK = svtkXdmf3DataSet::SVTKToXdmfArray(vArray, xmfAttribute.get());
      if (OK)
      {
        grid->insert(xmfAttribute);
      }
    }
  }

  svtkXdmf3DataSet::SetTime(grid.get(), hasTime, time);

  domain->insert(grid);
}

//==========================================================================
// TODO: meld this with Grid XdmfToSVTKAttributes
// TODO: enable set attribute selections
void svtkXdmf3DataSet::XdmfToSVTKAttributes(
  /*
    svtkXdmf3ArraySelection *fselection,
    svtkXdmf3ArraySelection *cselection,
    svtkXdmf3ArraySelection *pselection,
  */
  XdmfSet* grid, svtkDataObject* dObject, svtkXdmf3ArrayKeeper* keeper)
{
  svtkDataSet* dataSet = svtkDataSet::SafeDownCast(dObject);
  if (!dataSet)
  {
    return;
  }
  unsigned int numCells = dataSet->GetNumberOfCells();
  unsigned int numPoints = dataSet->GetNumberOfPoints();
  unsigned int numAttributes = grid->getNumberAttributes();
  for (unsigned int cc = 0; cc < numAttributes; cc++)
  {
    shared_ptr<XdmfAttribute> xmfAttribute = grid->getAttribute(cc);
    std::string attrName = xmfAttribute->getName();
    if (attrName.length() == 0)
    {
      cerr << "Skipping unnamed array." << endl;
      continue;
    }

    // figure out how many components in this array
    std::vector<unsigned int> dims = xmfAttribute->getDimensions();
    unsigned int ndims = static_cast<unsigned int>(dims.size());
    unsigned int nvals = 1;
    for (unsigned int i = 0; i < dims.size(); i++)
    {
      nvals = nvals * dims[i];
    }

    unsigned int ncomp = 1;

    svtkFieldData* fieldData = 0;

    shared_ptr<const XdmfAttributeCenter> attrCenter = xmfAttribute->getCenter();
    if (attrCenter == XdmfAttributeCenter::Grid())
    {
      /*
      if (!fselection->ArrayIsEnabled(attrName.c_str()))
        {
        continue;
        }
      */
      fieldData = dataSet->GetFieldData();
      ncomp = dims[ndims - 1];
    }
    else if (attrCenter == XdmfAttributeCenter::Cell())
    {
      /*
      if (!cselection->ArrayIsEnabled(attrName.c_str()))
        {
        continue;
        }
      */
      if (numCells == 0)
      {
        continue;
      }
      fieldData = dataSet->GetCellData();
      ncomp = nvals / numCells;
    }
    else if (attrCenter == XdmfAttributeCenter::Node())
    {
      /*
      if (!pselection->ArrayIsEnabled(attrName.c_str()))
        {
        continue;
        }
      */
      if (numPoints == 0)
      {
        continue;
      }
      fieldData = dataSet->GetPointData();
      ncomp = nvals / numPoints;
    }
    else
    {
      cerr << "skipping " << attrName << " unrecognized association" << endl;
      continue; // unhandled.
    }
    svtkDataSetAttributes* fdAsDSA = svtkDataSetAttributes::SafeDownCast(fieldData);

    shared_ptr<const XdmfAttributeType> attrType = xmfAttribute->getType();
    enum vAttType
    {
      NONE,
      SCALAR,
      VECTOR,
      TENSOR,
      MATRIX,
      TENSOR6,
      GLOBALID
    };
    int atype = NONE;
    if (attrType == XdmfAttributeType::Scalar() && ncomp == 1)
    {
      atype = SCALAR;
    }
    else if (attrType == XdmfAttributeType::Vector() && ncomp == 1)
    {
      atype = VECTOR;
    }
    else if (attrType == XdmfAttributeType::Tensor() && ncomp == 9)
    {
      atype = TENSOR;
    }
    else if (attrType == XdmfAttributeType::Matrix())
    {
      atype = MATRIX;
    }
    else if (attrType == XdmfAttributeType::Tensor6())
    {
      atype = TENSOR6;
    }
    else if (attrType == XdmfAttributeType::GlobalId() && ncomp == 1)
    {
      atype = GLOBALID;
    }

    svtkDataArray* array = XdmfToSVTKArray(xmfAttribute.get(), attrName, ncomp, keeper);
    if (array)
    {
      fieldData->AddArray(array);
      if (fdAsDSA)
      {
        switch (atype)
        {
          case SCALAR:
            if (!fdAsDSA->GetScalars())
            {
              fdAsDSA->SetScalars(array);
            }
            break;
          case VECTOR:
            if (!fdAsDSA->GetVectors())
            {
              fdAsDSA->SetVectors(array);
            }
            break;
          case TENSOR:
            if (!fdAsDSA->GetTensors())
            {
              fdAsDSA->SetTensors(array);
            }
            break;
          case GLOBALID:
            if (!fdAsDSA->GetGlobalIds())
            {
              fdAsDSA->SetGlobalIds(array);
            }
            break;
        }
      }
      array->Delete();
    }
  }
}

//--------------------------------------------------------------------------
void svtkXdmf3DataSet::XdmfSubsetToSVTK(XdmfGrid* grid, unsigned int setnum, svtkDataSet* dataSet,
  svtkUnstructuredGrid* subSet, svtkXdmf3ArrayKeeper* keeper)
{
  shared_ptr<XdmfSet> set = grid->getSet(setnum);
  bool releaseMe = svtkXdmf3DataSet_ReadIfNeeded(set.get());
  /*
  if (set->getType() == XdmfSetType::NoSetType())
    {
    }
  */
  if (set->getType() == XdmfSetType::Node())
  {
    svtkDataArray* ids = svtkXdmf3DataSet::XdmfToSVTKArray(set.get(), set->getName(), 1, keeper);

    svtkSmartPointer<svtkSelectionNode> selectionNode = svtkSmartPointer<svtkSelectionNode>::New();
    selectionNode->SetFieldType(svtkSelectionNode::POINT);
    selectionNode->SetContentType(svtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
    selection->AddNode(selectionNode);

    svtkSmartPointer<svtkExtractSelection> extractSelection =
      svtkSmartPointer<svtkExtractSelection>::New();
    extractSelection->SetInputData(0, dataSet);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    // remove arrays from grid, only care about subsets own arrays
    svtkUnstructuredGrid* dso = svtkUnstructuredGrid::SafeDownCast(extractSelection->GetOutput());
    dso->GetPointData()->Initialize();
    dso->GetCellData()->Initialize();
    dso->GetFieldData()->Initialize();
    subSet->ShallowCopy(dso);

    svtkXdmf3DataSet::XdmfToSVTKAttributes(set.get(), subSet, keeper);
    ids->Delete();
  }

  if (set->getType() == XdmfSetType::Cell())
  {
    svtkDataArray* ids = svtkXdmf3DataSet::XdmfToSVTKArray(set.get(), set->getName(), 1, keeper);

    svtkSmartPointer<svtkSelectionNode> selectionNode = svtkSmartPointer<svtkSelectionNode>::New();
    selectionNode->SetFieldType(svtkSelectionNode::CELL);
    selectionNode->SetContentType(svtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
    selection->AddNode(selectionNode);

    svtkSmartPointer<svtkExtractSelection> extractSelection =
      svtkSmartPointer<svtkExtractSelection>::New();
    extractSelection->SetInputData(0, dataSet);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    // remove arrays from grid, only care about subsets own arrays
    svtkUnstructuredGrid* dso = svtkUnstructuredGrid::SafeDownCast(extractSelection->GetOutput());
    dso->GetPointData()->Initialize();
    dso->GetCellData()->Initialize();
    dso->GetFieldData()->Initialize();
    subSet->ShallowCopy(dso);

    svtkXdmf3DataSet::XdmfToSVTKAttributes(set.get(), subSet, keeper);
    ids->Delete();
  }

  if (set->getType() == XdmfSetType::Face())
  {
    svtkPoints* pts = svtkPoints::New();
    subSet->SetPoints(pts);
    svtkSmartPointer<svtkMergePoints> mergePts = svtkSmartPointer<svtkMergePoints>::New();
    mergePts->InitPointInsertion(pts, dataSet->GetBounds());

    svtkDataArray* ids = svtkXdmf3DataSet::XdmfToSVTKArray(set.get(), set->getName(), 2, keeper);
    // ids is a 2 component array were each tuple is (cell-id, face-id).

    svtkIdType numFaces = ids->GetNumberOfTuples();
    for (svtkIdType cc = 0; cc < numFaces; cc++)
    {
      svtkIdType cellId = ids->GetComponent(cc, 0);
      svtkIdType faceId = ids->GetComponent(cc, 1);
      svtkCell* cell = dataSet->GetCell(cellId);
      if (!cell)
      {
        continue;
      }
      svtkCell* face = cell->GetFace(faceId);
      if (!face)
      {
        continue;
      }

      // Now insert this face a new cell in the output dataset.
      svtkIdType numPoints = face->GetNumberOfPoints();
      svtkPoints* facePoints = face->GetPoints();
      svtkIdType* outputPts = new svtkIdType[numPoints + 1];
      svtkIdType* cPt = outputPts;

      double ptCoord[3];
      for (svtkIdType pt = 0; pt < facePoints->GetNumberOfPoints(); pt++)
      {
        facePoints->GetPoint(pt, ptCoord);
        mergePts->InsertUniquePoint(ptCoord, cPt[pt]);
      }
      subSet->InsertNextCell(face->GetCellType(), numPoints, outputPts);
      delete[] outputPts;
    }

    svtkXdmf3DataSet::XdmfToSVTKAttributes(set.get(), subSet, keeper);

    ids->Delete();
    pts->Delete();
  }

  if (set->getType() == XdmfSetType::Edge())
  {
    svtkPoints* pts = svtkPoints::New();
    subSet->SetPoints(pts);
    svtkSmartPointer<svtkMergePoints> mergePts = svtkSmartPointer<svtkMergePoints>::New();
    mergePts->InitPointInsertion(pts, dataSet->GetBounds());

    svtkDataArray* ids = svtkXdmf3DataSet::XdmfToSVTKArray(set.get(), set->getName(), 3, keeper);
    // ids is a 3 component array were each tuple is (cell-id, face-id, edge-id).

    svtkIdType numEdges = ids->GetNumberOfTuples();
    for (svtkIdType cc = 0; cc < numEdges; cc++)
    {
      svtkIdType cellId = ids->GetComponent(cc, 0);
      svtkIdType faceId = ids->GetComponent(cc, 1);
      svtkIdType edgeId = ids->GetComponent(cc, 2);
      svtkCell* cell = dataSet->GetCell(cellId);
      if (!cell)
      {
        continue;
      }
      svtkCell* face = cell->GetFace(faceId);
      if (!face)
      {
        continue;
      }
      svtkCell* edge = face->GetEdge(edgeId);
      if (!edge)
      {
        continue;
      }

      // Now insert this edge a new cell in the output dataset.
      svtkIdType numPoints = edge->GetNumberOfPoints();
      svtkPoints* edgePoints = edge->GetPoints();
      svtkIdType* outputPts = new svtkIdType[numPoints + 1];
      svtkIdType* cPt = outputPts;

      double ptCoord[3];
      for (svtkIdType pt = 0; pt < edgePoints->GetNumberOfPoints(); pt++)
      {
        edgePoints->GetPoint(pt, ptCoord);
        mergePts->InsertUniquePoint(ptCoord, cPt[pt]);
      }
      subSet->InsertNextCell(edge->GetCellType(), numPoints, outputPts);
      delete[] outputPts;
    }

    svtkXdmf3DataSet::XdmfToSVTKAttributes(set.get(), subSet, keeper);

    ids->Delete();
    pts->Delete();
  }

  svtkXdmf3DataSet_ReleaseIfNeeded(set.get(), releaseMe);
  return;
}
//------------------------------------------------------------------------------
void svtkXdmf3DataSet::ParseFiniteElementFunction(svtkDataObject* dObject,
  shared_ptr<XdmfAttribute> xmfAttribute, svtkDataArray* array, XdmfGrid* grid,
  svtkXdmf3ArrayKeeper* keeper)
{

  svtkDataSet* dataSet_original = svtkDataSet::SafeDownCast(dObject);
  svtkUnstructuredGrid* dataSet_finite_element = svtkUnstructuredGrid::New();
  svtkUnstructuredGrid* dataSet = svtkUnstructuredGrid::SafeDownCast(dObject);

  // Mapping of dofs per component to the correct SVTK order
  std::vector<unsigned int> triangle_map = { 0, 1, 2 };
  std::vector<unsigned int> quadratic_triangle_map = { 0, 1, 2, 5, 3, 4 };
  std::vector<unsigned int> tetrahedron_map = { 0, 1, 2, 3, 4 };
  std::vector<unsigned int> quadratic_tetrahedron_map = { 0, 1, 2, 3, 9, 6, 8, 7, 5, 4 };
  std::vector<unsigned int> quadrilateral_map = { 0, 1, 3, 2 };
  std::vector<unsigned int> quadratic_quadrilateral_map = { 0, 1, 4, 3, 2, 7, 5, 6, 8 };
  std::vector<unsigned int> single_value_map = { 0 };
  std::vector<unsigned int> dof_to_svtk_map;

  // One array is xmfAttribute and other array is the first auxiliary array
  if (xmfAttribute->getNumberAuxiliaryArrays() < 1)
  {
    cerr << "There must be at least 2 children DataItems under "
            "FiniteElementFunction item type."
         << endl;
    return;
  }

  // First aux array are values of degrees of freedom
  shared_ptr<XdmfArray> dof_values = xmfAttribute->getAuxiliaryArray(0);
  bool freeMe = svtkXdmf3DataSet_ReadIfNeeded(dof_values.get());
  if (keeper && freeMe)
  {
    keeper->Insert(dof_values.get());
  }

  // Where new geometry will be stored
  svtkPoints* p_new = svtkPoints::New();
  unsigned long points_added = 0;

  // Where new data values will be stored
  svtkDataArray* new_array;
  new_array = svtkDataArray::CreateDataArray(SVTK_DOUBLE);
  new_array->SetName(array->GetName());

  shared_ptr<const XdmfTopology> xTopology = grid->getTopology();
  shared_ptr<const XdmfTopologyType> xCellType = xTopology->getType();

  // Index iterates through dofs in cells
  unsigned long index = 0;
  // Ncomp iterates through nontrivial (nonpadded) components
  unsigned int ncomp = 0;
  // Data_rank is int type for SVTK typedef
  int data_rank = -1;

  // Get number of dofs per cell
  unsigned int number_dofs_per_cell = xmfAttribute->getSize() / xTopology->getNumberElements();

  // For each cell/element
  for (unsigned int i = 0; i < xTopology->getNumberElements(); ++i)
  {
    // Get original already built cell
    // This cell was prepared in "copyshape" method before
    svtkCell* cell = dataSet->GetCell(i);

    // Retrieve new SVTK cell type, i.e. SVTK representation of xdmf finite
    // element function
    int new_cell_type = GetSVTKFiniteElementCellType(
      xmfAttribute->getElementDegree(), xmfAttribute->getElementFamily(), xCellType);

    // Get number of points for the new cell
    bool failed;
    unsigned int number_points_per_new_cell = GetNumberOfPointsPerCell(new_cell_type, failed);

    if (failed)
    {
      cerr << "Unable to get number of points for cell type " << new_cell_type << endl;
      return;
    }

    // Global indices to points in cell
    svtkIdType* ptIds = new svtkIdType[number_points_per_new_cell];

    // Get original cell points
    svtkPoints* cell_points = cell->GetPoints();

    // Store element degree
    unsigned int d = xmfAttribute->getElementDegree();

    // Prepare space for normal vectors
    double** normal = new double*[number_points_per_new_cell];
    for (unsigned int q = 0; q < number_points_per_new_cell; ++q)
    {
      normal[q] = new double[3];
    }

    // Determine number of components after embedding
    // the scalar/vector/tesor into 3D world
    unsigned int ncomp_padded = 0;
    if (xmfAttribute->getType() == XdmfAttributeType::Scalar())
    {
      ncomp_padded = 1;
      data_rank = svtkDataSetAttributes::SCALARS;
    }

    if (xmfAttribute->getType() == XdmfAttributeType::Vector())
    {
      ncomp_padded = 3;
      data_rank = svtkDataSetAttributes::VECTORS;
    }

    if (xmfAttribute->getType() == XdmfAttributeType::Tensor() ||
      xmfAttribute->getType() == XdmfAttributeType::Tensor6())
    {
      ncomp_padded = 9;
      data_rank = svtkDataSetAttributes::TENSORS;
    }

    new_array->SetNumberOfComponents(ncomp_padded);

    // For each new point in the cell
    for (unsigned int ix = 0; ix < number_points_per_new_cell; ++ix)
    {

      ptIds[ix] = static_cast<svtkIdType>(points_added);
      double coord[3];
      double coord_begin[3];
      double coord_end[3];
      unsigned int dim;

      // Prepare zero filled values
      double* tuple = new double[ncomp_padded];
      for (unsigned int q = 0; q < ncomp_padded; ++q)
      {
        tuple[q] = 0.0;
      }
      if ((xmfAttribute->getElementFamily() == "CG" || xmfAttribute->getElementFamily() == "DG") &&
        xmfAttribute->getElementCell() == "triangle")
      {
        //
        // CG and DG on triangles
        // ---------------------------------------------------------------------
        // Original points, i.e. vertices are unchanged for arbitrary degree
        //
        // For degree=2 SVTK_QUADRATIC_TRIANGLE with
        // degrees of freedom in midpoints is prepared

        // For original points
        if (ix < cell->GetNumberOfPoints())
        {
          cell_points->GetPoint(ix, coord);
        }
        else if (d == 2 && ix >= cell->GetNumberOfPoints())
        {
          // They are just tuples (i, i+i) but when i+1 = last point then
          // i+1 is in fact 0
          cell_points->GetPoint(static_cast<svtkIdType>(ix - 3), coord_begin);
          cell_points->GetPoint(static_cast<svtkIdType>((ix - 3 + 1) % 3), coord_end);

          // Additional points for CG2, DG2 are on midways
          for (unsigned int space_dim = 0; space_dim <= 2; ++space_dim)
          {
            coord[space_dim] = (coord_begin[space_dim] + coord_end[space_dim]) * 0.5;
          }
        }

        if (d == 0)
        {
          dof_to_svtk_map = single_value_map;
        }
        else if (d == 1)
        {
          dof_to_svtk_map = triangle_map;
        }
        else if (d == 2)
        {
          dof_to_svtk_map = quadratic_triangle_map;
        }
        else
        {
          dof_to_svtk_map = triangle_map;
        }

        dim = (d + 1) * (d + 2) / 2;
        ncomp = number_dofs_per_cell / dim;

        //
        // Fill data values
        //
        if (!(d == 0 && ix > 0))
        {
          for (unsigned int comp = 0; comp < ncomp; ++comp)
          {
            // If I am on point which doesn't have a corresponding value in dof
            // values I must compute it, this is subparametric element
            //
            // These values are in midpoints of referential cell and are
            // averages of values on nodes
            if (ix + 1 > dim)
            {
              unsigned long dof_index_begin =
                xmfAttribute->getValue<unsigned long>(index + dof_to_svtk_map[ix % 3] + comp * dim);
              unsigned long dof_index_end = xmfAttribute->getValue<unsigned long>(
                index + dof_to_svtk_map[(ix + 1) % 3] + comp * dim);

              tuple[comp] = (dof_values->getValue<double>(dof_index_begin) +
                              dof_values->getValue<double>(dof_index_end)) /
                2.0;
            }
            else
            {
              // For points having corresponding values just insert them
              unsigned long dof_index =
                xmfAttribute->getValue<unsigned long>(index + dof_to_svtk_map[ix] + comp * dim);

              tuple[comp] = dof_values->getValue<double>(dof_index);
            }
          }
        }
      }
      else if ((xmfAttribute->getElementFamily() == "CG" ||
                 xmfAttribute->getElementFamily() == "DG") &&
        xmfAttribute->getElementCell() == "tetrahedron")
      {
        //
        // CG and DG on tetrahedra
        // ---------------------------------------------------------------------
        //
        // Original points, i.e. vertices are unchanged for arbitrary degree
        //
        // For degree=2 SVTK_QUADRATIC_TETRA is prepared with degrees of
        // freedom in midpoints

        // For original points
        if (ix < cell->GetNumberOfPoints())
        {
          cell_points->GetPoint(ix, coord);
        }
        else if (d == 2 && ix >= cell->GetNumberOfPoints())
        {
          cell_points->GetPoint(static_cast<svtkIdType>((ix - 1) % 3), coord_begin);

          if (ix > 6)
          {
            cell_points->GetPoint(static_cast<svtkIdType>(3), coord_end);
          }
          else
          {
            cell_points->GetPoint(static_cast<svtkIdType>(ix % 3), coord_end);
          }

          // Additional points for CG2, DG2 are on midways
          for (unsigned int space_dim = 0; space_dim <= 2; ++space_dim)
          {
            coord[space_dim] = (coord_begin[space_dim] + coord_end[space_dim]) * 0.5;
          }
        }

        if (d == 0)
        {
          dof_to_svtk_map = single_value_map;
        }
        else if (d == 1)
        {
          dof_to_svtk_map = tetrahedron_map;
        }
        else if (d == 2)
        {
          dof_to_svtk_map = quadratic_tetrahedron_map;
        }
        else
        {
          dof_to_svtk_map = tetrahedron_map;
        }

        dim = (d + 1) * (d + 2) * (d + 3) / 6;
        ncomp = number_dofs_per_cell / dim;

        //
        // Fill data values
        //

        if (!(d == 0 && ix > 0))
        {
          for (unsigned int comp = 0; comp < ncomp; ++comp)
          {
            unsigned long dof_index =
              xmfAttribute->getValue<unsigned long>(index + dof_to_svtk_map[ix] + comp * dim);

            tuple[comp] = dof_values->getValue<double>(dof_index);
          }
        }
      }
      else if ((xmfAttribute->getElementFamily() == "Q" ||
                 xmfAttribute->getElementFamily() == "DQ") &&
        xmfAttribute->getElementCell() == "quadrilateral")
      {
        //
        // Q and DQ on quadrilaterals
        // ---------------------------------------------------------------------
        //
        // "Q" element family
        //
        // For degree=2 SVTK_BIQUADRATIC_QUAD with degrees of freedom on
        // midpoints of edges and in centroid is prepared

        double coord_orig[4][3];

        // For original points
        if (ix < cell->GetNumberOfPoints())
        {
          cell_points->GetPoint(ix, coord_orig[ix]);
          cell_points->GetPoint(ix, coord);
        }
        else if (ix <= 7)
        {
          cell_points->GetPoint(static_cast<svtkIdType>(ix % 4), coord_begin);

          cell_points->GetPoint(static_cast<svtkIdType>((ix + 1) % 4), coord_end);

          // Additional points for Q2, DQ2 are on midways
          for (unsigned int space_dim = 0; space_dim <= 2; ++space_dim)
          {
            coord[space_dim] = (coord_begin[space_dim] + coord_end[space_dim]) * 0.5;
          }
        }
        else if (ix == 8)
        {
          // The last point is in centroid of the quad
          for (unsigned int space_dim = 0; space_dim <= 2; ++space_dim)
          {
            coord[space_dim] = (coord_orig[0][space_dim] + coord_orig[1][space_dim] +
                                 coord_orig[2][space_dim] + coord_orig[3][space_dim]) /
              4;
          }
        }

        if (d == 0)
        {
          dof_to_svtk_map = single_value_map;
        }
        else if (d == 1)
        {
          dof_to_svtk_map = quadrilateral_map;
        }
        else if (d == 2)
        {
          dof_to_svtk_map = quadratic_quadrilateral_map;
        }
        else
        {
          dof_to_svtk_map = quadrilateral_map;
        }

        dim = pow(d + 1, 2);
        ncomp = number_dofs_per_cell / dim;

        //
        // Fill data values
        //

        if (!(d == 0 && ix > 0))
        {
          for (unsigned int comp = 0; comp < ncomp; ++comp)
          {
            unsigned long dof_index =
              xmfAttribute->getValue<unsigned long>(index + dof_to_svtk_map[ix] + comp * dim);

            tuple[comp] = dof_values->getValue<double>(dof_index);
          }
        }
      }
      else if (xmfAttribute->getElementFamily() == "RT" &&
        xmfAttribute->getElementCell() == "triangle")
      {
        //
        // RT (Raviart-Thomas) on triangles
        // ---------------------------------------------------------------------
        //
        // Degrees of freedom for degree=1 are on midpoints of edges
        // They represent normal component of vector field which is constant
        // on the whole edge. Therefore, in each vertex normal components
        // for both adjacent edges are known. These two components determine
        // the actual vector value.
        //
        // Higher order functions are not implemented.

        // For original points
        if (ix < cell->GetNumberOfPoints())
        {
          cell_points->GetPoint(ix, coord);
        }

        if (d == 1)
        {
          dof_to_svtk_map = triangle_map;
        }

        ncomp = 3;

        // This indices are used to choose normal vectors
        std::vector<unsigned int> normal_ixs = { ix, (ix + 2) % 3 };

        for (auto normal_ix : normal_ixs)
        {
          // Normals are computed from points on line (i, i+1) and when i+1
          // is the last point then (i, 0)
          cell_points->GetPoint(normal_ix, coord_begin);
          cell_points->GetPoint((normal_ix + 1) % 3, coord_end);

          // Orthogonal vector in 2D is computed just by switching coordinates
          // and multiplying -1 the second one
          normal[normal_ix][0] = coord_end[1] - coord_begin[1];
          normal[normal_ix][1] = -(coord_end[0] - coord_begin[0]);
          normal[normal_ix][2] = 0.0;

          // Compute euclidean norm
          double norm = sqrt(pow(normal[normal_ix][0], 2.) + pow(normal[normal_ix][1], 2.));

          // Normalize "normals"
          for (unsigned int space_dim = 0; space_dim <= 2; ++space_dim)
          {
            normal[normal_ix][space_dim] = normal[normal_ix][space_dim] / norm;
            if (normal_ix > ((normal_ix + 1) % 3))
              normal[normal_ix][space_dim] = -1 * normal[normal_ix][space_dim];
          }
        }

        // This index is used to choose the value of degree of freedom
        unsigned int ix1 = (ix + 2) % 3;
        unsigned int ix2 = (ix + 1) % 3;

        dof_to_svtk_map = triangle_map;

        // Get dof values
        double adjacent_dof1 = dof_values->getValue<double>(
          xmfAttribute->getValue<unsigned long>(index + dof_to_svtk_map[ix1]));
        double adjacent_dof2 = dof_values->getValue<double>(
          xmfAttribute->getValue<unsigned long>(index + dof_to_svtk_map[ix2]));

        // Dofs are scaled with the volume of corresponding facet
        adjacent_dof1 = adjacent_dof1 / sqrt(cell->GetEdge(ix)->GetLength2());
        adjacent_dof2 = adjacent_dof2 / sqrt(cell->GetEdge((ix + 2) % 3)->GetLength2());

        // Scalar product of the normals
        double normal_product = normal[normal_ixs[0]][0] * normal[normal_ixs[1] % 3][0] +
          normal[normal_ixs[0]][1] * normal[normal_ixs[1] % 3][1];

        // These coefficients are used to compute values at nodes
        // from values in midways
        double a =
          (adjacent_dof1 - adjacent_dof2 * normal_product) / (1.0 - pow(normal_product, 2.));

        double b =
          (adjacent_dof2 - adjacent_dof1 * normal_product) / (1.0 - pow(normal_product, 2.));

        tuple[0] = normal[normal_ixs[0]][0] * a + normal[normal_ixs[1] % 3][0] * b;

        tuple[1] = normal[normal_ixs[0]][1] * a + normal[normal_ixs[1] % 3][1] * b;

        tuple[2] = 0.0;
      }
      // Insert prepared point
      p_new->InsertNextPoint(coord);
      ++points_added;

      // If degree == 0 we want to add only first tuple because we store data as
      // CellData
      if (d == 0 && ix > 0)
      {
        delete[] tuple;
        continue;
      }
      // At this point, tuple is padded from the end, i.e. (1,0,0)
      // for one-component vector in 3D, but 2D tensor in 3D is padded
      // incorrectly as (1,1,1,1,0,0,0,0,0) and should be (1,1,0,1,1,0,0,0,0)
      // We need to rearrange values
      if (ncomp_padded == 9 && ncomp == 4)
      {
        tuple[4] = tuple[3];
        tuple[3] = tuple[2];
        tuple[2] = 0.0;
      }

      // Insert data value
      new_array->InsertNextTuple(tuple);

      delete[] tuple;
    }

    //
    // Add cell
    //

    dataSet_finite_element->InsertNextCell(new_cell_type, number_points_per_new_cell, ptIds);
    index = index + number_dofs_per_cell;

    delete[] ptIds;
    for (unsigned int q = 0; q < number_points_per_new_cell; ++q)
    {
      delete[] normal[q];
    }
    delete[] normal;
  }

  //
  // Add all points
  //

  dataSet_finite_element->SetPoints(p_new);
  p_new->Delete();

  // Copy prepared structure to the dataset
  dataSet->CopyStructure(dataSet_finite_element);

  svtkFieldData* fieldData = nullptr;

  // Insert values array to Cell/Point data
  if (xmfAttribute->getElementDegree() == 0)
  {
    fieldData = dataSet_original->GetCellData();
  }
  else
  {
    fieldData = dataSet_original->GetPointData();
  }

  fieldData->AddArray(new_array);

  svtkDataSetAttributes* dataSet_attributes = svtkDataSetAttributes::SafeDownCast(fieldData);

  if (data_rank >= 0)
    dataSet_attributes->SetAttribute(new_array, data_rank);

  new_array->Delete();
  array->Delete();
}
