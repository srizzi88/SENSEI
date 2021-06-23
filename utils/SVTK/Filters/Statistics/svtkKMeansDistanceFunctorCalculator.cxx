#include "svtkKMeansDistanceFunctorCalculator.h"

#include "svtkDoubleArray.h"
#include "svtkFunctionParser.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

#include <sstream>

svtkStandardNewMacro(svtkKMeansDistanceFunctorCalculator);
svtkCxxSetObjectMacro(svtkKMeansDistanceFunctorCalculator, FunctionParser, svtkFunctionParser);

// ----------------------------------------------------------------------
svtkKMeansDistanceFunctorCalculator::svtkKMeansDistanceFunctorCalculator()
{
  this->FunctionParser = svtkFunctionParser::New();
  this->DistanceExpression = nullptr;
  this->TupleSize = -1;
}

// ----------------------------------------------------------------------
svtkKMeansDistanceFunctorCalculator::~svtkKMeansDistanceFunctorCalculator()
{
  this->SetFunctionParser(nullptr);
  this->SetDistanceExpression(nullptr);
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctorCalculator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FunctionParser: " << this->FunctionParser << "\n";
  os << indent << "DistanceExpression: "
     << (this->DistanceExpression && this->DistanceExpression[0] ? this->DistanceExpression
                                                                 : "nullptr")
     << "\n";
  os << indent << "TupleSize: " << this->TupleSize << "\n";
}

// ----------------------------------------------------------------------
void svtkKMeansDistanceFunctorCalculator::operator()(
  double& distance, svtkVariantArray* clusterCoord, svtkVariantArray* dataCoord)
{
  distance = 0.0;
  svtkIdType nv = clusterCoord->GetNumberOfValues();
  if (nv != dataCoord->GetNumberOfValues())
  {
    cout << "The dimensions of the cluster and data do not match." << endl;
    distance = -1;
    return;
  }

  if (!this->DistanceExpression)
  {
    distance = -1;
    return;
  }

  this->FunctionParser->SetFunction(this->DistanceExpression);
  if (this->TupleSize != nv)
  { // Need to update the scalar variable names as well as values...
    this->FunctionParser->RemoveScalarVariables();
    for (svtkIdType i = 0; i < nv; ++i)
    {
      std::ostringstream xos;
      std::ostringstream yos;
      xos << "x" << i;
      yos << "y" << i;
      this->FunctionParser->SetScalarVariableValue(
        xos.str().c_str(), clusterCoord->GetValue(i).ToDouble());
      this->FunctionParser->SetScalarVariableValue(
        yos.str().c_str(), dataCoord->GetValue(i).ToDouble());
    }
  }
  else
  { // Use faster integer comparisons to set values...
    for (svtkIdType i = 0; i < nv; ++i)
    {
      this->FunctionParser->SetScalarVariableValue(2 * i, clusterCoord->GetValue(i).ToDouble());
      this->FunctionParser->SetScalarVariableValue(2 * i + 1, dataCoord->GetValue(i).ToDouble());
    }
  }
  distance = this->FunctionParser->GetScalarResult();
  /*
  cout << "f([";
  for ( svtkIdType i = 0; i < nv; ++ i )
    cout << " " << dataCoord->GetValue( i ).ToDouble();
  cout << " ],[";
  for ( svtkIdType i = 0; i < nv; ++ i )
    cout << " " << clusterCoord->GetValue( i ).ToDouble();
  cout << " ]) = " << distance << "\n";
  */
}
