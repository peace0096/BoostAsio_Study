// EchoClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <boost/asio.hpp>
using namespace std;

typedef boost::asio::ip::tcp::socket Socket;

int main()
{
    boost::asio::io_context io_context;
    Socket socket(io_context);
    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1", ec), 4242);

    if (ec)
    {
        cout << "endpoint error : " << ec.message() << endl;
        return 0;
    }
    socket.connect(endpoint, ec);

    if (!ec)
    {
        cout << "Connected!" << endl;
    }
    else
    {
        cout << "Connect Error : " << ec.message() << endl;
        return 0;
    }

    // 소켓 전송
    string request = "Hello, World";
    socket.write_some(boost::asio::buffer(request.data(), request.size()), ec);
    if (ec)
    {
        cout << "socket.write_some error : " << ec.message() << endl;
        return 0;
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);

    size_t size = socket.available();   // 받아올 사이즈 미리 구하기
    std::cout << "read available : " << size << endl;

    std::vector<char> recvBuffer(size);
    socket.read_some(boost::asio::buffer(recvBuffer.data(), recvBuffer.size()), ec);

    for (auto c : recvBuffer)
        cout << c;

    cout << "Terminated\n";

}
