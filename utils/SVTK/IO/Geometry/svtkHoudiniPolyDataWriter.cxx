/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHoudiniPolyDataWriter.h"

#include <algorithm>

#include "svtkAbstractArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkLongLongArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSetGet.h"
#include "svtkShortArray.h"
#include "svtkSignedCharArray.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedLongLongArray.h"
#include "svtkUnsignedShortArray.h"
#include "svtksys/FStream.hxx"

svtkStandardNewMacro(svtkHoudiniPolyDataWriter);

namespace
{
// Houdini geometry files store point/cell data in-line with the point/cell
// definition. So, the point data access pattern is to write a point's
// coordinates, followed by its data values for each point data attribute.
// This storage pattern differs from SVTK's, where all points are
// logically held in a contiguous memory block, followed by all of the values
// for a single data attribute. To accommodate this discrepancy in data
// access, we construct a facade for point/cell attributes that allows us to
// stream all of the values associated with a single point/cell.

struct AttributeBase
{
  virtual ~AttributeBase() = default;
  virtual void StreamHeader(std::ostream&) const = 0;
  virtual void StreamData(std::ostream&, svtkIdType) const = 0;
};

template <int AttributeId>
struct AttributeTrait;

#define DefineAttributeTrait(attId, attType, attName, svtkArray, attDefault)                        \
  template <>                                                                                      \
  struct AttributeTrait<attId>                                                                     \
  {                                                                                                \
    typedef attType Type;                                                                          \
    typedef svtkArray svtkArrayType;                                                                 \
    std::string Name() const { return std::string(attName); }                                      \
    attType Default() const { return static_cast<attType>(attDefault); }                           \
    static void Get(svtkIdType index, attType* in, svtkArray* array)                                 \
    {                                                                                              \
      array->GetTypedTuple(index, in);                                                             \
    }                                                                                              \
    static void Stream(std::ostream& out, attType t) { out << t; }                                 \
  }

DefineAttributeTrait(SVTK_DOUBLE, double, "float", svtkDoubleArray, 0.0);
DefineAttributeTrait(SVTK_FLOAT, float, "float", svtkFloatArray, 0.0);
DefineAttributeTrait(SVTK_LONG_LONG, long long, "int", svtkLongLongArray, 0);
DefineAttributeTrait(
  SVTK_UNSIGNED_LONG_LONG, unsigned long long, "int", svtkUnsignedLongLongArray, 0);
DefineAttributeTrait(SVTK_ID_TYPE, svtkIdType, "int", svtkIdTypeArray, 0);
DefineAttributeTrait(SVTK_LONG, long, "int", svtkLongArray, 0);
DefineAttributeTrait(SVTK_UNSIGNED_LONG, unsigned long, "int", svtkUnsignedLongArray, 0);
DefineAttributeTrait(SVTK_INT, int, "int", svtkIntArray, 0);
DefineAttributeTrait(SVTK_UNSIGNED_INT, unsigned int, "int", svtkUnsignedIntArray, 0);
DefineAttributeTrait(SVTK_SHORT, short, "int", svtkShortArray, 0);
DefineAttributeTrait(SVTK_UNSIGNED_SHORT, unsigned short, "int", svtkUnsignedShortArray, 0);

#undef DefineAttributeTrait

template <>
struct AttributeTrait<SVTK_CHAR>
{
  typedef char Type;
  typedef svtkCharArray svtkArrayType;
  std::string Name() const { return std::string("int"); }
  int Default() const { return static_cast<int>('0'); }
  static void Get(svtkIdType index, char* in, svtkCharArray* array)
  {
    array->GetTypedTuple(index, in);
  }
  static void Stream(std::ostream& out, char t) { out << static_cast<int>(t); }
};

template <>
struct AttributeTrait<SVTK_SIGNED_CHAR>
{
  typedef signed char Type;
  typedef svtkSignedCharArray svtkArrayType;
  std::string Name() const { return std::string("int"); }
  int Default() const { return static_cast<int>('0'); }
  static void Get(svtkIdType index, signed char* in, svtkSignedCharArray* array)
  {
    array->GetTypedTuple(index, in);
  }
  static void Stream(std::ostream& out, signed char t) { out << static_cast<int>(t); }
};

template <>
struct AttributeTrait<SVTK_UNSIGNED_CHAR>
{
  typedef unsigned char Type;
  typedef svtkUnsignedCharArray svtkArrayType;
  std::string Name() const { return std::string("int"); }
  int Default() const { return static_cast<int>('0'); }
  static void Get(svtkIdType index, unsigned char* in, svtkUnsignedCharArray* array)
  {
    array->GetTypedTuple(index, in);
  }
  static void Stream(std::ostream& out, unsigned char t) { out << static_cast<int>(t); }
};

template <>
struct AttributeTrait<SVTK_STRING>
{
  typedef svtkStdString Type;
  typedef svtkStringArray svtkArrayType;
};

template <int AttributeId>
struct Attribute : public AttributeBase
{
  typedef typename AttributeTrait<AttributeId>::svtkArrayType svtkArrayType;

