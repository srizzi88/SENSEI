/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataObjectGenerator.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

#include "svtkDataObjectTypes.h"
#include "svtkImageData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkUniformGrid.h"
#include "svtkUnstructuredGrid.h"

#include "svtkAMRBox.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkMultiBlockDataSet.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkPointData.h"

#include "svtkStreamingDemandDrivenPipeline.h"
#include <vector>

svtkStandardNewMacro(svtkDataObjectGenerator);

//============================================================================
enum svtkDataObjectGeneratorTokenCodes
{
  ID1,
  ID2,
  UF1,
  RG1,
  SG1,
  PD1,
  PD2,
  UG1,
  UG2,
  UG3,
  UG4,
  GS,
  GE,
  HBS,
  HBE,
  MBS,
  MBE,
  NUMTOKENS
};

const char svtkDataObjectGeneratorTokenStrings[NUMTOKENS][4] = {
  "ID1",
  "ID2",
  "UF1",
  "RG1",
  "SG1",
  "PD1",
  "PD2",
  "UG1",
  "UG2",
  "UG3",
  "UG4",
  "(",
  ")",
  "HB[",
  "]",
  "MB{",
  "}",
};

const char svtkDataObjectGeneratorTypeStrings[NUMTOKENS][30] = {
  "svtkImageData",
  "svtkImageData",
  "svtkUniformGrid",
  "svtkRectilinearGrid",
  "svtkStructuredGrid",
  "svtkPolyData",
  "svtkPolyData",
  "svtkUnstructuredGrid",
  "svtkUnstructuredGrid",
  "svtkUnstructuredGrid",
  "svtkUnstructuredGrid",
  "NA",
  "NA",
  "svtkHierarchicalBoxDataSet",
  "NA",
  "svtkMultiBlockDataSet",
  "NA",
};

//============================================================================
class svtkInternalStructureCache
{
  // a class to keep the overall structure in memory in. It is a simple tree
  // where each node has a data set type flag and pointers to children
public:
  svtkInternalStructureCache()
  {
    type = -1;
    parent = nullptr;
  }
  ~svtkInternalStructureCache()
  {
    std::vector<svtkInternalStructureCache*>::iterator it;
    for (it = this->children.begin(); it != this->children.end(); ++it)
    {
      delete *it;
    }
  }
  svtkInternalStructureCache* add_dataset(int t)
  {
    svtkInternalStructureCache* child = new svtkInternalStructureCache();
    child->type = t;
    child->parent = this;
    children.push_back(child);
    return child;
  }
  void print(int level = 0)
  {
    for (int i = 0; i < level; i++)
    {
      cerr << " ";
    }
    if (type >= 0)
    {
      cerr << svtkDataObjectGeneratorTokenStrings[type] << endl;
    }
    else
    {
      cerr << "HOLDER" << endl;
    }
    std::vector<svtkInternalStructureCache*>::iterator it;
    for (it = this->children.begin(); it != this->children.end(); ++it)
    {
      (*it)->print(level + 1);
    }
    if (type == GS || type == HBS || type == MBS)
    {
      for (int i = 0; i < level; i++)
      {
        cerr << " ";
      }
      switch (type)
      {
        case GS:
          cerr << svtkDataObjectGeneratorTokenStrings[GE] << endl;
          break;
        case HBS:
          cerr << svtkDataObjectGeneratorTokenStrings[HBE] << endl;
          break;
        case MBS:
          cerr << svtkDataObjectGeneratorTokenStrings[MBE] << endl;
          break;
      }
    }
  }

  int type;
  svtkInternalStructureCache* parent;
  std::vector<svtkInternalStructureCache*> children;
};

