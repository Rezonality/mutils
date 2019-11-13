#include <catch2/catch.hpp>
#include "mutils/compile/compile_messages.h"

using namespace MUtils;

TEST_CASE("CompileMessage", "Compile")
{
    CompileMessage msg;
    msg.line = 1;
    msg.msgType = CompileMessageType::Error;
    msg.range = std::make_pair(0, 5);
    msg.text = "An Error";
    REQUIRE(msg.text == "An Error");
}

