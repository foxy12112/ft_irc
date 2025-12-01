
# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ldick <ldick@student.42.fr>                +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/01/18 19:28:14 by ldick             #+#    #+#              #
#    Updated: 2025/08/29 16:17:13 by ldick            ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ircserv

#################################################################################################
#											Colors												#
#################################################################################################

CLR_RMV		:= \033[0m
RED			:= \033[1;31m
GREEN		:= \033[1;32m
YELLOW		:= \033[1;33m
BLUE		:= \033[1;34m
CYAN 		:= \033[1;36m
BOLD_BLUE	:= \033[0;34m
NC			:= \033[0m

#################################################################################################
#											Flags												#
#################################################################################################

COMPILER	=	c++
INCLUDES	=	-Iinclude
CFLAGS		=	-std=c++98 -g -Wall -Wextra -Werror -fsanitize=address
ERROR_FILE	=	error.log

#################################################################################################
#											Sources												#
#################################################################################################

_SERVER			=	newServer.cpp #Commands.cpp
SERVER			=	$(addprefix Server/, $(_SERVER))

_CLIENT			=	newClient.cpp
CLIENT			=	$(addprefix Client/, $(_CLIENT))

_SRCS			=	main.cpp $(CLIENT) $(SERVER)
SRCS			=	$(addprefix srcs/, $(_SRCS))

OBJS			=	$(SRCS:srcs/%.cpp=bin/%.o)

#################################################################################################
#											Rules												#
#################################################################################################

all:			$(NAME)

bin:
				@echo "\t\t\t$(BLUE) Making bin directory"
				@mkdir -p bin/Server
				@mkdir -p bin/Client

bin/%.o:		srcs/%.cpp | bin
				@echo "$(GREEN) Compiling $(Compiler) $(CLR_RMV) -c -o $(YELLOW) $@ $(CYAN) $^ $(GREEN) $(CFLAGS) $(GREEN) $(NC)"
				@$(COMPILER) -c -o $@ $^ $(EXTRA_FLAGS) $(CFLAGS) $(INCLUDES) 2> $(ERROR_FILE) || (cat $(ERROR_FILE) && echo "$(RED)Compilation failed :0\nfailed file: \t\t$(YELLOW)$<$(NC)\n\n" && exit 1$(NC))

$(NAME): $(OBJS)
				@$(COMPILER) -o $(NAME) $(OBJS) $(CFLAGS)
				@echo "\t\t\t\t$(RED) compilation success :3$(NC)"

clean:
				@rm -rf bin
				@rm -f $(ERROR_FILE)

fclean:			clean
				@rm -f $(NAME)

re:				fclean all

.PHONY:			all clean fclean re