//----------------------------------------------------------------------------
// search the head of the input string for one of the tokens we know how to
// do something with. If we see something, bump char ptr passed it, and return
// a code that says what we found. Skip over chars we don't recognize. When
// nothing is left in the string return -1.
static int svtkDataObjectGeneratorGetNextToken(char** str)
{
  if (!str || !*str)
  {
    return 0;
  }

  size_t len = strlen(*str);
  size_t l;
  while (len && *str)
  {
    for (int i = 0; i < NUMTOKENS; i++)
    {
      l = strlen(svtkDataObjectGeneratorTokenStrings[i]);
      if (len >= l && !strncmp(*str, svtkDataObjectGeneratorTokenStrings[i], l))
      {
        *str += l;
        return i;
      }
    }
    len--;
    *str += 1;
  }
  return -1;
}

//----------------------------------------------------------------------------
static svtkInternalStructureCache* svtkDataObjectGeneratorParseStructure(char* Program)
{
  svtkInternalStructureCache* structure = new svtkInternalStructureCache();
  svtkInternalStructureCache* sptr = structure;

  char* ptr = Program;
  bool done = false;
  while (!done)
  {
    switch (svtkDataObjectGeneratorGetNextToken(&ptr))
    {
      case ID1:
        sptr->add_dataset(ID1);
        break;
      case ID2:
        sptr->add_dataset(ID2);
        break;
      case UF1:
        sptr->add_dataset(UF1);
        break;
      case RG1:
        sptr->add_dataset(RG1);
        break;
      case SG1:
        sptr->add_dataset(SG1);
        break;
      case PD1:
        sptr->add_dataset(PD1);
        break;
      case PD2:
        sptr->add_dataset(PD2);
        break;
      case UG1:
        sptr->add_dataset(UG1);
        break;
      case UG2:
        sptr->add_dataset(UG2);
        break;
      case UG3:
        sptr->add_dataset(UG3);
        break;
      case UG4:
        sptr->add_dataset(UG4);
        break;
      case GS:
      {
        svtkInternalStructureCache* cptr = sptr->add_dataset(GS);
        sptr = cptr;
        break;
      }
      case HBS:
      {
        svtkInternalStructureCache* cptr = sptr->add_dataset(HBS);
        sptr = cptr;
        break;
      }
      case MBS:
      {
        svtkInternalStructureCache* cptr = sptr->add_dataset(MBS);
        sptr = cptr;
        break;
      }
      case GE:
      case HBE:
      case MBE:
        sptr = sptr->parent;
        break;
      default:
        done = true;
    }
  }

  return structure;
}

//----------------------------------------------------------------------------
svtkDataObjectGenerator::svtkDataObjectGenerator()
{
  this->SetNumberOfInputPorts(0);

  this->Program = nullptr;
  this->SetProgram("ID1");
  this->Structure = nullptr;

  this->CellIdCounter = 0;
  this->PointIdCounter = 0;
  this->XOffset = 0.0;
  this->YOffset = 0.0;
  this->ZOffset = 0.0;
}

//----------------------------------------------------------------------------
svtkDataObjectGenerator::~svtkDataObjectGenerator()
{
  this->SetProgram(nullptr);
  delete this->Structure;
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Program: " << (this->Program ? this->Program : "(none)") << "\n";
}

//----------------------------------------------------------------------------
int svtkDataObjectGenerator::RequestDataObject(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outV)
{
  svtkInformation* outInfo = outV->GetInformationObject(0);
  svtkDataObject* outData = nullptr;

  if (!this->Program)
  {
    // svtkErrorMacro("No string to generate data objects for");
    return SVTK_OK;
  }

  delete this->Structure;
  this->Structure = svtkDataObjectGeneratorParseStructure(this->Program);
  outData = this->CreateOutputDataObjects(this->Structure);
  if (outData)
  {
    outInfo->Set(svtkDataObject::DATA_OBJECT(), outData);
    outData->Delete();
  }
  return SVTK_OK;
}

