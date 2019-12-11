// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2012 by Mohammad Alisafaee, Eric Chung, Michael Ferdman, Brian
// Gold, Jangwoo Kim, Pejman Lotfi-Kamran, Onur Kocberber, Djordje Jevdjic,
// Jared Smolens, Stephen Somogyi, Evangelos Vlachos, Stavros Volos, Jason
// Zebchuk, Babak Falsafi, Nikos Hardavellas and Tom Wenisch for the SimFlex
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon
// University.
//
// For more information, see the SimFlex project website at:
//   http://www.ece.cmu.edu/~simflex
//
// You may not use the name "Carnegie Mellon University" or derivations
// thereof to endorse or promote products derived from this software.
//
// If you modify the software you must place a notice on or within any
// modified version provided or made available to any third party stating
// that you have modified the software.  The notice shall include at least
// your name, address, phone number, email address and the date and purpose
// of the modification.
//
// THE SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY OF ANY KIND, EITHER
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,
// CONTRACT, TORT OR OTHERWISE).
//
// DO-NOT-REMOVE end-copyright-block

#ifndef FLEXUS_armDECODER_EFFECTS_HPP_INCLUDED
#define FLEXUS_armDECODER_EFFECTS_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iostream>

#include <core/types.hpp>

#include "OperandCode.hpp"
#include <components/CommonQEMU/Slices/MemOp.hpp>
#include <components/uArchARM/RegisterType.hpp>
#include <components/uArchARM/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#include "InstructionComponentBuffer.hpp"
#include "Interactions.hpp"

namespace nuArchARM {
struct uArchARM;
struct SemanticAction;
enum eAccType;
} // namespace nuArchARM

