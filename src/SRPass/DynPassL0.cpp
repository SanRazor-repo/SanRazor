// This file is part of ASAP.
// Please see LICENSE.txt for copyright and licensing information.

#include "DynPassL0.h"
#include "SCIPass.h"
#include "utils.h"
#include "CostModel.h"
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
#define DEBUG_TYPE "DynPassL0"

using namespace llvm;



static cl::opt<std::string>
InputSCOV("scovL0", cl::desc("<input scov file>"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
InputUCOV("ucovL0", cl::desc("<input ucov file>"), cl::init(""), cl::Hidden);

static cl::opt<std::string>
InputLOG("logL0", cl::desc("<input log file>"), cl::init(""), cl::Hidden);

static cl::opt<double>
Threshold("use-asapL0", cl::desc("Fraction of dynamic checks to be preserved"), cl::init(-1.0));

bool DynPassL0::runOnModule(Module &m) {
    SCI = &getAnalysis<SCIPass>();
    TargetTransformInfoWrapperPass &TTIWP = getAnalysis<TargetTransformInfoWrapperPass>();
    std::string filename = m.getSourceFileName();
    filename = filename.substr(0, filename.rfind("."));
    

    struct tmp{
        uint64_t id;
        uint64_t count[3];
    }BrInfo;
    typedef uint64_t Info[3];
    struct stat{
        uint64_t id;
        uint64_t LB;
        uint64_t RB;
        Instruction* SC;
    };

    std::map<Instruction*, Info> SC_Stat;
    std::map<Instruction*, Info> UC_Stat;
    std::map<uint64_t, std::vector<stat>> SC_Pattern, SC_Pattern_opt;
    std::map<uint64_t, uint64_t> reducedSC;
    int count = 0;
    // for (Function &F: m) {
    //     for (Instruction *Inst: SCI->getSCBranches(&F)) {
    //         optimizeCheckAway(Inst);

    //     }
    // }

    FILE *fp_sc = fopen(Twine(InputSCOV).str().c_str(), "rb");
    // errs() << Twine(InputSCOV).str().c_str() << "\n";
    // assert(fp_sc != NULL && "No valid SCOV file");
    FILE *fp_uc = fopen(Twine(InputUCOV).str().c_str(), "rb");
    // assert(fp_uc != NULL && "No valid UCOV file");
    if (fp_sc != NULL && fp_uc != NULL) {
    errs() << "DynPassL0 on "<<filename << ";" << Twine(InputSCOV).str().c_str() << "\n";

    uint64_t flagSC = 0, costflagSC = 0; // Number of SCs
    uint64_t flagUC = 0, costflagUC = 0; // Number of UCs
    uint64_t flagSC_opt = 0, costflagSC_opt = 0; // Number of SCs after the redundant SCs about UCs are reduced
    uint64_t flagSC_opts = 0, costflagSC_opts = 0; // Number of SCs after the redundant SCs about SCs are reduced
    uint64_t test1 = 0, test2 = 0, test3 = 0;
    bool is_reduced = true;

    // Start reading and storing SC coverage records from InputSCOV
    for (Function &F: m) {
        for (Instruction *Inst: SCI->getSCBranches(&F)) {
            assert(Inst->getParent()->getParent() == &F && "SCI must only contain instructions of the current function.");
            BranchInst *BI = dyn_cast<BranchInst>(Inst);
            assert(BI && BI->isConditional() && "SCBranches must not contain instructions that aren't conditional branches.");
            fread(&BrInfo, sizeof(BrInfo), 1, fp_sc);
            // Revise the coverage pattern of SC
            // errs() <<"SC:"<<BrInfo.id << ":"<< BrInfo.count[0] <<":"<< BrInfo.count[1] <<":"<< BrInfo.count[2] << "\n";
            BrInfo.count[0] = BrInfo.count[1] + BrInfo.count[2];
            // Store the coverage data by UC_Stat
            // if (BrInfo.count[0] > 0) {
            //     optimizeCheckAway(Inst);
            // }
            
            for (size_t i=0;i<3;i++){
                SC_Stat[Inst][i] = BrInfo.count[i];
            }

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
        }
    }
    fclose(fp_sc);
    // Finish reading and storing SC coverage records from InputSCOV

    // Start reading and storing UC coverage records from InputUCOV
    // During this process, each UC will be compared with all SCs in SC_Pattern
    // For each SC in SC_Pattern, if its coverage pattern matches that of UC
    // And if the variable it operates is the same as that operated by UC
    // Then, the SC can be reduced.
    flagSC_opt = flagSC;
    costflagSC_opt = costflagSC;
    errs() <<flagSC <<":"<<costflagSC << "----\n";
    for (Function &F: m) {
        for (Instruction *Inst: SCI->getUCBranches(&F)) {
            assert(Inst->getParent()->getParent() == &F && "SCI must only contain instructions of the current function.");
            BranchInst *BI = dyn_cast<BranchInst>(Inst);
            assert(BI && BI->isConditional() && "UCBranches must not contain instructions that aren't conditional branches.");
            fread(&BrInfo, sizeof(BrInfo), 1, fp_uc);

            flagUC += 1;
            costflagUC += BrInfo.count[0];
            // For each instruction in UCBranch, check whether its coverage pattern matches certain patterns in SC_Pattern

            if (SC_Pattern.count(BrInfo.count[0]) > 0 && BrInfo.count[0] > 0) {
                // If UC and SC have ompletely the same dynamic pattern A+B:A:B
                for (stat Info: SC_Pattern[BrInfo.count[0]]) {
                    if ((Info.LB == BrInfo.count[1] && Info.RB == BrInfo.count[2]) || (Info.LB == BrInfo.count[2] && Info.RB == BrInfo.count[1])) {
                        // If UC and SC operate the same variable
                        // errs() << "TTT"<<Info.id << "---";

                        if (findSameDT(Info.SC, Inst, BrInfo.id, Info.id, 0)  && reducedSC.count(Info.id) == 0) {
                            optimizeCheckAway(Info.SC);
                            errs() << "Reduced::UC:" << BrInfo.id<<"SC:"<<Info.id <<":"<< BrInfo.count[0]<<"--------\n";
                            flagSC_opt -= 1;
                            costflagSC_opt -= BrInfo.count[0];
                            reducedSC[Info.id] = BrInfo.count[0];
                        }
                    }
                }
            }
            else if (SC_Pattern.count(BrInfo.count[1]) > 0 && BrInfo.count[1] > 0) {
                // UC has pattern A+B:A:B, while SC has pattern A:A:0
                for (stat Info: SC_Pattern[BrInfo.count[1]]) {
                    if (Info.LB == 0 || Info.RB == 0) {
                        // If UC and SC operate the same variable
                        if (findSameDT(Info.SC, Inst, BrInfo.id, Info.id, 0) && reducedSC.count(Info.id) == 0) {
                            optimizeCheckAway(Info.SC);
                            errs() << "Reduced::UC:" << BrInfo.id<<"SC:"<<Info.id <<":"<< BrInfo.count[1]<<"--------\n";
                            flagSC_opt -= 1;
                            costflagSC_opt -= BrInfo.count[1];
                            reducedSC[Info.id] = BrInfo.count[1];
                        }
                    }
                }
            }
            else if (SC_Pattern.count(BrInfo.count[2]) > 0 && BrInfo.count[2] > 0) {
                // UC has pattern A+B:A:B, while SC has pattern B:B:0
                for (stat Info: SC_Pattern[BrInfo.count[2]]) {
                    if (Info.LB == 0 || Info.RB == 0){
                        if (findSameDT(Info.SC, Inst, BrInfo.id, Info.id, 0) && reducedSC.count(Info.id) == 0) {
                            optimizeCheckAway(Info.SC);
                            errs() << "Reduced::UC:" << BrInfo.id<<"SC:"<<Info.id <<":"<< BrInfo.count[1]<< "--------\n";
                            flagSC_opt -= 1;
                            costflagSC_opt -= BrInfo.count[2];
                            reducedSC[Info.id] = BrInfo.count[2];
                        }
                    }
                }
            }
        }
    }
    fclose(fp_uc);
    // Finish reading and storing UC coverage records from InputUCOV

    // Reduce redundant SCs among SCs
    // For each SC read from SCOV file
    // Check whether its pattern matches any SCs in SC_Pattern_opt
    // If it is, this SC can be reduced
    flagSC_opts = flagSC_opt;
    costflagSC_opts = costflagSC_opt;
    errs() <<flagSC_opt <<":"<<costflagSC_opt << "----\n";
    fp_sc = fopen(Twine(InputSCOV).str().c_str(), "rb");
    uint64_t tmp = 0;
    uint64_t Cost = 0, Total_Cost = 0, Total_Cost_Opt = 0;
    for (Function &F: m) {
        const TargetTransformInfo &TTI = TTIWP.getTTI(F);
        for (Instruction *Inst: SCI->getSCBranches(&F)) {
            assert(Inst->getParent()->getParent() == &F && "SCI must only contain instructions of the current function.");
            BranchInst *BI = dyn_cast<BranchInst>(Inst);
            assert(BI && BI->isConditional() && "SCBranches must not contain instructions that aren't conditional branches.");
            fread(&BrInfo, sizeof(BrInfo), 1, fp_sc);
            // errs() <<"SC:"<<BrInfo.id << ":"<< BrInfo.count[0] <<":"<< BrInfo.count[1] <<":"<< BrInfo.count[2] << "\n";

            BrInfo.count[0] = BrInfo.count[1] + BrInfo.count[2];
            Cost = 0;
            for (Instruction *CI: SCI->getInstructionsBySanityCheck(BI)) {
                unsigned CurrentCost = CheckCost::getInstructionCost(CI, &TTI);
                if (CurrentCost == (unsigned)(-1)) {
                    CurrentCost = 1;
                }
                Cost += CurrentCost * BrInfo.count[0];
            }
            Total_Cost += Cost;
            Total_Cost_Opt += Cost;
            tmp++;
            // Set a flag to record whether the SC can be reduced
            is_reduced = false;
            // For each instruction in SCBranch, check whether its dynamic pattern matches certain patterns in SC_Pattern_opt
            if (SC_Pattern.count(BrInfo.count[0]) > 0 && BrInfo.count[0] > 0) {
                // New SC and existing SC have ompletely the same dynamic pattern A+B:A:B
                // Check all SCs in SC_Pattern_opt
                for (stat Info: SC_Pattern[BrInfo.count[0]]) {
                    if (BrInfo.count[1] == Info.LB || BrInfo.count[2] == Info.LB) {
                        // Also the same operation variable 
                        // errs() << tmp << ":"<<Info.id << "---";

                        if (BrInfo.id != Info.id && findSameDT(Inst, Info.SC, BrInfo.id, Info.id, 1)) {
                            is_reduced = true;
                            if (reducedSC.count(BrInfo.id) == 0) {
                                optimizeCheckAway(Inst);
                                errs() << "Reduced::SC:" << BrInfo.id<<"SC:"<<Info.id <<":"<< BrInfo.count[0]<< "--------\n";
                                flagSC_opts -= 1;
                                costflagSC_opts -= BrInfo.count[0];
                                reducedSC[BrInfo.id] = BrInfo.count[0];
                            }
                        }
                    }
                }
            }
            // Calculate reduced asap cost
            if (reducedSC.count(BrInfo.id) == 1) {
                Total_Cost_Opt -= Cost;
                test3 += 1;
            }
            // If the SC cannot be reduced, it should be inserted into SC_Pattern_opt, becoming one of the existing SCs.
            if (!is_reduced) {
                // Insert this Instruction into SC_Pattern_opt
                // Including its id, LB, RB, and *Inst
                struct stat SC_Info;
                SC_Info.id = BrInfo.id;
                SC_Info.LB = BrInfo.count[1];
                SC_Info.RB = BrInfo.count[2];
                SC_Info.SC = Inst;
                // SC_Pattern_opt[BrInfo.count[0]].push_back(SC_Info);
                if (reducedSC.count(BrInfo.id) == 0) {
                    test1 += 1;
                    test2 += BrInfo.count[0];
                }
            }
        }
    }
    fclose(fp_sc);
    errs() << "UC num :: " << flagUC << ";SC Num :: " << flagSC << ";SC percent after L1 :: " << flagSC_opt * 1.0 / (flagSC + 0.000000001) * 100 << "\%;SC percent after L2 :: " << flagSC_opts * 1.0 / (flagSC + 0.000000001) * 100 << "\%\n";
    errs() << "SC cost percent:: "<< costflagSC / (costflagSC + 0.000000001) * 100  << ";SC cost percent after L1 :: " << costflagSC_opt * 1.0 / (costflagSC + 0.000000001) * 100 << "\%;SC cost percent after L2 :: " << costflagSC_opts * 1.0 / (costflagSC + 0.000000001) * 100 << "\%\n";
    errs() <<"com:" << test1<<":"<<test2<<":"<<flagSC_opts<<":"<<costflagSC_opts<<"\n";
    errs() << "Total asap cost:" << Total_Cost << "Reduced total asap cost" << Total_Cost_Opt << "\n";
    FILE *fpp = fopen(Twine(InputLOG).str().c_str(), "ab");
    fprintf(fpp, "%s %lu %lu %lu %lu %lu %lu %lu %lu\n", filename.c_str(), flagSC, flagSC_opt,flagSC_opts,costflagSC,costflagSC_opt,costflagSC_opts, Total_Cost, Total_Cost_Opt);
    fclose(fpp);
    }

    struct brinfo{
        uint64_t id;
        uint64_t count[3];
    }BrInfo_new;

    struct info_new{
        double CostLevel1;
        double CostLevel2;
        double NumLevel1;
        double NumLevel2;
        uint64_t id;
        uint64_t count[3];
    };

    struct stat_new{
        uint64_t id;
        uint64_t LB;
        uint64_t RB;
        Instruction* SC;
    };

    struct coststat_new{
        double CostLevel1;
        double CostLevel2;
        double NumLevel1;
        double NumLevel2;
        Instruction* SC;
    };

    std::map<Instruction*, info_new> SC_Stat_new;
    std::map<uint64_t, std::vector<stat_new>> SC_Pattern_new, SC_Pattern_new_opt;
    std::map<uint64_t, std::vector<coststat_new>> CostLevelRange;
    std::vector<Instruction*> RSC;
    // StringRef SCType = CheckType;
    // StringRef SCLevel = SanType;

    fp_sc = fopen(Twine(InputSCOV).str().c_str(), "rb");
    // // errs() << Twine(InputSCOV).str().c_str() << "\n";
    // // assert(fp_sc != NULL && "No valid SCOV file");
    fp_uc = fopen(Twine(InputUCOV).str().c_str(), "rb");
    // FILE *fp_check = fopen(Twine(LogPath).str().c_str(), "ab");
    // assert(fp_uc != NULL && "No valid UCOV file");
    if (fp_sc != NULL && fp_uc != NULL) {
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

                fread(&BrInfo_new, sizeof(BrInfo_new), 1, fp_sc);
                // Revise the coverage pattern of SC
                // errs() <<"SC:"<<BrInfo_new.id << ":"<< BrInfo_new.count[0] <<":"<< BrInfo_new.count[1] <<":"<< BrInfo_new.count[2] << "\n";
                BrInfo_new.count[0] = BrInfo_new.count[1] + BrInfo_new.count[2];
                // if (BrInfo_new.id >= 53 && BrInfo_new.id <= 69) {
                //     errs() << BrInfo_new.id << "::" << BrInfo_new.count[0] << "---";
                //     reducedSC[63] = BrInfo_new.count[0];
                // }
                // if (BrInfo_new.count[0] > 0) {
                //     optimizeCheckAway(Inst);
                // }
                // Store the coverage data by UC_Stat
                for (size_t i=0;i<3;i++){
                    SC_Stat_new[Inst].count[i] = BrInfo_new.count[i];
                }
                SC_Stat_new[Inst].id = BrInfo_new.id;
                SC_Stat_new[Inst].CostLevel1 = 0;
                SC_Stat_new[Inst].CostLevel2 = 0;
                SC_Stat_new[Inst].NumLevel1 = 0;
                SC_Stat_new[Inst].NumLevel2 = 0;


                // Store the dynamic patterns of instructions in SCBranches in SC_Pattern_new (totalCovTime->SC_Info)
                // SC_Info contains id, LB, RB, and *Inst
                struct stat_new SC_Info;
                SC_Info.id = BrInfo_new.id;
                SC_Info.LB = BrInfo_new.count[1];
                SC_Info.RB = BrInfo_new.count[2];
                SC_Info.SC = Inst;
                SC_Pattern_new[BrInfo_new.count[0]].push_back(SC_Info);
                flagSC += 1;
                costflagSC += BrInfo_new.count[0];
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
                    Cost += CurrentCost * BrInfo_new.count[0];
                    total_cost += CurrentCost * BrInfo_new.count[0];
                }
                // CheckCostVec.push_back(std::make_pair(BI, Cost));
                struct coststat_new SCCostInfo;
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
        double b1=0, b2=0, b3=0, b4=0;
        if (total_cost == 0) {
            total_cost = 1;
        }
        if (total_num == 0) {
            total_num = 1;
        }
        errs() << "Total cost:" << total_cost << " ,total num:" << total_num << "\n";
        double budget_remove = 0, budget_count = 0, flag = 0;
        for (std::map<uint64_t, uint64_t>::iterator i=reducedSC.begin(); i!=reducedSC.end(); ++i){
            errs() << i->first << "::" << i->second;
        }
        for (std::map<uint64_t, std::vector<coststat_new>>::iterator I=CostLevelRange.begin(); I!=CostLevelRange.end(); ++I) {
            // errs() << I->first << "::" << I->second.size() << "---\n";

            for (uint64_t i=0; i<I->second.size(); i++) {

                CostLevel2 += I->first; //  * I->second.size();
                NumLevel2 += 1; // I->second.size();
                I->second[i].CostLevel1 = CostLevel1 / total_cost;
                I->second[i].CostLevel2 = CostLevel2 / total_cost;
                I->second[i].NumLevel1 = NumLevel1 / total_num;
                I->second[i].NumLevel2 = NumLevel2 / total_num;
                SC_Stat_new[I->second[i].SC].CostLevel1 = CostLevel1 / total_cost;
                SC_Stat_new[I->second[i].SC].CostLevel2 = CostLevel2 / total_cost;
                SC_Stat_new[I->second[i].SC].NumLevel1 = NumLevel1 / total_num;
                SC_Stat_new[I->second[i].SC].NumLevel2 = NumLevel2 / total_num;
                CostLevel1 = CostLevel2;
                NumLevel1 = NumLevel2;
                // if (SC_Stat_new[I->second[i].SC].id == 4 || SC_Stat_new[I->second[i].SC].id == 5 || SC_Stat_new[I->second[i].SC].id == 178 ||
                //  SC_Stat_new[I->second[i].SC].id == 160  || SC_Stat_new[I->second[i].SC].id == 164 || SC_Stat_new[I->second[i].SC].id == 156 ||
                //  SC_Stat_new[I->second[i].SC].id == 207 || SC_Stat_new[I->second[i].SC].id == 211 || SC_Stat_new[I->second[i].SC].id == 213 ||
                //  SC_Stat_new[I->second[i].SC].id == 4){
                //      errs() << "Id:" << SC_Stat_new[I->second[i].SC].id << " ," << SC_Stat_new[I->second[i].SC].CostLevel1 << ":" << SC_Stat_new[I->second[i].SC].CostLevel2 << "\n";
                //  }

                if (budget_count / total_cost < 0.05 & (budget_count+I->first) / total_cost > 0.05){
                    errs() << "Budget:" << 0.05 << " ,budget removed:" << budget_remove / total_cost << " ,budget new:" << (budget_count +budget_remove) / total_cost << "\n";
                    b1 = budget_remove / total_cost;
                }
                else if (budget_count / total_cost < 0.1 & (budget_count+I->first) / total_cost > 0.1){
                    errs() << "Budget:" << 0.10 << " ,budget removed:" << budget_remove / total_cost << " ,budget new:" << (budget_count +budget_remove) / total_cost << "\n";
                    b2 = budget_remove / total_cost;
                }
                else if (budget_count / total_cost < 0.15 & (budget_count+I->first) / total_cost > 0.15){
                    errs() << "Budget:" << 0.15 << " ,budget removed:" << budget_remove / total_cost << " ,budget new:" << (budget_count +budget_remove) / total_cost << "\n";
                    b3 = budget_remove / total_cost;
                }
                else if (budget_count / total_cost < 0.3 & (budget_count+I->first) / total_cost > 0.3){
                    errs() << "Budget:" << 0.30 << " ,budget removed:" << budget_remove / total_cost << " ,budget new:" << (budget_count +budget_remove) / total_cost << "\n";
                    b4 = budget_remove / total_cost;
                }
                else{
                    // errs() << "Budget:" << 0.15 << " ,budget removed:" << budget_remove / total_cost << " ,budget new:" << (budget_count +budget_remove) / total_cost << "\n";
                    flag = 0;
                }
                
                budget_count += I->first;
                
                if (reducedSC.count(SC_Stat_new[I->second[i].SC].id) == 1){
                    budget_remove += I->first;
                    errs() << SC_Stat_new[I->second[i].SC].id;
                }
                if (budget_count / total_cost > Threshold){
                    if (reducedSC.count(SC_Stat_new[I->second[i].SC].id) == 0) {
                        optimizeCheckAway(I->second[i].SC);
                    }
                }

            }
            // if (budget_count / total_cost > 0.15){
            //     break;
            // }

        }
        errs() << b1 << b2 << b3 << b4;
        FILE *fpp = fopen(Twine(InputLOG).str().c_str(), "ab");
        fprintf(fpp, "%s %f %f %f %f\n", filename.c_str(), b1,b2,b3,b4);
        fclose(fpp);
    }
    return true;
}

bool DynPassL0::findSameDT(Instruction *B1, Instruction *B2, uint64_t id1, uint64_t id2, uint64_t f) {
    bool flag = false;
    Instruction* C1;
    Instruction* C2;
    StringRef name1 = B1->getOpcodeName();
    StringRef name2 = B2->getOpcodeName();
    if (name1 == "br" && name2 == "br") {
        if (Instruction *Op1 = dyn_cast<Instruction>(B1->getOperand(0))){
            if (Instruction *Op2 = dyn_cast<Instruction>(B2->getOperand(0))){
                C1 = Op1;
                C2 = Op2;
                name1 = C1->getOpcodeName();
                name2 = C2->getOpcodeName();
            }
            else {
                return false;
            }
        }
        else{
            return false;
        }
    }
    else {
        C1 = B1;
        C2 = B2;
        name1 = C1->getOpcodeName();
        name2 = C2->getOpcodeName();
    }
    if (C1 == C2) {
        return true;
    }
    else if (name1 == name2 && name1 !="phi" && C1->getNumOperands() == C2->getNumOperands() && C1->getNumOperands() != 0) {
        flag = true;
        for (size_t i=0; i<C1->getNumOperands();i++) {
            if (C1->getOperand(i) != C2->getOperand(i)) {
                flag = false;
                if (Instruction *Op1 = dyn_cast<Instruction>(C1->getOperand(i))) {
                    if (Instruction *Op2 = dyn_cast<Instruction>(C2->getOperand(i))) {
                        flag = findSameDT(Op1, Op2, id1, id2, f);
                    }
                }
                if (!flag) {
                    break;
                }
            }
        }
        return flag;
    }
    return flag;
}

std::set<Value*> DynPassL0::TrackValueLoc(Instruction *C, uint64_t id) {
    std::set<Instruction*> Clist;
    std::set<Value*> FClist;
    Instruction *Inst;
    if (BranchInst *BI = dyn_cast<BranchInst>(C)) {
        if (Instruction *Op=dyn_cast<Instruction>(C->getOperand(0))) {
            Clist.insert(Op);
        }
    }
    else {
        bool is_end = true;
        for (Use &U: C->operands()) {
            if (Instruction *Op = dyn_cast<Instruction>(U.get())) {
                is_end = false;
            }
        }
        StringRef OpName = C->getOpcodeName();
        if (!is_end) {
            Clist.insert(C);
        }
        // Clist.insert(C);
    }

    StringRef OpName = C->getOpcodeName();
    while (Clist.size()!=0) {
        Inst = *Clist.begin(); 
        Clist.erase(Inst);           
        OpName = Inst->getOpcodeName();
        bool is_end = true;
        for (Use &U: Inst->operands()) {
            if (Instruction *Op = dyn_cast<Instruction>(U.get())) {
                is_end = false;
            }
        }
        // if (is_end && OpName != "load" && !OpName.startswith("getelementptr")) {
        //     errs() <<"id:" << id << "Opname:" << OpName << "\n";
        // }
        if (OpName!="load" && !OpName.startswith("getelementptr") && OpName!="phi" && !is_end) {
            for (Use &U: Inst->operands()) {
                if (Instruction *I = dyn_cast<Instruction>(U.get())) {
                    Clist.insert(I);
                }
                else if (Constant *C = dyn_cast<Constant>(U.get())) {
                    // Do nothing
                    // FClist.insert(U.get());
                }
                else {
                    FClist.insert(U.get());
                }
            }
        }
        else {
            FClist.insert(Inst);
        }
    }
    return FClist;
}


// Tries to remove a sanity check; returns true if it worked.
void DynPassL0::optimizeCheckAway(Instruction *Inst) {
    BranchInst *BI = cast<BranchInst>(Inst);
    assert(BI->isConditional() && "Sanity check must be conditional branch.");
    
    unsigned int RegularBranch = getRegularBranch(BI, SCI);
    
    bool Changed = false;
    if (RegularBranch == 0) {
        BI->setCondition(ConstantInt::getTrue(Inst->getContext()));
        Changed = true;
    } else if (RegularBranch == 1) {
        BI->setCondition(ConstantInt::getFalse(Inst->getContext()));
        Changed = true;
    } else {
        dbgs() << "Warning: Sanity check with no regular branch found.\n";
        dbgs() << "The sanity check has been kept intact.\n";
    }
    // return Changed;
}



void DynPassL0::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.addRequired<SCIPass>();
    AU.setPreservesAll();
}


char DynPassL0::ID = 0;
static RegisterPass<DynPassL0> X("DynPassL0",
        "Finds costs of sanity checks", false, false);



