
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind/bind.hpp>

using boost::asio::ip::tcp;
using namespace boost;
using namespace std;

int main()
{
    boost::asio::io_context io_context;
    tcp::endpoint endpoint = tcp::endpoint(tcp::v4(), 4242);

    cout << "server start in " << endpoint.port() << endl;

    while (true)
    {
        tcp::acceptor acceptor(io_context, endpoint);
        tcp::socket socket(io_context);
        system::error_code ec;
        acceptor.accept(socket, ec);

        if (!ec)
        {
            cout << "accpeted" << endl;
        }
        else
        {
            cout << "accpeted error : " << ec.message() << endl;
            return 0;
        }

        // Accept 이후로 계속 받아와야 함.

        while (true)
        {
            char recvBuffer[32] = { 0, };
            size_t receivedSize = socket.read_some(boost::asio::buffer(recvBuffer, 32), ec);
            if (!ec)
            {
                cout << "Recv size : " << receivedSize << endl;
            }
            else
            {
                cout << "Recv error : " << ec.message() << endl;
                break;
            }

            size_t sentSize = socket.write_some(boost::asio::buffer(recvBuffer, 32), ec);
            if (!ec)
            {
                cout << "Send size : " << sentSize << endl;
            }
            else
            {
                cout << "Send error : " << ec.message() << endl;
                break;
            }


        }

    }

    
    

}
