#ifndef CONCURRENTCALL_H
#define CONCURRENTCALL_H


#include "RemoteCall.h"
#include "Test.h"
#include <boost/lexical_cast.hpp>


using namespace boost::tuples;


std::string PerformRemoteCall(const NameAndArg & )
{
    throw std::runtime_error("Concurrent call not yet implemented.");
}


struct ConcurrentCall : public RemoteCall<std::vector<std::string>(std::vector<NameAndArg>)>
{
    typedef RemoteCall<std::vector<std::string>(std::vector<NameAndArg>)> Base;
    typedef Base::Arg Arg;
    typedef Base::Ret Ret;
    static std::string Name() { return "ConcurrentCall"; }

    ConcurrentCall(const Arg & arg) :
        Base(Name(), arg)
    {
    }

    #ifdef RPC_SERVER
    static std::vector<std::string> execute(const std::vector<NameAndArg> & args)
    {
        std::vector<std::string> result;
        for (std::size_t i = 0; i < args.size(); ++i)
        {
            const NameAndArg & na = args[i];
            result.push_back(PerformRemoteCall(na));
        }
        return result;
    }
    #endif
};

#ifdef RPC_SERVER
struct RegisterConcurrentCall
{
    RegisterConcurrentCall()
    {
        Register<ConcurrentCall>();
    }
} gRegisterConcurrentCall;
#endif


#ifdef RPC_CLIENT
template<typename C0, typename C1>
boost::tuples::tuple<typename C0::Ret, typename C1::Ret>
Concurrent(const C0 & c0, const C1 & c1)
{
    std::vector<NameAndArg> args;
    args.push_back(NameAndArg(c0.name(), serialize(c0.arg())));
    args.push_back(NameAndArg(c1.name(), serialize(c1.arg())));

    std::vector<std::string> result = ConcurrentCall(args).send();
    return boost::tuples::make_tuple(deserialize<typename C0::Ret>(result[0]),
                                     deserialize<typename C1::Ret>(result[1]));
}
#endif



#endif // CONCURRENTCALL_H