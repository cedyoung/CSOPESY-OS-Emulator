#include "instruction.h"
#include <algorithm>
#include <utility>

namespace{
constexpr std::int64_t UINT16_MAX_VALUE = 65535;

std::uint16_t clampToUint16(std::int64_t value){
    if (value< 0){
        return 0;
    }
  
    if (value >UINT16_MAX_VALUE){
        return static_cast<std::uint16_t>(UINT16_MAX_VALUE);
    }
  
    return static_cast<std::uint16_t>(value);
}


int randInt(std::mt19937& rng, int lo, int hi){
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

const char* const VARIABLE_POOL[] = {"a", "b", "c", "x", "y", "z"};
constexpr int VARIABLE_POOL_SIZE = 6;


std::string randomVariable(std::mt19937& rng){
    return VARIABLE_POOL[randInt(rng, 0, VARIABLE_POOL_SIZE - 1)];
}


Operand randomOperand(std::mt19937& rng) {
    if (randInt(rng, 0, 1) == 0){
        return Operand::variable(randomVariable(rng));
    }
  
    return Operand::literal(static_cast<std::uint16_t>(randInt(rng, 0, 100)));
  
}



InstructionPtr randomLeaf(const std::string& processName, std::mt19937& rng) {
    int roll = randInt(rng, 0, 99);
    if (roll< 30){
        return Instruction::makePrint("Hello world from " + processName + "!");
    }
  
    if (roll< 45) {
        return Instruction::makeDeclare(
            randomVariable(rng),
            static_cast<std::uint16_t>(randInt(rng, 0, 100)));  
    }
  
    if (roll <65){
        return Instruction::makeAdd(
            randomVariable(rng),
            Operand::variable(randomVariable(rng)),
            randomOperand(rng));
    }
  
    if (roll < 85){
        return Instruction::makeSubtract(
            randomVariable(rng),
            Operand::variable(randomVariable(rng)),
            randomOperand(rng));
    }
  
    return Instruction::makeSleep(static_cast<std::uint8_t>(randInt(rng, 1, 5)));

}

InstructionList buildBlock(const std::string& processName, std::uint64_t budget, int depth, std::mt19937& rng) {
    InstructionList block;

    while (budget > 0){
        bool canNest = depth < MAX_NESTING && budget >= 2;

        if (canNest && randInt(rng, 0, 99) < 20) {
            int maxRepeats = static_cast<int>(std::min<std::uint64_t>(budget, 10));
            int repeats = randInt(rng, 2, maxRepeats);
            int maxBody = static_cast<int>(std::min<std::uint64_t>(budget / static_cast<std::uint64_t>(repeats), 8));
            int bodySize = randInt(rng, 1, maxBody);
            block.push_back(Instruction::makeFor(buildBlock(processName, static_cast<std::uint64_t>(bodySize), depth + 1, rng), static_cast<std::uint32_t>(repeats)));

            budget -= static_cast<std::uint64_t>(bodySize) * static_cast<std::uint64_t>(repeats);
        }
        else {
            block.push_back(randomLeaf(processName, rng));
            budget--;
        }
    }

    return block;
  
}

std::string operandToString(const Operand& op){
    return op.isVar ? op.name : std::to_string(op.value);
}

}

Operand Operand::variable(const std::string& varName){
    Operand op;
    op.isVar = true;
    op.name = varName;
    return op;
  
}

Operand Operand::literal(std::uint16_t literalValue){
    Operand op;
    op.isVar = false;
    op.value = literalValue;
    return op;
  
}

InstructionPtr Instruction::makePrint(const std::string& message, const std::string& variable) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::PRINT;
    ins->message = message;
    ins->printVariable = variable;
    return ins;
  
}

InstructionPtr Instruction::makeDeclare(const std::string& var, std::uint16_t value) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::DECLARE;
    ins->var = var;
    ins->declareValue = value;
    return ins;
  
}

InstructionPtr Instruction::makeAdd(const std::string& dest, const Operand& lhs, const Operand& rhs) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::ADD;
    ins->var = dest;
    ins->lhs = lhs;
    ins->rhs = rhs;
    return ins;
  
}

InstructionPtr Instruction::makeSubtract(const std::string& dest, const Operand& lhs, const Operand& rhs) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::SUBTRACT;
    ins->var = dest;
    ins->lhs = lhs;
    ins->rhs = rhs;
    return ins;
  
}

InstructionPtr Instruction::makeSleep(std::uint8_t ticks) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::SLEEP;
    ins->sleepTicks = ticks;
    return ins;
}

InstructionPtr Instruction::makeFor(InstructionList body, std::uint32_t repeats) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::FOR;
    ins->body = std::make_shared<const InstructionList>(std::move(body));
    ins->repeats = repeats;
    return ins;
}

Execution::Execution(const std::string& processName, ProgramPtr program)
    : processName_(processName) {
    if (program) {
        stack_.push_back(Frame{ program, 0, 1 });
    }
    settle();
      
}

