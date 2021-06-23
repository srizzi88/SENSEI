/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLWriter
 * @brief   Superclass for SVTK's XML file writers.
 *
 * svtkXMLWriter provides methods implementing most of the
 * functionality needed to write SVTK XML file formats.  Concrete
 * subclasses provide actual writer implementations calling upon this
 * functionality.
 *
 * @par Thanks
 * CompressionLevel getters/setters exposed by Quincy Wofford
 * (qwofford@lanl.gov) and John Patchett (patchett@lanl.gov),
 * Los Alamos National Laboratory (2017)
 */

#ifndef svtkXMLWriter_h
#define svtkXMLWriter_h

#include "svtkAlgorithm.h"
#include "svtkIOXMLModule.h" // For export macro

#include <sstream> // For ostringstream ivar

class svtkAbstractArray;
class svtkArrayIterator;

template <class T>
class svtkArrayIteratorTemplate;

class svtkCellData;
class svtkDataArray;
class svtkDataCompressor;
class svtkDataSet;
class svtkDataSetAttributes;
class svtkFieldData;
class svtkOutputStream;
class svtkPointData;
class svtkPoints;
class svtkFieldData;
class svtkXMLDataHeader;

class svtkStdString;
class OffsetsManager;      // one per piece/per time
class OffsetsManagerGroup; // array of OffsetsManager
class OffsetsManagerArray; // array of OffsetsManagerGroup

class SVTKIOXML_EXPORT svtkXMLWriter : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkXMLWriter, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Enumerate big and little endian byte order settings.
   */
  enum
  {
    BigEndian,
    LittleEndian
  };

  /**
   * Enumerate the supported data modes.
   * Ascii = Inline ascii data.
   * Binary = Inline binary data (base64 encoded, possibly compressed).
   * Appended = Appended binary data (possibly compressed and/or base64).
   */
  enum
  {
    Ascii,
    Binary,
    Appended
  };

  /**
   * Enumerate the supported svtkIdType bit lengths.
   * Int32 = File stores 32-bit values for svtkIdType.
   * Int64 = File stores 64-bit values for svtkIdType.
   */
  enum
  {
    Int32 = 32,
    Int64 = 64
  };

  /**
   * Enumerate the supported binary data header bit lengths.
   * UInt32 = File stores 32-bit binary data header elements.
   * UInt64 = File stores 64-bit binary data header elements.
   */
  enum
  {
    UInt32 = 32,
    UInt64 = 64
  };

  //@{
  /**
   * Get/Set the byte order of data written to the file.  The default
   * is the machine's hardware byte order.
   */
  svtkSetMacro(ByteOrder, int);
  svtkGetMacro(ByteOrder, int);
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  //@}

  //@{
  /**
   * Get/Set the binary data header word type.  The default is UInt32.
   * Set to UInt64 when storing arrays requiring 64-bit indexing.
   */
  virtual void SetHeaderType(int);
  svtkGetMacro(HeaderType, int);
  void SetHeaderTypeToUInt32();
  void SetHeaderTypeToUInt64();
  //@}

  //@{
  /**
   * Get/Set the size of the svtkIdType values stored in the file.  The
   * default is the real size of svtkIdType.
   */
  virtual void SetIdType(int);
  svtkGetMacro(IdType, int);
  void SetIdTypeToInt32();
  void SetIdTypeToInt64();
  //@}

  //@{
  /**
   * Get/Set the name of the output file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  svtkSetMacro(WriteToOutputString, svtkTypeBool);
  svtkGetMacro(WriteToOutputString, svtkTypeBool);
  svtkBooleanMacro(WriteToOutputString, svtkTypeBool);
  std::string GetOutputString() { return this->OutputString; }
  //@}

  //@{
  /**
   * Get/Set the compressor used to compress binary and appended data
   * before writing to the file.  Default is a svtkZLibDataCompressor.
   */
  virtual void SetCompressor(svtkDataCompressor*);
  svtkGetObjectMacro(Compressor, svtkDataCompressor);
  //@}

  enum CompressorType
  {
    NONE,
    ZLIB,
    LZ4,
    LZMA
  };

  //@{
  /**
   * Convenience functions to set the compressor to certain known types.
   */
  void SetCompressorType(int compressorType);
  void SetCompressorTypeToNone() { this->SetCompressorType(NONE); }
  void SetCompressorTypeToLZ4() { this->SetCompressorType(LZ4); }
  void SetCompressorTypeToZLib() { this->SetCompressorType(ZLIB); }
  void SetCompressorTypeToLZMA() { this->SetCompressorType(LZMA); }

  void SetCompressionLevel(int compressorLevel);
  svtkGetMacro(CompressionLevel, int);
  //@}

  //@{
  /**
   * Get/Set the block size used in compression.  When reading, this
   * controls the granularity of how much extra information must be
   * read when only part of the data are requested.  The value should
   * be a multiple of the largest scalar data type.
   */
  virtual void SetBlockSize(size_t blockSize);
  svtkGetMacro(BlockSize, size_t);
  //@}

  //@{
  /**
   * Get/Set the data mode used for the file's data.  The options are
   * svtkXMLWriter::Ascii, svtkXMLWriter::Binary, and
   * svtkXMLWriter::Appended.
   */
  svtkSetMacro(DataMode, int);
  svtkGetMacro(DataMode, int);
  void SetDataModeToAscii();
  void SetDataModeToBinary();
  void SetDataModeToAppended();
  //@}

  //@{
  /**
   * Get/Set whether the appended data section is base64 encoded.  If
   * encoded, reading and writing will be slower, but the file will be
   * fully valid XML and text-only.  If not encoded, the XML
   * specification will be violated, but reading and writing will be
   * fast.  The default is to do the encoding.
   */
  svtkSetMacro(EncodeAppendedData, svtkTypeBool);
  svtkGetMacro(EncodeAppendedData, svtkTypeBool);
  svtkBooleanMacro(EncodeAppendedData, svtkTypeBool);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  svtkDataObject* GetInput(int port);
  svtkDataObject* GetInput() { return this->GetInput(0); }
  //@}

  /**
   * Get the default file extension for files written by this writer.
   */
  virtual const char* GetDefaultFileExtension() = 0;

  /**
   * Invoke the writer.  Returns 1 for success, 0 for failure.
   */
  int Write();

  // See the svtkAlgorithm for a description of what these do
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //@{
  /**
   * Set the number of time steps
   */
  svtkGetMacro(NumberOfTimeSteps, int);
  svtkSetMacro(NumberOfTimeSteps, int);
  //@}

  //@{
  /**
   * API to interface an outside the SVTK pipeline control
   */
  void Start();
  void Stop();
  void WriteNextTime(double time);
  //@}

