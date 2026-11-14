#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <random>

constexpr int MAX_NESTING = 3;

enum class InstructionCommand {
  PRINT,
  DECLARE,
  ADD,
  SUBTRACT,
  SLEEP,
  FOR
};

struct Operand {
  bool isVar = false;
  std::string name;
  std::uint16_t value = 0;
  static Operand variable(const std::string& varName);
  static Operand literal(std::uint16_t literalValue);
};

struct Instruction;


using InstructionPtr = std::shared_ptr<const Instruction>;
using InstructionList = std::vector<InstructionPtr>;
using ProgramPtr = std::shared_ptr<const InstructionList>;

struct Instruction {
  InstructionCommand type = InstructionCommand::PRINT;

std::string message;
std::string printVariable;


std::string var;
std::uint16_t declareValue= 0;
Operand lhs;
Operand rhs;


std::uint8_t sleepTicks = 0;


ProgramPtr body;
std::uint32_t repeats = 0;

static InstructionPtr makePrint(const std::string& message, const std::string& variable = "");

static InstructionPtr makeDeclare(const std::string& var, const std::uint16_t value);

static InstructionPtr makeAdd(const std::string& dest, const Operand& lhs, const Operand& rhs);

static InstructionPtr makeSubtract(const std::string& dest, const Operand& lhs, const Operand& rhs);

static InstructionPtr makeSleep(std::uint8_t ticks);

static InstructionPtr makeFor(InstructionList body, std::uint32_t repeats);

};

struct StepResult {
  bool executed = false;
  InstructionCommand type = InstructionCommand::PRINT;
  bool hasOutput = false;
  std::string output;
  bool sleepStarted = false;
  bool done = false;
};

class Execution {
  public:
    Execution() = default;
    Execution(const std::string& processName, ProgramPtr program);

    StepResult step();
    void tickSleep();
    
    bool isSleep() const {return sleepRemaining_ > 0;}
    bool isDone() const {return done_ && sleepRemaining_ == 0;}
    std::uint64_t instructionsExecuted() const {return executed_;}
    std::uint32_t sleepRemaining() const {return sleepRemaining_;}
    
    bool hasVar(const std::string& name) const;
    std::uint16_t getVar(const std::string& name) const;
    const std::unordered_map<std::string, std::uint16_t>& variables() const {return variables_;}

  private:
    struct Frame {
      ProgramPtr list;
      std::size_t index;
      std::uint32_t remaining;
    };

    const Instruction* settle();
    void execute(const Instruction& ins, StepResult& result);
    std::uint16_t resolve(const Operand& op);
    std::string processName_;
    std::vector<Frame> stack_;
    std::unordered_map<std::string, std::uint16_t> variables_;
    std::uint32_t sleepRemaining_=0;
    std::uint64_t executed_ = 0;
    bool done_ = false;
};

std::uint64_t countInstructions(const InstructionList& program);

int maxForDepth(const InstructionList& program);

ProgramPtr generateProgram(const std::string& processName, std::uint64_t targetInstructions, std::mt19937& rng);

std::string describe(const Instruction& instruction, int indent = 0);


#endif
