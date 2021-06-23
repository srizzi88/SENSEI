#include "svtkThresholdGraph.h"

#include "svtkDoubleArray.h"
#include "svtkExtractSelectedGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkThresholdGraph);

//-----------------------------------------------------------------------------
svtkThresholdGraph::svtkThresholdGraph()
  : svtkGraphAlgorithm()
  , LowerThreshold(0.0)
  , UpperThreshold(0.0)
{
}

//-----------------------------------------------------------------------------
svtkThresholdGraph::~svtkThresholdGraph()
{
  // Do nothing.
}

//-----------------------------------------------------------------------------
void svtkThresholdGraph::PrintSelf(ostream& os, svtkIndent indent)
{
  // Base class print.
  svtkGraphAlgorithm::PrintSelf(os, indent);

  os << indent << "LowerThreshold: " << this->LowerThreshold << endl;
  os << indent << "UpperThreshold: " << this->UpperThreshold << endl;
}

//-----------------------------------------------------------------------------
int svtkThresholdGraph::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!inputVector[0])
  {
    svtkErrorMacro("Error: nullptr or invalid input svtkInformationVector.");
    return 1;
  }

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    svtkErrorMacro("Error: nullptr or invalid input svtkInformation.");
    return 1;
  }

  svtkDataObject* inDataObj = inInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!inDataObj)
  {
    svtkErrorMacro("Error: nullptr or invalid input data object.");
    return 1;
  }

  if (!outputVector)
  {
    svtkErrorMacro("Error: nullptr or invalid output svtkInformationVector.");
    return 1;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    svtkErrorMacro("Error: nullptr of invalid output svtkInformation.");
  }

  svtkDataObject* outDataObj = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!outDataObj)
  {
    svtkErrorMacro("Error: nullptr or invalid output data object.");
    return 1;
  }

  svtkSmartPointer<svtkExtractSelectedGraph> extractThreshold(
    svtkSmartPointer<svtkExtractSelectedGraph>::New());
  svtkSmartPointer<svtkSelection> threshold(svtkSmartPointer<svtkSelection>::New());
  svtkSmartPointer<svtkSelectionNode> thresholdNode(svtkSmartPointer<svtkSelectionNode>::New());
  svtkSmartPointer<svtkDoubleArray> thresholdArr(svtkSmartPointer<svtkDoubleArray>::New());

  svtkInformationVector* inArrayVec = this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());

  if (!inArrayVec)
  {
    svtkErrorMacro("Problem finding array to process");
    return 1;
  }
  svtkInformation* inArrayInfo = inArrayVec->GetInformationObject(0);
  if (!inArrayInfo)
  {
    svtkErrorMacro("Problem getting name of array to process.");
    return 1;
  }
  if (!inArrayInfo->Has(svtkDataObject::FIELD_NAME()))
  {
    svtkErrorMacro("Missing field name.");
    return 1;
  }
  const char* arrayNameToProcess = inArrayInfo->Get(svtkDataObject::FIELD_NAME());

  if (!arrayNameToProcess)
  {
    svtkErrorMacro("Unable to find valid array.");
    return 1;
  }

  int fieldAssociation(-1);
  if (!inArrayInfo->Has(svtkDataObject::FIELD_ASSOCIATION()))
  {
    svtkErrorMacro("Unable to query field association for the scalar.");
    return 1;
  }
  fieldAssociation = inArrayInfo->Get(svtkDataObject::FIELD_ASSOCIATION());

  thresholdArr->SetName(arrayNameToProcess);
  thresholdArr->InsertNextValue(this->LowerThreshold);
  thresholdArr->InsertNextValue(this->UpperThreshold);

  thresholdNode->SetSelectionList(thresholdArr);
  thresholdNode->SetContentType(svtkSelectionNode::THRESHOLDS);

  if (fieldAssociation == svtkDataObject::FIELD_ASSOCIATION_EDGES)
  {
    thresholdNode->SetFieldType(svtkSelectionNode::EDGE);
  }
  else if (fieldAssociation == svtkDataObject::FIELD_ASSOCIATION_VERTICES)
  {
    thresholdNode->SetFieldType(svtkSelectionNode::VERTEX);
  }
  else
  {
    svtkErrorMacro("Array selected should be associated with vertex or "
      << "edge data.");
    return 1;
  }

  threshold->AddNode(thresholdNode);

  svtkSmartPointer<svtkDataObject> inputClone;
  inputClone.TakeReference(inDataObj->NewInstance());
  inputClone->ShallowCopy(inDataObj);

  extractThreshold->SetInputData(0, inputClone);
  extractThreshold->SetInputData(1, threshold);

  extractThreshold->Update();

  svtkDataObject* output = extractThreshold->GetOutputDataObject(0);

  if (!output)
  {
    svtkErrorMacro("nullptr or invalid output.");
    return 1;
  }

  outDataObj->ShallowCopy(output);

  return 1;
}