const Instruction* Execution::settle() {
    while (!stack_.empty()) {
        Frame& frame = stack_.back();

        if (frame.index >= frame.list->size()) {
            if (frame.remaining > 1) {
                --frame.remaining;
                frame.index = 0;
            }
            else {
                stack_.pop_back();
            }
            continue;
        }

        const InstructionPtr& ins = (*frame.list)[frame.index];

        if (ins->type == InstructionCommand::FOR){
            ++frame.index;
            if (ins->repeats > 0 && ins->body && !ins->body->empty()) {
                stack_.push_back(Frame{ ins->body, 0, ins->repeats });
              
            }
            continue;
        }

        return ins.get();
    }

    done_ = true;
    return nullptr;
}

StepResult Execution::step() {
    StepResult result;

    if (sleepRemaining_ > 0) {
        return result;
    }

  
    const Instruction* ins = settle();
    if (ins == nullptr) {
        result.done = true;
        return result;
    }

  
    result.executed = true;
    result.type = ins->type;
    execute(*ins, result);
    stack_.back().index++;
    ++executed_;
    settle();
    result.done = isDone();

    return result;
}

void Execution::tickSleep() {
    if (sleepRemaining_ > 0){
        --sleepRemaining_;
    }
}

bool Execution::hasVar(const std::string& name) const{
    return variables_.find(name) != variables_.end();
}

std::uint16_t Execution::getVar(const std::string& name) const{
    auto it = variables_.find(name);
    return it == variables_.end() ? 0 : it->second;
}

std::uint16_t Execution::resolve(const Operand& op){
    if (!op.isVar) {
        return op.value;
    }

    auto it = variables_.find(op.name);
    if (it == variables_.end()) {
        it = variables_.emplace(op.name, 0).first;
    }
    return it->second;
}

void Execution::execute(const Instruction& ins, StepResult& result) {
    switch (ins.type) {

    case InstructionCommand::PRINT: {
        std::string out = ins.message;

        if (!ins.printVariable.empty()) {
            Operand var = Operand::variable(ins.printVariable);
            out += std::to_string(resolve(var));
        }

        result.hasOutput = true;
        result.output = out;
        break;
    }

    case InstructionCommand::DECLARE:
        variables_[ins.var] = ins.declareValue;
        break;

    case InstructionCommand::ADD: {
        std::int64_t l = resolve(ins.lhs);
        std::int64_t r = resolve(ins.rhs);
        variables_[ins.var] = clampToUint16(l + r);
        break;
    }

    case InstructionCommand::SUBTRACT: {
        std::int64_t l = resolve(ins.lhs);
        std::int64_t r = resolve(ins.rhs);
        variables_[ins.var] = clampToUint16(l - r);
        break;
    }

    case InstructionCommand::SLEEP:
        sleepRemaining_ = ins.sleepTicks;
        result.sleepStarted = sleepRemaining_ > 0;
        break;

    case InstructionCommand::FOR:
        break;
    }
}

std::uint64_t countInstructions(const InstructionList& program){
    std::uint64_t total = 0;

    for (const InstructionPtr& ins : program) {
        if (ins->type == InstructionCommand::FOR) {
            if (ins->body) {
                total += static_cast<std::uint64_t>(ins->repeats) *
                         countInstructions(*ins->body);
            }
        }
          
        else {
            total += 1;
          
        }
    }

    return total;
}

int maxForDepth(const InstructionList& program){
    int deepest = 0;

    for (const InstructionPtr& ins : program) {
        if (ins->type == InstructionCommand::FOR && ins->body){
            deepest = std::max(deepest, 1 + maxForDepth(*ins->body));
        }
    }

    return deepest;
}

ProgramPtr generateProgram(const std::string& processName, std::uint64_t targetInstructions, std::mt19937& rng) {
    return std::make_shared<const InstructionList>(buildBlock(processName, targetInstructions, 0, rng));
}

std::string describe(const Instruction& ins, int indent) {
    std::string out;
    std::string pad(static_cast<std::size_t>(indent) * 2, ' ');

    out += pad;

    switch (ins.type) {
    case InstructionCommand::PRINT:
        out += "PRINT(\"" + ins.message + "\"";
        if (!ins.printVariable.empty()) {
            out += " + " + ins.printVariable;
        }
      
        out += ")\n";
        break;

    case InstructionCommand::DECLARE:
        out += "DECLARE(" + ins.var + ", " + std::to_string(ins.declareValue) + ")\n";
        break;

    case InstructionCommand::ADD:
        out += "ADD(" + ins.var + ", " + operandToString(ins.lhs)
             + ", " + operandToString(ins.rhs) + ")\n";
        break;

    case InstructionCommand::SUBTRACT:
        out += "SUBTRACT(" + ins.var + ", " + operandToString(ins.lhs)
             + ", " + operandToString(ins.rhs) + ")\n";
        break;

    case InstructionCommand::SLEEP:
        out += "SLEEP(" + std::to_string(static_cast<int>(ins.sleepTicks)) + ")\n";
        break;

    case InstructionCommand::FOR:
        out += "FOR(repeats=" + std::to_string(ins.repeats) + ") {\n";
        if (ins.body) {
            for (const InstructionPtr& child : *ins.body){
                out += describe(*child, indent + 1);
            }
          
        }
      
        out += pad + "}\n";
        break;
    }

    return out;
}


