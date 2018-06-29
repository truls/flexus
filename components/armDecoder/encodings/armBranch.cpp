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
// Project, Computer Architecture Lab at Carnegie Mellon, Carnegie Mellon University.
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

#include "armBranch.hpp"
#include "armUnallocated.hpp"

namespace narmDecoder {

using namespace nuArchARM;

void branch_always( SemanticInstruction * inst, bool immediate, VirtualMemoryAddress target) {
    DBG_(Tmp, (<<"DECODER: In branch_always"));

    inst->setClass(clsBranch, codeBranchUnconditional);

    inst->addDispatchEffect( branch( inst, target ) );
    inst->addRetirementEffect( updateUnconditional( inst, target ) );
}

void branch_cc( SemanticInstruction * inst, VirtualMemoryAddress target, eCondCode aCode) {
  inst->setClass(clsBranch, codeBranchConditional);

  std::list<InternalDependance> rs_deps;

  dependant_action br = branchCCAction( inst, target, false, condition(aCode), false) ;
  connectDependance( inst->retirementDependance(), br );
  rs_deps.push_back( br.dependance );

  inst->addDispatchAction( br );
  inst->addRetirementEffect( updateConditional(inst) );
}

/*
* Branch causes an unconditional branch to a label at a PC-relative offset,
* with a hint that this is not a subroutine call or return.
*
* * Operation:
* BranchTo(PC[] + offset, BranchType_JMP);
*/
SemanticInstruction * B(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    uint64_t addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 0, 26) * 4 - 4;
    VirtualMemoryAddress target(addr);
    inst->addDispatchEffect( branch( inst, target ) );
    return inst;

}

/*
* Branch with Link branches to a PC-relative offset, setting the register X30 to PC+4.
* It provides a hint that this is a subroutine call.
*
* Operation:
*  X[30] = PC[] + 4;
*  BranchTo(PC[] + offset, BranchType_CALL);
*/
SemanticInstruction * BL(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    uint64_t addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 0, 26) * 4 - 4;
    VirtualMemoryAddress target(addr);
    std::vector< std::list<InternalDependance> > rs_deps(2);
    inst->setOperand(kOperand1, aFetchedOpcode.thePC);
    inst->setOperand(kOperand2, 4);
    predicated_action exec = addExecute(inst, operation(kADD_),rs_deps);

    addDestination(inst, 30, exec);
    inst->addDispatchEffect( branch( inst, target ) );
    return inst;
}

/*
* Compare and Branch on Zero compares the value in a register with zero, and conditionally branches to a label
* at a PC-relative offset if the comparison is equal. It provides a hint that this is not a subroutine call
* or return. This instruction does not affect condition flags.
*
* Operation:
* bits(datasize) operand1 = X[t];
* if IsZero(operand1) == TRUE then
* BranchTo(PC[] + offset, BranchType_JMP);
*/
SemanticInstruction * CBZ(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    unsigned int sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);

    std::vector<std::list<InternalDependance> > rs_deps(1);
    addReadXRegister(inst, 1, rt, rs_deps[0], sf);

    uint64_t addr = aFetchedOpcode.thePC + (sextract32(aFetchedOpcode.theOpcode, 5, 19) * 4);
    VirtualMemoryAddress target(addr);

    branch_cc(inst, target, kCBZ_);
    return inst;
}

/*
* Compare and Branch on Nonzero compares the value in a register with zero, and conditionally branches to a label
* at a PC-relative offset if the comparison is equal. It provides a hint that this is not a subroutine call
* or return. This instruction does not affect condition flags.
*
* Operation:
* bits(datasize) operand1 = X[t];
* if IsZero(operand1) == FALSE then
* BranchTo(PC[] + offset, BranchType_JMP);
*/
SemanticInstruction * CBNZ(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );
    unsigned int sf = extract32(aFetchedOpcode.theOpcode, 31, 1);
    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);

    std::vector<std::list<InternalDependance> > rs_deps(1);
    addReadXRegister(inst, 1, rt, rs_deps[0], sf);

    uint64_t addr = aFetchedOpcode.thePC + (sextract32(aFetchedOpcode.theOpcode, 5, 19) * 4);
    VirtualMemoryAddress target(addr);

    branch_cc(inst, target, kCBNZ_);
    return inst;
}

/*
 * Test bit and Branch if Zero compares the value of a test bit with zero,
 * and conditionally branches to a label at a PC-relative offset if the
 * comparison is equal. It provides a hint that this is not a subroutine call
 * or return. This instruction does not affect condition flags.
 *
 * Operation:
 * bits(datasize) operand = X[t];
 * if operand<bit_pos> == op then
 * BranchTo(PC[] + offset, BranchType_JMP);
 *
*/
SemanticInstruction * TBZ(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int bit_pos = (extract32(aFetchedOpcode.theOpcode, 31, 1) << 5) | extract32(aFetchedOpcode.theOpcode, 19, 5);

    std::vector<std::list<InternalDependance> > rs_deps(1);
    addReadXRegister(inst, 1, rt, rs_deps[0]);


    uint64_t addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 5, 14) * 4 - 4;
    VirtualMemoryAddress target(addr);

    inst->setOperand(kOperand2, (1ULL << bit_pos));

    branch_cc(inst,target, kTBZ_);

    return inst;
}

