// This file is part of ASAP.
// Please see LICENSE.txt for copyright and licensing information.

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"



#include <map>
#include <set>
#include <list>

namespace llvm {
    class AnalysisUsage;
    class BasicBlock;
    class CallInst;
    class Function;
    class Instruction;
    class Value;
    class StringRef;
    class dyn_cast;
}

// struct BranchInfo {
//     static llvm::StringRef BranchName;
//     void insert(llvm::Instruction *Inst) {
//         llvm::BranchInst *BI = llvm::dyn_cast<llvm::BranchInst>(Inst);
//         int first = 0;
//         if (BI && BI->isConditional()) {
//             // Insert the first Instruction of each successor BasicBlock into each Instruction in SCBranch 
//             for (int i=0; i<=1; i++) {
//                 llvm::BasicBlock *BB = BI->getSuccessor(i);
//                 first = 0;
//                 for (llvm::Instruction &I: *BB) {
//                     first += 1;
//                     if(first == 1) {
//                         BranchSet[Inst].insert(&I);
//                         break;
//                     }
//                 }
//             }
//         }
//     }
// private:
//     std::map<llvm::Instruction*, llvm::SmallPtrSet<llvm::Instruction*, 2>> BranchSet;
// };

struct SCIPass : public llvm::ModulePass {
    static char ID;

    SCIPass() : ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &M);

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        AU.setPreservesAll();
    }

    // Types used to store sanity check blocks / instructions
    typedef std::set<llvm::BasicBlock*> BlockSet;
    typedef std::set<llvm::Instruction*> InstructionSet;
    typedef std::list<llvm::Instruction*> InstructionVec;

    const InstructionVec &getSCBranches(llvm::Function *F) const {
        return SCBranches.at(F);
    }

    const InstructionVec &getSCBranchesV(llvm::Function *F) const {
        return SCBranchesV.at(F);
    }
    
    const BlockSet &getSanityCheckBlocks(llvm::Function *F) const {
        return SanityCheckBlocks.at(F);
    }

    const InstructionVec &getUCBranches(llvm::Function *F) const {
        return UCBranches.at(F);
    }

    const InstructionSet &getInstructionsBySanityCheck(llvm::Instruction *Inst) const {
        return InstructionsBySanityCheck.at(Inst);
    }

    const InstructionSet &getChecksByInstruction(llvm::Instruction *Inst) const {
        return ChecksByInstruction.at(Inst);
    }

    // Searches the given basic block for a call instruction that corresponds to
    // a sanity check and will abort the program (e.g., __assert_fail).
    const llvm::CallInst *findSanityCheckCall(llvm::BasicBlock *BB) const;
    
private:

    // All blocks that abort due to sanity checks
    std::map<llvm::Function*, BlockSet> SanityCheckBlocks;
    // std::map<llvm::Function*, BlockSet> SanityCheckBlocksPlus;
    std::map<llvm::Instruction*, InstructionSet> ChecksByInstruction;

    // All instructions that belong to sanity checks
    std::map<llvm::Function*, InstructionSet> SanityCheckInstructions;
    std::map<llvm::Instruction*, InstructionSet> InstructionsBySanityCheck;
    
    // All sanity checks themselves (branch instructions that could lead to an abort)
    std::map<llvm::Function*, InstructionVec> SCBranches;
    std::map<llvm::Function*, InstructionVec> SCBranchesV;

    std::map<llvm::Function*, InstructionVec> UCBranches;

    void findInstructions(llvm::Function *F);
    bool onlyUsedInSanityChecks(llvm::Value *V);
};
