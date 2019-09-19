#include <string>
#include <vector>

#include <core/qemu/api_wrappers.hpp>
#include <core/target.hpp>

#include <core/qemu/configuration_api.hpp>

namespace Flexus {
namespace Qemu {

namespace aux_ {
API::conf_class_t *RegisterClass_stub([[maybe_unused]] std::string const &name,
                                      [[maybe_unused]] API::class_data_t *class_data) {
  return new API::conf_class_t;
}

API::conf_object_t *NewObject_stub([[maybe_unused]] API::conf_class_t *aClass,
                                   [[maybe_unused]] std::string const &aName) {
  return new API::conf_object_t;
}

} // namespace aux_

} // namespace Qemu
} // namespace Flexus
