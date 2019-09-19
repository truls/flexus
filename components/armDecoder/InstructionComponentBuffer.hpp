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

#ifndef FLEXUS_armDECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED
#define FLEXUS_armDECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <iostream>

#include <core/stats.hpp>
#include <core/types.hpp>

static const size_t kICBSize = 2048;

namespace narmDecoder {
extern Flexus::Stat::StatCounter theICBs;

struct InstructionComponentBuffer {

  char theComponentBuffer[kICBSize];
  char *theNextAlloc;
  size_t theFreeSpace;
  InstructionComponentBuffer *theExtensionBuffer;

  InstructionComponentBuffer()
      : theNextAlloc(theComponentBuffer), theFreeSpace(kICBSize), theExtensionBuffer(0) {
    ++theICBs;
  }

  ~InstructionComponentBuffer() {
    if (theExtensionBuffer) {
      delete theExtensionBuffer;
      theExtensionBuffer = 0;
    }
    --theICBs;
  }

  void *alloc(size_t aSize) {
    DBG_Assert(aSize < kICBSize);
    if (theFreeSpace < aSize) {
      if (theExtensionBuffer == 0) {
        theExtensionBuffer = new InstructionComponentBuffer();
      }
      return theExtensionBuffer->alloc(aSize);
    }
    void *tmp = theNextAlloc;
    theNextAlloc += aSize;
    DBG_Assert(theFreeSpace >= aSize);
    theFreeSpace -= aSize;
    return tmp;
  }
};

struct UncountedComponent {
  void operator delete(void *) {
    /*Nothing to do on delete*/ // FIXME: this leaks, since icb.alloc() calls
                                // new(...)
  }
  void *operator new(size_t aSize, InstructionComponentBuffer &aICB) {
    return aICB.alloc(aSize);
  }

private:
  void *operator new(size_t); // Not allowed
};

} // namespace narmDecoder

#endif // FLEXUS_armDECODER_INSTRUCTIONCOMPONENTBUFFER_HPP_INCLUDED
