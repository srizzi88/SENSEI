/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkX3DExporterFIWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Kristian Sons
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkX3DExporterFIWriter.h"

#include "svtkCellArray.h"
#include "svtkDataArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkUnsignedCharArray.h"
#include "svtkX3D.h"
#include "svtksys/FStream.hxx"

#include <cassert>
#include <sstream>
#include <vector>

//#define ENCODEASSTRING 1

using namespace svtkX3D;

/*======================================================================== */
struct NodeInfo
{
  NodeInfo(int _nodeId)
  {
    this->nodeId = _nodeId;
    this->isChecked = false;
    this->attributesTerminated = true;
  }
  int nodeId;
  bool attributesTerminated;
  bool isChecked;
};

/*======================================================================== */
typedef std::vector<NodeInfo> svtkX3DExporterFINodeInfoStackBase;
class svtkX3DExporterFINodeInfoStack : public svtkX3DExporterFINodeInfoStackBase
{
};

/*======================================================================== */
class svtkX3DExporterFIByteWriter
{
public:
  ~svtkX3DExporterFIByteWriter();
  svtkX3DExporterFIByteWriter() = default;
  // This is the current byte to fill
  unsigned char CurrentByte;
  // This is the current byte position. Range: 0-7
  unsigned char CurrentBytePos;

  // Opens the specified file in binary mode. Returns 0
  // if failed
  int OpenFile(const char* file);
  int OpenStream();

  // Puts a bitstring to the current byte bit by bit
  void PutBits(const std::string& bitstring);
  // Puts the integer value to the stream using count bits
  // for encoding
  void PutBits(unsigned int value, unsigned char count);
  // Puts on bit to the current byte true = 1, false = 0
  void PutBit(bool on);
  // Puts whole bytes to the file stream. CurrentBytePos must
  // be 0 for this
  void PutBytes(const char* bytes, size_t length);
  // Fills up the current byte with 0 values
  void FillByte();

  // Get stream info
  std::string GetStringStream(svtkIdType& size);

private:
  unsigned char Append(unsigned int value, unsigned char count);
  void TryFlush();
  ostream* Stream;

  int WriteToOutputString;

  svtkX3DExporterFIByteWriter(const svtkX3DExporterFIByteWriter&) = delete;
  void operator=(const svtkX3DExporterFIByteWriter&) = delete;
};

//----------------------------------------------------------------------------
svtkX3DExporterFIByteWriter::~svtkX3DExporterFIByteWriter()
{
  delete this->Stream;
  this->Stream = nullptr;
}

//----------------------------------------------------------------------------
int svtkX3DExporterFIByteWriter::OpenFile(const char* file)
{
  this->WriteToOutputString = 0;
  this->CurrentByte = 0;
  this->CurrentBytePos = 0;
  svtksys::ofstream* fileStream = new svtksys::ofstream();
  fileStream->open(file, ios::out | ios::binary);
  if (fileStream->fail())
  {
    delete fileStream;
    return 0;
  }
  else
  {
    this->Stream = fileStream;
    return 1;
  }
}

//----------------------------------------------------------------------------
int svtkX3DExporterFIByteWriter::OpenStream()
{
  this->WriteToOutputString = 1;
  this->CurrentByte = 0;
  this->CurrentBytePos = 0;
  this->Stream = new std::ostringstream();
  return 1;
}

