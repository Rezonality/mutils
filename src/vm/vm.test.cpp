#include <catch2/catch.hpp>

#include <variant>

#include "mutils/vm/vm.h"

using namespace MUtils;

using TValue = std::variant<std::string, int, VM_Reg>;

namespace MUtils
{
// Provide the add function
TValue Add(const TValue& lhs, const TValue& rhs)
{
    if (std::holds_alternative<int>(lhs) &&
        std::holds_alternative<int>(rhs))
    {
        return std::get<int>(lhs) + std::get<int>(rhs);
    }
    else
    {
        throw std::invalid_argument("Args need to be int");
    }
}
}

using TVM = VM<TValue>;

TEST_CASE("VM.Foo.Bar", "[VM]")
{
    std::ostringstream str;
    std::shared_ptr<TVM::VFunction> pEntry;
    std::unique_ptr<TVM> pVM;
    pVM = std::make_unique<TVM>(str, [](std::ostringstream& str, const TValue& r) {
        if (std::holds_alternative<int>(r))
        {
            str << std::get<int>(r);
        }
        else
        {
        }
    });

    pEntry = std::make_shared<TVM::VFunction>();
    pVM->AddFunction("main", pEntry);

    SECTION("Move")
    {
        pEntry->instructions.push_back(TVM::VInstruction{ VM_IType::Mov, VM_Reg::R0, 6 });
        pVM->Run(pEntry.get());
        REQUIRE(pVM->RegAs<int>(0) == 6);
    }
};