protected:
  svtkXMLWriter();
  ~svtkXMLWriter() override;

  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  // The name of the output file.
  char* FileName;

  // The output stream to which the XML is written.
  ostream* Stream;

  // Whether this object is writing to a string or a file.
  // Default is 0: write to file.
  svtkTypeBool WriteToOutputString;

  // The output string.
  std::string OutputString;

  // The output byte order.
  int ByteOrder;

  // The output binary header word type.
  int HeaderType;

  // The output svtkIdType.
  int IdType;

  // The form of binary data to write.  Used by subclasses to choose
  // how to write data.
  int DataMode;

  // Whether to base64-encode the appended data section.
  svtkTypeBool EncodeAppendedData;

  // The stream position at which appended data starts.
  svtkTypeInt64 AppendedDataPosition;

  // appended data offsets for field data
  OffsetsManagerGroup* FieldDataOM; // one per array

  // We need a 32 bit signed integer type to which svtkIdType will be
  // converted if Int32 is specified for the IdType parameter to this
  // writer.
#if SVTK_SIZEOF_SHORT == 4
  typedef short Int32IdType;
#elif SVTK_SIZEOF_INT == 4
  typedef int Int32IdType;
#elif SVTK_SIZEOF_LONG == 4
  typedef long Int32IdType;
