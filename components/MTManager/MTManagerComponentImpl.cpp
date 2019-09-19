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

#include <components/MTManager/MTManagerComponent.hpp>

#define DBG_DefineCategories MTMgr
#define DBG_SetDefaultOps AddCat(MTMgr)
#include DBG_Control()

#include <core/flexus.hpp>
#include <core/stats.hpp>

#define FLEXUS_BEGIN_COMPONENT MTManagerComponent
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/MTManager/MTManager.hpp>

namespace nMTManager {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

struct ScheduleState {
  uint32_t theRoundRobinPtr;
  uint32_t theThreadThisCycle;
  uint64_t theLastScheduledCycle;
  ScheduleState() : theRoundRobinPtr(0), theThreadThisCycle(0), theLastScheduledCycle(0) {
  }
};

MTManager *theMTManager = 0;

class FLEXUS_COMPONENT(MTManagerComponent), public MTManager {
  FLEXUS_COMPONENT_IMPL(MTManagerComponent);

  std::vector<ScheduleState> theFAGSchedules;
  std::vector<ScheduleState> theFSchedules;
  std::vector<ScheduleState> theDSchedules;
  std::vector<ScheduleState> theEXSchedules;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MTManagerComponent) : base(FLEXUS_PASS_CONSTRUCTOR_ARGS) {
    theMTManager = this;
  }

  bool isQuiesced() const {
    return true;
  }

  void initialize() {
    theFAGSchedules.resize(cfg.Cores);
    theFSchedules.resize(cfg.Cores);
    theDSchedules.resize(cfg.Cores);
    theEXSchedules.resize(cfg.Cores);
  }

  void finalize() {
  }

  int32_t getICount(int32_t aCore, int32_t aThread) {
    int32_t sum = 0;
    int32_t count = 0;
    FLEXUS_CHANNEL_ARRAY(FAQ_ICount, (aCore * cfg.Threads + aThread)) >> count;
    sum += count;
    FLEXUS_CHANNEL_ARRAY(FIQ_ICount, (aCore * cfg.Threads + aThread)) >> count;
    sum += count;
    FLEXUS_CHANNEL_ARRAY(ROB_ICount, (aCore * cfg.Threads + aThread)) >> count;
    sum += count;
    return sum;
  }