//----------------------------------------------------------------------------
std::string svtkX3DExporterFIByteWriter::GetStringStream(svtkIdType& size)
{
  if (this->WriteToOutputString && this->Stream)
  {
    std::ostringstream* ostr = static_cast<std::ostringstream*>(this->Stream);

    size = static_cast<svtkIdType>(ostr->str().size());
    return ostr->str();
  }

  size = 0;
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIByteWriter::TryFlush()
{
  if (this->CurrentBytePos == 8)
  {
    this->Stream->write((char*)(&(this->CurrentByte)), 1);
    this->CurrentByte = 0;
    this->CurrentBytePos = 0;
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIByteWriter::FillByte()
{
  while (this->CurrentBytePos != 0)
  {
    this->PutBit(0);
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIByteWriter::PutBit(bool on)
{
  assert(this->CurrentBytePos < 8);
  if (on)
  {
    unsigned char pos = this->CurrentBytePos;
    unsigned char mask = (unsigned char)(0x80 >> pos);
    this->CurrentByte |= mask;
  }
  this->CurrentBytePos++;
  TryFlush();
}

//----------------------------------------------------------------------------
unsigned char svtkX3DExporterFIByteWriter::Append(unsigned int value, unsigned char count)
{
  assert(this->CurrentBytePos < 8);
  while ((this->CurrentBytePos < 8) && count > 0)
  {
    // Value and der Stelle i
    unsigned int mask = 1;
    bool isSet = !(((mask << (count - 1)) & value) == 0);
    if (isSet)
    {
      this->CurrentByte |= static_cast<unsigned char>(0x80 >> this->CurrentBytePos);
    }
    this->CurrentBytePos++;
    count--;
  }
  TryFlush();
  return count;
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIByteWriter::PutBytes(const char* bytes, size_t length)
{
  if (this->CurrentBytePos == 0)
  {
    this->Stream->write(bytes, length);
  }
  else
  {
    // svtkErrorMacro(<< "Wrong position in svtkX3DExporterFIByteWriter::PutBytes");
    assert(false);
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIByteWriter::PutBits(unsigned int value, unsigned char count)
{
  // Can be optimized
  while (count > 0)
  {
    count = this->Append(value, count);
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIByteWriter::PutBits(const std::string& bitstring)
{
  std::string::const_iterator I = bitstring.begin();
  while (I != bitstring.end())
  {
    this->PutBit((*I) == '1');
    ++I;
  }
}

#include "svtkX3DExporterFIWriterHelper.h"

/* ------------------------------------------------------------------------- */
svtkStandardNewMacro(svtkX3DExporterFIWriter);
//----------------------------------------------------------------------------
svtkX3DExporterFIWriter::~svtkX3DExporterFIWriter()
{
  this->CloseFile();
  delete this->InfoStack;
  this->Compressor->Delete();
}

//----------------------------------------------------------------------------
svtkX3DExporterFIWriter::svtkX3DExporterFIWriter()
{
  this->InfoStack = new svtkX3DExporterFINodeInfoStack();
  this->Compressor = svtkZLibDataCompressor::New();
  this->Compressor->SetCompressionLevel(5);
  this->Writer = nullptr;
  this->IsLineFeedEncodingOn = true;
  this->Fastest = 0;
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Fastest: " << this->Fastest << endl;
}

//----------------------------------------------------------------------------
int svtkX3DExporterFIWriter::OpenFile(const char* file)
{
  std::string t(file);
  this->CloseFile();

  // Delegate to svtkX3DExporterFIByteWriter
  this->Writer = new svtkX3DExporterFIByteWriter();
  this->WriteToOutputString = 0;
  return this->Writer->OpenFile(file);
}

int svtkX3DExporterFIWriter::OpenStream()
{
  // Delegate to svtkX3DExporterFIByteWriter
  this->Writer = new svtkX3DExporterFIByteWriter();
  this->WriteToOutputString = 1;
  return this->Writer->OpenStream();
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::CloseFile()
{
  if (this->Writer != nullptr)
  {
    if (this->WriteToOutputString)
    {
      delete[] this->OutputString;
      std::string tmpstr = this->Writer->GetStringStream(this->OutputStringLength);
      const char* tmp = tmpstr.c_str();
      this->OutputString = new char[this->OutputStringLength];
      memcpy(this->OutputString, tmp, this->OutputStringLength);
    }
    delete this->Writer;
    this->Writer = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::StartDocument()
{
  const char* external_voc = "urn:external-vocabulary";
  // ITU 12.6: 1110000000000000
  this->Writer->PutBits("1110000000000000");
  // ITU 12.7 / 12.9: Version of standard: 1 as 16bit
  this->Writer->PutBits("0000000000000001");
  // ITU 12.8: The bit '0' (padding) shall then be appended to the bit stream
  this->Writer->PutBit(0);
  // ITU C.2.3
  this->Writer->PutBit(0); // additional-data
  this->Writer->PutBit(1); // initial-vocabulary
  this->Writer->PutBit(0); // notations
  this->Writer->PutBit(0); // unparsed-entities
  this->Writer->PutBit(0); // character-encoding-scheme
  this->Writer->PutBit(0); // standalone
  this->Writer->PutBit(0); // and version
  // ITU C.2.5: padding '000' for optional component initial-vocabulary
  this->Writer->PutBits("000");
  // ITU C.2.5.1: For each of the thirteen optional components:
  // presence ? 1 : 0
  this->Writer->PutBits("1000000000000"); // 'external-vocabulary'
  // ITU C.2.5.2: external-vocabulary is present
  this->Writer->PutBit(0);
  // Write "urn:external-vocabulary"
  // ITU C.22.3.1: Length is < 65
  this->Writer->PutBit(0);
  // Writer->PutBits("010110"); // = strlen(external_voc) - 1
  this->Writer->PutBits(static_cast<unsigned int>(strlen(external_voc) - 1), 6);
  this->Writer->PutBytes(external_voc, strlen(external_voc));
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::EndDocument()
{
  // ITU C.2.12: The four bits '1111' (termination) are appended
  this->Writer->PutBits("1111");
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::StartNode(int elementID)
{
  if (!this->InfoStack->empty())
  {
    this->CheckNode(false);
    if (this->IsLineFeedEncodingOn)
    {
      svtkX3DExporterFIWriterHelper::EncodeLineFeed(this->Writer);
    }
    this->Writer->FillByte();
  }

  this->InfoStack->push_back(NodeInfo(elementID));

  // ITU C.3.7.2: element is present
  this->Writer->PutBit(0);
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::EndNode()
{
  assert(!this->InfoStack->empty());
  this->CheckNode(false);
  if (this->IsLineFeedEncodingOn)
  {
    svtkX3DExporterFIWriterHelper::EncodeLineFeed(this->Writer);
  }
  if (!this->InfoStack->back().attributesTerminated)
  {
    // cout << "Terminated in EndNode: could be wrong" << endl;
    // ITU C.3.6.2: End of attribute
    this->Writer->PutBits("1111");
  }
  // ITU C.3.8: The four bits '1111' (termination) are appended.
  this->Writer->PutBits("1111");
  this->InfoStack->pop_back();
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::CheckNode(bool callerIsAttribute)
{
  if (!this->InfoStack->back().isChecked)
  {
    if (callerIsAttribute) // Element has attributes
    {
      // ITU C.3.3: then the bit '1' (presence) is appended
      this->Writer->PutBit(1);
      this->InfoStack->back().attributesTerminated = false;
    }
    else // Element has no attributes
    {
      // ITU C.3.3: otherwise, the bit '0' (absence) is appended
      this->Writer->PutBit(0);
    }
    // Write Node name (starting at third bit)
    // ITU: C.18.4 If the alternative name-surrogate-index is present,
    // it is encoded as described in C.27.
    svtkX3DExporterFIWriterHelper::EncodeInteger3(this->Writer, this->InfoStack->back().nodeId + 1);
    this->InfoStack->back().isChecked = true;
  }
  // Element has attributes and childs
  else if (!callerIsAttribute && !this->InfoStack->back().attributesTerminated)
  {
    // ITU C.3.6.2: End of attribute
    this->Writer->PutBits("1111");
    this->InfoStack->back().attributesTerminated = true;
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::StartAttribute(int attributeID, bool literal, bool addToTable)
{
  this->CheckNode();
  // ITU C.3.6.1: Start of attribute
  this->Writer->PutBit(0);
  // ITU C.4.3 The value of qualified-name is encoded as described in C.17.
  svtkX3DExporterFIWriterHelper::EncodeInteger2(this->Writer, attributeID + 1);

  // ITU C.14.3: If the alternative literal-character-string is present,
  // then the bit '0' (discriminant) is appended
  // ITU C.14.4: If the alternative string-index is present,
  // then the bit '1' (discriminant) is appended
  this->Writer->PutBit(literal ? 0 : 1);
  if (literal)
  {
    // ITU C.14.3.1 If the value of the component add-to-table is TRUE,
    // then the bit '1' is appended to the bit stream;
    this->Writer->PutBit(addToTable);
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::EndAttribute() {}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, int type, const double* d)
{
  this->StartAttribute(attributeID, true, false);

#ifdef ENCODEASSTRING
  const double* loc = nullptr;
  size_t size = 0;
  double temp[4];
  switch (type)
  {
    case (SFVEC3F):
    case (SFCOLOR):
      size = 3;
      loc = d;
      break;
    case (SFROTATION):
      size = 4;
      temp[0] = d[1];
      temp[1] = d[2];
      temp[2] = d[3];
      temp[3] = svtkMath::RadiansFromDegrees(-d[0]);
      loc = temp;
      break;
    default:
      cerr << "UNKNOWN DATATYPE";
      assert(false);
  }
  svtkX3DExporterFIWriterHelper::EncodeFloatFI(this->Writer, loc, size);
#else
  std::ostringstream ss;
  switch (type)
  {
    case (SFVEC3F):
    case (SFCOLOR):
      ss << static_cast<float>(d[0]) << " " << static_cast<float>(d[1]) << " "
         << static_cast<float>(d[2]);
      break;
    case (SFROTATION):
      ss << static_cast<float>(d[1]) << " " << static_cast<float>(d[2]) << " "
         << static_cast<float>(d[3]) << " "
         << static_cast<float>(svtkMath::RadiansFromDegrees(-d[0]));
      break;
    default:
      cout << "UNKNOWN DATATYPE";
      assert(false);
  }
  svtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
#endif
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, int type, svtkDataArray* a)
{
  this->StartAttribute(attributeID, true, false);

#ifdef ENCODEASSTRING
  std::ostringstream ss;
  switch (type)
  {
    case (MFVEC3F):
    case (MFVEC2F):
      svtkIdType i;
      for (i = 0; i < a->GetNumberOfTuples(); i++)
      {
        double* d = a->GetTuple(i);
        ss << d[0] << " " << d[1];
        if (type == MFVEC3F)
          ss << " " << d[2];
        ss << ",";
      }
      svtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
      break;
    default:
      cerr << "UNKNOWN DATATYPE";
      assert(false);
  }
#else
  std::vector<double> values;
  switch (type)
  {
    case (MFVEC3F):
    case (MFVEC2F):
      svtkIdType i;
      for (i = 0; i < a->GetNumberOfTuples(); i++)
      {
        double* d = a->GetTuple(i);
        values.push_back(d[0]);
        values.push_back(d[1]);
        if (type == MFVEC3F)
        {
          values.push_back(d[2]);
        }
      }
      if (!this->Fastest && (values.size() > 15))
      {
        X3DEncoderFunctions::EncodeQuantizedzlibFloatArray(
          this->Writer, &(values.front()), values.size(), this->Compressor);
      }
      else
      {
        svtkX3DExporterFIWriterHelper::EncodeFloatFI(this->Writer, &(values.front()), values.size());
      }
      break;
    default:
      svtkErrorMacro("UNKNOWN DATATYPE");
      assert(false);
  }
#endif
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, const double* values, size_t size)
{
  this->StartAttribute(attributeID, true, false);
  if (!this->Fastest && (size > 15))
  {
    X3DEncoderFunctions::EncodeQuantizedzlibFloatArray(
      this->Writer, values, size, this->Compressor);
  }
  else
  {
    svtkX3DExporterFIWriterHelper::EncodeFloatFI(this->Writer, values, size);
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, const int* values, size_t size, bool image)
{
  this->StartAttribute(attributeID, true, false);
  if (size > 15)
  {
    X3DEncoderFunctions::EncodeIntegerDeltaZ(this->Writer, values, size, this->Compressor, image);
  }
  else
  {
    svtkX3DExporterFIWriterHelper::EncodeIntegerFI(this->Writer, values, size);
  }
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, int type, svtkCellArray* a)
{
  svtkIdType npts = 0;
  const svtkIdType* indx = nullptr;

  this->StartAttribute(attributeID, true, false);

#ifdef ENCODEASSTRING
  switch (type)
  {
    case (MFINT32):
      int i;
      for (a->InitTraversal(); a->GetNextCell(npts, indx);)
      {
        for (i = 0; i < npts; i++)
        {
          // treating svtkIdType as int
          ss << (int)indx[i] << " ";
        }
        ss << "-1";
      }
      svtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
      break;
    default:
      cerr << "UNKNOWN DATATYPE";
      assert(false);
  }
#else
  std::vector<int> values;
  switch (type)
  {
    case (MFINT32):
      int i;
      for (a->InitTraversal(); a->GetNextCell(npts, indx);)
      {
        for (i = 0; i < npts; i++)
        {
          values.push_back(indx[i]);
        }
        values.push_back(-1);
      }
      svtkX3DExporterFIWriterHelper::EncodeIntegerFI(this->Writer, &(values.front()), values.size());
      break;
    default:
      cerr << "UNKNOWN DATATYPE";
      assert(false);
  }
#endif
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, int value)
{
  std::ostringstream ss;
  this->StartAttribute(attributeID, true, false);

  // Xj3D writes out single value fields in string encoding. Expected:
  // FIEncoderFunctions::EncodeFloatFI<float>(this->Writer, &value, 1);
  ss << value;
  svtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, float value)
{
  std::ostringstream ss;

  this->StartAttribute(attributeID, true, false);

  // Xj3D writes out single value fields in string encoding. Expected:
  // FIEncoderFunctions::EncodeFloatFI<float>(this->Writer, &value, 1);
  ss << value;
  svtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, ss.str());
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int svtkNotUsed(attributeID), double svtkNotUsed(value))
{
  cout << "Function not implemented yet." << endl;
  assert(false);
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, bool value)
{
  this->StartAttribute(attributeID, false);
  svtkX3DExporterFIWriterHelper::EncodeInteger2(this->Writer, value ? 2 : 1);
}

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::SetField(int attributeID, const char* value, bool svtkNotUsed(mfstring))
{
  this->StartAttribute(attributeID, true, true);
  svtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, std::string(value));
}

//----------------------------------------------------------------------------
/*void svtkX3DExporterFIWriter::SetField(int attributeID, const std::string &value)
  {
  this->StartAttribute(attributeID, true, true);
  svtkX3DExporterFIWriterHelper::EncodeCharacterString3(this->Writer, value);
  }*/

//----------------------------------------------------------------------------
/*void svtkX3DExporterFIWriter::SetField(int attributeID, int type, std::string value)
  {
  assert(type == MFSTRING);
  type++;
  this->SetField(attributeID, value);
  }*/

//----------------------------------------------------------------------------
void svtkX3DExporterFIWriter::Flush() {}
