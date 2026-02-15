#include "newServer.hpp"

void    Server::Nick(std::string cmd, Client &cli)
{
    std::string nick = cmd.substr(5);
    if (nick.empty())
    {
        cli.queueResponse(":server 431 :No nickname given\r\n");
        return;
    }
    if (isNameInUse(nick, true, cli.getFd()))
    {
        std::string newNick = nick;
        do {
            newNick += "_";
        } while (isNameInUse(newNick, true, cli.getFd()));
        std::cout << "[nick dup -> adjusted] fd=" << cli.getFd() << " tried '" << nick << "' -> assigned '" << newNick << "'\n";
        cli.setNickName(newNick);
        if (!cli.getUserName().empty())
            cli.queueResponse(":server NOTICE " + cli.getNickName() + " :Your nickname was changed to " + newNick + " because the requested nick was already in use\r\n");
        else
            cli.queueResponse(":server NOTICE * :Your nickname was changed to " + newNick + " because the requested nick was already in use\r\n");
        return;
    }
    std::cout << "[nick set] fd=" << cli.getFd() << " -> nick='" << nick << "'\n";
    sendToChannel(":"+cli.getNickName()+"!~"+cli.getUserName()+"@127.0.0.1 NICK "+nick, cli.getChannelIndex());
    cli.setNickName(nick);
    // After setting nick, check registration and send welcome numerics
    Server *srv = this;
    srv->sendWelcome(cli);
}