  bool isFAGStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(FAGStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  bool isFStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(FStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  bool isDStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(DStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  bool isEXStalled(int32_t aCore, int32_t aThread) {
    bool stalled;
    FLEXUS_CHANNEL_ARRAY(EXStalled, (aCore * cfg.Threads + aThread)) >> stalled;
    return stalled;
  }

  void scheduleICount(int32_t aCore, ScheduleState & aSchedule,
                      bool (nMTManager::MTManagerComponentComponent::*isStalled)(int, int)) {
    std::vector<uint32_t> icount(cfg.Threads);
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      icount[i] = getICount(aCore, i);
    }
    int32_t selected_td = aSchedule.theRoundRobinPtr;
    uint32_t min_icount = static_cast<uint32_t>(-1);
    int32_t td = selected_td;
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      DBG_(Iface, (<< "   Td[" << td << "] icount: " << icount[td] << " stalled? "
                   << (this->*isStalled)(aCore, td)));
      if (icount[td] < min_icount && !(this->*isStalled)(aCore, td)) {
        selected_td = td;
        min_icount = icount[td];
      }
      td = (td + 1) % cfg.Threads;
    }
    aSchedule.theThreadThisCycle = selected_td;
    DBG_(Iface, (<< "   Td[" << selected_td << "] selected"));
    aSchedule.theRoundRobinPtr = (aSchedule.theRoundRobinPtr + 1) % cfg.Threads;
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  void scheduleExecuteRoundRobin(int32_t aCore, ScheduleState & aSchedule) {
    int32_t selected_td = aSchedule.theRoundRobinPtr;
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      if (isEXStalled(aCore, selected_td)) {
        DBG_(Iface, (<< "   Td[" << selected_td << "] stalled"));
        selected_td = (selected_td + 1) % cfg.Threads;
      } else {
        DBG_(Iface, (<< "   Td[" << selected_td << "] selected"));
        break;
      }
    }
    aSchedule.theThreadThisCycle = selected_td;
    aSchedule.theRoundRobinPtr = (aSchedule.theRoundRobinPtr + 1) % cfg.Threads;
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  void scheduleDispatchRoundRobin(int32_t aCore, ScheduleState & aSchedule) {
    int32_t selected_td = aSchedule.theRoundRobinPtr;
    for (uint32_t i = 0; i < cfg.Threads; ++i) {
      if (isDStalled(aCore, selected_td)) {
        DBG_(Iface, (<< "   Td[" << selected_td << "] stalled"));
        selected_td = (selected_td + 1) % cfg.Threads;
      } else {
        DBG_(Iface, (<< "   Td[" << selected_td << "] selected"));
        break;
      }
    }
    aSchedule.theThreadThisCycle = selected_td;
    aSchedule.theRoundRobinPtr = (aSchedule.theRoundRobinPtr + 1) % cfg.Threads;
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  void scheduleStrictRoundRobin(ScheduleState & aSchedule) {
    aSchedule.theThreadThisCycle = aSchedule.theRoundRobinPtr;
    aSchedule.theRoundRobinPtr++;
    if (aSchedule.theRoundRobinPtr == cfg.Threads) {
      aSchedule.theRoundRobinPtr = 0;
    }
    aSchedule.theLastScheduledCycle = Flexus::Core::theFlexus->cycleCount();
  }

  uint32_t scheduleThread(std::vector<ScheduleState> & aSchedules, char const *aResource,
                          bool (nMTManager::MTManagerComponentComponent::*isStalled)(int, int),
                          uint32_t aCoreIndex) {
    DBG_Assert(aCoreIndex < cfg.Cores);
    ScheduleState &sched = aSchedules[aCoreIndex];
    if (cfg.FrontEndPolicy == kFE_ICount) {
      DBG_(Iface, (<< "Core[" << aCoreIndex << "] Schedule " << aResource << " via ICount "));
      scheduleICount(aCoreIndex, sched, isStalled);
    } else {
      DBG_(Iface, (<< "Core[" << aCoreIndex << "] Schedule " << aResource << " via RoundRobin "));
      scheduleStrictRoundRobin(sched);
    }
    DBG_(Iface,
         (<< "Core[" << aCoreIndex << "] " << aResource << ": " << sched.theThreadThisCycle));
    return sched.theThreadThisCycle;
  }

  uint32_t scheduleFAGThread(uint32_t aCoreIndex) {
    return scheduleThread(theFAGSchedules, "FAG",
                          &nMTManager::MTManagerComponentComponent::isFAGStalled, aCoreIndex);
  }
  uint32_t scheduleFThread(uint32_t aCoreIndex) {
    return scheduleThread(theFSchedules, "F", &nMTManager::MTManagerComponentComponent::isFStalled,
                          aCoreIndex);
  }

  bool runThisEX(uint32_t anIndex) {
    uint32_t core_index = anIndex / cfg.Threads;
    uint32_t thread_index = anIndex % cfg.Threads;
    DBG_Assert(core_index < cfg.Cores);
    ScheduleState &sched = theEXSchedules[core_index];

    if (sched.theLastScheduledCycle != Flexus::Core::theFlexus->cycleCount()) {
      if (cfg.BackEndPolicy == kBE_SmartRoundRobin) {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule EX via SmartRoundRobin"));
        scheduleExecuteRoundRobin(core_index, sched);
      } else {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule EX via StrictRoundRobin"));
        scheduleStrictRoundRobin(sched);
      }
      DBG_(Verb, (<< "Core[" << core_index << "] EX: " << sched.theThreadThisCycle));
    }
    return sched.theThreadThisCycle == thread_index;
  }

  bool runThisD(uint32_t anIndex) {
    uint32_t core_index = anIndex / cfg.Threads;
    uint32_t thread_index = anIndex % cfg.Threads;
    DBG_Assert(core_index < cfg.Cores);
    ScheduleState &sched = theDSchedules[core_index];

    if (sched.theLastScheduledCycle != Flexus::Core::theFlexus->cycleCount()) {
      if (cfg.BackEndPolicy == kBE_SmartRoundRobin) {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule D via SmartRoundRobin"));
        // scheduleThread(  theDSchedules, "D",
        // &nMTManager::MTManagerComponentComponent::isDStalled, core_index);
        scheduleDispatchRoundRobin(core_index, sched);
      } else {
        DBG_(Iface, (<< "Core[" << core_index << "] Schedule D via StrictRoundRobin"));
        scheduleStrictRoundRobin(sched);
      }
    }
    return sched.theThreadThisCycle == thread_index;
  }
};

MTManager *MTManager::get() {
  return theMTManager;
}

} // end namespace nMTManager

FLEXUS_COMPONENT_INSTANTIATOR(MTManagerComponent, nMTManager);

FLEXUS_PORT_ARRAY_WIDTH_NO_CFG(MTManagerComponent, EXStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH_NO_CFG(MTManagerComponent, FAGStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH_NO_CFG(MTManagerComponent, FStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH_NO_CFG(MTManagerComponent, DStalled) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH_NO_CFG(MTManagerComponent, FAQ_ICount) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH_NO_CFG(MTManagerComponent, FIQ_ICount) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}
FLEXUS_PORT_ARRAY_WIDTH_NO_CFG(MTManagerComponent, ROB_ICount) {
  return Flexus::Core::ComponentManager::getComponentManager().systemWidth();
}

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MTManagerComponent

#define DBG_Reset
#include DBG_Control()
