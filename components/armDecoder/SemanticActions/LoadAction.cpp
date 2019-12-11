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

#include <iomanip>
#include <iostream>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/throw_exception.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
namespace ll = boost::lambda;

#include <boost/none.hpp>

#include <core/debug/debug.hpp>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uArchARM/uArchInterfaces.hpp>

#include "../Effects.hpp"
#include "../SemanticActions.hpp"
#include "../SemanticInstruction.hpp"
#include "PredicatedSemanticAction.hpp"

#define DBG_DeclareCategories armDecoder
#define DBG_SetDefaultOps AddCat(armDecoder)
#include DBG_Control()

namespace narmDecoder {

using namespace nuArchARM;
using nuArchARM::Instruction;

struct LoadAction : public PredicatedSemanticAction {
  eSize theSize;
  eSignCode theSignExtend;
  boost::optional<eOperandCode> theBypass;
  bool theLoadExtended;

  LoadAction(SemanticInstruction *anInstruction, eSize aSize, eSignCode aSigncode,
             boost::optional<eOperandCode> aBypass, bool aLoadExtended)
      : PredicatedSemanticAction(anInstruction, 1, true), theSize(aSize), theSignExtend(aSigncode),
        theBypass(aBypass), theLoadExtended(aLoadExtended) {
  }

  void satisfy(int32_t anArg) {
    BaseSemanticAction::satisfy(anArg);
    SEMANTICS_DBG(*this);
    if (!cancelled() && ready() && thePredicate) {
      doLoad();
    }
  }

  void predicate_on(int32_t anArg) {
    PredicatedSemanticAction::predicate_on(anArg);
    if (!cancelled() && ready() && thePredicate) {
      doLoad();
    }
  }

  void doLoad() {
    SEMANTICS_DBG(*this);
    bits value;

    if (theLoadExtended) {
      value = core()->retrieveExtendedLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
    } else {
      value = core()->retrieveLoadValue(boost::intrusive_ptr<Instruction>(theInstruction));
    }

    switch (theSize) {
    case kByte:
      value &= 0xFFULL;
      if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x80ULL)) {
        value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFFFF00ULL : 0ULL;
      }
      break;
    case kHalfWord:
      if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x8000ULL)) {
        value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFFFF00ULL : 0ULL;
      }
      break;
    case kWord:
      if ((theSignExtend != kNoExtension) && anyBits(value & (bits)0x80000000ULL)) {
        value |= theSignExtend == kSignExtend ? (bits)0xFFFFFFFFFFFFFF00ULL : 0ULL;
      }
      break;
    case kDoubleWord:
      break;
    case kQuadWord:
    default:
      DBG_Assert(false);
      break;
    }

    theInstruction->setOperand(kResult, (uint64_t)value);
    SEMANTICS_DBG(*this << " received load value=" << std::hex << value);
    if (theBypass) {
      mapped_reg name = theInstruction->operand<mapped_reg>(*theBypass);
      SEMANTICS_DBG(*this << " bypassing value=" << std::hex << value << " to " << name);
      core()->bypass(name, (uint64_t)value);
    }
    satisfyDependants();
  }

  void doEvaluate() {
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " LoadAction";
  }
};

predicated_dependant_action loadAction(SemanticInstruction *anInstruction, eSize aSize,
                                       eSignCode aSignCode, boost::optional<eOperandCode> aBypass) {
  LoadAction *act(new (anInstruction->icb())
                      LoadAction(anInstruction, aSize, aSignCode, aBypass, false));
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}

predicated_dependant_action casAction(SemanticInstruction *anInstruction, eSize aSize,
                                      eSignCode aSignCode, boost::optional<eOperandCode> aBypass) {
  LoadAction *act(new (anInstruction->icb())
                      LoadAction(anInstruction, aSize, aSignCode, aBypass, false));
  return predicated_dependant_action(act, act->dependance(), act->predicate());
}

} // namespace narmDecoder
