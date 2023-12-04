#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "Packet.h"
#include "Chat.pb.h"

using namespace boost;
using boost::asio::ip::tcp;

class ChatClient
{
public:
	ChatClient(asio::io_context& io_context) : _socket(io_context)
	{
		memset(_recvBuffer, 0, RecvBufferSize);
	}

	~ChatClient()
	{
	}

	void Connect(std::string host, int port)
	{
		tcp::endpoint endpoint(asio::ip::make_address(host), port);
		_socket.async_connect(endpoint, boost::bind(&ChatClient::OnConnect, this, asio::placeholders::error));
	}

	void AsyncWrite(std::string msg)
	{
		_sendMsg = msg;
		asio::async_write(_socket, asio::buffer(_sendMsg),
			boost::bind(&ChatClient::OnWrite, this,
				asio::placeholders::error, asio::placeholders::bytes_transferred
			)
		);
	}
	
	void AsyncWrite(asio::mutable_buffer& buffer)
	{
		asio::async_write(_socket, buffer,
			boost::bind(&ChatClient::OnWrite, this, asio::placeholders::error
				, asio::placeholders::bytes_transferred
			)
		);
	}

private:
	void OnConnect(const boost::system::error_code& err)
	{
		std::cout << "OnConnect " << std::endl;
		if (!err)
		{
			AsyncRead();
		}
	}

	void OnWrite(const boost::system::error_code& err, size_t bytes_transferred)
	{
		if (!err)
		{

		}
		else
		{
			std::cout << "error code : " << err.value() << ", msg : " << err.message() << std::endl;
		}
	}

	void OnRead(const boost::system::error_code& err, size_t size)
	{
		std::string msg(_recvBuffer, size);
		std::cout << "OnRead Size : " << size << std::endl;

		if (!err)
		{
			HandlePacket(_recvBuffer, size);
			AsyncRead();
		}
		else
		{
			std::cout << "error code : " << err.value() << ", msg : " << err.message() << std::endl;
		}
	}

	void AsyncRead()
	{
		memset(_recvBuffer, 0, RecvBufferSize);
		_socket.async_read_some(
			asio::buffer(_recvBuffer, RecvBufferSize),
			boost::bind(&ChatClient::OnRead, this,
				asio::placeholders::error,
				asio::placeholders::bytes_transferred
			)
		);
	}

	void HandlePacket(char* ptr, size_t size)
	{
		asio::mutable_buffer buffer = asio::buffer(ptr, size);
		int offset = 0;
		PacketHeader header;
		PacketUtil::ParseHeader(buffer, &header, offset);

		std::cout << "HandlePacket " << chat::MessageCode_Name(header.Code) << std::endl;
		switch (header.Code)
		{
		case chat::MessageCode::LOGIN_RES:
			break;
		case chat::MessageCode::CHAT_NOTI:
			HandleChatNoti(buffer, header, offset);
			break;
		case chat::MessageCode::LIST_RES:
			HandleListRes(buffer, header, offset);
			break;
		}
	}

	void HandleListRes(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
	{
		chat::ListRes pbMsg;
		PacketUtil::Parse(pbMsg, buffer, header.Length, offset);
		for (int i = 0; i < pbMsg.names_size(); i++)
		{
			std::cout << pbMsg.names(i) << std::endl;
		}
	}

	void HandleChatNoti(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
	{
		chat::ChatNoti pbMsg;
		PacketUtil::Parse(pbMsg, buffer, header.Length, offset);
		std::cout << pbMsg.sender(0) << " : " << pbMsg.message(0) << std::endl;
	}

private:
	static const int RecvBufferSize = 1024;
	tcp::socket _socket;
	char _recvBuffer[RecvBufferSize];
	std::string _sendMsg;
	
};

static void ExecuteCommand(const std::string& line, ChatClient& client)
{
	typedef std::vector<std::string> Tokens;
	Tokens tokens;
	boost::split(tokens, line, boost::is_any_of(" "));
	const std::string& mainCmd = tokens[0];
	if (mainCmd == "login")
	{
		std::string& name = tokens[1];
		chat::LoginReq loginReq;
		auto n = loginReq.add_name();
		*n = name;
		const size_t requiredSize = PacketUtil::RequiredSize(loginReq);
		char* rawBuffer = new char[requiredSize];
		auto buffer = asio::buffer(rawBuffer, requiredSize);
		if (!PacketUtil::Serialize(buffer, chat::MessageCode::LOGIN_REQ, loginReq))
		{

		}
		client.AsyncWrite(buffer);
	}
	else if (mainCmd == "list")
	{
		chat::ListReq listReq;
		const size_t requiredSize = PacketUtil::RequiredSize(listReq);

		char* rawBuffer = new char[requiredSize];
		auto buffer = asio::buffer(rawBuffer, requiredSize);
		if (!PacketUtil::Serialize(buffer, chat::MessageCode::LIST_REQ, listReq))
		{

		}
		client.AsyncWrite(buffer);
	}
	else if (mainCmd == "chat")
	{
		std::string& message = tokens[1];
		chat::ChatReq chatReq;
		auto m = chatReq.add_message();
		*m = message;
		const size_t requiredSize = PacketUtil::RequiredSize(chatReq);

		char* rawBuffer = new char[requiredSize];
		auto buffer = asio::buffer(rawBuffer, requiredSize);
		if (!PacketUtil::Serialize(buffer, chat::MessageCode::CHAT_REQ, chatReq))
		{

		}
		client.AsyncWrite(buffer);
	}
}
//int main()
//{
//	boost::asio::io_context io_context;
//	ChatClient client(io_context);
//	client.Connect(std::string("127.0.0.1"), 4242);
//	std::thread t(boost::bind(&boost::asio::io_context::run, &io_context));
//	char line[256] = { 0, };
//	while (std::cin.getline(line, 256))
//	{
//		ExecuteCommand(line, client);
//		memset(line, 0, 256);
//	}
//}