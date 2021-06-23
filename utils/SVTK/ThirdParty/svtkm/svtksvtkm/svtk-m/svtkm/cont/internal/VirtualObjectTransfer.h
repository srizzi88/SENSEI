//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_VirtualObjectTransfer_h
#define svtk_m_cont_internal_VirtualObjectTransfer_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/VirtualObjectBase.h>
#include <svtkm/cont/DeviceAdapterTag.h>

#include <array>
#include <memory>

namespace svtkm
{
namespace cont
{
namespace internal
{

struct CreateTransferInterface; //forward declare for friendship

template <typename VirtualDerivedType, typename DeviceAdapter>
struct VirtualObjectTransfer
#ifdef SVTKM_DOXYGEN_ONLY
{
  /// A VirtualObjectTransfer is constructed with a pointer to the derived type that (eventually)
  /// gets transferred to the execution environment of the given DeviceAdapter.
  ///
  SVTKM_CONT VirtualObjectTransfer(const VirtualDerivedType* virtualObject);

  /// \brief Transfers the virtual object to the execution environment.
  ///
  /// This method transfers the virtual object to the execution environment and returns a pointer
  /// to the object that can be used in the execution environment (but not necessarily the control
  /// environment). If the \c updateData flag is true, then the data is always copied to the
  /// execution environment (such as if the data were updated since the last call to \c
  /// PrepareForExecution). If the \c updateData flag is false and the object was already
  /// transferred previously, the previously created object is returned.
  ///
  SVTKM_CONT const VirtualDerivedType* PrepareForExecution(bool updateData);

  /// \brief Frees up any resources in the execution environment.
  ///
  /// Any previously returned virtual object from \c PrepareForExecution becomes invalid.
  ///
  SVTKM_CONT void ReleaseResources();
}
#endif
;

class SVTKM_CONT_EXPORT TransferInterface
{
public:
  SVTKM_CONT virtual ~TransferInterface();

  SVTKM_CONT virtual const svtkm::VirtualObjectBase* PrepareForExecution(svtkm::Id) = 0;
  SVTKM_CONT virtual void ReleaseResources() = 0;
};

template <typename VirtualDerivedType, typename DeviceAdapter>
class TransferInterfaceImpl final : public TransferInterface
{
public:
  SVTKM_CONT TransferInterfaceImpl(const VirtualDerivedType* virtualObject)
    : LastModifiedCount(-1)
    , Transfer(virtualObject)
  {
  }

  SVTKM_CONT const svtkm::VirtualObjectBase* PrepareForExecution(svtkm::Id hostModifiedCount) override
  {
    bool updateData = (this->LastModifiedCount != hostModifiedCount);
    const svtkm::VirtualObjectBase* executionObject = this->Transfer.PrepareForExecution(updateData);
    this->LastModifiedCount = hostModifiedCount;
    return executionObject;
  }

  SVTKM_CONT void ReleaseResources() override { this->Transfer.ReleaseResources(); }

private:
  svtkm::Id LastModifiedCount;
  svtkm::cont::internal::VirtualObjectTransfer<VirtualDerivedType, DeviceAdapter> Transfer;
};


struct SVTKM_CONT_EXPORT TransferState
{
  TransferState() = default;

  ~TransferState() { this->ReleaseResources(); }

  bool DeviceIdIsValid(svtkm::cont::DeviceAdapterId deviceId) const;

  bool WillReleaseHostPointer() const { return this->DeleteFunction != nullptr; }


  void UpdateHost(svtkm::VirtualObjectBase* host, void (*deleteFunction)(void*))
  {
    if (this->HostPointer != host)
    {
      this->ReleaseResources();
      this->HostPointer = host;
      this->DeleteFunction = deleteFunction;
    }
  }

  void ReleaseResources()
  {
    this->ReleaseExecutionResources();

    //This needs to be updated to release all execution information

    if (this->DeleteFunction)
    {
      this->DeleteFunction(this->HostPointer);
    }
    this->HostPointer = nullptr;
    this->DeleteFunction = nullptr;
  }

  void ReleaseExecutionResources()
  {
    //This needs to be updated to only release the active execution part
    for (auto& state : this->DeviceTransferState)
    {
      if (state)
      {
        state->ReleaseResources();
      }
    }
  }

  const svtkm::VirtualObjectBase* PrepareForExecution(svtkm::cont::DeviceAdapterId deviceId) const
  {
    //make sure the device is up to date
    auto index = static_cast<std::size_t>(deviceId.GetValue());
    svtkm::Id count = this->HostPointer->GetModifiedCount();
    return this->DeviceTransferState[index]->PrepareForExecution(count);
  }

  svtkm::VirtualObjectBase* HostPtr() const { return this->HostPointer; }

private:
  friend struct CreateTransferInterface;

  svtkm::VirtualObjectBase* HostPointer = nullptr;
  void (*DeleteFunction)(void*) = nullptr;

  std::array<std::unique_ptr<TransferInterface>, 8> DeviceTransferState;
};
}
}
} // svtkm::cont::internal

#endif // svtkm_cont_internal_VirtualObjectTransfer_h
