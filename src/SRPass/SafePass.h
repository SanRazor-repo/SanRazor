// This file is part of ASAP.
// Please see LICENSE.txt for copyright and licensing information.

#include "llvm/Pass.h"

#include <utility>
#include <vector>
#include <map>
#include <set>

namespace sanitychecks {
    class GCOVFile;
}

namespace llvm {
    class BranchInst;
    class raw_ostream;
    class Instruction;
    class Value;
}

struct SCIPass;

struct SafePass : public llvm::ModulePass {
    static char ID;
    std::map<llvm::Instruction*,llvm::Instruction*> ReducedInst;

    SafePass() : ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &M);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    
    void optimizeCheckAway(llvm::Instruction *Inst);
    bool findPhiInst(llvm::Instruction *SC_Inst, llvm::Instruction *UC_Inst);
    bool findSameSource(llvm::Instruction *BI1, llvm::Instruction *BI2,  uint64_t id1, uint64_t id2, size_t flag, llvm::StringRef SCType, llvm::StringRef SCLevel);
    bool reduceInstByCheck(llvm::Instruction *Inst);

    bool SameMemoryLoc(std::set<llvm::Instruction*> Clist1, std::set<llvm::Instruction*> Clist2, llvm::StringRef SCLevel);
    std::set<llvm::Instruction*> TrackMemoryLoc(llvm::Instruction *Inst, llvm::StringRef type, uint64_t id, llvm::StringRef SCLevel);
    bool SameStaticPattern(llvm::Instruction *C1, llvm::Instruction *C2);

    typedef std::pair<llvm::BranchInst *, uint64_t> CheckCostPair;
    const std::vector<CheckCostPair> &getCheckCostVec() const {
        return CheckCostVec;
    };

private:
    std::map<uint64_t, uint64_t> reducedSC;
    std::vector<CheckCostPair> CheckCostVec;
    SCIPass *SCI;
};
