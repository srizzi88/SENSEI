/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedWidthTextReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkFixedWidthTextReader.h"
#include "svtkCommand.h"
#include "svtkIOStream.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"
#include "svtksys/FStream.hxx"

#include <algorithm>
#include <fstream>
#include <vector>

#include <cctype>

svtkStandardNewMacro(svtkFixedWidthTextReader);
svtkCxxSetObjectMacro(svtkFixedWidthTextReader, TableErrorObserver, svtkCommand);

// Function body at bottom of file
static int splitString(const svtkStdString& input, unsigned int fieldWidth, bool stripWhitespace,
  std::vector<svtkStdString>& results, bool includeEmpties = true);

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
static int my_getline(std::istream& stream, svtkStdString& output, char delim = '\n');

// ----------------------------------------------------------------------

svtkFixedWidthTextReader::svtkFixedWidthTextReader()
{
  this->FileName = nullptr;
  this->StripWhiteSpace = false;
  this->HaveHeaders = false;
  this->FieldWidth = 10;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->TableErrorObserver = nullptr;
}

// ----------------------------------------------------------------------

svtkFixedWidthTextReader::~svtkFixedWidthTextReader()
{
  this->SetFileName(nullptr);
  if (this->TableErrorObserver)
  {
    this->TableErrorObserver->Delete();
  }
}

// ----------------------------------------------------------------------

void svtkFixedWidthTextReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Field width: " << this->FieldWidth << endl;
  os << indent << "Strip leading/trailing whitespace: " << (this->StripWhiteSpace ? "Yes" : "No")
     << endl;
  os << indent << "HaveHeaders: " << (this->HaveHeaders ? "Yes" : "No") << endl;
}

// ----------------------------------------------------------------------

int svtkFixedWidthTextReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  int numLines = 0;

  // Check that the filename has been specified
  if (!this->FileName)
  {
    svtkErrorMacro("svtkFixedWidthTextReader: You must specify a filename!");
    return 2;
  }

  svtksys::ifstream infile(this->FileName, ios::in);
  if (!infile || infile.fail())
  {
    svtkErrorMacro(<< "svtkFixedWidthTextReader: Couldn't open file!");
    return 2;
  }

  // The first line of the file might contain the headers, so we want
  // to be a little bit careful about it.  If we don't have headers
  // we'll have to make something up.
  std::vector<svtkStdString> headers;
  std::vector<svtkStdString> firstLineFields;
  svtkStdString firstLine;

  my_getline(infile, firstLine);

  //  svtkDebugMacro(<<"First line of file: " << firstLine.c_str());

  if (this->HaveHeaders)
  {
    splitString(firstLine, this->FieldWidth, this->StripWhiteSpace, headers);
  }
  else
  {
    splitString(firstLine, this->FieldWidth, this->StripWhiteSpace, firstLineFields);

    for (unsigned int i = 0; i < firstLineFields.size(); ++i)
    {
      char fieldName[64];
      snprintf(fieldName, sizeof(fieldName), "Field %u", i);
      headers.push_back(fieldName);
    }
  }

  svtkTable* table = svtkTable::GetData(outputVector);
  if (this->TableErrorObserver)
  {
    table->AddObserver(svtkCommand::ErrorEvent, this->TableErrorObserver);
  }

  // Now we can create the arrays that will hold the data for each
  // field.
  std::vector<svtkStdString>::const_iterator fieldIter;
  for (fieldIter = headers.begin(); fieldIter != headers.end(); ++fieldIter)
  {
    svtkStringArray* array = svtkStringArray::New();
    array->SetName((*fieldIter).c_str());
    table->AddColumn(array);
    array->Delete();
  }

  // If the first line did not contain headers then we need to add it
  // to the table.
  if (!this->HaveHeaders)
  {
    svtkVariantArray* dataArray = svtkVariantArray::New();
    std::vector<svtkStdString>::const_iterator I;
    for (I = firstLineFields.begin(); I != firstLineFields.end(); ++I)
    {
      dataArray->InsertNextValue(svtkVariant(*I));
    }

    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
  }

  // Read the file line-by-line and add it to the table.
  svtkStdString nextLine;
  while (my_getline(infile, nextLine))
  {
    ++numLines;
    if (numLines > 0 && numLines % 100 == 0)
    {
      float numLinesRead = numLines;
      this->InvokeEvent(svtkCommand::ProgressEvent, &numLinesRead);
    }

    svtkDebugMacro(<< "Next line: " << nextLine.c_str());
    std::vector<svtkStdString> dataVector;

    // Split string on the delimiters
    splitString(nextLine, this->FieldWidth, this->StripWhiteSpace, dataVector);

    svtkDebugMacro(<< "Split into " << dataVector.size() << " fields");
    // Add data to the output arrays

    // Convert from vector to variant array
    svtkVariantArray* dataArray = svtkVariantArray::New();
    std::vector<svtkStdString>::const_iterator I;
    for (I = dataVector.begin(); I != dataVector.end(); ++I)
    {
      dataArray->InsertNextValue(svtkVariant(*I));
    }

    // Pad out any missing columns
    while (dataArray->GetNumberOfTuples() < table->GetNumberOfColumns())
    {
      dataArray->InsertNextValue(svtkVariant());
    }

    // Insert the data into the table
    table->InsertNextRow(dataArray);
    dataArray->Delete();
  }

  infile.close();

  return 1;
}

// ----------------------------------------------------------------------

static int splitString(const svtkStdString& input, unsigned int fieldWidth, bool stripWhitespace,
  std::vector<svtkStdString>& results, bool includeEmpties)
{
  if (input.empty())
  {
    return 0;
  }

  unsigned int thisField = 0;
  svtkStdString thisFieldText;
  svtkStdString parsedField;

  while (thisField * fieldWidth < input.size())
  {
    thisFieldText = input.substr(thisField * fieldWidth, fieldWidth);

    if (stripWhitespace)
    {
      unsigned int startIndex = 0, endIndex = static_cast<unsigned int>(thisFieldText.size()) - 1;
      while (startIndex < thisFieldText.size() &&
        isspace(static_cast<int>(thisFieldText.at(startIndex))))
      {
        ++startIndex;
      }
      while (endIndex > 0 && isspace(static_cast<int>(thisFieldText.at(endIndex))))
      {
        --endIndex;
      }

      if (startIndex <= endIndex)
      {
        parsedField = thisFieldText.substr(startIndex, (endIndex - startIndex) + 1);
      }
      else
      {
        parsedField = svtkStdString();
      }
    }
    else
    {
      parsedField = thisFieldText;
    }
    ++thisField;
    if (!parsedField.empty() || includeEmpties)
    {
      results.push_back(parsedField);
    }
  }

  return static_cast<int>(results.size());
}

// ----------------------------------------------------------------------

static int my_getline(istream& in, svtkStdString& out, char delimiter)
{
  out = svtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = in.get()) != EOF && numCharactersRead < out.max_size())
  {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter && downcast != 0x0d)
    {
      out += downcast;
    }
    else
    {
      return numCharactersRead;
    }
  }

  return numCharactersRead;
}