/*
 * Test bit and Branch if Nonzero compares the value of a test bit with zero,
 * and conditionally branches to a label at a PC-relative offset if the
 * comparison is equal. It provides a hint that this is not a subroutine call
 * or return. This instruction does not affect condition flags.
 *
 * Operation:
 * bits(datasize) operand = X[t];
 * if operand<bit_pos> == op then
 * BranchTo(PC[] + offset, BranchType_JMP);
 *
*/
SemanticInstruction * TBNZ(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int rt = extract32(aFetchedOpcode.theOpcode, 0, 5);
    unsigned int bit_pos = (extract32(aFetchedOpcode.theOpcode, 31, 1) << 5) | extract32(aFetchedOpcode.theOpcode, 19, 5);

    std::vector<std::list<InternalDependance> > rs_deps(1);
    addReadXRegister(inst, 1, rt, rs_deps[0]);


    uint64_t addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 5, 14) * 4 - 4;
    VirtualMemoryAddress target(addr);

    inst->setOperand(kOperand2, (1ULL << bit_pos));

    branch_cc(inst,target, kTBNZ_);

    return inst;
}

/*
 * Branch conditionally to a label at a PC-relative offset,
 * with a hint that this is not a subroutine call or return.
 *
 * Operation:
 * if ConditionHolds(condition) then
 * BranchTo(PC[] + offset, BranchType_JMP);
 */
SemanticInstruction * BCOND(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int cond = extract32(aFetchedOpcode.theOpcode, 0, 4);
    inst->setOperand(kOperand1, cond);

    uint64_t addr = aFetchedOpcode.thePC + sextract32(aFetchedOpcode.theOpcode, 5, 19) * 4;
    VirtualMemoryAddress target(addr);

    if (cond < 0x0e) {
        /* genuinely conditional branches */
        branch_cc(inst, target, kBCOND_);

    } else {
        /* 0xe and 0xf are both "always" conditions */
        branch_always(inst, false, target);
    }


    return inst;
}

/*
 * Branch to Register branches unconditionally to an address in a register,
 * with a hint that this is not a subroutine return.
 *
 * Operation:
 * bits(64) target = X[n];
 * BranchTo(target, BranchType_JMP);
 */
SemanticInstruction * BR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    std::vector<std::list<InternalDependance> > rs_deps(1);

    addReadXRegister(inst, 1, rn, rs_deps[0]);

    simple_action target = calcAddressAction( inst, rs_deps);
    dependant_action br = branchToCalcAddressAction( inst );
    connectDependance( br.dependance, target );
    connectDependance( inst->retirementDependance(), br );
    inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

    return inst;
}

/* >> -- same as RET
 * Branch with Link to Register calls a subroutine at an address in a register,
 * setting register X30 to PC+
 *
 * Operation:
 * bits(64) target = X[n];
 * if branch_type == BranchType_CALL then X[30] = PC[] + 4;
 * BranchTo(target, branch_type);
 */
SemanticInstruction * BLR(armcode const & aFetchedOpcode, uint32_t  aCPU, int64_t aSequenceNo) {
    SemanticInstruction * inst( new SemanticInstruction(aFetchedOpcode.thePC, aFetchedOpcode.theOpcode,
                                                        aFetchedOpcode.theBPState, aCPU, aSequenceNo) );

    unsigned int rn = extract32(aFetchedOpcode.theOpcode, 5, 5);
    unsigned int op = extract32(aFetchedOpcode.theOpcode, 21, 2);

    if (op > 2) {
        return unallocated_encoding(aFetchedOpcode, aCPU, aSequenceNo);
    }

    std::vector<std::list<InternalDependance> > rs_deps(1);
    addReadXRegister(inst, 1, rn, rs_deps[0]);

    simple_action target = calcAddressAction( inst, rs_deps);
    dependant_action br = branchToCalcAddressAction( inst );
    connectDependance( br.dependance, target );
    connectDependance( inst->retirementDependance(), br );
    inst->addRetirementEffect( updateUnconditional( inst, kAddress ) );

    // Link
    Operation& op = operation(kADD_);
    op.setOperands(aFetchedOpcode.thePC);
    op.setOperands(4);
    predicated_action exec = addExecute(inst, operation(kADD_),rs_deps);
    addDestination(inst, 30, exec);

    return inst;
}


} // narmDecoder
