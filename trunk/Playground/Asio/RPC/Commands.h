#ifndef RPC_COMMANDS_H
#define RPC_COMMANDS_H


#include "RPC/Command.h"
#include "RPC/RemoteObjects.h"


namespace RPC {


using boost::tuples::tuple;
typedef std::string Name;


struct CreateStopwatch : public ConcreteCommand<RemoteStopwatch(tuple<RemoteServer, Name>)>
{
    static const char * CommandName() { return "CreateStopwatch"; }
    CreateStopwatch(const Arg & inArgs) : Super(CommandName(), inArgs) { }
};


struct StartStopwatch : public ConcreteCommand<bool(RemoteStopwatch)>
{
    static const char * CommandName() { return "StartStopwatch"; }
    StartStopwatch(const Arg & inArg) : Super(CommandName(), inArg) { }
};


struct StopStopwatch : public ConcreteCommand<bool(RemoteStopwatch)>
{
    static const char * CommandName() { return "StopStopwatch"; }
    StopStopwatch(const Arg & inArg) : Super(CommandName(), inArg) { }
};


} // namespace RPC


#endif // RPC_COMMANDS_H