// This file is part of ASAP.
// Please see LICENSE.txt for copyright and licensing information.

namespace llvm {
  class Instruction;
  class TargetTransformInfo;
}

namespace CheckCost {
  /// Returns the expected cost of the instruction.
  /// Returns -1 if the cost is unknown.
  /// Note, this method does not cache the cost calculation and it
  /// can be expensive in some cases.
  unsigned getInstructionCost(const llvm::Instruction *I,
      const llvm::TargetTransformInfo *TTI);
}  // namespace CheckCost
