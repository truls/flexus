/*! \file simulator_name.hpp
    \brief Forward declaration of simulator identification variables
    and simulator name declaration

    \def FLEXUS_DECLARE_SIMULATOR(Name)
    Set the Name and metadata of simulator.
    extern declaration of theSimulatorName. Used to allow the
    simulator to print out its name and versioning information
    information. This macro will also embed these details in the
    qflex_version section of the compiled ELF.

    \def FLEXUS_GIT_HASH
    Used to set the variable theSimulatorCommitHash.

    \def FLEXUS_GIT_BRANCH
    Used to set the variable theSimulatorBrnachName.

    \def FLEXUS_GIT_TREE_STATUS
    Used to set the variable theSimualtorTreeStatus.

 */

#ifndef FLEXUS_SIMULATOR_NAME_HPP_INCLUDED
#define FLEXUS_SIMULATOR_NAME_HPP_INCLUDED

#ifndef FLEXUS_GIT_HASH
#define FLEXUS_GIT_HASH "<unknown>"
#endif //FLEXUS_GIT_HASH

#ifndef FLEXUS_GIT_BRANCH
#define FLEXUS_GIT_BRANCH "<unknown>"
#endif //FLEXUS_GIT_BRANCH

#ifndef FLEXUS_GIT_TREE_STATUS
#define FLEXUS_GIT_TREE_STATUS ""
#endif //FLEXUS_GIT_TREE_STATUS

#define FLEXUS_DECLARE_SIMULATOR(Name)                                  \
  namespace Flexus {                                                    \
    const char theSimulatorName[] = Name;                               \
    const char theSimulatorCommitHash[] = FLEXUS_GIT_HASH;              \
    const char theSimulatorBranchName[] = FLEXUS_GIT_BRANCH;            \
    const char theSimulatorTreeStatus[] = FLEXUS_GIT_TREE_STATUS;       \
    volatile const char theSimulatorIDString[]                          \
    __attribute__((section("qflex_version"))) =                         \
      "QFlex simulator. Built as " Name " from commit hash "            \
      FLEXUS_GIT_HASH " " FLEXUS_GIT_TREE_STATUS " from branch "        \
      FLEXUS_GIT_BRANCH ;                                               \
  }                                                                     \
  struct semicolon_eater__

namespace Flexus {
  extern "C" {
  // The name of the simulator
  extern const char theSimulatorName[];
  // The hash of the git commit this simulator was compiled from
  extern const char theSimulatorCommitHash[];
  // The name of the git branch this simulator was compiled from
  extern const char theSimulatorBranchName[];
  // Holds the string 'dirty' if the git working tree was dirty when
  // the wimulator was compiled
  extern const char theSimulatorTreeStatus[];
  }

} // namespace Flexus

#endif // FLEXUS_SIMULATOR_NAME_HPP_INCLUDED
