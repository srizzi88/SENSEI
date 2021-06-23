/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLHyperTreeGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXMLHyperTreeGridWriter.h"

#include "svtkAbstractArray.h"
#include "svtkBitArray.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridNonOrientedCursor.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnsignedLongArray.h"

#define svtkXMLOffsetsManager_DoNotInclude
#include "svtkXMLOffsetsManager.h"
#undef svtkXMLOffsetsManager_DoNotInclude

#include <cassert>

svtkStandardNewMacro(svtkXMLHyperTreeGridWriter);

//----------------------------------------------------------------------------
void svtkXMLHyperTreeGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkHyperTreeGrid* svtkXMLHyperTreeGridWriter::GetInput()
{
  return static_cast<svtkHyperTreeGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLHyperTreeGridWriter::GetDefaultFileExtension()
{
  return "htg";
}

//----------------------------------------------------------------------------
const char* svtkXMLHyperTreeGridWriter::GetDataSetName()
{
  return "HyperTreeGrid";
}

//----------------------------------------------------------------------------
int svtkXMLHyperTreeGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLHyperTreeGridWriter::WriteData()
{
  // write XML header and SVTK file header and file attributes
  if (!this->StartFile())
  {
    return 0;
  }

  svtkIndent indent = svtkIndent().GetNextIndent();

  // Header attributes
  if (!this->StartPrimaryElement(indent))
  {
    return 0;
  }

  // Coordinates for grid (can be replaced by origin and scale)
  if (!this->WriteGrid(indent.GetNextIndent()))
  {
    return 0;
  }

  if (this->GetDataSetMajorVersion() < 1 && !this->WriteTrees_0(indent.GetNextIndent()))
  {
    return 0;
  }
  else if (this->GetDataSetMajorVersion() >= 1 && !this->WriteTrees_1(indent.GetNextIndent()))
  {
    return 0;
  }

  this->WriteFieldData(indent.GetNextIndent());

  if (!this->FinishPrimaryElement(indent))
  {
    return 0;
  }

  // Write all appended data by tree using Helper function
  if (this->DataMode == svtkXMLWriter::Appended)
  {
    svtkHyperTreeGrid* input = this->GetInput();
    svtkPointData* pd = input->GetPointData();
    svtkIdType numberOfPointDataArrays = pd->GetNumberOfArrays();

    this->StartAppendedData();

    // Write the field data arrays.
    if (this->FieldDataOM->GetNumberOfElements())
    {
      svtkNew<svtkFieldData> fieldDataCopy;
      this->UpdateFieldData(fieldDataCopy);

      this->WriteFieldDataAppendedData(fieldDataCopy, this->CurrentTimeIndex, this->FieldDataOM);
      if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
      {
        return 0;
      }
    }

    // Write the Coordinates arrays
    if (this->CoordsOMG->GetNumberOfElements())
    {
      assert(this->CoordsOMG->GetNumberOfElements() == 3);
      this->WriteAppendedArrayDataHelper(input->GetXCoordinates(), this->CoordsOMG->GetElement(0));
      this->WriteAppendedArrayDataHelper(input->GetYCoordinates(), this->CoordsOMG->GetElement(1));
      this->WriteAppendedArrayDataHelper(input->GetZCoordinates(), this->CoordsOMG->GetElement(2));
    }

    // Write the data for each tree
    svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    svtkIdType inIndex;
    int treeIndx = 0;

    if (this->GetDataSetMajorVersion() < 1) // Major version < 1
    {
      while (it.GetNextTree(inIndex))
      {
        svtkHyperTreeGridNonOrientedCursor* inCursor = input->NewNonOrientedCursor(inIndex);
        svtkHyperTree* tree = inCursor->GetTree();
        svtkIdType numberOfVertices = tree->GetNumberOfVertices();

        // Tree Descriptor
        this->WriteAppendedArrayDataHelper(
          this->Descriptors[treeIndx], this->DescriptorOMG->GetElement(treeIndx));
        // Tree Mask
        this->WriteAppendedArrayDataHelper(
          this->Masks[treeIndx], this->MaskOMG->GetElement(treeIndx));
        // Point Data
        for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
        {
          svtkAbstractArray* array = pd->GetAbstractArray(i);
          int pdIndx = treeIndx * numberOfPointDataArrays + i;
          this->WritePointDataAppendedArrayDataHelper(
            array, numberOfVertices, this->PointDataOMG->GetElement(pdIndx), tree);
        }
        ++treeIndx;
        inCursor->Delete();
      }
    }
    else // Major version >= 1
    {
      while (it.GetNextTree(inIndex))
      {
        // Tree Descriptor
        this->WriteAppendedArrayDataHelper(
          this->Descriptors[treeIndx], this->DescriptorOMG->GetElement(treeIndx));
        // Tree NbVerticesbyLevels
        this->WriteAppendedArrayDataHelper(
          this->NbVerticesbyLevels[treeIndx], this->NbVerticesbyLevelOMG->GetElement(treeIndx));
        // Tree Mask
        this->WriteAppendedArrayDataHelper(
          this->Masks[treeIndx], this->MaskOMG->GetElement(treeIndx));
        // Point Data
        svtkIdList* ids = this->Ids[treeIndx];
        svtkIdType numberOfVertices = ids->GetNumberOfIds();
        for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
        {
          svtkAbstractArray* a = pd->GetAbstractArray(i);
          svtkAbstractArray* b = a->NewInstance();
          int numberOfComponents = a->GetNumberOfComponents();
          b->SetNumberOfTuples(numberOfVertices);
          b->SetNumberOfComponents(numberOfComponents);
          b->SetNumberOfValues(numberOfComponents * numberOfVertices);
          // BitArray processed
          svtkBitArray* aBit = svtkArrayDownCast<svtkBitArray>(a);
          if (aBit)
          {
            svtkBitArray* bBit = svtkArrayDownCast<svtkBitArray>(b);
            aBit->GetTuples(ids, bBit);
          } // DataArray processed
          else
          {
            a->GetTuples(ids, b);
          }
          // Write the data or XML description for appended data
          int pdIndx = treeIndx * numberOfPointDataArrays + i;
          this->WriteAppendedArrayDataHelper(b, this->PointDataOMG->GetElement(pdIndx));
          b->Delete();
        }
        ++treeIndx;
      }
    }
    this->EndAppendedData();
  }
  if (!this->EndFile())
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLHyperTreeGridWriter::StartPrimaryElement(svtkIndent indent)
{
  ostream& os = *(this->Stream);

  return (!this->WritePrimaryElement(os, indent)) ? 0 : 1;
}

//----------------------------------------------------------------------------
void svtkXMLHyperTreeGridWriter::WritePrimaryElementAttributes(ostream& os, svtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  svtkHyperTreeGrid* input = this->GetInput();

  int extent[6];
  input->GetExtent(extent);

  if (this->GetDataSetMajorVersion() < 1) // Major version < 1
  {
    this->WriteScalarAttribute("Dimension", (int)input->GetDimension());
    this->WriteScalarAttribute("Orientation", (int)input->GetOrientation());
  }

  this->WriteScalarAttribute("BranchFactor", (int)input->GetBranchFactor());
  this->WriteScalarAttribute("TransposedRootIndexing", (bool)input->GetTransposedRootIndexing());
  this->WriteVectorAttribute(
    "Dimensions", 3, (int*)const_cast<unsigned int*>(input->GetDimensions()));
  if (input->GetHasInterface())
  {
    this->WriteStringAttribute("InterfaceNormalsName", input->GetInterfaceNormalsName());
  }
  if (input->GetHasInterface())
  {
    this->WriteStringAttribute("InterfaceInterceptsName", input->GetInterfaceInterceptsName());
  }

  if (this->GetDataSetMajorVersion() < 1)
  {
    this->WriteScalarAttribute("NumberOfVertices", input->GetNumberOfVertices());
  }
}

//----------------------------------------------------------------------------
int svtkXMLHyperTreeGridWriter::WriteGrid(svtkIndent indent)
{
  svtkHyperTreeGrid* input = this->GetInput();
  ostream& os = *(this->Stream);
  os << indent << "<Grid>\n";

  if (this->DataMode == Appended)
  {
    // Coordinates of the grid
    this->CoordsOMG->Allocate(3, this->NumberOfTimeSteps);
    this->WriteArrayAppended(input->GetXCoordinates(), indent.GetNextIndent(),
      this->CoordsOMG->GetElement(0), "XCoordinates",
      input->GetXCoordinates()->GetNumberOfTuples());
    this->WriteArrayAppended(input->GetYCoordinates(), indent.GetNextIndent(),
      this->CoordsOMG->GetElement(1), "YCoordinates",
      input->GetYCoordinates()->GetNumberOfTuples());
    this->WriteArrayAppended(input->GetZCoordinates(), indent.GetNextIndent(),
      this->CoordsOMG->GetElement(2), "ZCoordinates",
      input->GetZCoordinates()->GetNumberOfTuples());
  }
  else
  {
    // Coordinates of the grid
    this->WriteArrayInline(input->GetXCoordinates(), indent.GetNextIndent(), "XCoordinates",
      input->GetXCoordinates()->GetNumberOfValues());
    this->WriteArrayInline(input->GetYCoordinates(), indent.GetNextIndent(), "YCoordinates",
      input->GetYCoordinates()->GetNumberOfValues());
    this->WriteArrayInline(input->GetZCoordinates(), indent.GetNextIndent(), "ZCoordinates",
      input->GetZCoordinates()->GetNumberOfValues());
  }

  os << indent << "</Grid>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
svtkXMLHyperTreeGridWriter::svtkXMLHyperTreeGridWriter()
  : CoordsOMG(new OffsetsManagerGroup)
  , DescriptorOMG(new OffsetsManagerGroup)
  , NbVerticesbyLevelOMG(new OffsetsManagerGroup)
  , MaskOMG(new OffsetsManagerGroup)
  , PointDataOMG(new OffsetsManagerGroup)
{
}

//----------------------------------------------------------------------------
svtkXMLHyperTreeGridWriter::~svtkXMLHyperTreeGridWriter()
{
  delete this->CoordsOMG;
  delete this->DescriptorOMG;
  delete this->NbVerticesbyLevelOMG;
  delete this->MaskOMG;
  delete this->PointDataOMG;
  for (auto i = this->Descriptors.begin(); i != this->Descriptors.end(); ++i)
  {
    (*i)->Delete();
  }
  for (auto i = this->NbVerticesbyLevels.begin(); i != this->NbVerticesbyLevels.end(); ++i)
  {
    (*i)->Delete();
  }
  for (auto i = this->Masks.begin(); i != this->Masks.end(); ++i)
  {
    (*i)->Delete();
  }
  for (auto i = this->Ids.begin(); i != this->Ids.end(); ++i)
  {
    (*i)->Delete();
  }
}

//----------------------------------------------------------------------------
namespace
{
//
// Depth first recursion to walk the tree in child order
// Used to create the breadth first BitArray descriptor appending
// node and leaf indicator by level
//
void BuildDescriptor(svtkHyperTreeGridNonOrientedCursor* inCursor, int level,
  std::vector<std::string>& descriptor, std::vector<std::string>& mask)
{
  // Retrieve input grid
  svtkHyperTreeGrid* input = inCursor->GetGrid();

  // Append to mask string
  svtkIdType id = inCursor->GetGlobalNodeIndex();
  if (input->HasMask())
  {
    if (input->GetMask()->GetValue(id))
      mask[level] += '1';
    else
      mask[level] += '0';
  }

  // Append to descriptor string
  if (!inCursor->IsLeaf())
  {
    descriptor[level] += 'R';

    // If input cursor is not a leaf, recurse to all children
    int numChildren = input->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      // Create child cursor from parent in input grid
      svtkHyperTreeGridNonOrientedCursor* childCursor = inCursor->Clone();
      childCursor->ToChild(child);

      // Recurse
      BuildDescriptor(childCursor, level + 1, descriptor, mask);

      // Clean up
      childCursor->Delete();
    } // child
  }
  else
  {
    descriptor[level] += '.';
  }
}
}

//----------------------------------------------------------------------------
int svtkXMLHyperTreeGridWriter::WriteTrees_0(svtkIndent indent)
{
  svtkHyperTreeGrid* input = this->GetInput();
  svtkIdType maxLevels = input->GetNumberOfLevels();
  svtkPointData* pd = input->GetPointData();
  svtkIdType numberOfPointDataArrays = pd->GetNumberOfArrays();

  // Count the actual number of hypertrees represented in this hypertree grid
  svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  this->NumberOfTrees = 0;
  svtkIdType inIndex;
  while (it.GetNextTree(inIndex))
  {
    ++this->NumberOfTrees;
  }

  // Allocate offsets managers for appended data
  if (this->DataMode == Appended && this->NumberOfTrees > 0)
  {
    this->DescriptorOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->MaskOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->PointDataOMG->Allocate(
      this->NumberOfTrees * numberOfPointDataArrays, this->NumberOfTimeSteps);
  }

  ostream& os = *(this->Stream);
  os << indent << "<Trees>\n";
  svtkIndent treeIndent = indent.GetNextIndent();

  // Collect description by processing depth first and writing breadth first
  input->InitializeTreeIterator(it);
  int treeIndx = 0;
  svtkIdType globalOffset = 0;
  while (it.GetNextTree(inIndex))
  {
    // Initialize new grid cursor at root of current input tree
    svtkHyperTreeGridNonOrientedCursor* inCursor = input->NewNonOrientedCursor(inIndex);
    svtkHyperTree* tree = inCursor->GetTree();
    svtkIdType numberOfVertices = tree->GetNumberOfVertices();

    os << treeIndent << "<Tree";
    this->WriteScalarAttribute("Index", inIndex);
    this->WriteScalarAttribute("GlobalOffset", globalOffset);
    this->WriteScalarAttribute("NumberOfVertices", numberOfVertices);
    os << ">\n";

    // Recursively compute descriptor for this tree, appending any
    // entries for each of the levels in descByLevel for output.
    // Collect the masked indicator at the same time
    std::vector<std::string> descByLevel(maxLevels);
    std::vector<std::string> maskByLevel(maxLevels);
    BuildDescriptor(inCursor, 0, descByLevel, maskByLevel);

    // Clean up
    inCursor->Delete();

    // Descriptor BitArray
    svtkBitArray* descriptor = svtkBitArray::New();
    std::string::const_iterator liter;
    for (int l = 0; l < maxLevels; ++l)
    {
      for (liter = descByLevel[l].begin(); liter != descByLevel[l].end(); ++liter)
      {
        switch (*liter)
        {
          case 'R': //  Refined cell
            descriptor->InsertNextValue(1);
            break;
          case '.': // Leaf cell
            descriptor->InsertNextValue(0);
            break;
          default:
            svtkErrorMacro(<< "Unrecognized character: " << *liter << " in string "
                          << descByLevel[l]);
            return 0;
        }
      }
    }
    descriptor->Squeeze();
    this->Descriptors.push_back(descriptor);

    // Mask BitAarray
    svtkBitArray* mask = svtkBitArray::New();
    if (input->GetMask())
    {
      for (int l = 0; l < maxLevels; ++l)
      {
        for (liter = maskByLevel[l].begin(); liter != maskByLevel[l].end(); ++liter)
        {
          switch (*liter)
          {
            case '0': // Not masked
              mask->InsertNextValue(0);
              break;
            case '1': // Masked
              mask->InsertNextValue(1);
              break;
            default:
              svtkErrorMacro(<< "Unrecognized character: " << *liter << " in string "
                            << maskByLevel[l]);
              return 0;
          }
        }
      }
      mask->Squeeze();
      this->Masks.push_back(mask);
    }

    svtkIndent infoIndent = treeIndent.GetNextIndent();

    // Write the descriptor and mask BitArrays
    if (this->DataMode == Appended)
    {
      this->WriteArrayAppended(descriptor, infoIndent, this->DescriptorOMG->GetElement(treeIndx),
        "Descriptor", descriptor->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayAppended(
          mask, infoIndent, this->MaskOMG->GetElement(treeIndx), "Mask", mask->GetNumberOfValues());
      }
    }
    else
    {
      this->WriteArrayInline(descriptor, infoIndent, "Descriptor", descriptor->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayInline(mask, infoIndent, "Mask", mask->GetNumberOfValues());
      }
    }

    // Write the point data
    os << infoIndent << "<PointData>\n";
    for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
    {
      svtkAbstractArray* a = pd->GetAbstractArray(i);
      svtkAbstractArray* b = a->NewInstance();
      int numberOfComponents = a->GetNumberOfComponents();
      b->SetNumberOfTuples(numberOfVertices);
      b->SetNumberOfComponents(numberOfComponents);
      for (int e = 0; e < numberOfVertices; ++e)
      {
        // note - we unravel the array contents which may be interleaved in input array.
        // The reader expect that each grid's data will be contiguous and uses "GlobalOffset"
        // to assemble a big array on the other side.
        // The in memory order of elements then isn't necessarily the same but HTG handles that.
        int aDataOffset = tree->GetGlobalIndexFromLocal(e) * numberOfComponents;
        int bDataOffset = e * numberOfComponents;
        for (int c = 0; c < numberOfComponents; ++c)
        {
          b->SetVariantValue(bDataOffset + c, a->GetVariantValue(aDataOffset + c));
        }
      }

      // Write the data or XML description for appended data
      if (this->DataMode == Appended)
      {
        this->WriteArrayAppended(b, infoIndent.GetNextIndent(),
          this->PointDataOMG->GetElement(treeIndx * numberOfPointDataArrays + i), a->GetName(),
          numberOfVertices * numberOfComponents);
      }
      else
      {
        this->WriteArrayInline(
          b, infoIndent.GetNextIndent(), a->GetName(), numberOfVertices * numberOfComponents);
      }
      b->Delete();
    }
    ++treeIndx;

    // Increment to next tree with PointData
    os << infoIndent << "</PointData>\n";
    os << treeIndent << "</Tree>\n";
    globalOffset += numberOfVertices;
  }
  os << indent << "</Trees>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLHyperTreeGridWriter::WriteTrees_1(svtkIndent indent)
{
  svtkHyperTreeGrid* input = this->GetInput();
  svtkPointData* pd = input->GetPointData();
  svtkIdType numberOfPointDataArrays = pd->GetNumberOfArrays();

  // Count the actual number of hypertrees represented in this hypertree grid
  svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  this->NumberOfTrees = 0;
  svtkIdType inIndex;
  while (it.GetNextTree(inIndex))
  {
    ++this->NumberOfTrees;
  }

  // Allocate offsets managers for appended data
  if (this->DataMode == Appended && this->NumberOfTrees > 0)
  {
    this->DescriptorOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->NbVerticesbyLevelOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->MaskOMG->Allocate(this->NumberOfTrees, this->NumberOfTimeSteps);
    this->PointDataOMG->Allocate(
      this->NumberOfTrees * numberOfPointDataArrays, this->NumberOfTimeSteps);
  }

  ostream& os = *(this->Stream);
  os << indent << "<Trees>\n";
  svtkIndent treeIndent = indent.GetNextIndent();

  // Collect description by processing depth first and writing breadth first
  input->InitializeTreeIterator(it);
  int treeIndx = 0;
  while (it.GetNextTree(inIndex))
  {
    os << treeIndent << "<Tree";
    this->WriteScalarAttribute("Index", inIndex);
    svtkHyperTree* tree = input->GetTree(inIndex);
    svtkIdType numberOfLevels = tree->GetNumberOfLevels();
    this->WriteScalarAttribute("NumberOfLevels", numberOfLevels);

    svtkUnsignedLongArray* nbVerticesbyLevel = svtkUnsignedLongArray::New();
    svtkBitArray* descriptor = svtkBitArray::New();
    svtkBitArray* mask = svtkBitArray::New();
    svtkIdList* ids = svtkIdList::New();
    tree->GetByLevelForWriter(input->GetMask(), nbVerticesbyLevel, descriptor, mask, ids);
    this->NbVerticesbyLevels.push_back(nbVerticesbyLevel);
    this->Descriptors.push_back(descriptor);
    this->Masks.push_back(mask);
    this->Ids.push_back(ids);

    svtkIndent infoIndent = treeIndent.GetNextIndent();

    svtkIdType numberOfVertices = ids->GetNumberOfIds();
    // Because non describe last False values...
    assert(numberOfVertices >= descriptor->GetNumberOfTuples());
    assert(numberOfVertices >= mask->GetNumberOfTuples());
    this->WriteScalarAttribute("NumberOfVertices", numberOfVertices);
    os << ">\n";

    // Write the descriptor and mask BitArrays
    if (this->DataMode == Appended)
    {
      this->WriteArrayAppended(descriptor, infoIndent, this->DescriptorOMG->GetElement(treeIndx),
        "Descriptor", descriptor->GetNumberOfValues());
      this->WriteArrayAppended(nbVerticesbyLevel, infoIndent,
        this->NbVerticesbyLevelOMG->GetElement(treeIndx), "NbVerticesByLevel",
        nbVerticesbyLevel->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayAppended(
          mask, infoIndent, this->MaskOMG->GetElement(treeIndx), "Mask", mask->GetNumberOfValues());
      }
    }
    else
    {
      this->WriteArrayInline(descriptor, infoIndent, "Descriptor", descriptor->GetNumberOfValues());
      this->WriteArrayInline(
        nbVerticesbyLevel, infoIndent, "NbVerticesByLevel", nbVerticesbyLevel->GetNumberOfValues());
      if (input->GetMask())
      {
        this->WriteArrayInline(mask, infoIndent, "Mask", mask->GetNumberOfValues());
      }
    }

    // Write the point data
    os << infoIndent << "<PointData>\n";

    for (int i = 0; i < pd->GetNumberOfArrays(); ++i)
    {
      svtkAbstractArray* a = pd->GetAbstractArray(i);

      // Write the data or XML description for appended data
      if (this->DataMode == Appended)
      {
        this->WriteArrayAppended(a, infoIndent.GetNextIndent(),
          this->PointDataOMG->GetElement(treeIndx * numberOfPointDataArrays + i), a->GetName(), 0);
        // Last parameter will eventually become the size of the stored arrays.
        // When this modification is done, 0 should be replaced by:
        // numberOfVertices * a->GetNumberOfComponents());
      }
      else
      {
        svtkAbstractArray* b = a->NewInstance();
        int numberOfComponents = a->GetNumberOfComponents();
        b->SetNumberOfTuples(numberOfVertices);
        b->SetNumberOfComponents(numberOfComponents);
        b->SetNumberOfValues(numberOfComponents * numberOfVertices);
        // BitArray processed
        svtkBitArray* aBit = svtkArrayDownCast<svtkBitArray>(a);
        if (aBit)
        {
          svtkBitArray* bBit = svtkArrayDownCast<svtkBitArray>(b);
          aBit->GetTuples(ids, bBit);
        } // DataArray processed
        else
        {
          a->GetTuples(ids, b);
        }
        this->WriteArrayInline(
          b, infoIndent.GetNextIndent(), a->GetName(), b->GetNumberOfTuples() * numberOfComponents);
        b->Delete();
      }
    }
    ++treeIndx;

    // Increment to next tree with PointData
    os << infoIndent << "</PointData>\n";
    os << treeIndent << "</Tree>\n";
  }
  os << indent << "</Trees>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLHyperTreeGridWriter::FinishPrimaryElement(svtkIndent indent)
{
  ostream& os = *(this->Stream);

  // End the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLHyperTreeGridWriter::WriteAppendedArrayDataHelper(
  svtkAbstractArray* array, OffsetsManager& offsets)
{
  this->WriteArrayAppendedData(array, offsets.GetPosition(this->CurrentTimeIndex),
    offsets.GetOffsetValue(this->CurrentTimeIndex));

  svtkDataArray* dArray = svtkArrayDownCast<svtkDataArray>(array);
  if (dArray)
  {
    double* range = dArray->GetRange(-1);
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMinPosition(this->CurrentTimeIndex), range[0], "RangeMin");
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMaxPosition(this->CurrentTimeIndex), range[1], "RangeMax");
  }
}

//----------------------------------------------------------------------------
void svtkXMLHyperTreeGridWriter::WritePointDataAppendedArrayDataHelper(
  svtkAbstractArray* a, svtkIdType numberOfVertices, OffsetsManager& offsets, svtkHyperTree* tree)
{
  svtkAbstractArray* b = a->NewInstance();
  int numberOfComponents = a->GetNumberOfComponents();

  b->SetNumberOfComponents(numberOfComponents);
  b->SetNumberOfTuples(numberOfVertices);
  for (int e = 0; e < numberOfComponents * numberOfVertices; ++e)
  {
    b->SetVariantValue(e, a->GetVariantValue(tree->GetGlobalIndexFromLocal(e)));
  }

  this->WriteArrayAppendedData(
    b, offsets.GetPosition(this->CurrentTimeIndex), offsets.GetOffsetValue(this->CurrentTimeIndex));

  svtkDataArray* dArray = svtkArrayDownCast<svtkDataArray>(a);
  if (dArray)
  {
    double* range = dArray->GetRange(-1);
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMinPosition(this->CurrentTimeIndex), range[0], "RangeMin");
    this->ForwardAppendedDataDouble(
      offsets.GetRangeMaxPosition(this->CurrentTimeIndex), range[1], "RangeMax");
  }
  b->Delete();
}
