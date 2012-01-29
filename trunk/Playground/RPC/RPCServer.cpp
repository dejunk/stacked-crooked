#include "RPCServer.h"
#include "Asio.h"
#include "Serialization.h"
#include <boost/tuple/tuple.hpp>


struct RPCServer::Impl
{
    Impl() :
        mPort(),
        mUDPServer()
    {
    }

    ~Impl()
    {
    }

    void listen(unsigned port)
    {
        mUDPServer.reset(new UDPServer(port));
        mUDPServer->run(boost::bind(&Impl::processRequest, this, _1));
    }

    static void addHandler(const std::string & inName, const Handler & inHandler)
    {
        if (GetHandlers().find(inName) != GetHandlers().end())
        {
            throw std::runtime_error("Handler already added for " + inName);
        }
        GetHandlers().insert(std::make_pair(inName, inHandler));
    }

    std::string processRequest(const std::string & inRequest)
    {
        NameAndArg name_arg = deserialize<NameAndArg>(inRequest);
        try
        {
            const std::string & name = name_arg.get<0>();
            const std::string & arg  = name_arg.get<1>();
            return serialize(RetOrError(true, processRequest(name, arg)));
        }
        catch (const std::exception & exc)
        {
            return serialize(RetOrError(false, exc.what()));
        }
    }

    std::string processRequest(const std::string & inName, const std::string & inArg)
    {
        Handlers::iterator it = GetHandlers().find(inName);
        if (it == GetHandlers().end())
        {
            throw std::runtime_error("Command not registered: " + inName);
        }
        Handler & handler = it->second;
        return handler(inArg);
    }

    unsigned mPort;
    boost::scoped_ptr<UDPServer> mUDPServer;

    static Handlers & GetHandlers()
    {
        static Handlers fHandlers;
        return fHandlers;
    }
};


RPCServer::RPCServer() :
    mImpl(new Impl)
{
}


RPCServer::~RPCServer()
{
}


void RPCServer::listen(unsigned port)
{
    mImpl->listen(port);
}


void RPCServer::addHandler(const std::string & inName, const Handler & inHandler)
{
    Impl::addHandler(inName, inHandler);
}