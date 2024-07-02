#include "IoTask.hpp"
#include <iostream>
#include "Chat.hpp"
#include "Constants.hpp"
#include "StartTask.hpp"

using namespace std;

static auto chatFromStdin(ChatMessage& msg)
{
    getline(cin, msg);
    chat(msg);
}

void ioTask()
{
    startTask("io");

    ChatMessage msg;
    while (msg != quit_msg)
        chatFromStdin(msg);
}
