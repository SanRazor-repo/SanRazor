// This file is part of ASAP.
// Please see LICENSE.txt for copyright and licensing information.

#include "SafePass.h"
#include "SCIPass.h"
#include "CostModel.h"
#include "utils.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Analysis/TargetTransformInfo.h"

#include <algorithm>
#include <memory>
#include <system_error>
#define DEBUG_TYPE "safepass"

using namespace llvm;


static cl::opt<std::string>
InputSCOV("safe-scov", cl::desc("<input scov file>"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
InputUCOV("safe-ucov", cl::desc("<input ucov file>"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
CheckType("san-type", cl::desc("<sanitizer type, choose asan or ubsan>"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
SanType("san-level", cl::desc("<sanitizer level, choose L0 or L1 or L2 or L3>"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
CheckID("checkcost-id", cl::desc("printCheckID"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
CheckFile("checkcost-file", cl::desc("printCheckFile"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
LogPath("checkcost-logpath", cl::desc("<input ucov file>"), cl::init(""), cl::Hidden);

namespace {
    bool largerCost(const SafePass::CheckCostPair &a,
                     const SafePass::CheckCostPair &b) {
        return a.second > b.second;
    }
}

bool SafePass::runOnModule(Module &m) {
    SCI = &getAnalysis<SCIPass>();
    TargetTransformInfoWrapperPass &TTIWP = getAnalysis<TargetTransformInfoWrapperPass>();
    std::string filename = m.getSourceFileName();
    filename = filename.substr(0, filename.rfind("."));
    const char *checkid = CheckID.c_str();

    struct brinfo{
        uint64_t id;
        uint64_t count[3];
    }BrInfo;

    struct info{
        double CostLevel1;
        double CostLevel2;
        double NumLevel1;
        double NumLevel2;
        uint64_t id;
        uint64_t count[3];
    };

    struct stat{
        uint64_t id;
        uint64_t LB;
        uint64_t RB;
        Instruction* SC;
    };

    struct coststat{
        double CostLevel1;
        double CostLevel2;
        double NumLevel1;
        double NumLevel2;
        Instruction* SC;
    };

    std::map<Instruction*, info> SC_Stat;
    std::map<uint64_t, std::vector<stat>> SC_Pattern, SC_Pattern_opt;
    std::map<uint64_t, std::vector<coststat>> CostLevelRange;
    std::vector<Instruction*> RSC;
    StringRef SCType = CheckType;
    StringRef SCLevel = SanType;

    FILE *fp_sc = fopen(Twine(InputSCOV).str().c_str(), "rb");
    // errs() << Twine(InputSCOV).str().c_str() << "\n";
    // assert(fp_sc != NULL && "No valid SCOV file");
    FILE *fp_uc = fopen(Twine(InputUCOV).str().c_str(), "rb");
    FILE *fp_check = fopen(Twine(LogPath).str().c_str(), "ab");
    // assert(fp_uc != NULL && "No valid UCOV file");
    if (fp_sc != NULL && fp_uc != NULL && fp_check != NULL) {
        errs() << "SafePass jj on "<<filename << "\n";

        uint64_t flagSC = 0, costflagSC = 0; // Number of SCs
        uint64_t flagUC = 0, costflagUC = 0; // Number of UCs
        uint64_t flagSC_opt = 0, costflagSC_opt = 0; // Number of SCs after the redundant SCs about UCs are reduced
        uint64_t flagSC_opts = 0, costflagSC_opts = 0; // Number of SCs after the redundant SCs about SCs are reduced
        uint64_t test1 = 0, test2 = 0;
        uint64_t nInstructions = 0, nFreeInstructions = 0, total_cost = 0, total_num = 0;
        bool is_reduced = true;

        // Start reading and storing SC coverage records from InputSCOV
        for (Function &F: m) {
            const TargetTransformInfo &TTI = TTIWP.getTTI(F);
            for (Instruction *Inst: SCI->getSCBranches(&F)) {
                assert(Inst->getParent()->getParent() == &F && "SCI must only contain instructions of the current function.");

                BranchInst *BI = dyn_cast<BranchInst>(Inst);
                assert(BI && BI->isConditional() && "SCBranches must not contain instructions that aren't conditional branches.");

                fread(&BrInfo, sizeof(BrInfo), 1, fp_sc);
                // Revise the coverage pattern of SC
                // errs() <<"SC:"<<BrInfo.id << ":"<< BrInfo.count[0] <<":"<< BrInfo.count[1] <<":"<< BrInfo.count[2] << "\n";
                BrInfo.count[0] = BrInfo.count[1] + BrInfo.count[2];
                if (BrInfo.id >= 53 && BrInfo.id <= 69) {
                    errs() << BrInfo.id << "::" << BrInfo.count[0] << "---";
                    reducedSC[63] = BrInfo.count[0];
                }
                // if (BrInfo.count[0] > 0) {
                //     optimizeCheckAway(Inst);
                // }
                // Store the coverage data by UC_Stat
                for (size_t i=0;i<3;i++){
                    SC_Stat[Inst].count[i] = BrInfo.count[i];
                }
                SC_Stat[Inst].id = BrInfo.id;
                SC_Stat[Inst].CostLevel1 = 0;
                SC_Stat[Inst].CostLevel2 = 0;
                SC_Stat[Inst].NumLevel1 = 0;
                SC_Stat[Inst].NumLevel2 = 0;


                // Store the dynamic patterns of instructions in SCBranches in SC_Pattern (totalCovTime->SC_Info)
                // SC_Info contains id, LB, RB, and *Inst
                struct stat SC_Info;
                SC_Info.id = BrInfo.id;
                SC_Info.LB = BrInfo.count[1];
                SC_Info.RB = BrInfo.count[2];
                SC_Info.SC = Inst;
                SC_Pattern[BrInfo.count[0]].push_back(SC_Info);
                flagSC += 1;
                costflagSC += BrInfo.count[0];
                total_num += 1;
                
                uint64_t Cost = 0;
                // Calculate cost of each checks
                for (Instruction *CI: SCI->getInstructionsBySanityCheck(BI)) {
                    unsigned CurrentCost = CheckCost::getInstructionCost(CI, &TTI);
                    if (CurrentCost == (unsigned)(-1)) {
                        CurrentCost = 1;
                    }
                    if (CurrentCost == 0) {
                        nFreeInstructions += 1;
                    }
                    Cost += CurrentCost * BrInfo.count[0];
                    total_cost += CurrentCost * BrInfo.count[0];
                }
                // CheckCostVec.push_back(std::make_pair(BI, Cost));
                struct coststat SCCostInfo;
                SCCostInfo.CostLevel1 = 0;
                SCCostInfo.CostLevel2 = 0;
                SCCostInfo.NumLevel1 = 0;
                SCCostInfo.NumLevel2 = 0;
                SCCostInfo.SC = Inst;
                CostLevelRange[Cost].push_back(SCCostInfo);
            }
        }
        // std::sort(CheckCostVec.begin(), CheckCostVec.end(), largerCost);
        fclose(fp_sc);
        // Finish reading and storing SC coverage records from InputSCOV
        // Calculate the cost level of 
        double CostLevel1 = 0, CostLevel2 = 0, NumLevel1 = 0, NumLevel2 = 0;
        if (total_cost == 0) {
            total_cost = 1;
        }
        if (total_num == 0) {
            total_num = 1;
        }
        errs() << "Total cost:" << total_cost << " ,total num:" << total_num << "\n";
        for (std::map<uint64_t, std::vector<coststat>>::iterator I=CostLevelRange.begin(); I!=CostLevelRange.end(); ++I) {
            // errs() << I->first << "::" << I->second.size() << "---\n";
            CostLevel2 += I->first * I->second.size();
            NumLevel2 += I->second.size();

            for (uint64_t i=0; i<I->second.size(); i++) {
                
                I->second[i].CostLevel1 = CostLevel1 / total_cost;
                I->second[i].CostLevel2 = CostLevel2 / total_cost;
                I->second[i].NumLevel1 = NumLevel1 / total_num;
                I->second[i].NumLevel2 = NumLevel2 / total_num;
                SC_Stat[I->second[i].SC].CostLevel1 = CostLevel1 / total_cost;
                SC_Stat[I->second[i].SC].CostLevel2 = CostLevel2 / total_cost;
                SC_Stat[I->second[i].SC].NumLevel1 = NumLevel1 / total_cost;
                SC_Stat[I->second[i].SC].NumLevel2 = NumLevel2 / total_cost;
                if (SC_Stat[I->second[i].SC].id == atoi(checkid) && InputSCOV == CheckFile) {
                    fprintf(fp_check, \
                    "Total cost: %lu\nNumber of sanitizer checks: %lu\nNo. %lu in a total of %lu sanitizer check with cost %lu\nCost range: [%lf, %lf], Rank range: [%lf, %f]\n",
                    total_cost, total_num, i, I->second.size(), I->first, \
                    I->second[i].CostLevel1, I->second[i].CostLevel2, \
                    I->second[i].NumLevel1, I->second[i].NumLevel2);
                    errs() << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n";
                    errs() << i << "in" << I->second.size() << ":" << I->second[i].CostLevel1 << "::" << I->second[i].CostLevel2 << 
                    ":" << I->second[i].NumLevel1 << "::" << I->second[i].NumLevel2 << "----\n";
                    errs() << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n";
                }
            }
            CostLevel1 = CostLevel2;
            NumLevel1 = NumLevel2;
        }
    }
    return true;
}

void SafePass::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.addRequired<SCIPass>();
    AU.setPreservesAll();
}

char SafePass::ID = 0;
static RegisterPass<SafePass> X("SafePass",
        "Finds costs of sanity checks", false, false);

