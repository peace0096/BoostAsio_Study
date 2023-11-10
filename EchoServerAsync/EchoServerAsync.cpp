#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/buffer.hpp>

using boost::asio::ip::tcp;
using namespace boost;
using namespace std;

class TcpSession
{
public:
    TcpSession(asio::io_context& io_context) : _socket(io_context)
    {
        memset(_recvBuffer, RecvBufferSize, 0);
        memset(_sendBuffer, SendBufferSize, 0);
    }

    void Start()
    {
        AsyncRead();
    }

    tcp::socket& GetSocket()
    {
        return _socket;
    }

    void AsyncRead()
    {
        _socket.async_read_some(
            asio::buffer(_recvBuffer, RecvBufferSize),
            boost::bind(
                &TcpSession::OnRead,
                this,
                asio::placeholders::error,
                asio::placeholders::bytes_transferred));
    }

    void OnRead(system::error_code err, size_t bytes_transferred)
    {
        cout << "OnRead " << bytes_transferred << ", " << _recvBuffer << endl;

        if (!err)
        {
            AsyncRead();
            AsyncWrite(_recvBuffer, bytes_transferred);
        }
        else
        {
            cout << "Error : " << err.message() << endl;
        }
    }

    void AsyncWrite(char* message, size_t size)
    {
        memcpy(_sendBuffer, message, size);
        asio::async_write(
            _socket,
            asio::buffer(_sendBuffer, SendBufferSize),
            boost::bind(&TcpSession::OnWrite,
                this,
                asio::placeholders::error,
                asio::placeholders::bytes_transferred));

    }

    void OnWrite(system::error_code err, size_t bytes_transferred)
    {
        cout << "OnWrite " << bytes_transferred << endl;

        if (!err)
        {
        }
        else
        {
            cout << "Error : " << err.message() << endl;
        }
    }

private:
    tcp::socket _socket;
    const static int RecvBufferSize = 1024;
    const static int SendBufferSize = 1024;

    char _recvBuffer[RecvBufferSize];
    char _sendBuffer[SendBufferSize];
};

class TcpServer
{
public:
    TcpServer(asio::io_context& io_context, int port)
        : _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
        , _io_context(io_context)
    {

    }

    void StartAccept()
    {
        TcpSession* session = new TcpSession(_io_context);
        _acceptor.async_accept(
            session->GetSocket(),
            boost::bind(&TcpServer::OnAccept,
                this,
                session,
                boost::asio::placeholders::error));
    }

protected:
    void OnAccept(TcpSession* session, system::error_code err)
    {
        if (!err)
        {
            cout << "Accept" << endl;
            session->Start();
        }
        else
        {

        }
    }

    
private:
    boost::asio::io_context& _io_context;
    boost::asio::ip::tcp::acceptor _acceptor;
};

int main()
{
    boost::asio::io_context io_context;
    TcpServer tcpServer(io_context, 4242);
    tcpServer.StartAccept();
    cout << "Server Start ! " << endl;
    io_context.run();


    return 0;
}
