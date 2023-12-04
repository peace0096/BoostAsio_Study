#include <iostream>
#include <set>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Packet.h"
#include "Chat.pb.h"

using boost::asio::ip::tcp;
using namespace boost;

class ChatMember
{
public:
	ChatMember() {}
	virtual ~ChatMember() {}
	virtual void SendChat(const std::string& msg) abstract;
	virtual void SetName(const std::string& msg) abstract;
	virtual std::string GetName() abstract;
	virtual void Send(asio::mutable_buffer& buffer) abstract;
};

typedef boost::shared_ptr<ChatMember> ChatMemberPtr;

class ChatRoom
{
public:
	void Join(ChatMemberPtr member)
	{
		chat::ChatNoti pbNoti;
		auto m = pbNoti.add_message();
		m->assign("We have new member.");
		auto s = pbNoti.add_sender();
		s->assign("admin");
		const size_t requiredSize = PacketUtil::RequiredSize(pbNoti);

		char* rawBuffer = new char[requiredSize];
		auto sendBuffer = asio::buffer(rawBuffer, requiredSize);
		PacketUtil::Serialize(sendBuffer, chat::MessageCode::CHAT_NOTI, pbNoti);
		Broadcast(sendBuffer);

		_members.insert(member);
	}

	void Leave(ChatMemberPtr member)
	{
		_members.erase(member);

		chat::ChatNoti pbNoti;
		auto m = pbNoti.add_message();
		m->assign("A member left...");
		auto s = pbNoti.add_sender();
		s->assign("admin");
		const size_t requiredSize = PacketUtil::RequiredSize(pbNoti);

		char* rawBuffer = new char[requiredSize];
		auto sendBuffer = asio::buffer(rawBuffer, requiredSize);
		PacketUtil::Serialize(sendBuffer, chat::MessageCode::CHAT_NOTI, pbNoti);
		Broadcast(sendBuffer);
	}

	void Broadcast(const std::string& msg)
	{
		std::for_each(_members.begin(), _members.end(), [msg](ChatMemberPtr p)
			{
				p->SendChat(msg);
			});
	}

	void Broadcast(asio::mutable_buffer& buffer)
	{
		std::for_each(_members.begin(), _members.end(), [&buffer](ChatMemberPtr p)
			{
				p->Send(buffer);
			});
	}

	void SendList(ChatMemberPtr member)
	{
		chat::ListRes pbRes;
		std::for_each(_members.begin(), _members.end(),
			[&pbRes](ChatMemberPtr p)
			{
				auto name = pbRes.add_names();
				name->assign(p->GetName());
			});

		const size_t requiredSize = PacketUtil::RequiredSize(pbRes);

		char* rawBuffer = new char[requiredSize];
		auto buffer = asio::buffer(rawBuffer, requiredSize);
		PacketUtil::Serialize(buffer, chat::MessageCode::LIST_RES, pbRes);
		member->Send(buffer);

	}

private:
	std::set<ChatMemberPtr> _members;
};

