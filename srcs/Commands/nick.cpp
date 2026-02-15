#include "newServer.hpp"

void    Server::Nick(std::string cmd, Client &cli)
{
    std::istringstream iss(cmd.substr(5));
    std::string nick, extra;
    iss >> nick >> extra;
    std::string host = _hostname;
    if (nick.empty()) {
        cli.queueResponse(":" + host + " 431 :No nickname given\r\n");
        return;
    }
    if (!extra.empty()) {
        cli.queueResponse(":" + host + " 461 NICK :Too many parameters\r\n");
        return;
    }
    // Nick command does not set real name or username
    if (isNameInUse(nick, true, cli.getFd()))
    {
        std::string newNick = nick;
        do {
            newNick += "_";
        } while (isNameInUse(newNick, true, cli.getFd()));
        std::cout << "[nick dup -> adjusted] fd=" << cli.getFd() << " tried '" << nick << "' -> assigned '" << newNick << "'\n";
        cli.setNickName(newNick);
        if (!cli.getUserName().empty())
            cli.queueResponse(":" + host + " NOTICE " + cli.getNickName() + " :Your nickname was changed to " + newNick + " because the requested nick was already in use\r\n");
        else
            cli.queueResponse(":" + host + " NOTICE * :Your nickname was changed to " + newNick + " because the requested nick was already in use\r\n");
        return;
    }
    std::cout << "[nick set] fd=" << cli.getFd() << " -> nick='" << nick << "'\n";
    sendToChannel(":"+cli.getNickName()+"!~"+cli.getUserName()+"@"+ host +" NICK "+nick, cli.getChannelIndex());
    cli.setNickName(nick);
    // After setting nick, check registration and send welcome numerics
    Server *srv = this;
    srv->sendWelcome(cli);
}