//---------------------------------------------------------------------------
svtkDataObject* svtkDataObjectGenerator::CreateOutputDataObjects(svtkInternalStructureCache* structure)
{
  svtkDataObject* outData;
  switch (structure->type)
  {
    case -1: // top holder it should hold a single data set, use it
    {
      if (structure->children.empty())
      {
        return nullptr;
      }
      return this->CreateOutputDataObjects(structure->children.front());
    }
    case ID1:
    case ID2:
    case UF1:
    case RG1:
    case SG1:
    case PD1:
    case PD2:
    case UG1:
    case UG2:
    case UG3:
    case UG4:
    {
      /*
      cerr
        << "Creating "
        << svtkDataObjectGeneratorTypeStrings[structure->type]
        << endl;
      */
      outData =
        svtkDataObjectTypes::NewDataObject(svtkDataObjectGeneratorTypeStrings[structure->type]);
      return outData;
    }
    case HBS:
    case MBS:
    {
      // only create top level struct in RequestDataObject, do not recurse
      // the contents of the structure is cleared before RequestData anyway
      /*
      cerr
        << "Creating "
        << svtkDataObjectGeneratorTypeStrings[structure->type]
        << endl;
      */
      outData =
        svtkDataObjectTypes::NewDataObject(svtkDataObjectGeneratorTypeStrings[structure->type]);
      return outData;
    }
    case HBE: // should never be created
    case MBE: // should never be created
    case GS:  // should be skipped over by MBS
    case GE:  // should never be created
    default:
      // cerr << "UH OH" << endl;
      return nullptr;
  }
}

//----------------------------------------------------------------------------
int svtkDataObjectGenerator::RequestInformation(
  svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV)
{
  if (!this->Structure)
  {
    // svtkErrorMacro("Program has not been parsed.");
    return SVTK_OK;
  }

  if (this->Structure->children.empty())
  {
    svtkErrorMacro("Program was invalid.");
    return SVTK_ERROR;
  }

  // Say that this filter can break up its output into any number of pieces
  svtkInformation* outInfo = outV->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // If my output is an atomic structured type, fill in the whole extent info
  svtkInternalStructureCache* top = this->Structure->children.front();
  int t = top->type;
  if (t == ID1 || t == RG1 || t == SG1)
  {
    int ext[6];
    ext[0] = 0;
    ext[1] = 1;
    ext[2] = 0;
    ext[3] = 1;
    ext[4] = 0;
    ext[5] = 1;
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
    double spacing[3];
    spacing[0] = 1.0;
    spacing[1] = 1.0;
    spacing[2] = 1.0;
    outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
    double origin[3];
    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = 0.0;
    outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  }
  if (t == ID2)
  {
    int ext[6];
    ext[0] = 0;
    ext[1] = 2;
    ext[2] = 0;
    ext[3] = 3;
    ext[4] = 0;
    ext[5] = 4;
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
    double spacing[3];
    spacing[0] = 1.0;
    spacing[1] = 1.0;
    spacing[2] = 1.0;
    outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
    double origin[3];
    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = 0.0;
    outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  }
  if (t == UF1)
  {
    int ext[6];
    ext[0] = 0;
    ext[1] = 2;
    ext[2] = 0;
    ext[3] = 2;
    ext[4] = 0;
    ext[5] = 2;
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
    double spacing[3];
    spacing[0] = 0.5;
    spacing[1] = 0.5;
    spacing[2] = 0.5;
    outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
    double origin[3];
    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = 0.0;
    outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  }

  // Could create svtkCompositeDataInformation here.
  return this->Superclass::RequestInformation(req, inV, outV);
}

//----------------------------------------------------------------------------
int svtkDataObjectGenerator::RequestUpdateExtent(
  svtkInformation* req, svtkInformationVector** inV, svtkInformationVector* outV)
{
  // This is a source and doesn't have any inputs.
  // I can defer this to the parent class because it does not have any
  // inputs to request extent/pieces from dependent on what is requested by
  // my outputs.
  return this->Superclass::RequestUpdateExtent(req, inV, outV);
}

