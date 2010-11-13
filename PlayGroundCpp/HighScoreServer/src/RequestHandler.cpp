#include "RequestHandler.h"
#include "ContentType.h"
#include "Renderer.h"
#include "Utils.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Util/Application.h"
#include "Poco/StreamCopier.h"
#include "Poco/String.h"
#include "Poco/StringTokenizer.h"
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>
#include <sstream>


using namespace Poco::Data;


namespace HSServer
{

    Poco::Logger & GetLogger()
    {
        return Poco::Util::Application::instance().logger();
    }


    MissingArgumentException::MissingArgumentException(const std::string & inMessage) :
        std::runtime_error(inMessage)
    {
    }

    
    RequestHandler::RequestHandler(RequestMethod inRequestMethod, const std::string & inLocation, ContentType inContentType) :
        mSession(SessionFactory::instance().create("SQLite", "HighScores.db")),
        mRequestMethod(inRequestMethod),
        mLocation(inLocation),
        mContentType(inContentType)
    {
        // Create the table if it doesn't already exist
        static bool fFirstTime(true);
        if (fFirstTime)
        {
            mSession << "CREATE TABLE IF NOT EXISTS HighScores("
                     << "Id INTEGER PRIMARY KEY, "
                     << "Timestamp INTEGER, "
                     << "Name VARCHAR(20), "
                     << "Score INTEGER(5))", now;
            fFirstTime = false;
        }
    }


    ContentType RequestHandler::getContentType() const
    {
        return mContentType;
    }


    const std::string & RequestHandler::getPath() const
    {
        return mLocation;
    }

    
    void RequestHandler::handleRequest(Poco::Net::HTTPServerRequest& inRequest,
                                       Poco::Net::HTTPServerResponse& outResponse)
    {
        
        GetLogger().information("Request from " + inRequest.clientAddress().toString());
        GetLogger().information("Request Accept header: " + inRequest.get("Accept"));
        outResponse.setChunkedTransferEncoding(true);
        outResponse.setContentType(ToString(getContentType()));

        try
        {
            generateResponse(inRequest, outResponse);
        }
        catch (const std::exception & inException)
        {
            GetLogger().error(inException.what());
        }
    }


    HTMLErrorResponse::HTMLErrorResponse(const std::string & inErrorMessage) :
        RequestHandler(RequestMethod_Get, "", ContentType_TextHTML),
        mErrorMessage(inErrorMessage)
    {
    }


    void HTMLErrorResponse::generateResponse(Poco::Net::HTTPServerRequest& inRequest, Poco::Net::HTTPServerResponse& outResponse)
    {
        std::string html = MakeHTMLDocument("Error", MakeHTML("p", mErrorMessage, HTMLFormatting_OneLiner));
        outResponse.setContentLength(html.size());
        outResponse.send() << html;
    }


    RequestHandler * HighScorePostForm_Get::Create(const Poco::Net::HTTPServerRequest & inRequest)
    {
        return new HighScorePostForm_Get;
    }


    HighScorePostForm_Get::HighScorePostForm_Get() :
        RequestHandler(GetRequestMethod(), GetLocation(), GetContentType())
    {
    }


    void HighScorePostForm_Get::generateResponse(Poco::Net::HTTPServerRequest& inRequest, Poco::Net::HTTPServerResponse& outResponse)
    {        
        std::string body;
        ReadEntireFile("html/add.html", body);
        outResponse.setContentLength(body.size());
        outResponse.send() << body;
    }


    RequestHandler * HighScoreDeleteForm_Get::Create(const Poco::Net::HTTPServerRequest & inRequest)
    {
        return new HighScoreDeleteForm_Get;
    }


    HighScoreDeleteForm_Get::HighScoreDeleteForm_Get() :
        RequestHandler(GetRequestMethod(), GetLocation(), GetContentType())
    {
    }


    void HighScoreDeleteForm_Get::generateResponse(Poco::Net::HTTPServerRequest& inRequest, Poco::Net::HTTPServerResponse& outResponse)
    {        
        std::string body;
        ReadEntireFile("html/delete.html", body);
        outResponse.setContentLength(body.size());
        outResponse.send() << body;
    }

    
    RequestHandler * HighScore_Post::Create(const Poco::Net::HTTPServerRequest & inRequest)
    {
        return new HighScore_Post;
    }


    HighScore_Post::HighScore_Post() :
        RequestHandler(GetRequestMethod(), GetLocation(), GetContentType())
    {
    }


    void HighScore_Post::generateResponse(Poco::Net::HTTPServerRequest& inRequest,
                                         Poco::Net::HTTPServerResponse& outResponse)
    {
        std::string requestBody;
        inRequest.stream() >> requestBody;

        Args args;
        GetArgs(requestBody, args);
        
        std::string name = URIDecode(GetArg(args, "name"));
        const std::string & score = GetArg(args, "score");

        Statement insert(mSession);
        insert << "INSERT INTO HighScores VALUES(NULL, strftime('%s', 'now'), ?, ?)", use(name), use(score);
        insert.execute();

        // Return an URL instead of a HTML page.
        // This is because the client is the JavaScript application in this case.
        std::string body = LocationId2String<LocationId_HighScorePostOk_Get>::GetValue() + std::string("?name=") + name + "&score=" + score;
        outResponse.setContentLength(body.size());
        outResponse.send() << body;
    }

    
    RequestHandler * HighScore_Delete::Create(const Poco::Net::HTTPServerRequest & inRequest)
    {
        return new HighScore_Delete;
    }


    HighScore_Delete::HighScore_Delete() :
        RequestHandler(GetRequestMethod(), GetLocation(), GetContentType())
    {
    }


    void HighScore_Delete::generateResponse(Poco::Net::HTTPServerRequest& inRequest,
                                           Poco::Net::HTTPServerResponse& outResponse)
    {
        std::string requestBody;
        inRequest.stream() >> requestBody;

        GetLogger().information("Request body is: " + requestBody);

        std::string sql = Poco::replace<std::string>("DELETE FROM HighScores WHERE {{args}}",
                                                     "{{args}}",
                                                     Args2String(GetArgs(requestBody)));

        GetLogger().information("SQL statement is: " + sql);

        Statement insert(mSession);
        insert << sql;
        insert.execute();

        // Return an URL instead of a HTML page.
        // This is because the client is the JavaScript application in this case.
        std::string body = "Succesfully performed the following SQL statement: " + sql;
        outResponse.setContentLength(body.size());
        outResponse.send() << body;
    }


    RequestHandler * PostHighScoreOk_Get::Create(const Poco::Net::HTTPServerRequest & inRequest)
    {
        Args args;
        GetArgs(inRequest.getURI(), args);
        return new PostHighScoreOk_Get(GetArg(args, "name"), GetArg(args, "score"));
    }


    PostHighScoreOk_Get::PostHighScoreOk_Get(const std::string & inName, const std::string & inScore) :
        RequestHandler(GetRequestMethod(), GetLocation(), GetContentType()),
        mName(inName),
        mScore(inScore)
    {
    }


    void PostHighScoreOk_Get::generateResponse(Poco::Net::HTTPServerRequest& inRequest, Poco::Net::HTTPServerResponse& outResponse)
    {
        std::string body;
        body += MakeHTML("h1", "High Score Server", HTMLFormatting_OneLiner);
        body += MakeHTML("p", "Succesfully added highscore for " + mName + " of " + mScore + ".", HTMLFormatting_OneLiner);
        std::string html = MakeHTMLDocument("High Score Added", body);
        outResponse.setContentLength(html.size());
        outResponse.send() << html;
    }

} // namespace HSServer
