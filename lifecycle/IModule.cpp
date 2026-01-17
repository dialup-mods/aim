#include "IModule.h"

PublicInterface::PublicInterface(std::type_index type, std::shared_ptr<const IModule> ptr)
  : type(type)
  , ptr(std::move(ptr)) {}

std::shared_ptr<void> PublicInterface::getErased() const { return std::static_pointer_cast<void>(std::const_pointer_cast<IModule>(ptr)); }