  Attribute(svtkAbstractArray* array)
    : AttributeBase()
  {
    this->Array = svtkArrayType::SafeDownCast(array);
    assert(this->Array != nullptr);
    this->Value.resize(this->Array->GetNumberOfComponents());
  }

  void StreamHeader(std::ostream& out) const override
  {
    std::string s = this->Array->GetName();
    std::replace(s.begin(), s.end(), ' ', '_');
    std::replace(s.begin(), s.end(), '\t', '-');

    AttributeTrait<AttributeId> trait;
    out << s << " " << this->Array->GetNumberOfComponents() << " " << trait.Name() << " "
        << trait.Default();
    for (int i = 1; i < this->Array->GetNumberOfComponents(); i++)
    {
      out << " ";
      AttributeTrait<AttributeId>::Stream(out, trait.Default());
    }
  }

  void StreamData(std::ostream& out, svtkIdType index) const override
  {
    assert(index < this->Array->GetNumberOfTuples());

    AttributeTrait<AttributeId>::Get(index, &this->Value[0], this->Array);
    AttributeTrait<AttributeId>::Stream(out, this->Value[0]);

    for (int i = 1; i < this->Array->GetNumberOfComponents(); i++)
    {
      out << " ";
      AttributeTrait<AttributeId>::Stream(out, this->Value[i]);
    }
  }

protected:
  mutable std::vector<typename AttributeTrait<AttributeId>::Type> Value;
  mutable svtkArrayType* Array;
};

class Attributes
{
public:
  typedef std::vector<AttributeBase*>::iterator AttIt;

  class Header
  {
    friend class Attributes;
    Header(Attributes* atts)
      : Atts(atts)
    {
    }
    void operator=(const Attributes::Header&) = delete;

    friend ostream& operator<<(ostream& out, const Attributes::Header& header)
    {
      for (Attributes::AttIt it = header.Atts->AttVec.begin(); it != header.Atts->AttVec.end();
           ++it)
      {
        (*it)->StreamHeader(out);
        out << endl;
      }
      return out;
    }

  public:
    Attributes* Atts;
  };

  class Component
  {
    friend class Attributes;

    Component(Attributes* atts, svtkIdType index)
      : Atts(atts)
      , Index(index)
    {
    }

    Attributes* Atts;
    svtkIdType Index;

    friend ostream& operator<<(ostream& out, const Attributes::Component& component)
    {
      for (Attributes::AttIt it = component.Atts->AttVec.begin();
           it != component.Atts->AttVec.end(); ++it)
      {
        (*it)->StreamData(out, component.Index);

        if (it + 1 != component.Atts->AttVec.end())
        {
          out << " ";
        }
      }
      return out;
    }
  };

  Attributes()
    : Hdr(nullptr)
  {
    this->Hdr.Atts = this;
  }
  virtual ~Attributes()
  {
    for (AttIt it = this->AttVec.begin(); it != this->AttVec.end(); ++it)
    {
      delete *it;
    }
  }

  Header& GetHeader() { return this->Hdr; }

  Component operator[](svtkIdType i) { return Attributes::Component(this, i); }

  template <int TypeId>
  void AddAttribute(svtkAbstractArray* array)
  {
    this->AttVec.push_back(new Attribute<TypeId>(array));
  }

  Header Hdr;
  std::vector<AttributeBase*> AttVec;
};

template <int TypeId>
void AddAttribute(Attributes& atts, svtkAbstractArray* array)
{
  atts.AddAttribute<TypeId>(array);
}
}

//----------------------------------------------------------------------------
svtkHoudiniPolyDataWriter::svtkHoudiniPolyDataWriter()
{
  this->FileName = nullptr;
}

