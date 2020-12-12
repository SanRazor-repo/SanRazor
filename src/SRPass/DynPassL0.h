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

struct DynPassL0 : public llvm::ModulePass {
    static char ID;
    std::map<llvm::Instruction*,llvm::Instruction*> ReducedInst;

    DynPassL0() : ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &M);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    
    void optimizeCheckAway(llvm::Instruction *Inst);
    bool reduceInstByCheck(llvm::Instruction *Inst);
    std::set<llvm::Value*> TrackValueLoc(llvm::Instruction *C, uint64_t id);
    bool findSameDT(llvm::Instruction *C1, llvm::Instruction *C2, uint64_t id1, uint64_t id2, uint64_t flag);
private:

    SCIPass *SCI;
};
