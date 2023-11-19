#include <iostream>
#include <set>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>

using boost::asio::ip::tcp;
using namespace boost;

class ChatMember
{
public:
    ChatMember() {}
    virtual ~ChatMember() {}
    virtual void SendChat(const std::string& msg) = 0;
};
using ChatMemberPtr = std::shared_ptr<ChatMember>;

class ChatRoom
{
public:
    void Join(ChatMemberPtr member)
    {
        Broadcast(std::string("We have new Member"));
        _members.insert(member);
    }

    void Leave(ChatMemberPtr member)
    {
        _members.erase(member);
        Broadcast(std::string("A member Left"));
    }

    void Broadcast(const std::string& msg)
    {
        for (const auto member : _members)
        {
            member->SendChat(msg);
        }
    }

private:
    std::set<ChatMemberPtr> _members;
};

class ChatSession : public ChatMember, public std::enable_shared_from_this<ChatSession>
{
public:
    ChatSession(asio::io_context& io_context, ChatRoom& room)
        : _socket(io_context), _room(room)
    {
        memset(_recvBuffer, 0, RecvBufferSize);
        memset(_sendBuffer, 0, SendBufferSize);
    }

    void Start()
    {
        _room.Join(this->shared_from_this());
        AsyncRead();
    }

    tcp::socket& GetScoket()
    {
        return _socket;
    }

    void SendChat(const std::string& msg) override
    {
        std::cout << " SendChat " << msg.c_str() << std::endl;
        AsyncWrite(msg.c_str(), msg.size());
    }

protected:

    void AsyncWrite(const char* msg, size_t size)
    {
        memcpy(_sendBuffer, msg, size);
        asio::async_write(_socket, asio::buffer(_sendBuffer, size), [this](boost::system::error_code err, size_t bytes_transferred)
            {
                this->OnWrite(err, bytes_transferred);
            });
    }

    void OnWrite(system::error_code& err, size_t bytes_transferred)
    {
        if (!err)
        {

        }
        else
        {
            std::cout << "error code: " << err.value() << ", msg : " << err.message() << std::endl;
        }
    }

    void AsyncRead()
    {
        memset(_recvBuffer, 0, RecvBufferSize);
        _socket.async_read_some(asio::buffer(_recvBuffer, RecvBufferSize), [this](boost::system::error_code err, size_t bytes_transferred) {
            this->OnRead(err, bytes_transferred);
            });
    }

    void OnRead(const system::error_code& err, size_t bytes_transferred)
    {
        std::cout << "OnRead" << bytes_transferred << std::endl;
        if (!err)
        {
            _room.Broadcast(std::string(_recvBuffer, bytes_transferred));
            AsyncRead();
        }
        else
        {
            std::cout << "error code: " << err.value() << ", msg : " << err.message() << std::endl;
        }
    }

private:
    tcp::socket _socket;
    const static int RecvBufferSize = 1024;
    char _recvBuffer[RecvBufferSize];
    const static int SendBufferSize = 1024;
    char _sendBuffer[SendBufferSize];

    ChatRoom& _room;
};

using ChatSessionPtr = std::shared_ptr<ChatSession>;

class ChatServer
{
public:
    ChatServer(asio::io_context& io_context, int port)
        : _acceptor(io_context, tcp::endpoint(tcp::v4(), port)), _io_context(io_context) {}

    void StartAccept()
    {
        ChatSession* session = new ChatSession(_io_context, _room);
        ChatSessionPtr sessionPtr(session);
        _acceptor.async_accept(sessionPtr->GetScoket(), [this, sessionptr = sessionPtr](system::error_code err)
            {
                this->OnAccept(sessionptr, err);
            });
    }

protected:
    void OnAccept(ChatSessionPtr session, system::error_code err)
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
        int port = 4242;
        boost::asio::io_context io_context;
        ChatServer s(io_context, port);
        s.StartAccept();
        std::cout << "server Start " << port << std::endl;
        io_context.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

