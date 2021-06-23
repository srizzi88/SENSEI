/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiProcessStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiProcessStream
 * @brief   stream used to pass data across processes
 * using svtkMultiProcessController.
 *
 * svtkMultiProcessStream is used to pass data across processes. Using
 * svtkMultiProcessStream it is possible to send data whose length is not known
 * at the receiving end.
 *
 * @warning
 * Note, stream operators cannot be combined with the Push/Pop array operators.
 * For example, if you push an array to the stream,
 */

#ifndef svtkMultiProcessStream_h
#define svtkMultiProcessStream_h

#include "svtkObject.h"
#include "svtkParallelCoreModule.h" // For export macro
#include <string>                  // needed for string.
#include <vector>                  // needed for vector.

class SVTKPARALLELCORE_EXPORT svtkMultiProcessStream
{
public:
  svtkMultiProcessStream();
  svtkMultiProcessStream(const svtkMultiProcessStream&);
  ~svtkMultiProcessStream();
  svtkMultiProcessStream& operator=(const svtkMultiProcessStream&);

  //@{
  /**
   * Add-to-stream operators. Adds to the end of the stream.
   */
  svtkMultiProcessStream& operator<<(double value);
  svtkMultiProcessStream& operator<<(float value);
  svtkMultiProcessStream& operator<<(int value);
  svtkMultiProcessStream& operator<<(char value);
  svtkMultiProcessStream& operator<<(bool value);
  svtkMultiProcessStream& operator<<(unsigned int value);
  svtkMultiProcessStream& operator<<(unsigned char value);
  svtkMultiProcessStream& operator<<(svtkTypeInt64 value);
  svtkMultiProcessStream& operator<<(svtkTypeUInt64 value);
  svtkMultiProcessStream& operator<<(const std::string& value);
  // Without this operator, the compiler would convert
  // a char* to a bool instead of a std::string.
  svtkMultiProcessStream& operator<<(const char* value);
  svtkMultiProcessStream& operator<<(const svtkMultiProcessStream&);
  //@}

  //@{
  /**
   * Remove-from-stream operators. Removes from the head of the stream.
   */
  svtkMultiProcessStream& operator>>(double& value);
  svtkMultiProcessStream& operator>>(float& value);
  svtkMultiProcessStream& operator>>(int& value);
  svtkMultiProcessStream& operator>>(char& value);
  svtkMultiProcessStream& operator>>(bool& value);
  svtkMultiProcessStream& operator>>(unsigned int& value);
  svtkMultiProcessStream& operator>>(unsigned char& value);
  svtkMultiProcessStream& operator>>(svtkTypeInt64& value);
  svtkMultiProcessStream& operator>>(svtkTypeUInt64& value);
  svtkMultiProcessStream& operator>>(std::string& value);
  svtkMultiProcessStream& operator>>(svtkMultiProcessStream&);
  //@}

  //@{
  /**
   * Add-array-to-stream methods. Adds to the end of the stream
   */
  void Push(double array[], unsigned int size);
  void Push(float array[], unsigned int size);
  void Push(int array[], unsigned int size);
  void Push(char array[], unsigned int size);
  void Push(unsigned int array[], unsigned int size);
  void Push(unsigned char array[], unsigned int size);
  void Push(svtkTypeInt64 array[], unsigned int size);
  void Push(svtkTypeUInt64 array[], unsigned int size);
  //@}

  //@{
  /**
   * Remove-array-to-stream methods. Removes from the head of the stream.
   * Note: If the input array is nullptr, the array will be allocated internally
   * and the calling application is responsible for properly de-allocating it.
   * If the input array is not nullptr, it is expected to match the size of the
   * data internally, and this method would just fill in the data.
   */
  void Pop(double*& array, unsigned int& size);
  void Pop(float*& array, unsigned int& size);
  void Pop(int*& array, unsigned int& size);
  void Pop(char*& array, unsigned int& size);
  void Pop(unsigned int*& array, unsigned int& size);
  void Pop(unsigned char*& array, unsigned int& size);
  void Pop(svtkTypeInt64*& array, unsigned int& size);
  void Pop(svtkTypeUInt64*& array, unsigned int& size);
  //@}

  /**
   * Clears everything in the stream.
   */
  void Reset();

  /**
   * Returns the size of the stream.
   */
  int Size();

  /**
   * Returns the size of the raw data returned by GetRawData. This
   * includes 1 byte to store the endian type.
   */
  int RawSize() { return (this->Size() + 1); }

  /**
   * Returns true iff the stream is empty.
   */
  bool Empty();

  //@{
  /**
   * Serialization methods used to save/restore the stream to/from raw data.
   * Note: The 1st byte of the raw data buffer consists of the endian type.
   */
  void GetRawData(std::vector<unsigned char>& data) const;
  void GetRawData(unsigned char*& data, unsigned int& size);
  void SetRawData(const std::vector<unsigned char>& data);
  void SetRawData(const unsigned char*, unsigned int size);
  std::vector<unsigned char> GetRawData() const;
  //@}

private:
  class svtkInternals;
  svtkInternals* Internals;
  unsigned char Endianness;
  enum
  {
    BigEndian,
    LittleEndian
  };
};

#endif

// SVTK-HeaderTest-Exclude: svtkMultiProcessStream.h
