#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>

class ChatClient
{
public:
    ChatClient(boost::asio::io_context& io_context) : _socket(io_context)
    {
        memset(_recvBuffer, 0, RecvBufferSize);
    }

    ~ChatClient()
    {
    }

    void Connect(std::string host, int port)
    {
        const boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
        _socket.async_connect(endpoint, [this](const boost::system::error_code& err) { this->OnConnect(err); });
    }

    void OnConnect(const boost::system::error_code& err)
    {
        if (!err)
        {
            std::cout << "OnConnect Success" << std::endl;
            AsyncRead();
        }
        else
        {
            std::cout << "OnConnect Fail" << std::endl;
        }
        
    }

    void AsyncWrite(std::string msg)
    {
        _sendMsg = msg;

        boost::asio::async_write(_socket, boost::asio::buffer(_sendMsg), [this](const boost::system::error_code& err, const size_t bytes_transferred)
            {
                this->OnWrite(err, bytes_transferred);
            });
    }

    void OnWrite(const boost::system::error_code& err, const size_t bytes_transferred)
    {
        if (!err)
        {
            std::cout << "OnWrite " << bytes_transferred << std::endl;
        }
        else
        {
            std::cout << "error code : " << err.value() << ", msg" << err.message() << std::endl;
        }
    }

    void AsyncRead()
    {
        memset(_recvBuffer, 0, RecvBufferSize);
        _socket.async_read_some(boost::asio::buffer(_recvBuffer), [this](const boost::system::error_code err, const size_t bytes_transferred) {
            this->OnRead(err, bytes_transferred);
            });
    }

    void OnRead(const boost::system::error_code& err, const size_t bytes_transferred)
    {
        std::string msg(_recvBuffer);
        std::cout << "OnRead size: " << bytes_transferred << ", msg : " << msg.c_str() << std::endl;
        if (!err)
        {
            AsyncRead();
        }
        else
        {
            std::cout << "error code : " << err.value() << ", msg" << err.message() << std::endl;
        }
    }


private:
    boost::asio::ip::tcp::socket _socket;
    static const int RecvBufferSize = 1024;
    char _recvBuffer[RecvBufferSize];
    std::string _sendMsg;
};

int main()
{
    boost::asio::io_context io_context;
    ChatClient client(io_context);
    client.Connect(std::string("127.0.0.1"), 4242);

    // 이 스레드가 Read/Write를 맡을 것임
    std::thread t1([&io_context]() { io_context.run(); });

    // Main 스레드는 cin을 담당할 것임
    char line[256];
    while (std::cin.getline(line, 256))
    {
        client.AsyncWrite(std::string(line));
        memset(line, 0, 256);
    }

    t1.join();
}

