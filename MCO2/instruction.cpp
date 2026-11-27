#include "instruction.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>


// anonymous helpers
namespace {

constexpr std::int64_t UINT16_MAX_VALUE = 65535;

std::uint16_t clampToUint16(std::int64_t value) {
    if (value < 0) return 0;
    if (value > UINT16_MAX_VALUE) return static_cast<std::uint16_t>(UINT16_MAX_VALUE);
    return static_cast<std::uint16_t>(value);
}

int randInt(std::mt19937& rng, int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

const char* const VARIABLE_POOL[] = { "a", "b", "c", "x", "y", "z" };
constexpr int VARIABLE_POOL_SIZE = 6;

std::string randomVariable(std::mt19937& rng) {
    return VARIABLE_POOL[randInt(rng, 0, VARIABLE_POOL_SIZE - 1)];
}

Operand randomOperand(std::mt19937& rng) {
    if (randInt(rng, 0, 1) == 0)
        return Operand::variable(randomVariable(rng));
    return Operand::literal(static_cast<std::uint16_t>(randInt(rng, 0, 100)));
}

// One random leaf instruction. Memory-aware so READ/WRITE stay in bounds.
InstructionPtr randomLeaf(const std::string& processName,
                          std::size_t memoryBytes,
                          std::mt19937& rng) {
    const int roll = randInt(rng, 0, 99);

    if (roll < 25) return Instruction::makePrint("Hello world from " + processName + "!");

    if (roll < 35) return Instruction::makeDeclare(
        randomVariable(rng),
        static_cast<std::uint16_t>(randInt(rng, 0, 100)));

    if (roll < 45) return Instruction::makeAdd(
        randomVariable(rng),
        Operand::variable(randomVariable(rng)),
        randomOperand(rng));

    if (roll < 55) return Instruction::makeSubtract(
        randomVariable(rng),
        Operand::variable(randomVariable(rng)),
        randomOperand(rng));

    if (roll < 65) return Instruction::makeSleep(
        static_cast<std::uint8_t>(randInt(rng, 1, 5)));

    // ---- 35% memory access, guarded by memoryBytes ----
    if (memoryBytes < 2) return Instruction::makeSleep(1);

    // Even addresses only, in [0, memoryBytes - 2].
    const std::uint64_t maxEven = (memoryBytes - 2) & ~std::uint64_t{1};
    const int lo = 0;
    const int hi = static_cast<int>(std::min<std::uint64_t>(maxEven, 32766));
    const auto addr = static_cast<std::uint16_t>(randInt(rng, lo, hi));

    if (roll < 82) return Instruction::makeRead(randomVariable(rng), addr);
    return Instruction::makeWrite(addr, randomOperand(rng));
}

InstructionList buildBlock(const std::string& processName,
                           std::uint64_t budget,
                           int depth,
                           std::mt19937& rng,
                           std::size_t memoryBytes) {
    InstructionList block;

    while (budget > 0) {
        const bool canNest = depth < MAX_NESTING && budget >= 2;

        if (canNest && randInt(rng, 0, 99) < 20) {
            const int maxRepeats = static_cast<int>(
                std::min<std::uint64_t>(budget, 10));
            const int repeats = randInt(rng, 2, maxRepeats);
            const int maxBody = static_cast<int>(
                std::min<std::uint64_t>(budget / static_cast<std::uint64_t>(repeats), 8));
            const int bodySize = randInt(rng, 1, maxBody);

            block.push_back(Instruction::makeFor(
                buildBlock(processName,
                           static_cast<std::uint64_t>(bodySize),
                           depth + 1, rng, memoryBytes),
                static_cast<std::uint32_t>(repeats)));

            budget -= static_cast<std::uint64_t>(bodySize)
                    * static_cast<std::uint64_t>(repeats);
        } else {
            block.push_back(randomLeaf(processName, memoryBytes, rng));
            --budget;
        }
    }

    return block;
}

std::string operandToString(const Operand& op) {
    return op.isVar ? op.name : std::to_string(op.value);
}

std::string hexAddr(std::uint16_t addr) {
    std::ostringstream o;
    o << "0x" << std::hex << std::uppercase << addr;
    return o.str();
}


// parsing helpers (for parseUserInstructions)
std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    const auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::uint64_t parseHexOrDec(const std::string& s) {
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return std::stoull(s.substr(2), nullptr, 16);
    return std::stoull(s, nullptr, 10);
}

Operand operandFromToken(const std::string& t) {
    if (!t.empty() && std::isdigit(static_cast<unsigned char>(t[0])))
        return Operand::literal(static_cast<std::uint16_t>(
            std::min<std::uint64_t>(std::stoull(t), 65535)));
    return Operand::variable(t);
}

InstructionPtr parseOne(const std::string& raw) {
    const std::string s = trim(raw);
    if (s.empty()) throw std::invalid_argument("empty");

    const auto  sp   = s.find_first_of(" \t(");
    const std::string op   = (sp == std::string::npos) ? s : s.substr(0, sp);
    const std::string rest = (sp == std::string::npos) ? "" : trim(s.substr(sp + 1));

    if (op == "DECLARE") {
        std::istringstream in(rest);
        std::string name;
        std::uint32_t val = 0;
        if (!(in >> name >> val)) throw std::invalid_argument("DECLARE");
        return Instruction::makeDeclare(
            name, static_cast<std::uint16_t>(std::min<std::uint32_t>(val, 65535)));
    }

    if (op == "ADD" || op == "SUBTRACT") {
        std::istringstream in(rest);
        std::string d, l, r;
        if (!(in >> d >> l >> r)) throw std::invalid_argument(op);
        return (op == "ADD")
            ? Instruction::makeAdd     (d, operandFromToken(l), operandFromToken(r))
            : Instruction::makeSubtract(d, operandFromToken(l), operandFromToken(r));
    }

    if (op == "SLEEP") {
        if (rest.empty()) throw std::invalid_argument("SLEEP");
        return Instruction::makeSleep(
            static_cast<std::uint8_t>(std::stoul(rest)));
    }

    if (op == "READ") {
        std::istringstream in(rest);
        std::string var, addr;
        if (!(in >> var >> addr)) throw std::invalid_argument("READ");
        return Instruction::makeRead(var,
            static_cast<std::uint16_t>(parseHexOrDec(addr)));
    }

    if (op == "WRITE") {
        std::istringstream in(rest);
        std::string addr, val;
        if (!(in >> addr >> val)) throw std::invalid_argument("WRITE");
        return Instruction::makeWrite(
            static_cast<std::uint16_t>(parseHexOrDec(addr)),
            operandFromToken(val));
    }

    if (op.rfind("PRINT", 0) == 0) {
        const auto lp = s.find('(');
        if (lp == std::string::npos || s.back() != ')')
            throw std::invalid_argument("PRINT");
        std::string payload = trim(s.substr(lp + 1, s.size() - lp - 2));

        const auto q1 = payload.find('"');
        const auto q2 = payload.rfind('"');
        if (q1 == std::string::npos || q2 == q1)
            throw std::invalid_argument("PRINT");

        std::string message  = payload.substr(q1 + 1, q2 - q1 - 1);
        std::string variable;
        const std::string after = trim(payload.substr(q2 + 1));
        if (!after.empty()) {
            if (after[0] != '+') throw std::invalid_argument("PRINT");
            variable = trim(after.substr(1));
        }
        return Instruction::makePrint(message, variable);
    }

    throw std::invalid_argument("unknown opcode: " + op);
}

} // namespace


Operand Operand::variable(const std::string& varName) {
    Operand op;
    op.isVar = true;
    op.name  = varName;
    return op;
}

Operand Operand::literal(std::uint16_t literalValue) {
    Operand op;
    op.isVar = false;
    op.value = literalValue;
    return op;
}


InstructionPtr Instruction::makePrint(const std::string& message,
                                        const std::string& variable) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::PRINT;
    ins->message = message;
    ins->printVariable = variable;
    return ins;
}

