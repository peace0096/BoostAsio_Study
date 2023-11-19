#include <iostream>
#include <boost/asio.hpp>

static boost::asio::chrono::milliseconds gTimerDelay(200);
const int MaxCount = 500;

class Printer
{
public:
    // 초기화
    Printer(boost::asio::io_context& io)
        : strand_(boost::asio::make_strand(io)), _timer1(io, gTimerDelay), _timer2(io, gTimerDelay), _count(0)
    {
        _timer1.async_wait(boost::asio::bind_executor(strand_, [this](boost::system::error_code err)
            {
                this->Print1();
            }));
        _timer2.async_wait(boost::asio::bind_executor(strand_, [this](boost::system::error_code err)
            {
                this->Print2();
            }));
    }

    ~Printer()
    {
        std::cout << "Final count is " << _count << std::endl;
    }

    void Print1()
    {
        if (_count < MaxCount)
        {
            std::cout << "Timer1 : " << _count << ", id: " << std::this_thread::get_id() << std::endl;
            ++_count;

            // 객체의 만료시간 재정의(gTimerDelay만큼 늘린다)
            _timer1.expires_at(_timer1.expiry() + gTimerDelay);
            _timer1.async_wait(boost::asio::bind_executor(strand_, [this](boost::system::error_code err)
                {
                    this->Print1();
                }));
        }
    }

    void Print2()
    {
        if (_count < MaxCount)
        {
            std::cout << "Timer2 : " << _count << ", id: " << std::this_thread::get_id() << std::endl;

            // 객체의 만료시간 재정의(gTimerDelay만큼 늘린다)
            _timer2.expires_at(_timer1.expiry() + gTimerDelay);
            _timer2.async_wait(boost::asio::bind_executor(strand_, [this](boost::system::error_code err)
                {
                    this->Print2();
                }));
        }
    }

private:
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::steady_timer _timer1;
    boost::asio::steady_timer _timer2;
    int _count;

};


int main()
{
    boost::asio::io_context io;
    Printer p(io);

    std::thread t1([&io]() { io.run(); });
    std::thread t2([&io]() { io.run(); });

    t1.join();
    t2.join();

    return 0;
}