//----------------------------------------------------------------------------
int svtkDataObjectGenerator::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outV)
{
  if (!this->Structure)
  {
    // svtkErrorMacro("Program has not been parsed");
    return SVTK_OK;
  }

  // For parallel processing, this will stripe the datasets contained
  // in the first level of composite data sets.
  svtkInformation* outInfo = outV->GetInformationObject(0);
  svtkDataObject* outStructure = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!outStructure)
  {
    return SVTK_ERROR;
  }

  this->Rank = 0;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    this->Rank = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  }
  this->Processors = 1;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    this->Processors = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }

  // this->Rank = 0;
  // this->Processors = 1;

  this->CellIdCounter = 0;
  this->PointIdCounter = 0;

  svtkDataObject* outData = this->FillOutputDataObjects(this->Structure, -1);
  outStructure->ShallowCopy(outData);
  if (outData)
  {
    outData->Delete();
  }
  else
  {
    svtkErrorMacro("Program was invalid.");
    return SVTK_ERROR;
  }

  return SVTK_OK;
}

//---------------------------------------------------------------------------
svtkDataObject* svtkDataObjectGenerator::FillOutputDataObjects(
  svtkInternalStructureCache* structure, int level, int stripe)
{
  svtkDataObject* outData = nullptr;
  int t = structure->type;
  if (t != -1 && t != HBE && t != MBE && t != GS && t != GE)
  {
    if (level == 1 && (structure->parent->parent->type == MBS) &&
      ((stripe % this->Processors) != this->Rank))
    {
      // for parallel processing, each processor gets a different set of
      // stripes of the data sets within the groups in the first level of
      // composite data sets
      /*
      cerr << this->Rank << "/" << this->Processors
           << " Ignoring "
           << stripe << "->"
           << svtkDataObjectGeneratorTypeStrings[t]
           << endl;
      */
      return nullptr;
    }
    else
    {
      /*
      cerr << "Filling "
           << svtkDataObjectGeneratorTypeStrings[t]
           << endl;
      */
      outData = svtkDataObjectTypes::NewDataObject(svtkDataObjectGeneratorTypeStrings[t]);
    }
  }

  switch (t)
  {
    case -1: // top level is a holder, use the single data set inside instead
    {
      if (structure->children.empty())
      {
        return nullptr;
      }
      return this->FillOutputDataObjects(structure->children.front(), level + 1);
    }
    case ID1:
    {
      this->MakeImageData1(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case ID2:
    {
      this->MakeImageData2(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case UF1:
    {
      this->MakeUniformGrid1(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case RG1:
    {
      this->MakeRectilinearGrid1(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case SG1:
    {
      this->MakeStructuredGrid1(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case PD1:
    {
      this->MakePolyData1(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case PD2:
    {
      this->MakePolyData2(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case UG1:
    {
      this->MakeUnstructuredGrid1(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case UG2:
    {
      this->MakeUnstructuredGrid2(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case UG3:
    {
      this->MakeUnstructuredGrid3(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case UG4:
    {
      this->MakeUnstructuredGrid4(svtkDataSet::SafeDownCast(outData));
      return outData;
    }
    case HBS:
    {
      // Making octrees, structured can grid up space arbitratily though

      svtkHierarchicalBoxDataSet* hbo = svtkHierarchicalBoxDataSet::SafeDownCast(outData);

      std::vector<int> blocksPerLevel;
      std::vector<svtkInternalStructureCache*>::iterator git;
      for (git = structure->children.begin(); git != structure->children.end(); ++git)
      {
        svtkInternalStructureCache* gptr = *git;
        svtkIdType nds = static_cast<svtkIdType>(gptr->children.size());
        blocksPerLevel.push_back(nds);
      }

      double origin[3] = { 0, 0, 0 };
      hbo->Initialize(static_cast<int>(blocksPerLevel.size()), &blocksPerLevel[0]);
      hbo->SetOrigin(origin);
      hbo->SetGridDescription(SVTK_XYZ_GRID);
      svtkIdType gcnt = 0;
      for (git = structure->children.begin(); git != structure->children.end(); ++git)
      {
        // cerr << "LVL=" << gcnt  << endl;

        svtkInternalStructureCache* gptr = *git;
        // gptr->type should be a group

        // each of the dimensions of each parent cell are broken into this
        // many pieces this must be the inverse of the spacing for the geometry
        // to line up
        int refinement = 2;
        hbo->SetRefinementRatio(gcnt, refinement);

        std::vector<svtkInternalStructureCache*>::iterator dit;
        int dcnt = 0; // TODO: read in a location to create sparse trees

        int maxchildren = static_cast<int>(pow(8.0, static_cast<double>(gcnt)));
        // making octrees, this is total number of possible children in this level

        int r2 = static_cast<int>(pow(static_cast<double>(refinement), static_cast<double>(gcnt)));
        // how many children across each dimension

        for (dit = gptr->children.begin();
             dit != gptr->children.end() && dcnt < maxchildren // ignore extra children
             ;
             ++dit)
        {
          // cerr << "DS=" << dcnt  << endl;
          svtkInternalStructureCache* dptr = *dit;
          // dptr->type should be UF1

          // Figure out where in the parent level the cells of this new data
          // set resides, this is used to create blanking parent child
          // relationships
          //*2 is because each child is 2 cells across
          //+1 (in hi) is because we are counting cells inclusively.
          // If children were 3x3x3 it would be *3+2
          // Note, other orderings are equally valid
          const int lo[3] = { dcnt / (r2 * r2) % r2 * 2, dcnt / r2 % r2 * 2, dcnt % r2 * 2 };
          const int hi[3] = { dcnt / (r2 * r2) % r2 * 2 + 1, dcnt / r2 % r2 * 2 + 1,
            dcnt % r2 * 2 + 1 };

          /*
          cerr << "LO=" << lo[0] << "," << lo[1] << "," << lo[2] << " "
               << "HI=" << hi[0] << "," << hi[1] << "," << hi[2] << endl;
          */
          svtkDataObject* dobj = nullptr;
          double spacing = pow(0.5, static_cast<double>(gcnt + 1)); //==1.0/(2*r2)

          // restrict HierarchicalBoxes's to contain only UniformGrids
          // until I make it read location to make sparse creation easy, put
          // dummy dataobjects in as placeholders
          if (dptr->type == UF1)
          {
            dobj = this->FillOutputDataObjects(dptr, level + 1, dcnt);
            svtkUniformGrid* uf = svtkUniformGrid::SafeDownCast(dobj);
            // scale and translate the children to align with the parent the
            // blanking information
            uf->SetSpacing(spacing, spacing, spacing);
            double spa[3];
            uf->GetSpacing(spa);
            // cerr << "SPACE=" <<spa[0] <<"," <<spa[1] <<"," <<spa[2] <<endl;
            double org[3];
            uf->SetOrigin(lo[0] * spacing, lo[1] * spacing, lo[2] * spacing);
            uf->GetOrigin(org);
            // cerr << "ORIGIN=" <<org[0] <<"," <<org[1] <<"," <<org[2] <<endl;
            uf->SetExtent(0, 2, 0, 2, 0, 2); // octrees, 2 cells == 3 points across
            int ex[6];
            uf->GetExtent(ex);
          }

          svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(dobj);
          if (grid)
          {
            hbo->SetDataSet(gcnt, dcnt, grid);
          }
          else
          {
            svtkAMRBox box(lo, hi);
            double h[3] = { spacing, spacing, spacing };
            hbo->SetSpacing(gcnt, h);
            hbo->SetAMRBox(gcnt, dcnt, box);
          }

          if (dobj)
          {
            dobj->Delete();
          }
          dcnt++;
        }
        gcnt++;
      }
      return outData;
    }
    case MBS:
    {
      svtkMultiBlockDataSet* mbo = svtkMultiBlockDataSet::SafeDownCast(outData);

      this->YOffset += 1.0;
      // fill in the contents of this multiblockdataset
      // by iterating over the children of all my children (which must be groups)
      mbo->SetNumberOfBlocks(static_cast<unsigned int>(structure->children.size()));
      std::vector<svtkInternalStructureCache*>::iterator git;
      svtkIdType gcnt = 0;

      for (git = structure->children.begin(); git != structure->children.end(); ++git)
      {
        this->ZOffset += 1.0;
        svtkInternalStructureCache* gptr = *git;
        if (gptr->type == GS)
        {
          svtkErrorMacro("Group inside multi-block is not supported");
          continue;
        }
        svtkDataObject* dobj = this->FillOutputDataObjects(gptr, level + 1, 0);
        mbo->SetBlock(gcnt, dobj);
        if (dobj)
        {
          dobj->Delete();
        }
        gcnt++;
      }
      this->ZOffset -= gcnt;

      this->YOffset -= 1.0;

      return outData;
    }
    case HBE: // should never be created
    case MBE: // should never be created
    case GS:  // should be skipped over by MBS
    case GE:  // should never be created
    default:
      // cerr << "UH OH" << endl;
      return nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeValues(svtkDataSet* ds)
{
  svtkIdTypeArray* ids;
  svtkDoubleArray* xcoords;
  svtkDoubleArray* ycoords;
  svtkDoubleArray* zcoords;
  svtkIdType num;

  num = ds->GetNumberOfCells();

  // give each cell a unique id and record its centroid
  ids = svtkIdTypeArray::New();
  ids->SetName("Cell Ids");
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(num);
  xcoords = svtkDoubleArray::New();
  xcoords->SetName("Cell X");
  xcoords->SetNumberOfComponents(1);
  xcoords->SetNumberOfTuples(num);
  ycoords = svtkDoubleArray::New();
  ycoords->SetName("Cell Y");
  ycoords->SetNumberOfComponents(1);
  ycoords->SetNumberOfTuples(num);
  zcoords = svtkDoubleArray::New();
  zcoords->SetName("Cell Z");
  zcoords->SetNumberOfComponents(1);
  zcoords->SetNumberOfTuples(num);

  for (svtkIdType i = 0; i < num; i++)
  {
    ids->SetValue(i, this->CellIdCounter++);
    const double* bds = ds->GetCell(i)->GetBounds();
    xcoords->SetValue(i, (bds[0] + bds[1]) * 0.5);
    ycoords->SetValue(i, (bds[2] + bds[3]) * 0.5);
    zcoords->SetValue(i, (bds[4] + bds[5]) * 0.5);
  }
  ds->GetCellData()->SetGlobalIds(ids);
  ds->GetCellData()->AddArray(xcoords);
  ds->GetCellData()->AddArray(ycoords);
  ds->GetCellData()->AddArray(zcoords);

  ids->Delete();
  xcoords->Delete();
  ycoords->Delete();
  zcoords->Delete();

  // give each point a unique id and record its location
  num = ds->GetNumberOfPoints();
  ids = svtkIdTypeArray::New();
  ids->SetName("Point Ids");
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(num);
  xcoords = svtkDoubleArray::New();
  xcoords->SetName("Point X");
  xcoords->SetNumberOfComponents(1);
  xcoords->SetNumberOfTuples(num);
  ycoords = svtkDoubleArray::New();
  ycoords->SetName("Point Y");
  ycoords->SetNumberOfComponents(1);
  ycoords->SetNumberOfTuples(num);
  zcoords = svtkDoubleArray::New();
  zcoords->SetName("Point Z");
  zcoords->SetNumberOfComponents(1);
  zcoords->SetNumberOfTuples(num);

  for (svtkIdType i = 0; i < num; i++)
  {
    ids->SetValue(i, this->PointIdCounter++);
    double* coords = ds->GetPoint(i);
    xcoords->SetValue(i, coords[0]);
    ycoords->SetValue(i, coords[1]);
    zcoords->SetValue(i, coords[2]);
  }
  ds->GetPointData()->SetGlobalIds(ids);
  ds->GetPointData()->AddArray(xcoords);
  ds->GetPointData()->AddArray(ycoords);
  ds->GetPointData()->AddArray(zcoords);

  ids->Delete();
  xcoords->Delete();
  ycoords->Delete();
  zcoords->Delete();
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeImageData1(svtkDataSet* ids)
{
  // ID1 == an ImageData of 1 voxel
  svtkImageData* ds = svtkImageData::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }

  ds->Initialize();
  ds->SetDimensions(2, 2, 2); // 1 cell
  ds->SetOrigin(this->XOffset, this->YOffset, this->ZOffset);
  ds->SetSpacing(1.0, 1.0, 1.0);

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeImageData2(svtkDataSet* ids)
{
  // ID2 == an ImageData of 24 voxel2
  svtkImageData* ds = svtkImageData::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }

  ds->Initialize();
  ds->SetDimensions(3, 4, 5); // 24 cells
  ds->SetOrigin(this->XOffset, this->YOffset, this->ZOffset);
  ds->SetSpacing(1.0, 1.0, 1.0);

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeUniformGrid1(svtkDataSet* ids)
{
  // UF1 == an UniformGrid of 8 voxels
  svtkUniformGrid* ds = svtkUniformGrid::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }

  ds->Initialize();
  ds->SetDimensions(3, 3, 3); // 8 cells to make octrees
  ds->SetOrigin(this->XOffset, this->YOffset, this->ZOffset);
  ds->SetSpacing(0.5, 0.5, 0.5);

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeRectilinearGrid1(svtkDataSet* ids)
{
  // RG1 = a RectilnearGrid of 1 voxel
  svtkRectilinearGrid* ds = svtkRectilinearGrid::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }

  ds->Initialize();
  ds->SetDimensions(2, 2, 2); // 1 cell
  svtkDoubleArray* da;

  da = svtkDoubleArray::New();
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(2);
  da->SetName("X Coords");
  da->SetValue(0, this->XOffset);
  da->SetValue(1, this->XOffset + 1.0);
  ds->SetXCoordinates(da);
  da->Delete();

  da = svtkDoubleArray::New();
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(2);
  da->SetName("Y Coords");
  da->SetValue(0, this->YOffset);
  da->SetValue(1, this->YOffset + 1.0);
  ds->SetYCoordinates(da);
  da->Delete();

  da = svtkDoubleArray::New();
  da->SetNumberOfComponents(1);
  da->SetNumberOfTuples(2);
  da->SetName("Z Coords");
  da->SetValue(0, this->ZOffset);
  da->SetValue(1, this->ZOffset + 1.0);
  ds->SetZCoordinates(da);
  da->Delete();

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeStructuredGrid1(svtkDataSet* ids)
{
  // SG1 = a StructuredGrid of 1 voxel
  svtkStructuredGrid* ds = svtkStructuredGrid::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }

  ds->Initialize();
  ds->SetDimensions(2, 2, 2); // 1 cell
  svtkPoints* pts = svtkPoints::New();
  const double& XO = this->XOffset;
  const double& YO = this->YOffset;
  const double& ZO = this->ZOffset;
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 1.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 1.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 1.0);
  pts->InsertNextPoint(XO + 1.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 1.0, ZO + 1.0);
  ds->SetPoints(pts);
  pts->Delete();

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakePolyData1(svtkDataSet* ids)
{
  // PD1 = a PolyData of 1 triangle
  svtkPolyData* ds = svtkPolyData::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }

  ds->Initialize();
  svtkPoints* pts = svtkPoints::New();
  const double& XO = this->XOffset;
  const double& YO = this->YOffset;
  const double& ZO = this->ZOffset;
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 0.0);
  ds->SetPoints(pts);
  pts->Delete();
  ds->AllocateExact(0, 0, 0, 0, 1, 3, 0, 0);
  svtkIdType verts[3] = { 0, 1, 2 };
  ds->InsertNextCell(SVTK_TRIANGLE, 3, verts);
  ds->Squeeze();

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakePolyData2(svtkDataSet* ids)
{
  // PD2 = a PolyData of 1 triangle and 1 point
  svtkPolyData* ds = svtkPolyData::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }

  ds->Initialize();
  svtkPoints* pts = svtkPoints::New();
  const double& XO = this->XOffset;
  const double& YO = this->YOffset;
  const double& ZO = this->ZOffset;
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 2.0, YO + 0.5, ZO + 0.5);
  ds->SetPoints(pts);
  pts->Delete();
  ds->AllocateExact(1, 1, 0, 0, 1, 3, 0, 0);
  svtkIdType verts[3] = { 0, 1, 2 };
  ds->InsertNextCell(SVTK_TRIANGLE, 3, verts);
  svtkIdType points[1] = { 3 };
  ds->InsertNextCell(SVTK_VERTEX, 1, points);
  ds->Squeeze();

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeUnstructuredGrid1(svtkDataSet* ids)
{
  // UG1 = an UnstructuredGrid of 1 triangle
  svtkUnstructuredGrid* ds = svtkUnstructuredGrid::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }
  ds->Initialize();
  svtkPoints* pts = svtkPoints::New();
  const double& XO = this->XOffset;
  const double& YO = this->YOffset;
  const double& ZO = this->ZOffset;
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 0.0);
  ds->SetPoints(pts);
  pts->Delete();
  ds->Allocate();
  svtkIdType verts[3] = { 0, 1, 2 };
  ds->InsertNextCell(SVTK_TRIANGLE, 3, verts);
  ds->Squeeze();

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeUnstructuredGrid2(svtkDataSet* ids)
{
  // UG2 = an UnstructuredGrid of 2 triangles
  svtkUnstructuredGrid* ds = svtkUnstructuredGrid::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }
  ds->Initialize();
  svtkPoints* pts = svtkPoints::New();
  const double& XO = this->XOffset;
  const double& YO = this->YOffset;
  const double& ZO = this->ZOffset;
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 1.0, ZO + 0.0);
  ds->SetPoints(pts);
  pts->Delete();
  ds->Allocate();
  svtkIdType verts[6] = { 0, 1, 2, 2, 1, 3 };
  ds->InsertNextCell(SVTK_TRIANGLE, 3, &verts[0]);
  ds->InsertNextCell(SVTK_TRIANGLE, 3, &verts[3]);
  ds->Squeeze();

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeUnstructuredGrid3(svtkDataSet* ids)
{
  // UG3 = an UnstructuredGrid of 1 tet
  svtkUnstructuredGrid* ds = svtkUnstructuredGrid::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }
  ds->Initialize();
  svtkPoints* pts = svtkPoints::New();
  const double& XO = this->XOffset;
  const double& YO = this->YOffset;
  const double& ZO = this->ZOffset;
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.5, YO + 0.5, ZO + 1.0);
  ds->SetPoints(pts);
  pts->Delete();
  ds->Allocate();
  svtkIdType verts[6] = { 0, 1, 2, 3 };
  ds->InsertNextCell(SVTK_TETRA, 4, &verts[0]);
  ds->Squeeze();

  this->MakeValues(ds);
}

//----------------------------------------------------------------------------
void svtkDataObjectGenerator::MakeUnstructuredGrid4(svtkDataSet* ids)
{
  // UG4 = an UnstructuredGrid of 2 triangles and 1 tetraheda
  svtkUnstructuredGrid* ds = svtkUnstructuredGrid::SafeDownCast(ids);
  if (!ds)
  {
    return;
  }
  ds->Initialize();
  svtkPoints* pts = svtkPoints::New();
  const double& XO = this->XOffset;
  const double& YO = this->YOffset;
  const double& ZO = this->ZOffset;
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 1.0, YO + 1.0, ZO + 0.0);
  pts->InsertNextPoint(XO + 0.0, YO + 0.0, ZO + 1.0);
  pts->InsertNextPoint(XO + 0.0, YO + 1.0, ZO + 1.0);
  pts->InsertNextPoint(XO + 1.0, YO + 0.0, ZO + 1.0);
  pts->InsertNextPoint(XO + 0.5, YO + 0.5, ZO + 2.0);
  ds->SetPoints(pts);
  pts->Delete();
  ds->Allocate();

  svtkIdType verts[10] = { 0, 1, 2, 2, 1, 3, 4, 5, 6, 7 };
  ds->InsertNextCell(SVTK_TRIANGLE, 3, &verts[0]);
  ds->InsertNextCell(SVTK_TRIANGLE, 3, &verts[3]);
  ds->InsertNextCell(SVTK_TETRA, 4, &verts[6]);

  ds->Squeeze();

  this->MakeValues(ds);
}
