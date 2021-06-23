/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActor2DCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor2DCollection.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkActor2DCollection);

// protected function to delete an element. Internal use only.
void svtkActor2DCollection::DeleteElement(svtkCollectionElement* e)
{
  svtkCollection::DeleteElement(e);
}

// Destructor for the svtkActor2DCollection class. This removes all
// objects from the collection.
svtkActor2DCollection::~svtkActor2DCollection()
{
  this->RemoveAllItems();
}

// Sort and then render the collection of 2D actors.
void svtkActor2DCollection::RenderOverlay(svtkViewport* viewport)
{
  if (this->NumberOfItems != 0)
  {
    this->Sort();
    svtkActor2D* tempActor;
    svtkCollectionSimpleIterator adit;
    for (this->InitTraversal(adit); (tempActor = this->GetNextActor2D(adit));)
    {
      // Make sure that the actor is visible before rendering
      if (tempActor->GetVisibility() == 1)
      {
        tempActor->RenderOverlay(viewport);
      }
    }
  }
}

// Add an actor to the list.  The new actor is
// inserted in the list according to it's layer
// number.
void svtkActor2DCollection::AddItem(svtkActor2D* a)
{
  svtkCollectionElement* indexElem;
  svtkCollectionElement* elem = new svtkCollectionElement;

  // Check if the top item is nullptr
  if (this->Top == nullptr)
  {
    svtkDebugMacro(<< "svtkActor2DCollection::AddItem - Adding item to top of the list");

    this->Top = elem;
    elem->Item = a;
    elem->Next = nullptr;
    this->Bottom = elem;
    this->NumberOfItems++;
    a->Register(this);
    return;
  }

  for (indexElem = this->Top; indexElem != nullptr; indexElem = indexElem->Next)
  {

    svtkActor2D* tempActor = static_cast<svtkActor2D*>(indexElem->Item);
    if (a->GetLayerNumber() < tempActor->GetLayerNumber())
    {
      // The indexElem item's layer number is larger, so swap
      // the new item and the indexElem item.
      svtkDebugMacro(<< "svtkActor2DCollection::AddItem - Inserting item");
      elem->Item = indexElem->Item;
      elem->Next = indexElem->Next;
      indexElem->Item = a;
      indexElem->Next = elem;
      this->NumberOfItems++;
      a->Register(this);
      return;
    }
  }

  // End of list found before a larger layer number
  svtkDebugMacro(<< "svtkActor2DCollection::AddItem - Adding item to end of the list");
  elem->Item = a;
  elem->Next = nullptr;
  this->Bottom->Next = elem;
  this->Bottom = elem;
  this->NumberOfItems++;
  a->Register(this);
}

// Sorts the svtkActor2DCollection by layer number.  Smaller layer
// numbers are first.  Layer numbers can be any integer value.
void svtkActor2DCollection::Sort()
{
  int index;

  svtkDebugMacro(<< "svtkActor2DCollection::Sort");

  int numElems = this->GetNumberOfItems();

  // Create an array of pointers to actors
  svtkActor2D** actorPtrArr = new svtkActor2D*[numElems];

  svtkDebugMacro(<< "svtkActor2DCollection::Sort - Getting actors from collection");

  // Start at the beginning of the collection
  svtkCollectionSimpleIterator ait;
  this->InitTraversal(ait);

  // Fill the actor array with the items in the collection
  for (index = 0; index < numElems; index++)
  {
    actorPtrArr[index] = this->GetNextActor2D(ait);
  }

  svtkDebugMacro(<< "svtkActor2DCollection::Sort - Starting selection sort");
  // Start the sorting - selection sort
  int i, j, min;
  svtkActor2D* t;

  for (i = 0; i < numElems - 1; i++)
  {
    min = i;
    for (j = i + 1; j < numElems; j++)
    {
      if (actorPtrArr[j]->GetLayerNumber() < actorPtrArr[min]->GetLayerNumber())
      {
        min = j;
      }
    }
    t = actorPtrArr[min];
    actorPtrArr[min] = actorPtrArr[i];
    actorPtrArr[i] = t;
  }

  svtkDebugMacro(<< "svtkActor2DCollection::Sort - Selection sort done.");

  for (index = 0; index < numElems; index++)
  {
    svtkDebugMacro(<< "svtkActor2DCollection::Sort - actorPtrArr[" << index
                  << "] layer: " << actorPtrArr[index]->GetLayerNumber());
  }

  svtkDebugMacro(<< "svtkActor2DCollection::Sort - Rearraging the linked list.");
  // Now move the items around in the linked list -
  // keep the links the same, but swap around the items

  svtkCollectionElement* elem = this->Top;
  elem->Item = actorPtrArr[0];

  for (i = 1; i < numElems; i++)
  {
    elem = elem->Next;
    elem->Item = actorPtrArr[i];
  }

  delete[] actorPtrArr;
}