//----------------------------------------------------------------------------
svtkHoudiniPolyDataWriter::~svtkHoudiniPolyDataWriter()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void svtkHoudiniPolyDataWriter::WriteData()
{
  // Grab the input data
  svtkPolyData* input = svtkPolyData::SafeDownCast(this->GetInput());
  if (!input)
  {
    svtkErrorMacro(<< "Missing input polydata!");
    return;
  }

  // Open the file for streaming
  svtksys::ofstream file(this->FileName, svtksys::ofstream::out);

  if (file.fail())
  {
    svtkErrorMacro(<< "Unable to open file: " << this->FileName);
    return;
  }

  svtkIdType nPrims = 0;
  {
    nPrims += input->GetNumberOfVerts();
    nPrims += input->GetNumberOfLines();
    nPrims += input->GetNumberOfPolys();

    svtkCellArray* stripArray = input->GetStrips();
    svtkIdType nPts;
    const svtkIdType* pts;

    stripArray->InitTraversal();
    while (stripArray->GetNextCell(nPts, pts))
    {
      nPrims += nPts - 2;
    }
  }

  // Write generic header info
  file << "PGEOMETRY V2" << endl;
  file << "NPoints " << input->GetNumberOfPoints() << " "
       << "NPrims " << nPrims << endl;
  file << "NPointGroups " << 0 << " NPrimGroups " << 0 << endl;
  file << "NPointAttrib " << input->GetPointData()->GetNumberOfArrays() << " "
       << "NVertexAttrib " << 0 << " "
       << "NPrimAttrib " << input->GetCellData()->GetNumberOfArrays() << " "
       << "NAttrib " << 0 << endl;

#define svtkHoudiniTemplateMacroCase(typeN, atts, arr)                                              \
  case typeN:                                                                                      \
  {                                                                                                \
    AddAttribute<typeN>(atts, arr);                                                                \
  }                                                                                                \
  break
#define svtkHoudiniTemplateMacro(atts, arr)                                                         \
  svtkHoudiniTemplateMacroCase(SVTK_DOUBLE, atts, arr);                                              \
  svtkHoudiniTemplateMacroCase(SVTK_FLOAT, atts, arr);                                               \
  svtkHoudiniTemplateMacroCase(SVTK_LONG_LONG, atts, arr);                                           \
  svtkHoudiniTemplateMacroCase(SVTK_UNSIGNED_LONG_LONG, atts, arr);                                  \
  svtkHoudiniTemplateMacroCase(SVTK_ID_TYPE, atts, arr);                                             \
  svtkHoudiniTemplateMacroCase(SVTK_LONG, atts, arr);                                                \
  svtkHoudiniTemplateMacroCase(SVTK_UNSIGNED_LONG, atts, arr);                                       \
  svtkHoudiniTemplateMacroCase(SVTK_INT, atts, arr);                                                 \
  svtkHoudiniTemplateMacroCase(SVTK_UNSIGNED_INT, atts, arr);                                        \
  svtkHoudiniTemplateMacroCase(SVTK_SHORT, atts, arr);                                               \
  svtkHoudiniTemplateMacroCase(SVTK_UNSIGNED_SHORT, atts, arr);                                      \
  svtkHoudiniTemplateMacroCase(SVTK_CHAR, atts, arr);                                                \
  svtkHoudiniTemplateMacroCase(SVTK_SIGNED_CHAR, atts, arr);                                         \
  svtkHoudiniTemplateMacroCase(SVTK_UNSIGNED_CHAR, atts, arr)

  // Construct Attributes instance for points
  Attributes pointAttributes;
  for (svtkIdType i = 0; i < input->GetPointData()->GetNumberOfArrays(); i++)
  {
    svtkAbstractArray* array = input->GetPointData()->GetAbstractArray(i);
    switch (array->GetDataType())
    {
      svtkHoudiniTemplateMacro(pointAttributes, array);
#if 0
    case SVTK_STRING: { AddAttribute<SVTK_STRING>(pointAttributes, array); } break;
#endif
      default:
        svtkGenericWarningMacro(<< "Unsupported data type!");
    }
  }

  // Write point attributes header info
  if (input->GetPointData()->GetNumberOfArrays() != 0)
  {
    file << "PointAttrib" << endl;
    file << pointAttributes.GetHeader();
  }

  // Write point data
  svtkPoints* points = input->GetPoints();
  double xyz[3];
  for (svtkIdType i = 0; i < input->GetNumberOfPoints(); i++)
  {
    points->GetPoint(i, xyz);
    file << xyz[0] << " " << xyz[1] << " " << xyz[2] << " " << 1;
    if (input->GetPointData()->GetNumberOfArrays() != 0)
    {
      file << " (" << pointAttributes[i] << ")";
    }
    file << endl;
  }

  // Construct Attributes instance for cells
  Attributes cellAttributes;
  for (svtkIdType i = 0; i < input->GetCellData()->GetNumberOfArrays(); i++)
  {
    svtkAbstractArray* array = input->GetCellData()->GetAbstractArray(i);
    switch (array->GetDataType())
    {
      svtkHoudiniTemplateMacro(cellAttributes, array);
#if 0
    case SVTK_STRING: { AddAttribute<SVTK_STRING>(cellAttributes, array); } break;
#endif
      default:
        svtkGenericWarningMacro(<< "Unsupported data type!");
    }
  }

#undef svtkHoudiniTemplateMacro
#undef svtkHoudiniTemplateMacroCase

  // Write cell attributes header info
  if (input->GetCellData()->GetNumberOfArrays() != 0 && input->GetNumberOfCells() != 0)
  {
    file << "PrimitiveAttrib" << endl;
    file << cellAttributes.GetHeader();
  }

  if (input->GetNumberOfVerts() != 0)
  {
    // Write vertex data as a particle system
    svtkCellArray* vertArray = input->GetVerts();
    svtkIdType nPts;
    const svtkIdType* pts;
    svtkIdType cellId;

    if (input->GetNumberOfVerts() > 1)
    {
      file << "Run " << input->GetNumberOfVerts() << " Part" << endl;
    }
    else
    {
      file << "Part ";
    }
    cellId = 0;

    vertArray->InitTraversal();
    while (vertArray->GetNextCell(nPts, pts))
    {
      file << nPts;
      for (svtkIdType i = 0; i < nPts; i++)
      {
        file << " " << pts[i];
      }
      if (input->GetCellData()->GetNumberOfArrays() != 0)
      {
        file << " [" << cellAttributes[cellId] << "]";
      }
      file << endl;
      cellId++;
    }
  }

  if (input->GetNumberOfLines() != 0)
  {
    // Write line data as open polygons
    file << "Run " << input->GetNumberOfLines() << " Poly" << endl;

    svtkCellArray* lineArray = input->GetLines();
    svtkIdType nPts;
    const svtkIdType* pts;
    svtkIdType cellId;

    cellId = input->GetNumberOfVerts();

    lineArray->InitTraversal();
    while (lineArray->GetNextCell(nPts, pts))
    {
      file << nPts << " : " << pts[0];
      for (svtkIdType i = 1; i < nPts; i++)
      {
        file << " " << pts[i];
      }
      if (input->GetCellData()->GetNumberOfArrays() != 0)
      {
        file << " [" << cellAttributes[cellId++] << "]";
      }
      file << endl;
    }
  }

  if (input->GetNumberOfPolys() != 0)
  {
    // Write polygon data
    file << "Run " << input->GetNumberOfPolys() << " Poly" << endl;

    svtkCellArray* polyArray = input->GetPolys();
    svtkIdType nPts;
    const svtkIdType* pts;
    svtkIdType cellId;

    cellId = (input->GetNumberOfVerts() + input->GetNumberOfLines());

    polyArray->InitTraversal();
    while (polyArray->GetNextCell(nPts, pts))
    {
      file << nPts << " < " << pts[0];
      for (svtkIdType i = 1; i < nPts; i++)
      {
        file << " " << pts[i];
      }
      if (input->GetCellData()->GetNumberOfArrays() != 0)
      {
        file << " [" << cellAttributes[cellId++] << "]";
      }
      file << endl;
    }
  }

  if (input->GetNumberOfStrips() != 0)
  {
    // Write triangle strip data as polygons
    svtkCellArray* stripArray = input->GetStrips();
    svtkIdType nPts;
    const svtkIdType* pts;
    svtkIdType cellId;

    cellId = (input->GetNumberOfVerts() + input->GetNumberOfLines() + input->GetNumberOfPolys());

    stripArray->InitTraversal();
    while (stripArray->GetNextCell(nPts, pts))
    {
      if (nPts > 3)
      {
        file << "Run " << nPts - 2 << " Poly" << endl;
      }
      else
      {
        file << "Poly ";
      }

      for (svtkIdType i = 2; i < nPts; i++)
      {
        if (i % 2 == 0)
        {
          file << "3 < " << pts[i - 2] << " " << pts[i - 1] << " " << pts[i];
        }
        else
        {
          file << "3 < " << pts[i - 1] << " " << pts[i - 2] << " " << pts[i];
        }
        if (input->GetCellData()->GetNumberOfArrays() != 0)
        {
          file << " [" << cellAttributes[cellId] << "]";
        }
        file << endl;
      }
      cellId++;
    }
  }

  file << "beginExtra" << endl;
  file << "endExtra" << endl;

  file.close();
}

//----------------------------------------------------------------------------
int svtkHoudiniPolyDataWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void svtkHoudiniPolyDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}
