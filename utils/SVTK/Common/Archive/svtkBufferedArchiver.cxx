/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBufferedArchiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBufferedArchiver.h"

#include <svtkObjectFactory.h>

#include <archive.h>
#include <archive_entry.h>

#include <set>

struct svtkBufferedArchiver::Internal
{
  struct archive* Archive;
  char* Buffer;
  size_t AllocatedSize;
  size_t BufferSize;
  std::set<std::string> Entries;
};

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkBufferedArchiver);

//----------------------------------------------------------------------------
svtkBufferedArchiver::svtkBufferedArchiver()
  : Internals(new svtkBufferedArchiver::Internal)
{
  this->Internals->AllocatedSize = 100000;
  this->Internals->Buffer = nullptr;
  this->SetArchiveName("");
}

//----------------------------------------------------------------------------
svtkBufferedArchiver::~svtkBufferedArchiver()
{
  free(this->Internals->Buffer);
  delete this->Internals;
}

//----------------------------------------------------------------------------
void svtkBufferedArchiver::OpenArchive()
{
  this->Internals->Archive = archive_write_new();

  // use zip format
  archive_write_set_format_zip(this->Internals->Archive);

  this->Internals->Buffer = (char*)malloc(this->Internals->AllocatedSize);
  if (this->Internals->Buffer == nullptr)
  {
    svtkErrorMacro(<< "Error allocating memory for buffer.");
    return;
  }

  archive_write_open_memory(this->Internals->Archive, this->Internals->Buffer,
    this->Internals->AllocatedSize, &this->Internals->BufferSize);
}

//----------------------------------------------------------------------------
void svtkBufferedArchiver::CloseArchive()
{
  archive_write_free(this->Internals->Archive);
}

//----------------------------------------------------------------------------
void svtkBufferedArchiver::InsertIntoArchive(
  const std::string& relativePath, const char* data, std::size_t size)
{
  struct archive_entry* entry;

  entry = archive_entry_new();
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);
  archive_entry_set_size(entry, size);
  archive_entry_set_pathname(entry, relativePath.c_str());
  archive_write_header(this->Internals->Archive, entry);
  archive_write_data(this->Internals->Archive, data, size);
  archive_entry_free(entry);

  this->Internals->Entries.insert(relativePath);
}

//----------------------------------------------------------------------------
bool svtkBufferedArchiver::Contains(const std::string& relativePath)
{
  return this->Internals->Entries.find(relativePath) != this->Internals->Entries.end();
}

//----------------------------------------------------------------------------
const char* svtkBufferedArchiver::GetBuffer()
{
  return this->Internals->Buffer;
}

//----------------------------------------------------------------------------
const void* svtkBufferedArchiver::GetBufferAddress()
{
  return this->Internals->Buffer;
}

//----------------------------------------------------------------------------
void svtkBufferedArchiver::SetAllocatedSize(std::size_t size)
{
  this->Internals->AllocatedSize = size;
}

//----------------------------------------------------------------------------
std::size_t svtkBufferedArchiver::GetAllocatedSize()
{
  return this->Internals->AllocatedSize;
}

//----------------------------------------------------------------------------
std::size_t svtkBufferedArchiver::GetBufferSize()
{
  return this->Internals->BufferSize;
}

//----------------------------------------------------------------------------
void svtkBufferedArchiver::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
