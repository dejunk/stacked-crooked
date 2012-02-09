#include "RPC.h"


#ifdef RPC_SERVER
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
        mUDPServer.reset(new UDPServer(port, boost::bind(&Impl::handleUDPMessage, this, _1)));
    }

    void addHandler(const std::string & inName, const Handler & inHandler)
    {
        if (mHandlers.find(inName) != mHandlers.end())
        {
            throw std::runtime_error("Handler already added for " + inName);
        }
        mHandlers.insert(std::make_pair(inName, inHandler));
    }

    std::string handleUDPMessage(const std::string & inMessage)
    {
        return process(deserialize<NameAndArg>(inMessage));
    }

    std::string process(const NameAndArg & name_arg)
    {
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
        Handlers::iterator it = mHandlers.find(inName);
        if (it == mHandlers.end())
        {
            throw std::runtime_error("Command not registered: " + inName);
        }
        Handler & handler = it->second;
        return handler(inArg);
    }

    unsigned mPort;
    boost::scoped_ptr<UDPServer> mUDPServer;
    Handlers mHandlers;
};


RPCServer::RPCServer() :
    mImpl(new Impl())
{
}


RPCServer::~RPCServer()
{
}


void RPCServer::addHandler(const std::string & inName, const Handler & inHandler)
{
    return mImpl->addHandler(inName, inHandler);
}


std::vector<std::string> RPCServer::getRegisteredCommands()
{
    std::vector<std::string> result;
    for (Handlers::const_iterator it = mImpl->mHandlers.begin(), end = mImpl->mHandlers.end(); it != end; ++it)
    {
        result.push_back(it->first);
    }
    return result;
}


void RPCServer::listen(unsigned port)
{
    mImpl->listen(port);
}


std::string RPCServer::process(const NameAndArg & inNameAndArg)
{
    return mImpl->process(inNameAndArg);
}


#endif // RPC_SERVER