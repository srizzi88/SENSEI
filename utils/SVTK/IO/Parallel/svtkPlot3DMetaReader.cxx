/*=========================================================================

  Program:   ParaView
  Module:    svtkPlot3DMetaReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPlot3DMetaReader.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiBlockPLOT3DReader.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtksys/FStream.hxx"
#include <svtksys/SystemTools.hxx>

#include <map>
#include <string>
#include <vector>

#include "svtk_jsoncpp.h"

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

svtkStandardNewMacro(svtkPlot3DMetaReader);

typedef void (svtkPlot3DMetaReader::*Plot3DFunction)(Json::Value* val);

struct Plot3DTimeStep
{
  double Time;
  std::string XYZFile;
  std::string QFile;
  std::string FunctionFile;
};

struct svtkPlot3DMetaReaderInternals
{
  std::map<std::string, Plot3DFunction> FunctionMap;
  std::vector<Plot3DTimeStep> TimeSteps;

  std::string ResolveFileName(const std::string& metaFileName, std::string fileName)
  {
    if (svtksys::SystemTools::FileIsFullPath(fileName.c_str()))
    {
      return fileName;
    }
    else
    {
      std::string path = svtksys::SystemTools::GetFilenamePath(metaFileName);
      std::vector<std::string> components;
      components.push_back(path + "/");
      components.push_back(fileName);
      return svtksys::SystemTools::JoinPath(components);
    }
  }
};

//-----------------------------------------------------------------------------
svtkPlot3DMetaReader::svtkPlot3DMetaReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Reader = svtkMultiBlockPLOT3DReader::New();
  this->Reader->AutoDetectFormatOn();

  this->FileName = nullptr;

  this->Internal = new svtkPlot3DMetaReaderInternals;

  this->Internal->FunctionMap["auto-detect-format"] = &svtkPlot3DMetaReader::SetAutoDetectFormat;
  this->Internal->FunctionMap["byte-order"] = &svtkPlot3DMetaReader::SetByteOrder;
  this->Internal->FunctionMap["precision"] = &svtkPlot3DMetaReader::SetPrecision;
  this->Internal->FunctionMap["multi-grid"] = &svtkPlot3DMetaReader::SetMultiGrid;
  this->Internal->FunctionMap["format"] = &svtkPlot3DMetaReader::SetFormat;
  this->Internal->FunctionMap["blanking"] = &svtkPlot3DMetaReader::SetBlanking;
  this->Internal->FunctionMap["language"] = &svtkPlot3DMetaReader::SetLanguage;
  this->Internal->FunctionMap["2D"] = &svtkPlot3DMetaReader::Set2D;
  this->Internal->FunctionMap["R"] = &svtkPlot3DMetaReader::SetR;
  this->Internal->FunctionMap["gamma"] = &svtkPlot3DMetaReader::SetGamma;
  this->Internal->FunctionMap["filenames"] = &svtkPlot3DMetaReader::SetFileNames;
  this->Internal->FunctionMap["functions"] = &svtkPlot3DMetaReader::AddFunctions;
  this->Internal->FunctionMap["function-names"] = &svtkPlot3DMetaReader::SetFunctionNames;
}

//-----------------------------------------------------------------------------
svtkPlot3DMetaReader::~svtkPlot3DMetaReader()
{
  this->Reader->Delete();

  delete this->Internal;

  delete[] this->FileName;
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetAutoDetectFormat(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
  {
    this->Reader->AutoDetectFormatOn();
  }
  else
  {
    this->Reader->AutoDetectFormatOff();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetByteOrder(Json::Value* val)
{
  std::string value = val->asString();
  if (value == "little")
  {
    this->Reader->SetByteOrderToLittleEndian();
  }
  else if (value == "big")
  {
    this->Reader->SetByteOrderToBigEndian();
  }
  else
  {
    svtkErrorMacro("Unrecognized byte order: " << value.c_str()
                                              << ". Valid options are \"little\" and \"big\"."
                                                 " Setting to little endian");
    this->Reader->SetByteOrderToLittleEndian();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetLanguage(Json::Value* val)
{
  std::string value = val->asString();
  if (value == "fortran")
  {
    this->Reader->HasByteCountOn();
  }
  else if (value == "C")
  {
    this->Reader->HasByteCountOff();
  }
  else
  {
    svtkErrorMacro("Unrecognized language: " << value.c_str()
                                            << ". Valid options are \"fortran\" and \"C\"."
                                               " Setting to little fortran");
    this->Reader->HasByteCountOn();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetPrecision(Json::Value* val)
{
  int value = val->asInt();
  if (value == 32)
  {
    this->Reader->DoublePrecisionOff();
  }
  else if (value == 64)
  {
    this->Reader->DoublePrecisionOn();
  }
  else
  {
    svtkErrorMacro("Unrecognized precision: " << value
                                             << ". Valid options are 32 and 64 (bits)."
                                                " Setting to 32 bits");
    this->Reader->DoublePrecisionOff();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetMultiGrid(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
  {
    this->Reader->MultiGridOn();
  }
  else
  {
    this->Reader->MultiGridOff();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetFormat(Json::Value* val)
{
  std::string value = val->asString();
  if (value == "binary")
  {
    this->Reader->BinaryFileOn();
  }
  else if (value == "ascii")
  {
    this->Reader->BinaryFileOff();
  }
  else
  {
    svtkErrorMacro("Unrecognized file type: " << value.c_str()
                                             << ". Valid options are \"binary\" and \"ascii\"."
                                                " Setting to binary");
    this->Reader->BinaryFileOn();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetBlanking(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
  {
    this->Reader->IBlankingOn();
  }
  else
  {
    this->Reader->IBlankingOff();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::Set2D(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
  {
    this->Reader->TwoDimensionalGeometryOn();
  }
  else
  {
    this->Reader->TwoDimensionalGeometryOff();
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetR(Json::Value* val)
{
  double R = val->asDouble();
  this->Reader->SetR(R);
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetGamma(Json::Value* val)
{
  double gamma = val->asDouble();
  this->Reader->SetGamma(gamma);
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::AddFunctions(Json::Value* val)
{
  const Json::Value& functions = *val;
  for (size_t index = 0; index < functions.size(); ++index)
  {
    this->Reader->AddFunction(functions[(int)index].asInt());
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetFileNames(Json::Value* val)
{
  const Json::Value& filenames = *val;
  for (size_t index = 0; index < filenames.size(); ++index)
  {
    const Json::Value& astep = filenames[(int)index];
    bool doAdd = true;
    Plot3DTimeStep aTime;
    if (!astep.isMember("time"))
    {
      svtkErrorMacro("Missing time value in timestep " << index);
      doAdd = false;
    }
    else
    {
      aTime.Time = astep["time"].asDouble();
    }

    if (!astep.isMember("xyz"))
    {
      svtkErrorMacro("Missing xyz filename in timestep " << index);
      doAdd = false;
    }
    else
    {
      std::string xyzfile = astep["xyz"].asString();
      aTime.XYZFile = this->Internal->ResolveFileName(this->FileName, xyzfile);
    }

    if (astep.isMember("q"))
    {
      std::string qfile = astep["q"].asString();
      aTime.QFile = this->Internal->ResolveFileName(this->FileName, qfile);
    }

    if (astep.isMember("function"))
    {
      std::string functionfile = astep["function"].asString();
      aTime.FunctionFile = this->Internal->ResolveFileName(this->FileName, functionfile);
    }

    if (doAdd)
    {
      this->Internal->TimeSteps.push_back(aTime);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::SetFunctionNames(Json::Value* val)
{
  const Json::Value& functionNames = *val;
  for (size_t index = 0; index < functionNames.size(); ++index)
  {
    this->Reader->AddFunctionName(functionNames[(int)index].asString());
  }
}

//----------------------------------------------------------------------------
int svtkPlot3DMetaReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(svtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

  this->Internal->TimeSteps.clear();

  this->Reader->RemoveAllFunctions();

  if (!this->FileName)
  {
    svtkErrorMacro("No file name was specified. Cannot execute.");
    return 0;
  }

  svtksys::ifstream file(this->FileName);

  Json::CharReaderBuilder rbuilder;
  rbuilder["collectComments"] = true;

  Json::Value root;
  std::string formattedErrorMessages;

  bool parsingSuccessful = Json::parseFromStream(rbuilder, file, &root, &formattedErrorMessages);

  if (!parsingSuccessful)
  {
    // report to the user the failure and their locations in the document.
    svtkErrorMacro("Failed to parse configuration\n" << formattedErrorMessages);
    return 0;
  }

  Json::Value::Members members = root.getMemberNames();
  Json::Value::Members::iterator memberIterator;
  for (memberIterator = members.begin(); memberIterator != members.end(); ++memberIterator)
  {
    std::map<std::string, Plot3DFunction>::iterator iter =
      this->Internal->FunctionMap.find(*memberIterator);
    if (iter != this->Internal->FunctionMap.end())
    {
      Json::Value val = root[*memberIterator];
      Plot3DFunction func = iter->second;
      CALL_MEMBER_FN(*this, func)(&val);
    }
    else
    {
      svtkErrorMacro(
        "Syntax error in file. Option \"" << memberIterator->c_str() << "\" is not valid.");
    }
  }

  std::vector<Plot3DTimeStep>::iterator iter;
  std::vector<double> timeValues;
  for (iter = this->Internal->TimeSteps.begin(); iter != this->Internal->TimeSteps.end(); ++iter)
  {
    timeValues.push_back(iter->Time);
  }

  size_t numSteps = timeValues.size();
  if (numSteps > 0)
  {
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeValues[0], (int)numSteps);

    double timeRange[2];
    timeRange[0] = timeValues[0];
    timeRange[1] = timeValues[numSteps - 1];
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPlot3DMetaReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::GetData(outputVector->GetInformationObject(0));

  double timeValue = 0;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    timeValue = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  int tsLength = outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (tsLength < 1)
  {
    svtkErrorMacro("No timesteps were found. Please specify at least one "
                  "filenames entry in the input file.");
    return 0;
  }

  // find the first time value larger than requested time value
  // this logic could be improved
  int cnt = 0;
  while (cnt < tsLength - 1 && steps[cnt] < timeValue)
  {
    cnt++;
  }

  int updateTime = cnt;

  if (updateTime >= tsLength)
  {
    updateTime = tsLength - 1;
  }

  if (tsLength > updateTime)
  {
    this->Reader->SetXYZFileName(this->Internal->TimeSteps[updateTime].XYZFile.c_str());
    const char* qname = this->Internal->TimeSteps[updateTime].QFile.c_str();
    if (strlen(qname) > 0)
    {
      this->Reader->SetQFileName(qname);
    }
    else
    {
      this->Reader->SetQFileName(nullptr);
    }
    const char* fname = this->Internal->TimeSteps[updateTime].FunctionFile.c_str();
    if (strlen(fname) > 0)
    {
      this->Reader->SetFunctionFileName(fname);
    }
    else
    {
      this->Reader->SetFunctionFileName(nullptr);
    }
    this->Reader->UpdatePiece(outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    svtkDataObject* ioutput = this->Reader->GetOutput();
    output->ShallowCopy(ioutput);
    output->GetInformation()->Set(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(),
      ioutput->GetInformation()->Get(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS()));
  }
  else
  {
    svtkErrorMacro("Time step " << updateTime << " was not found.");
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkPlot3DMetaReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
