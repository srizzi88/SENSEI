#include "svtkLabelSizeCalculator.h"

#include "svtkCellData.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkFieldData.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLabelHierarchy.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"

#include <map>

class svtkLabelSizeCalculator::Internals
{
public:
  std::map<int, svtkSmartPointer<svtkTextProperty> > FontProperties;
};

svtkStandardNewMacro(svtkLabelSizeCalculator);
svtkCxxSetObjectMacro(svtkLabelSizeCalculator, FontUtil, svtkTextRenderer);

svtkLabelSizeCalculator::svtkLabelSizeCalculator()
{
  this->Implementation = new Internals;
  // Always defined but user may set to nullptr.
  this->Implementation->FontProperties[0] = svtkSmartPointer<svtkTextProperty>::New();
  this->FontUtil = svtkTextRenderer::New(); // Never a nullptr moment.
  this->LabelSizeArrayName = nullptr;
  this->SetLabelSizeArrayName("LabelSize");
  this->DPI = 72;
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "LabelText");
  this->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Type");
}

svtkLabelSizeCalculator::~svtkLabelSizeCalculator()
{
  this->SetFontUtil(nullptr);
  this->SetLabelSizeArrayName(nullptr);
  delete this->Implementation;
}

void svtkLabelSizeCalculator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LabelSizeArrayName: " << this->LabelSizeArrayName << "\n";
  os << indent << "FontProperties: ";
  std::map<int, svtkSmartPointer<svtkTextProperty> >::iterator it, itEnd;
  it = this->Implementation->FontProperties.begin();
  itEnd = this->Implementation->FontProperties.end();
  for (; it != itEnd; ++it)
  {
    os << indent << "  " << it->first << ": " << it->second << endl;
  }
  os << indent << "FontUtil: " << this->FontUtil << "\n";
}

int svtkLabelSizeCalculator::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

void svtkLabelSizeCalculator::SetFontProperty(svtkTextProperty* prop, int type)
{
  this->Implementation->FontProperties[type] = prop;
}

svtkTextProperty* svtkLabelSizeCalculator::GetFontProperty(int type)
{
  if (this->Implementation->FontProperties.find(type) != this->Implementation->FontProperties.end())
  {
    return this->Implementation->FontProperties[type];
  }
  return nullptr;
}

int svtkLabelSizeCalculator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkFieldData* inFD = nullptr;
  svtkFieldData* outFD = nullptr;

  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(input);
  svtkDataSet* dsOutput = svtkDataSet::SafeDownCast(output);
  svtkGraph* graphInput = svtkGraph::SafeDownCast(input);
  svtkGraph* graphOutput = svtkGraph::SafeDownCast(output);

  // if input is empty, we are done
  if (graphInput && graphInput->GetNumberOfVertices() == 0)
  {
    return 1;
  }
  if (dsInput && dsInput->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  if (!this->Implementation->FontProperties[0])
  {
    svtkErrorMacro("nullptr default font property, so I cannot compute label sizes.");
    return 0;
  }

  if (!this->LabelSizeArrayName)
  {
    svtkErrorMacro("nullptr value for LabelSizeArrayName.");
    return 0;
  }

  // Figure out which array to process
  svtkAbstractArray* inArr = this->GetInputAbstractArrayToProcess(0, inputVector);
  if (!inArr)
  {
    svtkErrorMacro("No input array available.");
    return 0;
  }
  svtkIntArray* typeArr =
    svtkArrayDownCast<svtkIntArray>(this->GetInputAbstractArrayToProcess(1, inputVector));

  svtkInformation* inArrInfo = this->GetInputArrayInformation(0);
  int fieldAssoc = inArrInfo->Get(svtkDataObject::FIELD_ASSOCIATION());

  svtkIntArray* lsz = this->LabelSizesForArray(inArr, typeArr);
#if 0
  cout
    << "Input array... port: " << port << " connection: " << connection
    << " field association: " << fieldAssoc << " attribute type: " << attribType << "\n";
#endif // 0

  if (dsInput)
  {
    dsOutput->CopyStructure(dsInput);
    dsOutput->CopyAttributes(dsInput);
    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_NONE ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_VERTICES ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_NONE)
    {
      outFD = dsOutput->GetPointData();
      outFD->AddArray(lsz);
    }
    if (!inFD &&
      (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
        fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_CELLS ||
        fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_EDGES))
    {
      outFD = dsOutput->GetCellData();
      outFD->AddArray(lsz);
    }
    svtkLabelHierarchy* hierarchyOutput = svtkLabelHierarchy::SafeDownCast(output);
    if (hierarchyOutput)
    {
      hierarchyOutput->SetSizes(lsz);
    }
  }
  else if (graphInput)
  {
    graphOutput->ShallowCopy(graphInput);
    if (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_NONE ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_VERTICES ||
      fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_NONE)
    {
      outFD = graphOutput->GetVertexData();
      outFD->AddArray(lsz);
    }
    if (!inFD &&
      (fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS ||
        fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_CELLS ||
        fieldAssoc == svtkDataObject::FIELD_ASSOCIATION_EDGES))
    {
      outFD = graphOutput->GetEdgeData();
      outFD->AddArray(lsz);
    }
  }
  lsz->Delete();

  return 1;
}

svtkIntArray* svtkLabelSizeCalculator::LabelSizesForArray(
  svtkAbstractArray* labels, svtkIntArray* types)
{
  svtkIdType nl = labels->GetNumberOfTuples();

  svtkIntArray* lsz = svtkIntArray::New();
  lsz->SetName(this->LabelSizeArrayName);
  lsz->SetNumberOfComponents(4);
  lsz->SetNumberOfTuples(nl);

  int bbox[4];
  int* bds = lsz->GetPointer(0);
  for (svtkIdType i = 0; i < nl; ++i)
  {
    int type = 0;
    if (types)
    {
      type = types->GetValue(i);
    }
    svtkTextProperty* prop = this->Implementation->FontProperties[type];
    if (!prop)
    {
      prop = this->Implementation->FontProperties[0];
    }
    this->FontUtil->GetBoundingBox(
      prop, labels->GetVariantValue(i).ToString().c_str(), bbox, this->DPI);
    bds[0] = bbox[1] - bbox[0];
    bds[1] = bbox[3] - bbox[2];
    bds[2] = bbox[0];
    bds[3] = bbox[2];

    if (this->GetDebug())
    {
      cout << "LSC: " << bds[0] << " " << bds[1] << " " << bds[2] << " " << bds[3] << " \""
           << labels->GetVariantValue(i).ToString().c_str() << "\"\n";
    }

    bds += 4;
  }

  return lsz;
}