namespace narmDecoder {

using Flexus::SharedTypes::VirtualMemoryAddress;
using nuArchARM::eOperation;
using nuArchARM::eRegisterType;
using nuArchARM::eSize;
using nuArchARM::SemanticAction;
using nuArchARM::uArchARM;

class BaseSemanticAction;
struct SemanticInstruction;
struct Condition;

struct Effect : UncountedComponent {
  Effect *theNext;
  Effect() : theNext(0) {
  }
  virtual ~Effect() {
  }
  virtual void invoke(SemanticInstruction &anInstruction) {
    if (theNext) {
      theNext->invoke(anInstruction);
    }
  }
  virtual void describe(std::ostream &anOstream) const {
    if (theNext) {
      theNext->describe(anOstream);
    }
  }
  // NOTE: No virtual destructor because effects are never destructed.
};

struct EffectChain {
  Effect *theFirst;
  Effect *theLast;
  EffectChain();
  void invoke(SemanticInstruction &anInstruction);
  void describe(std::ostream &anOstream) const;
  void append(Effect *anEffect);
  bool empty() const {
    return theFirst == 0;
  }
};

struct DependanceTarget {
  void invokeSatisfy(int32_t anArg) {
    void (narmDecoder::DependanceTarget::*satisfy_pt)(int32_t) =
        &narmDecoder::DependanceTarget::satisfy;
    DBG_(VVerb, (<< std::hex << "Satisfy: " << satisfy_pt << "\n"));
    satisfy(anArg);
    DBG_(VVerb, (<< "After satisfy"));
  }
  void invokeSquash(int32_t anArg) {
    squash(anArg);
  }
  virtual ~DependanceTarget() {
  }
  virtual void satisfy(int32_t anArg) = 0;
  virtual void squash(int32_t anArg) = 0;

protected:
  DependanceTarget() {
  }
};

struct InternalDependance {
  DependanceTarget *theTarget;
  int32_t theArg;
  InternalDependance() : theTarget(0), theArg(0) {
  }
  InternalDependance(InternalDependance const &id) : theTarget(id.theTarget), theArg(id.theArg) {
  }
  InternalDependance(DependanceTarget *tgt, int32_t arg) : theTarget(tgt), theArg(arg) {
  }
  void satisfy() {
    theTarget->satisfy(theArg);
  }
  void squash() {
    theTarget->squash(theArg);
  }
  InternalDependance &operator=(const InternalDependance&) = default;
};

Effect *mapSource(SemanticInstruction *inst, eOperandCode anInputCode, eOperandCode anOutputCode);
Effect *freeMapping(SemanticInstruction *inst, eOperandCode aMapping);
Effect *disconnectRegister(SemanticInstruction *inst, eOperandCode aMapping);
Effect *mapCCDestination(SemanticInstruction *inst);
Effect *mapDestination(SemanticInstruction *inst);
Effect *mapRD1Destination(SemanticInstruction *inst);
Effect *mapRD2Destination(SemanticInstruction *inst);
Effect *mapDestination_NoSquashEffects(SemanticInstruction *inst);
Effect *mapRD1Destination_NoSquashEffects(SemanticInstruction *inst);
Effect *mapRD2Destination_NoSquashEffects(SemanticInstruction *inst);
Effect *unmapDestination(SemanticInstruction *inst);
Effect *mapFDestination(SemanticInstruction *inst, int32_t anIndex);
Effect *unmapFDestination(SemanticInstruction *inst, int32_t anIndex);
Effect *restorePreviousDestination(SemanticInstruction *inst);
Effect *satisfy(SemanticInstruction *inst, InternalDependance const &aDependance);
Effect *squash(SemanticInstruction *inst, InternalDependance const &aDependance);
Effect *annulNext(SemanticInstruction *inst);
Effect *branch(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *returnFromTrap(SemanticInstruction *inst, bool isDone);
Effect *branchAfterNext(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *branchAfterNext(SemanticInstruction *inst, eOperandCode aCode);
Effect *branchConditionally(SemanticInstruction *inst, VirtualMemoryAddress aTarget, bool anAnnul,
                            Condition &aCondition, bool isFloating);
Effect *branchRegConditionally(SemanticInstruction *inst, VirtualMemoryAddress aTarget,
                               bool anAnnul, uint32_t aCondition);
Effect *allocateLoad(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                     nuArchARM::eAccType type);
Effect *allocateCAS(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *allocateCAS(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *allocateCASP(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                     nuArchARM::eAccType type);
Effect *allocateCAS(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *allocateRMW(SemanticInstruction *inst, eSize aSize, InternalDependance const &aDependance,
                    nuArchARM::eAccType type);
Effect *eraseLSQ(SemanticInstruction *inst);
Effect *allocateStore(SemanticInstruction *inst, eSize aSize, bool aBypassSB,
                      nuArchARM::eAccType type);
Effect *allocateMEMBAR(SemanticInstruction *inst, eAccType type);
Effect *retireMem(SemanticInstruction *inst);
Effect *commitStore(SemanticInstruction *inst);
Effect *accessMem(SemanticInstruction *inst);
Effect *updateConditional(SemanticInstruction *inst);
Effect *updateUnconditional(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *updateUnconditional(SemanticInstruction *inst, eOperandCode anOperandCode);
Effect *updateCall(SemanticInstruction *inst, VirtualMemoryAddress aTarget);
Effect *updateNonBranch(SemanticInstruction *inst);
Effect *readPR(SemanticInstruction *inst, ePrivRegs aPR);
Effect *writePR(SemanticInstruction *inst, ePrivRegs aPR);
Effect *writePSTATE(SemanticInstruction *inst, uint8_t anOp1, uint8_t anOp2);
Effect *writeNZCV(SemanticInstruction *inst);
Effect *clearExclusiveMonitor(SemanticInstruction *inst);
Effect *SystemRegisterTrap(SemanticInstruction *inst);
Effect *checkSystemAccess(SemanticInstruction *inst, uint8_t anOp0, uint8_t anOp1, uint8_t anOp2,
                          uint8_t aCRn, uint8_t aCRm, uint8_t aRT, uint8_t aRead);
Effect *exceptionEffect(SemanticInstruction *inst, eExceptionType aType);
Effect *markExclusiveMonitor(SemanticInstruction *inst, eOperandCode anAddressCode, eSize aSize);
Effect *exclusiveMonitorPass(SemanticInstruction *inst, eOperandCode anAddressCode, eSize aSize);

Effect *checkDAIFAccess(SemanticInstruction *inst, uint8_t anOp1);
Effect *checkSysRegAccess(SemanticInstruction *inst, ePrivRegs aPrivReg, uint8_t is_read);

Effect *mapXTRA(SemanticInstruction *inst);
Effect *forceResync(SemanticInstruction *inst);
Effect *immuException(SemanticInstruction *inst);
Effect *mmuPageFaultCheck(SemanticInstruction *inst);

} // namespace narmDecoder

#endif // FLEXUS_armDECODER_EFFECTS_HPP_INCLUDED
