#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <cassert>
#include <iomanip>
#include <functional>

namespace MUtils
{

enum class VM_Reg
{
    R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8
};

enum class VM_IType
{
    Push,
    Pop,
    PopArgs,
    Ret,
    Call,
    Add,
    Mov
};

template <class TValue>
class VM
{
public:
    using DumpArgFn = std::function<void(std::ostringstream&, const TValue&)>;

    struct VInstruction
    {
        VM_IType type;
        TValue arg1;
        TValue arg2;
    };

    using JNativeFunction = std::function<TValue(int argCount)>;
    struct VFunction
    {
        std::string name;
        std::vector<TValue> args;
        std::vector<VInstruction> instructions;
        JNativeFunction pFnNative = nullptr;
    };

public:
    VM(std::ostringstream& log, DumpArgFn fn)
        : m_log(log),
        m_dumpArgFn(fn)
    {
    }

    ~VM()
    {
    }

    const VInstruction& Instruction() const
    {
        assert(!m_callStack.empty());
        return m_callStack.front()->instructions[m_pc];
    }

    void Dump()
    {
        std::ostringstream& code = m_log;
        code << "\nVM:\n";

        for (auto& fn : m_functions)
        {
            code << "\n"
                 << fn->name << ":\n";

            uint32_t index = 0;
            for (auto& i : fn->instructions)
            {
                code << index++ << ": ";
                switch (i.type)
                {
                case VM_IType::Call:
                {
                    code << "CALL ";
                    m_dumpArgFn(code, i.arg1);
                    auto pFn = GetFunction(std::get<std::string>(i.arg1));
                    if (pFn->pFnNative)
                    {
                        code << " (native)";
                    }
                }
                break;
                case VM_IType::Ret:
                    code << "RET\n";
                    break;
                case VM_IType::Push:
                    code << "PUSH ";
                    m_dumpArgFn(code, i.arg1);
                    break;
                case VM_IType::Pop:
                    code << "POP ";
                    m_dumpArgFn(code, i.arg1);
                    break;
                case VM_IType::PopArgs:
                    code << "POPARGS";
                    break;
                case VM_IType::Mov:
                    code << "MOV ";
                    m_dumpArgFn(code, i.arg1);
                    code << ", ";
                    m_dumpArgFn(code, i.arg2);
                    break;
                }

                code << "\n";
            }
            code << "\n";
        }
    }

    void Execute(const VInstruction& instruction)
    {
        switch (instruction.type)
        {
        case VM_IType::Call:
        {
            assert(std::holds_alternative<std::string>(instruction.arg1));
            auto fnName = std::get<std::string>(instruction.arg1);

            auto fun = GetFunction(fnName);

            // Pop into registers, backwards ;)
            int count = std::get<int>(m_stack.front());
            int keptCount = count;

            m_stack.pop_front();
            while (count > 0)
            {
                m_registers[count - 1] = m_stack.front();
                m_stack.pop_front();

                count--;
            }

            // If native, just call it with the VM State, and then we'll return to the next instruction
            if (fun->pFnNative)
            {
                m_registers[0] = fun->pFnNative(keptCount);
            }
            else
            {
                // Push the PC
                m_stack.push_front(m_pc + 1);
                m_pc = 0;
                m_callStack.push_front(fun);
            }
        }
        break;
        case VM_IType::Ret:
        {
            assert(!m_callStack.empty());
            m_pc = std::get<int>(m_stack.front());
            m_stack.pop_back();
            m_callStack.pop_front();
        }
        break;
        case VM_IType::Push:
        {
            if (std::holds_alternative<VM_Reg>(instruction.arg1))
            {
                m_stack.push_front(m_registers[(int)std::get<VM_Reg>(instruction.arg1)]);
            }
            else
            {
                m_stack.push_front(instruction.arg1);
            }
        }
        break;
        case VM_IType::Pop:
        {
            // Pop into a register - validate?
            assert(std::holds_alternative<VM_Reg>(instruction.arg1));
            m_registers[(int)std::get<VM_Reg>(instruction.arg1)] = m_stack.front();
            m_stack.pop_front();
        }
        break;
        case VM_IType::PopArgs:
        {
            // Pop into registers
            VM_Reg r = VM_Reg::R0;
            int count = std::get<int>(m_stack.front());
            int args = count;
            m_stack.pop_front();
            while (count > 0)
            {
                assert(!"");
                m_registers[args - count] = m_stack.front();
                m_stack.pop_front();
            }
        }
        break;
        case VM_IType::Mov:
        {
            assert(std::holds_alternative<VM_Reg>(instruction.arg1));
            m_registers[(uint32_t)std::get<VM_Reg>(instruction.arg1)] = instruction.arg2;
        }
        break;
        case VM_IType::Add:
        {
            assert(std::holds_alternative<VM_Reg>(instruction.arg1));
            auto& target = m_registers[(uint32_t)std::get<VM_Reg>(instruction.arg1)];

            TValue source;
            if (std::holds_alternative<VM_Reg>(instruction.arg2))
            {
                source = m_registers[(uint32_t)std::get<VM_Reg>(instruction.arg1)];
            }
            else
            {
                source = instruction.arg1;
            }

            extern TValue Add(const TValue& lhs, const TValue& rhs);
            target = Add(target, source);
        }
        break;
        }
    }
    void Run(VFunction* pFn)
    {
        m_callStack.push_front(pFn);
        m_pc = 0;

        std::ostringstream& code = m_log;
        code << "\nRunning " << pFn->name << "\n";

        while (m_pc < m_callStack.front()->instructions.size())
        {
            auto& i = m_callStack.front()->instructions[m_pc];

            Execute(i);

            std::ostringstream inst;
            switch (i.type)
            {
            case VM_IType::Call:
            {
                inst << "CALL ";
                m_dumpArgFn(inst, i.arg1);
                auto pFn = GetFunction(std::get<std::string>(i.arg1));
                if (pFn->pFnNative)
                {
                    inst << " (native)";
                }
            }
            break;
            case VM_IType::Ret:
                inst << "RET";
                break;
            case VM_IType::Push:
                inst << "PUSH ";
                m_dumpArgFn(inst, i.arg1);
                break;
            case VM_IType::Pop:
                inst << "POP ";
                m_dumpArgFn(inst, i.arg1);
                break;
            case VM_IType::PopArgs:
                inst << "POPARGS";
                break;
            case VM_IType::Mov:
                inst << "MOV ";
                m_dumpArgFn(inst, i.arg1);
                inst << ", ";
                m_dumpArgFn(inst, i.arg2);
                break;
            case VM_IType::Add:
                inst << "Add ";
                m_dumpArgFn(inst, i.arg1);
                inst << ", ";
                m_dumpArgFn(inst, i.arg2);
                break;
            }

            code << std::setw(20) << std::left << inst.str() << "Stack:";

            bool first = true;
            for (const auto& s : m_stack)
            {
                code << " [";
                m_dumpArgFn(code, s);
                code << "]";
            }
            code << "\n";
            m_pc++;
        }
    }

