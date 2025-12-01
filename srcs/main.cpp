/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bszikora <bszikora@student.42helbronn.d    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 01:29:33 by bszikora          #+#    #+#             */
/*   Updated: 2025/11/09 02:42:31 by bszikora         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "newServer.hpp" // if you see this line in red, ignore it. It compiles.

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return (1);
	}
	
	try
	{
		int port = std::atoi(argv[1]);
		std::string password = argv[2];
		Server server(port, password);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