#else
#error "No native data type can represent a signed 32-bit integer."
#endif

  // Buffer for svtkIdType conversion.
  Int32IdType* Int32IdTypeBuffer;

  // The byte swapping buffer.
  unsigned char* ByteSwapBuffer;

  // Compression information.
  svtkDataCompressor* Compressor;
  size_t BlockSize;
  size_t CompressionBlockNumber;
  svtkXMLDataHeader* CompressionHeader;
  svtkTypeInt64 CompressionHeaderPosition;
  // Compression Level for svtkDataCompressor objects
  // 1 (worst compression, fastest) ... 9 (best compression, slowest)
  int CompressionLevel = 5;

  // The output stream used to write binary and appended data.  May
  // transparently encode the data.
  svtkOutputStream* DataStream;

  // Allow subclasses to set the data stream.
  virtual void SetDataStream(svtkOutputStream*);
  svtkGetObjectMacro(DataStream, svtkOutputStream);

  // Method to drive most of actual writing.
  virtual int WriteInternal();

  // Method defined by subclasses to write data.  Return 1 for
  // success, 0 for failure.
  virtual int WriteData() { return 1; }

  // Method defined by subclasses to specify the data set's type name.
  virtual const char* GetDataSetName() = 0;

  // Methods to define the file's major and minor version numbers.
  virtual int GetDataSetMajorVersion();
  virtual int GetDataSetMinorVersion();

  // Utility methods for subclasses.
  svtkDataSet* GetInputAsDataSet();
  virtual int StartFile();
  virtual void WriteFileAttributes();
  virtual int EndFile();
  void DeleteAFile();
  void DeleteAFile(const char* name);

  virtual int WritePrimaryElement(ostream& os, svtkIndent indent);
  virtual void WritePrimaryElementAttributes(ostream& os, svtkIndent indent);
  void StartAppendedData();
  void EndAppendedData();

  // Write enough space to go back and write the given attribute with
  // at most "length" characters in the value.  Returns the stream
  // position at which attribute should be later written.  The default
  // length of 20 is enough for a 64-bit integer written in decimal or
  // a double-precision floating point value written to 13 digits of
  // precision (the other 7 come from a minus sign, decimal place, and
  // a big exponent like "e+300").
  svtkTypeInt64 ReserveAttributeSpace(const char* attr, size_t length = 20);

  svtkTypeInt64 GetAppendedDataOffset();
  void WriteAppendedDataOffset(
    svtkTypeInt64 streamPos, svtkTypeInt64& lastoffset, const char* attr = nullptr);
  void ForwardAppendedDataOffset(
    svtkTypeInt64 streamPos, svtkTypeInt64 offset, const char* attr = nullptr);
  void ForwardAppendedDataDouble(svtkTypeInt64 streamPos, double value, const char* attr);

  int WriteScalarAttribute(const char* name, int data);
  int WriteScalarAttribute(const char* name, float data);
  int WriteScalarAttribute(const char* name, double data);
#ifdef SVTK_USE_64BIT_IDS
  int WriteScalarAttribute(const char* name, svtkIdType data);
#endif

  int WriteVectorAttribute(const char* name, int length, int* data);
  int WriteVectorAttribute(const char* name, int length, float* data);
  int WriteVectorAttribute(const char* name, int length, double* data);
#ifdef SVTK_USE_64BIT_IDS
  int WriteVectorAttribute(const char* name, int length, svtkIdType* data);
