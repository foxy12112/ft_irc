*This project has been created as part of the 42 curriculum by ldick, julcalde, bszikora.*

# Description
ft_irc is a small IRC server written in C++98. It implements a subset of the IRC protocol so multiple clients can connect, authenticate, join channels, and exchange messages. The goal is to understand event-driven networking, socket programming, and IRC command handling.

# Instructions
## Build
```sh
make
```

## Run
```sh
./ircserv <port> <password>
```
Example:
```sh
./ircserv 6667 hunter2
```

## Connect
Use IRC client irssi to connect to the server, then authenticate with the password above.

# Resources
- https://modern.ircdocs.horse/
- https://irssi.org/documentation/
- https://www.youtube.com/watch?v=dEHZb9JsmOU&t=440s
- https://www.youtube.com/watch?v=O-yMs3T0APU
- https://www.youtube.com/watch?v=XXfdzwEsxFk&t=116s
- https://www.youtube.com/watch?v=gntyAFoZp-E&t=15s
- https://www.geeksforgeeks.org/cpp/set-in-cpp-stl/
- https://en.cppreference.com/w/cpp/container/set.html
- AI usage: Was often used to help understand official documentations, as well as for formatting/splitting the existing code from only a few files to their own ones and similar formatting jobs. It was also used to help find and define every case that should havee an IRC standard numeric reply defined for them. It is also out of AI recommendation to ditch indexing access and use iterators in some cases. Some test scripts were also created by AI to test heavy flooding, hundreds of concurrent users and such.