InstructionPtr Instruction::makeDeclare(const std::string& var, std::uint16_t value) {
    auto ins = std::make_shared<Instruction>();
    ins->type         = InstructionCommand::DECLARE;
    ins->var          = var;
    ins->declareValue = value;
    return ins;
}

InstructionPtr Instruction::makeAdd(const std::string& dest,
                                    const Operand& lhs, const Operand& rhs) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::ADD;
    ins->var = dest;
    ins->lhs = lhs;
    ins->rhs = rhs;
    return ins;
}

InstructionPtr Instruction::makeSubtract(const std::string& dest,
                                        const Operand& lhs, const Operand& rhs) {
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

InstructionPtr Instruction::makeRead(const std::string& var, std::uint16_t addr) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::READ;
    ins->readTarget = var;
    ins->memoryAddress = addr;
    return ins;
}

InstructionPtr Instruction::makeWrite(std::uint16_t addr, const Operand& value) {
    auto ins = std::make_shared<Instruction>();
    ins->type = InstructionCommand::WRITE;
    ins->memoryAddress = addr;
    ins->lhs = value;
    return ins;
}


Execution::Execution(const std::string& processName,
                     ProgramPtr program,
                     int pid,
                     std::size_t memoryBytes,
                     DemandPagingAllocator& allocator)
    : processName_(processName),
      memory_(allocator, pid, memoryBytes) {
    if (program) stack_.push_back(Frame{ program, 0, 1 });
    settle();
}

const Instruction* Execution::settle() {
    while (!stack_.empty()) {
        Frame& frame = stack_.back();

        if (frame.index >= frame.list->size()) {
            if (frame.remaining > 1) {
                --frame.remaining;
                frame.index = 0;
            } else {
                stack_.pop_back();
            }
            continue;
        }

        const InstructionPtr& ins = (*frame.list)[frame.index];

        if (ins->type == InstructionCommand::FOR) {
            ++frame.index;
            if (ins->repeats > 0 && ins->body && !ins->body->empty())
                stack_.push_back(Frame{ ins->body, 0, ins->repeats });
            continue;
        }

        return ins.get();
    }

    done_ = true;
    return nullptr;
}