    VFunction* GetFunction(const std::string& name)
    {
        auto itr = m_functionMap.find(name);
        if (itr == m_functionMap.end())
        {
            auto pFn = std::make_shared<VFunction>();
            pFn->name = name;
            m_functions.push_back(pFn);
            m_functionMap[name] = (uint32_t)(m_functions.size() - 1);
            return pFn.get();
        }
        return (m_functions[itr->second].get());
    }

    void AddFunction(const std::string& name, std::shared_ptr<VFunction> spFunction)
    {
        assert(m_functionMap.find(name) == m_functionMap.end());
        spFunction->name = name;
        m_functions.push_back(spFunction);
        m_functionMap[name] = (uint32_t)(m_functions.size() - 1);
    }

    VM_Reg RegisterByIndex(uint32_t index) const
    {
        return VM_Reg((uint32_t)VM_Reg::R0 + index);
    }

    // TODO: These are reserved keywords!
    uint32_t AddReservedVariable(const std::string& name)
    {
        assert(m_mapVariables.find(name) == m_mapVariables.end());
        m_variables.push_back(name);
        m_mapVariables[name] = uint32_t(m_variables.size() - 1);
        return uint32_t(m_variables.size() - 1);
    }

    template <class T>
    T RegAs(uint32_t index) const
    {
        return std::get<T>(m_registers[index]);
    }

    template <class T>
    T* RegAsPtr(uint32_t index) const
    {
        return std::get<std::shared_ptr<T>>(m_registers[index]).get();
    }

    template <class T>
    bool RegIs(uint32_t index)
    {
        return std::holds_alternative<T>(m_registers[index]);
    }

    void AddNativeFunction(const std::string& name, std::function<TValue(uint32_t argCount)> fn) 
    {
        auto pFunction = std::make_shared<VFunction>();
        AddFunction(name, pFunction);
        pFunction->pFnNative = fn;
    };
       
public:
    std::map<std::string, uint32_t> m_functionMap;
    std::vector<std::shared_ptr<VFunction>> m_functions;

    // Runtime data
    int m_pc = 0;
    std::deque<TValue> m_stack;
    std::deque<VFunction*> m_callStack;

    // TODO: This could blow if we were building, say, a large pattern.
    // Perhaps function returns should push onto the stack instead?
    std::array<TValue, 1024> m_registers;

    std::vector<TValue> m_variables;
    std::map<std::string, uint32_t> m_mapVariables;

    std::ostringstream& m_log;
    DumpArgFn m_dumpArgFn;
};

} // namespace MUtils
