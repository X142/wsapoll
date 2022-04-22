#include "header.h"

#include <ws2tcpip.h> // struct sockaddr_in6 (ws2ipdef.h), inet_ntop

// ---------------------------------------------------------------
#define NUM_listening_Port 12345
#define NUM_backlog_for_listen 10
// 追加 ---------------
#define NUM_Sock 4
#define NUM_server_Sock 2
#define NUM_client_Sock (NUM_Sock - NUM_server_Sock)
// ----------------

class Server {
public:
	Server(const USHORT num_listening_port, const int num_backlog_for_listen)
		: mc_pcs_backlog_for_listen{ num_backlog_for_listen }
	{
		try
		{
			m_fd_sock_listener = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			if (m_fd_sock_listener == INVALID_SOCKET)
				throw "socket";

			sockaddr_in6 addr_listener;
			memset(&addr_listener, 0, sizeof addr_listener);
			addr_listener.sin6_family = AF_INET6;
			addr_listener.sin6_port = htons(num_listening_port);
			addr_listener.sin6_addr = in6addr_loopback;
			//addr_listener.sin6_addr = in6addr_any;

			if (bind(m_fd_sock_listener, (sockaddr*)&addr_listener, sizeof addr_listener) != 0)
				throw "bind";
		}
		catch (const char*)
		{
			if (m_fd_sock_listener != INVALID_SOCKET)
				closesocket(m_fd_sock_listener);
			throw;
		}
	}

	~Server()
	{
		if (m_fd_sock_listener != INVALID_SOCKET)
		{
			closesocket(m_fd_sock_listener);
			std::cout << "closed listen socket" << std::endl;
		}
	}

	SOCKET Start_to_Listen()
	{
		if (listen(m_fd_sock_listener, mc_pcs_backlog_for_listen) != 0)
			throw "listen";
		// ---------------- 追加
		return m_fd_sock_listener;
		// ----------------
	}

	SOCKET Accept()
	{
		const auto fd_sock_client = accept(m_fd_sock_listener, nullptr, nullptr);
		if (fd_sock_client == INVALID_SOCKET)
			throw "accept";
		return fd_sock_client;
	}
private:
	SOCKET m_fd_sock_listener = INVALID_SOCKET;
	const int mc_pcs_backlog_for_listen;
};

class Client
{
public:
	Client(const SOCKET fd_sock_client) : mc_fd_sock_client{ fd_sock_client } {}
	// 追加 ----------------
	Client(const Client& other) : mc_fd_sock_client{ other.mc_fd_sock_client } {}
	Client& operator=(const Client& other)
	{
		if (mc_fd_sock_client != INVALID_SOCKET)
			closesocket(mc_fd_sock_client);
		return *this;
	}
	// ----------------
	~Client()
	{
		if (mc_fd_sock_client != INVALID_SOCKET)
		{
			closesocket(mc_fd_sock_client);
			std::cout << "closed client socket" << std::endl;
		}
	}

	void Read()
	{
		enum { EN_bytes_buffer = 2000 };

		char buf[EN_bytes_buffer];
		int bytes_recvd = recv(mc_fd_sock_client, buf, EN_bytes_buffer, 0);

		if (bytes_recvd > 0)
		{
			std::cout << "+++ 受信 bytes -> " << bytes_recvd << std::endl;

			if (bytes_recvd < EN_bytes_buffer)
			{
				buf[bytes_recvd] = 0;
			}
			else
			{
				buf[EN_bytes_buffer - 1] = 0;
			}
			std::cout << buf << std::endl;
		}
	}

	void Show_Info()
	{
		sockaddr_in6 addr_client;
		socklen_t addrlen = sizeof(addr_client);
		if (getpeername(mc_fd_sock_client, (sockaddr*)&addr_client, &addrlen) != 0)
			throw "getpeername";
		char str_client_ip_addr[INET6_ADDRSTRLEN];
		if (inet_ntop(AF_INET6, (sockaddr*)&addr_client.sin6_addr, str_client_ip_addr, sizeof(str_client_ip_addr)) == NULL)
			throw "inet_ntop";
		std::cout << "addr_client.sin6_addr is " << str_client_ip_addr << std::endl;
		std::cout << "addr_client.sin6_port is " << addr_client.sin6_port << std::endl;
	}
private:
	const SOCKET mc_fd_sock_client;
};

int main_2()
{
	// 追加 ----------------
	int i;
	
	Server servers[NUM_server_Sock] {
		Server{NUM_listening_Port, NUM_backlog_for_listen},
		Server{NUM_listening_Port + 1, NUM_backlog_for_listen}
	};

	Client clients[NUM_client_Sock]{
		Client(INVALID_SOCKET),
		Client(INVALID_SOCKET)
	};

	struct pollfd fdarray[NUM_Sock];
	for (i = 0; i < NUM_Sock; i++)
		fdarray[i].fd = -1;

	std::cout << "listening..." << std::endl;
	for (i = 0; i < NUM_server_Sock; i++)
	{
		fdarray[i].fd = servers[i].Start_to_Listen();
		fdarray[i].events = POLLIN;
	}

	int ret_wsapoll;
	int count = NUM_server_Sock;
	for (;;)
	{
		ret_wsapoll = WSAPoll(fdarray, count, 2500);
		if (ret_wsapoll == SOCKET_ERROR)
			throw "WSAPoll";
		else if (ret_wsapoll == 0)
		{
			std::cout << "timeout..." << std::endl;
			continue;
		}
		else
		{
			for (i = 0; i < NUM_server_Sock; i++)
			{
				if (fdarray[i].revents | POLLIN)
				{
					if (count == NUM_Sock)
						throw "ソケットをこれ以上受付できません";
					fdarray[count].fd = servers[i].Accept(); // バグ１：accept から抜けない
					fdarray[count].events = POLLIN;
					// clients[i] = Client(fdarray[count].fd); // バグ２：構文が間違っていると思う
					std::cout << "server, i: " << i << ", count: " << count <<  std::endl;
					count++;
				}
			}
			for (i = 0; i < count - NUM_server_Sock; i++)
			{
				if (fdarray[NUM_server_Sock + i].revents | POLLIN)
				{
					std::cout << "client, i: " << i << ", count: " << count <<  std::endl;
					clients[i].Read();
				}
			}
		}
	}



	// ----------------

	//Server server{ NUM_listening_Port, NUM_backlog_for_listen };
	//std::cout << "listening..." << std::endl;
	//server.Start_to_Listen();

	//const auto fd_sock_client = server.Accept();
	//std::cout << "accepted" << std::endl;

	//Client client{ fd_sock_client };
	//client.Show_Info();
	//client.Read();

	return 0;
}



