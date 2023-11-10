#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind/bind.hpp>

using namespace std;

class TcpClient
{
public:
    TcpClient(boost::asio::io_context& io_context) : _socket(io_context)
    {
        memset(_recvBuffer, 0, RecvBufferSize);
    }

    void Connect(string host, int port)
    {
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(host), port);
        _socket.async_connect(ep, boost::bind(&TcpClient::OnConnect, this, boost::asio::placeholders::error));
    }
private:
    void OnConnect(const boost::system::error_code& err)
    {
        cout << "OnConnected" << endl;
        if (!err)
        {
            AsyncWrite("Hello, World!");
            AsyncRead();
        }
        else
        {
            cout << "Connect error : " << err.message() << endl;
            return;
        }
    }

    void AsyncWrite(string msg)
    {
        _sendMsg = msg;
        _socket.async_write_some(boost::asio::buffer(_sendMsg), boost::bind(&TcpClient::OnWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void OnWrite(const boost::system::error_code& err, size_t bytes_transferred)
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

    void AsyncRead()
    {
        _socket.async_read_some(
            boost::asio::buffer(_recvBuffer, RecvBufferSize),
            boost::bind(
                &TcpClient::OnRead,
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred
            ));
    }

    void OnRead(const boost::system::error_code& err, size_t bytes_transferred)
    {
        cout << "OnRead " << bytes_transferred << endl;
        if (!err)
        {
            AsyncRead();
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(500ms);
            AsyncWrite("Hello, world! 222");
        }
        else
        {
            cout << "Error : " << err.message() << endl;
        }
    }


private:
    static const int RecvBufferSize = 1024;

    boost::asio::ip::tcp::socket _socket;
    char _recvBuffer[RecvBufferSize];
    string _sendMsg;
    
};


int main()
{
    boost::asio::io_context io_context;
    TcpClient client(io_context);
    client.Connect("127.0.0.1", 4242);
    io_context.run();
}