#endif

  int WriteDataModeAttribute(const char* name);
  int WriteWordTypeAttribute(const char* name, int dataType);
  int WriteStringAttribute(const char* name, const char* value);

  // Returns true if any keys were written.
  bool WriteInformation(svtkInformation* info, svtkIndent indent);

  void WriteArrayHeader(svtkAbstractArray* a, svtkIndent indent, const char* alternateName,
    int writeNumTuples, int timestep);
  virtual void WriteArrayFooter(
    ostream& os, svtkIndent indent, svtkAbstractArray* a, int shortFormat);
  virtual void WriteArrayInline(svtkAbstractArray* a, svtkIndent indent,
    const char* alternateName = nullptr, int writeNumTuples = 0);
  virtual void WriteInlineData(svtkAbstractArray* a, svtkIndent indent);

  void WriteArrayAppended(svtkAbstractArray* a, svtkIndent indent, OffsetsManager& offs,
    const char* alternateName = nullptr, int writeNumTuples = 0, int timestep = 0);
  int WriteAsciiData(svtkAbstractArray* a, svtkIndent indent);
  int WriteBinaryData(svtkAbstractArray* a);
  int WriteBinaryDataInternal(svtkAbstractArray* a);
  void WriteArrayAppendedData(svtkAbstractArray* a, svtkTypeInt64 pos, svtkTypeInt64& lastoffset);

  // Methods for writing points, point data, and cell data.
  void WriteFieldData(svtkIndent indent);
  void WriteFieldDataInline(svtkFieldData* fd, svtkIndent indent);
  void WritePointDataInline(svtkPointData* pd, svtkIndent indent);
  void WriteCellDataInline(svtkCellData* cd, svtkIndent indent);
  void WriteFieldDataAppended(svtkFieldData* fd, svtkIndent indent, OffsetsManagerGroup* fdManager);
  void WriteFieldDataAppendedData(svtkFieldData* fd, int timestep, OffsetsManagerGroup* fdManager);
  void WritePointDataAppended(svtkPointData* pd, svtkIndent indent, OffsetsManagerGroup* pdManager);
  void WritePointDataAppendedData(svtkPointData* pd, int timestep, OffsetsManagerGroup* pdManager);
  void WriteCellDataAppended(svtkCellData* cd, svtkIndent indent, OffsetsManagerGroup* cdManager);
  void WriteCellDataAppendedData(svtkCellData* cd, int timestep, OffsetsManagerGroup* cdManager);
  void WriteAttributeIndices(svtkDataSetAttributes* dsa, char** names);
  void WritePointsAppended(svtkPoints* points, svtkIndent indent, OffsetsManager* manager);
  void WritePointsAppendedData(svtkPoints* points, int timestep, OffsetsManager* pdManager);
  void WritePointsInline(svtkPoints* points, svtkIndent indent);
  void WriteCoordinatesInline(
    svtkDataArray* xc, svtkDataArray* yc, svtkDataArray* zc, svtkIndent indent);
  void WriteCoordinatesAppended(svtkDataArray* xc, svtkDataArray* yc, svtkDataArray* zc,
    svtkIndent indent, OffsetsManagerGroup* coordManager);
  void WriteCoordinatesAppendedData(svtkDataArray* xc, svtkDataArray* yc, svtkDataArray* zc,
    int timestep, OffsetsManagerGroup* coordManager);
  void WritePPointData(svtkPointData* pd, svtkIndent indent);
  void WritePCellData(svtkCellData* cd, svtkIndent indent);
  void WritePPoints(svtkPoints* points, svtkIndent indent);
  void WritePArray(svtkAbstractArray* a, svtkIndent indent, const char* alternateName = nullptr);
  void WritePCoordinates(svtkDataArray* xc, svtkDataArray* yc, svtkDataArray* zc, svtkIndent indent);

  // Internal utility methods.
  int WriteBinaryDataBlock(unsigned char* in_data, size_t numWords, int wordType);
  void PerformByteSwap(void* data, size_t numWords, size_t wordSize);
  int CreateCompressionHeader(size_t size);
  int WriteCompressionBlock(unsigned char* data, size_t size);
  int WriteCompressionHeader();
  size_t GetWordTypeSize(int dataType);
  const char* GetWordTypeName(int dataType);
  size_t GetOutputWordTypeSize(int dataType);

  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);

  // The current range over which progress is moving.  This allows for
  // incrementally fine-tuned progress updates.
  virtual void GetProgressRange(float range[2]);
  virtual void SetProgressRange(const float range[2], int curStep, int numSteps);
  virtual void SetProgressRange(const float range[2], int curStep, const float* fractions);
  virtual void SetProgressPartial(float fraction);
  virtual void UpdateProgressDiscrete(float progress);
  float ProgressRange[2];

  // This shallows copy input field data to the passed field data and
  // then adds any additional field arrays. For example, TimeValue.
  void UpdateFieldData(svtkFieldData*);

  ostream* OutFile;
  std::ostringstream* OutStringStream;

  int OpenStream();
  int OpenFile();
  int OpenString();
  void CloseStream();
  void CloseFile();
  void CloseString();

  // The timestep currently being written
  int CurrentTimeIndex;
  int NumberOfTimeSteps;

  // Dummy boolean var to start/stop the continue executing:
  // when using the Start/Stop/WriteNextTime API
  int UserContinueExecuting; // can only be -1 = invalid, 0 = stop, 1 = start

  // This variable is used to ease transition to new versions of SVTK XML files.
  // If data that needs to be written satisfies certain conditions,
  // the writer can use the previous file version version.
  // For version change 0.1 -> 2.0 (UInt32 header) and 1.0 -> 2.0
  // (UInt64 header), if data does not have a svtkGhostType array,
  // the file is written with version: 0.1/1.0.
  bool UsePreviousVersion;

  svtkTypeInt64* NumberOfTimeValues; // one per piece / per timestep

  friend class svtkXMLWriterHelper;

private:
  svtkXMLWriter(const svtkXMLWriter&) = delete;
  void operator=(const svtkXMLWriter&) = delete;
};

#endif