StepResult Execution::step() {
    StepResult result;
    if (sleepRemaining_ > 0) return result;

    const Instruction* ins = settle();
    if (ins == nullptr) { result.done = true; return result; }

    result.executed = true;
    result.type = ins->type;
    execute(*ins, result); // may throw MemoryAccessViolation
    stack_.back().index++;
    ++executed_;
    settle();
    result.done = isDone();
    return result;
}

void Execution::tickSleep() {
    if (sleepRemaining_ > 0) --sleepRemaining_;
}

std::uint16_t Execution::resolveOperand(const Operand& op) {
    return op.isVar ? memory_.readVar(op.name) : op.value;
}

void Execution::execute(const Instruction& ins, StepResult& result) {
    switch (ins.type) {

    case InstructionCommand::PRINT: {
        std::string out = ins.message;
        if (!ins.printVariable.empty())
            out += std::to_string(memory_.readVar(ins.printVariable));
        result.hasOutput = true;
        result.output = out;
        break;
    }

    case InstructionCommand::DECLARE:
        memory_.declare(ins.var, ins.declareValue);
        break;

    case InstructionCommand::ADD: {
        const std::int64_t l = resolveOperand(ins.lhs);
        const std::int64_t r = resolveOperand(ins.rhs);
        memory_.writeVar(ins.var, clampToUint16(l + r));
        break;
    }

    case InstructionCommand::SUBTRACT: {
        const std::int64_t l = resolveOperand(ins.lhs);
        const std::int64_t r = resolveOperand(ins.rhs);
        memory_.writeVar(ins.var, clampToUint16(l - r));
        break;
    }

    case InstructionCommand::SLEEP:
        sleepRemaining_ = ins.sleepTicks;
        result.sleepStarted = sleepRemaining_ > 0;
        break;

    case InstructionCommand::READ: {
        const std::uint16_t v = memory_.readU16(ins.memoryAddress);
        memory_.writeVar(ins.readTarget, v);
        break;
    }

    case InstructionCommand::WRITE: {
        const std::uint16_t v = resolveOperand(ins.lhs);
        memory_.writeU16(ins.memoryAddress, v);
        break;
    }

    case InstructionCommand::FOR:
        break;
    }
}


std::uint64_t countInstructions(const InstructionList& program) {
    std::uint64_t total = 0;
    for (const InstructionPtr& ins : program) {
        if (ins->type == InstructionCommand::FOR) {
            if (ins->body)
                total += static_cast<std::uint64_t>(ins->repeats)
                       * countInstructions(*ins->body);
        } else {
            ++total;
        }
    }
    return total;
}

int maxForDepth(const InstructionList& program) {
    int deepest = 0;
    for (const InstructionPtr& ins : program) {
        if (ins->type == InstructionCommand::FOR && ins->body)
            deepest = std::max(deepest, 1 + maxForDepth(*ins->body));
    }
    return deepest;
}

ProgramPtr generateProgram(const std::string& processName,
                           std::uint64_t      targetInstructions,
                           std::mt19937&      rng,
                           std::size_t        memoryBytes) {
    return std::make_shared<const InstructionList>(
        buildBlock(processName, targetInstructions, 0, rng, memoryBytes));
}

std::string describe(const Instruction& ins, int indent) {
    std::string out;
    const std::string pad(static_cast<std::size_t>(indent) * 2, ' ');
    out += pad;

    switch (ins.type) {
    case InstructionCommand::PRINT:
        out += "PRINT(\"" + ins.message + "\"";
        if (!ins.printVariable.empty()) out += " + " + ins.printVariable;
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

    case InstructionCommand::READ:
        out += "READ(" + ins.readTarget + ", " + hexAddr(ins.memoryAddress) + ")\n";
        break;

    case InstructionCommand::WRITE:
        out += "WRITE(" + hexAddr(ins.memoryAddress) + ", "
             + operandToString(ins.lhs) + ")\n";
        break;

    case InstructionCommand::FOR:
        out += "FOR(repeats=" + std::to_string(ins.repeats) + ") {\n";
        if (ins.body)
            for (const InstructionPtr& child : *ins.body)
                out += describe(*child, indent + 1);
        out += pad + "}\n";
        break;
    }

    return out;
}

std::vector<InstructionPtr> parseUserInstructions(const std::string& source) {
    std::vector<InstructionPtr> out;
    std::stringstream ss(source);
    std::string chunk;

    try {
        while (std::getline(ss, chunk, ';')) {
            const std::string t = trim(chunk);
            if (!t.empty()) out.push_back(parseOne(t));
        }
    } catch (const std::invalid_argument&) {
        throw;
    } catch (const std::exception&) {
        throw std::invalid_argument("malformed instruction");
    }

    if (out.empty() || out.size() > 50)
        throw std::invalid_argument("instruction count must be 1..50");
    return out;
}