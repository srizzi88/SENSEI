#include "svtkUniforms.h"
#include "svtkObjectFactory.h"

//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
svtkAbstractObjectFactoryNewMacro(svtkUniforms);

//-----------------------------------------------------------------------------
void svtkUniforms::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

std::string svtkUniforms::TupleTypeToString(TupleType tt)
{
  std::string str;
  switch (tt)
  {
    case svtkUniforms::TupleTypeScalar:
      str = "TupleTypeScalar";
      break;
    case svtkUniforms::TupleTypeVector:
      str = "TupleTypeVector";
      break;
    case svtkUniforms::TupleTypeMatrix:
      str = "TupleTypeMatrix";
      break;
    default:
      str = "TupleTypeInvalid";
      break;
  }
  return str;
}

svtkUniforms::TupleType svtkUniforms::StringToTupleType(const std::string& s)
{
  if (s == "TupleTypeScalar")
  {
    return svtkUniforms::TupleTypeScalar;
  }
  else if (s == "TupleTypeVector")
  {
    return svtkUniforms::TupleTypeVector;
  }
  else if (s == "TupleTypeMatrix")
  {
    return svtkUniforms::TupleTypeMatrix;
  }
  return svtkUniforms::TupleTypeInvalid;
}

/* We only support int and float as internal data types for uniform variables */
std::string svtkUniforms::ScalarTypeToString(int scalarType)
{
  if (scalarType == SVTK_INT)
  {
    return "int";
  }
  else if (scalarType == SVTK_FLOAT)
  {
    return "float";
  }
  return "invalid";
}

int svtkUniforms::StringToScalarType(const std::string& s)
{
  if (s == "int")
  {
    return SVTK_INT;
  }
  else if (s == "float")
  {
    return SVTK_FLOAT;
  }
  else
  {
    return SVTK_VOID;
  }
}
