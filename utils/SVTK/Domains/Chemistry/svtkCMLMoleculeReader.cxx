/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCMLMoleculeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkCMLMoleculeReader.h"

#include "svtkDataObject.h"
#include "svtkExecutive.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPeriodicTable.h"
#include "svtkXMLParser.h"

#include "svtksys/SystemTools.hxx"

#include <string>
#include <vector>

// Subclass of svtkXMLParser -- definitions at end of file
class svtkCMLParser : public svtkXMLParser
{
public:
  svtkTypeMacro(svtkCMLParser, svtkXMLParser);
  static svtkCMLParser* New();

  svtkSetObjectMacro(Target, svtkMolecule);
  svtkGetObjectMacro(Target, svtkMolecule);

protected:
  svtkCMLParser();
  ~svtkCMLParser() override;
  void StartElement(const char* name, const char** attr) override;
  void EndElement(const char* name) override;

  std::vector<std::string> AtomNames;

  svtkMolecule* Target;

  void NewMolecule(const char** attr);
  void NewAtom(const char** attr);
  void NewBond(const char** attr);

  svtkNew<svtkPeriodicTable> pTab;

private:
  svtkCMLParser(const svtkCMLParser&) = delete;
  void operator=(const svtkCMLParser&) = delete;
};

svtkStandardNewMacro(svtkCMLMoleculeReader);

//----------------------------------------------------------------------------
svtkCMLMoleculeReader::svtkCMLMoleculeReader()
  : FileName(nullptr)
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkCMLMoleculeReader::~svtkCMLMoleculeReader()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
svtkMolecule* svtkCMLMoleculeReader::GetOutput()
{
  return svtkMolecule::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
void svtkCMLMoleculeReader::SetOutput(svtkMolecule* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

int svtkCMLMoleculeReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{

  svtkMolecule* output = svtkMolecule::SafeDownCast(svtkDataObject::GetData(outputVector));

  if (!output)
  {
    svtkErrorMacro(<< "svtkCMLMoleculeReader does not have a svtkMolecule "
                     "as output.");
    return 1;
  }

  svtkCMLParser* parser = svtkCMLParser::New();
  parser->SetDebug(this->GetDebug());
  parser->SetFileName(this->FileName);
  parser->SetTarget(output);

  if (!parser->Parse())
  {
    svtkWarningMacro(<< "Cannot parse file " << this->FileName << " as CML.");
    parser->Delete();
    return 1;
  }

  parser->Delete();

  return 1;
}

int svtkCMLMoleculeReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMolecule");
  return 1;
}

void svtkCMLMoleculeReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//
// svtkCMLParser Methods
//

svtkStandardNewMacro(svtkCMLParser);

svtkCMLParser::svtkCMLParser()
  : svtkXMLParser()
  , Target(nullptr)
{
}

svtkCMLParser::~svtkCMLParser()
{
  this->SetTarget(nullptr);
}

void svtkCMLParser::StartElement(const char* name, const char** attr)
{
  if (strcmp(name, "atom") == 0)
  {
    this->NewAtom(attr);
  }
  else if (strcmp(name, "bond") == 0)
  {
    this->NewBond(attr);
  }
  else if (strcmp(name, "molecule") == 0)
  {
    this->NewMolecule(attr);
  }
  else if (this->GetDebug())
  {
    std::string desc;
    desc += "Unhandled CML Element. Name: ";
    desc += name;
    desc += "\n\tAttributes:";
    int attrIndex = 0;
    while (const char* cur = attr[attrIndex])
    {
      if (attrIndex > 0)
      {
        desc.push_back(' ');
      }
      desc += cur;
      ++attrIndex;
    }
    svtkDebugMacro(<< desc);
  }
}

void svtkCMLParser::EndElement(const char*) {}

void svtkCMLParser::NewMolecule(const char**)
{
  this->Target->Initialize();
}

void svtkCMLParser::NewAtom(const char** attr)
{
  svtkAtom atom = this->Target->AppendAtom();
  int attrInd = 0;
  unsigned short atomicNum = 0;
  float pos[3];
  const char* id = nullptr;
  while (const char* cur = attr[attrInd])
  {
    // Get atomic number
    if (strcmp(cur, "elementType") == 0)
    {
      const char* symbol = attr[++attrInd];
      atomicNum = pTab->GetAtomicNumber(symbol);
    }

    // Get position
    else if (strcmp(cur, "x3") == 0)
      pos[0] = atof(attr[++attrInd]);
    else if (strcmp(cur, "y3") == 0)
      pos[1] = atof(attr[++attrInd]);
    else if (strcmp(cur, "z3") == 0)
      pos[2] = atof(attr[++attrInd]);

    // string id / names
    else if (strcmp(cur, "id") == 0)
      id = attr[++attrInd];

    else
    {
      svtkDebugMacro(<< "Unhandled atom attribute: " << cur);
    }

    ++attrInd;
  }

  atom.SetAtomicNumber(atomicNum);
  atom.SetPosition(pos);

  // Store name for lookups
  size_t atomId = static_cast<size_t>(atom.GetId());
  if (atomId >= this->AtomNames.size())
  {
    this->AtomNames.resize(atomId + 1);
  }

  this->AtomNames[atomId] = std::string(id);

  svtkDebugMacro(<< "Added atom #" << atomId << " ('" << id << "') ");
}

void svtkCMLParser::NewBond(const char** attr)
{
  int attrInd = 0;
  svtkIdType atomId1 = -1;
  svtkIdType atomId2 = -1;
  unsigned short order = 0;

  while (const char* cur = attr[attrInd])
  {
    // Get names of bonded atoms
    if (strcmp(cur, "atomRefs2") == 0)
    {
      std::string atomRefs = attr[++attrInd];
      std::vector<std::string> words;
      // Parse out atom names:
      svtksys::SystemTools::Split(atomRefs, words, ' ');
      std::vector<std::string>::const_iterator words_iter = words.begin();
      while (words_iter != words.end())
      {
        svtkIdType currentAtomId;
        bool found = false;
        for (currentAtomId = 0; currentAtomId < static_cast<svtkIdType>(this->AtomNames.size());
             ++currentAtomId)
        {
          if (this->AtomNames[currentAtomId].compare(*words_iter) == 0)
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          // Create list of known atom names:
          std::string allAtomNames;
          for (size_t i = 0; i < this->AtomNames.size(); ++i)
          {
            allAtomNames += this->AtomNames[i];
            allAtomNames.push_back(' ');
          }
          svtkWarningMacro(<< "NewBond(): unknown atom name '" << *words_iter << "'. Known atoms:\n"
                          << allAtomNames);

          ++words_iter;
          continue;
        }
        else if (atomId1 == -1)
        {
          atomId1 = currentAtomId;
        }
        else if (atomId2 == -1)
        {
          atomId2 = currentAtomId;
        }
        else
        {
          svtkWarningMacro(<< "NewBond(): atomRef2 string has >2 atom names: " << atomRefs);
        }

        ++words_iter;
      }
    }

    // Get bond order
    else if (strcmp(cur, "order") == 0)
    {
      order = static_cast<unsigned short>(atoi(attr[++attrInd]));
    }

    else
    {
      svtkDebugMacro(<< "Unhandled bond attribute: " << cur);
    }

    ++attrInd;
  }

  if (atomId1 < 0 || atomId2 < 0)
  {
    svtkWarningMacro(<< "NewBond(): Invalid atom ids: " << atomId1 << " " << atomId2);
    return;
  }

  svtkDebugMacro(<< "Adding bond between atomids " << atomId1 << " " << atomId2);

  this->Target->AppendBond(atomId1, atomId2, order);
}
