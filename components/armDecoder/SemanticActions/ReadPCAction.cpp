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

#include <boost/dynamic_bitset.hpp>

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

// static const bits kAM = 0x8;

struct ReadPCAction : public PredicatedSemanticAction {
  eOperandCode theResult;

  ReadPCAction(SemanticInstruction *anInstruction, eOperandCode aResult)
      : PredicatedSemanticAction(anInstruction, 1, true), theResult(aResult) {
    setReady(0, true);
  }

  void doEvaluate() {
    //    bits pstate = theInstruction->core()->getPSTATE() ;
    //    if (pstate & kAM ) {
    //      //Need to mask upper 32 bits when AM is set
    //      theInstruction->setOperand(theResult,
    //      static_cast<bits>(theInstruction->pc()) & 0xFFFFFFFFULL);

    //    } else {
    uint64_t pc = theInstruction->pc();
    theInstruction->setOperand(theResult, (bits)pc);
    //    }
    DBG_(VVerb, (<< *this << " read PC"));
    satisfyDependants();
  }

  void describe(std::ostream &anOstream) const {
    anOstream << theInstruction->identify() << " Read PC store in " << theResult;
  }
};

predicated_action readPCAction(SemanticInstruction *anInstruction) {
  ReadPCAction *act(new (anInstruction->icb()) ReadPCAction(anInstruction, kResult));

  return predicated_action(act, act->predicate());
}

} // namespace narmDecoder