class ChatSession
	: public ChatMember
	, public boost::enable_shared_from_this<ChatSession>
{
public:
	ChatSession(asio::io_context& io_context, ChatRoom& room)
		: _socket(io_context)
		, _room(room)
		, _strand(asio::make_strand(io_context))
	{
		memset(_recvBuffer, 0, RecvBufferSize);
		memset(_sendBuffer, 0, SendBufferSize);
	}

	void Start()
	{
		_room.Join(this->shared_from_this());
		AsyncRead();
	}

	tcp::socket& GetSocket()
	{
		return _socket;
	}

	void SendChat(const std::string& msg) override
	{
		std::cout << "SendChat : " << msg.c_str() << std::endl;
		AsyncWrite(msg.c_str(), msg.size());
	}

	void SetName(const std::string& name) override
	{
		_name = name;
	}

	std::string GetName() override
	{
		return _name;
	}

	void Send(asio::mutable_buffer& buffer) override
	{
		AsyncWrite(static_cast<const char*>(buffer.data()), buffer.size());
	}

protected:
	void AsyncRead()
	{
		memset(_recvBuffer, 0, RecvBufferSize);
		_socket.async_read_some(asio::buffer(_recvBuffer, RecvBufferSize), 
			boost::bind(
				&ChatSession::OnRead, this,
				asio::placeholders::error,
				asio::placeholders::bytes_transferred
			)
		);
	}

	void OnRead(const boost::system::error_code err, size_t size)
	{
		std::cout << "OnRead " << size << std::endl;
		if (!err)
		{
			HandlePacket(_recvBuffer, size);
			AsyncRead();
		}
		else
		{
			std::cout << "error code : " << err.value() << ", msg : " << err.message() << std::endl;
			_room.Leave(this->shared_from_this());
		}
	}

	void AsyncWrite(const char* message, size_t size)
	{
		memcpy(_sendBuffer, message, size);
		asio::async_write(_socket, 
			asio::buffer(_sendBuffer, size),
			asio::bind_executor(_strand, 
				boost::bind(
					&ChatSession::OnWrite,
					this, 
					asio::placeholders::error,
					asio::placeholders::bytes_transferred
				)
			)
		);
	}

	void OnWrite(const boost::system::error_code& err, size_t size)
	{
		if (!err)
		{
			
		}
		else
		{
			std::cout << "error code : " << err.value() << ", msg : " << err.message() << std::endl;
			_room.Leave(this->shared_from_this());
		}
	}

	void HandlePacket(char* ptr, size_t size)
	{
		asio::mutable_buffer buffer = asio::buffer(ptr, size);
		int offset = 0;
		PacketHeader header;
		PacketUtil::ParseHeader(buffer, &header, offset);

		std::cout << "HandlePacket" << chat::MessageCode_Name(header.Code) << std::endl;
		switch (header.Code)
		{
		case chat::MessageCode::LOGIN_REQ:
			HandleLoginReq(buffer, header, offset);
			break;
		case chat::MessageCode::LIST_REQ:
			HandleListReq(buffer, header, offset);
			break;
		case chat::MessageCode::CHAT_REQ:
			HandleChatReq(buffer, header, offset);
			break;
		}

	}

	void HandleLoginReq(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
	{
		std::cout << "HandleLoginReq" << std::endl;
		chat::LoginReq pbMsg;
		PacketUtil::Parse(pbMsg, buffer, header.Length, offset);
		SetName(pbMsg.name(0));

		chat::LoginRes pbRes;
		pbRes.set_result(true);
		const size_t requiredSize = PacketUtil::RequiredSize(pbRes);

		char* rawBuffer = new char[requiredSize];
		auto sendBuffer = asio::buffer(rawBuffer, requiredSize);
		PacketUtil::Serialize(sendBuffer, header.Code, pbRes);
		this->Send(sendBuffer);
	}

	void HandleListReq(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
	{
		std::cout << "HandleListReq" << std::endl;
		_room.SendList(this->shared_from_this());
	}

	void HandleChatReq(asio::mutable_buffer& buffer, const PacketHeader& header, int& offset)
	{
		std::cout << "HandleChatReq" << std::endl;
		chat::ChatReq pbReq;
		PacketUtil::Parse(pbReq, buffer, header.Length, offset);

		chat::ChatNoti pbNoti;
		auto m = pbNoti.add_message();
		*m = pbReq.message(0);

		auto s = pbNoti.add_sender();
		*s = this->GetName();
		const size_t requiredSize = PacketUtil::RequiredSize(pbNoti);

		char* rawBuffer = new char[requiredSize];
		auto sendBuffer = asio::buffer(rawBuffer, requiredSize);
		PacketUtil::Serialize(sendBuffer, chat::MessageCode::CHAT_NOTI, pbNoti);
		_room.Broadcast(sendBuffer);
	}



private:
	tcp::socket _socket;
	const static int RecvBufferSize = 1024;
	char _recvBuffer[RecvBufferSize];
	const static int SendBufferSize = 1024;
	char _sendBuffer[SendBufferSize];

	ChatRoom& _room;
	asio::strand<asio::io_context::executor_type> _strand;
	std::string _name;
};

typedef boost::shared_ptr<ChatSession> ChatSessionPtr;

class ChatServer
{
public:
	ChatServer(asio::io_context& io_context, int port)
		: _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
		, _io_context(io_context)
	{}

	void StartAccept()
	{
		ChatSession* session = new ChatSession(_io_context, _room);
		ChatSessionPtr sessionPtr(session);
		_acceptor.async_accept(sessionPtr->GetSocket(),
			boost::bind(&ChatServer::OnAccept, this, sessionPtr, asio::placeholders::error));
	}

protected:
	void OnAccept(ChatSessionPtr session, boost::system::error_code err)
	{
		if (!err)
		{
			std::cout << "Connected " << std::endl;
			session->Start();
		}
		StartAccept();
	}

private:
	tcp::acceptor _acceptor;
	asio::io_context& _io_context;
	ChatRoom _room;
};

typedef boost::shared_ptr<ChatServer> ChatServerPtr;

int main()
{
	try
	{
		constexpr int port = 4242;
		asio::io_context io_context;
		ChatServer s(io_context, port);
		s.StartAccept();
		std::cout << "Server Start " << port << std::endl;
		io_context.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception " << e.what() << "\n";
	}
}