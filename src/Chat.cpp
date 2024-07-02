#include "Chat.hpp"
#include <mutex>

using namespace std;

ChatMessage chat(const ChatMessage& new_msg)
{
    static mutex mtx;
    lock_guard<mutex> lg{mtx};

    static ChatMessage msg;
    return exchange(msg, new_msg);
}
