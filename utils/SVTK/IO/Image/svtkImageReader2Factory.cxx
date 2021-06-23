/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReader2Factory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageReader2Factory.h"

#include "svtkBMPReader.h"
#include "svtkGESignaReader.h"
#include "svtkHDRReader.h"
#include "svtkImageReader2.h"
#include "svtkImageReader2Collection.h"
#include "svtkJPEGReader.h"
#include "svtkMetaImageReader.h"
#include "svtkObjectFactory.h"
#include "svtkObjectFactoryCollection.h"
#include "svtkPNGReader.h"
#include "svtkPNMReader.h"
#include "svtkSLCReader.h"
#include "svtkTIFFReader.h"

// Destroying the prototype readers requires information keys.
// Include the manager here to make sure the keys are not destroyed
// until after the AvailableReaders singleton has been destroyed.
#include "svtkFilteringInformationKeyManager.h"

svtkStandardNewMacro(svtkImageReader2Factory);

class svtkImageReader2FactoryCleanup
{
public:
  inline void Use() {}
  ~svtkImageReader2FactoryCleanup()
  {
    if (svtkImageReader2Factory::AvailableReaders)
    {
      svtkImageReader2Factory::AvailableReaders->Delete();
      svtkImageReader2Factory::AvailableReaders = nullptr;
    }
  }
};
static svtkImageReader2FactoryCleanup svtkImageReader2FactoryCleanupGlobal;

svtkImageReader2Collection* svtkImageReader2Factory::AvailableReaders;

void svtkImageReader2Factory::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Available Readers : ";
  if (AvailableReaders)
  {
    AvailableReaders->PrintSelf(os, indent);
  }
  else
  {
    os << "None.";
  }
}

svtkImageReader2Factory::svtkImageReader2Factory() = default;

svtkImageReader2Factory::~svtkImageReader2Factory() = default;

void svtkImageReader2Factory::RegisterReader(svtkImageReader2* r)
{
  svtkImageReader2Factory::InitializeReaders();
  AvailableReaders->AddItem(r);
}

svtkImageReader2* svtkImageReader2Factory::CreateImageReader2(const char* path)
{
  svtkImageReader2Factory::InitializeReaders();
  svtkImageReader2* ret;
  svtkCollection* collection = svtkCollection::New();
  svtkObjectFactory::CreateAllInstance("svtkImageReaderObject", collection);
  svtkObject* o;
  // first try the current registered object factories to see
  // if one of them can
  for (collection->InitTraversal(); (o = collection->GetNextItemAsObject());)
  {
    if (o)
    {
      ret = svtkImageReader2::SafeDownCast(o);
      if (ret && ret->CanReadFile(path))
      {
        return ret;
      }
    }
  }
  // get rid of the collection
  collection->Delete();
  svtkCollectionSimpleIterator sit;
  for (svtkImageReader2Factory::AvailableReaders->InitTraversal(sit);
       (ret = svtkImageReader2Factory::AvailableReaders->GetNextImageReader2(sit));)
  {
    if (ret->CanReadFile(path))
    {
      // like a new call
      return ret->NewInstance();
    }
  }
  return nullptr;
}

void svtkImageReader2Factory::InitializeReaders()
{
  if (svtkImageReader2Factory::AvailableReaders)
  {
    return;
  }
  svtkImageReader2FactoryCleanupGlobal.Use();
  svtkImageReader2Factory::AvailableReaders = svtkImageReader2Collection::New();
  svtkImageReader2* reader;

  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkPNGReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkPNMReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkTIFFReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkBMPReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkSLCReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkHDRReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkJPEGReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkGESignaReader::New()));
  reader->Delete();
  svtkImageReader2Factory::AvailableReaders->AddItem((reader = svtkMetaImageReader::New()));
  reader->Delete();
}

void svtkImageReader2Factory::GetRegisteredReaders(svtkImageReader2Collection* collection)
{
  svtkImageReader2Factory::InitializeReaders();
  // get all dynamic readers
  svtkObjectFactory::CreateAllInstance("svtkImageReaderObject", collection);
  // get the current registered readers
  svtkImageReader2* ret;
  svtkCollectionSimpleIterator sit;
  for (svtkImageReader2Factory::AvailableReaders->InitTraversal(sit);
       (ret = svtkImageReader2Factory::AvailableReaders->GetNextImageReader2(sit));)
  {
    collection->AddItem(ret);
  }
}
