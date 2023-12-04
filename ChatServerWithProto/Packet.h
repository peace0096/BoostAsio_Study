#include <google/protobuf/message.h>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>

using namespace boost;
using namespace std;

enum
{
	TEST_PACKET = 1
};

struct PacketHeader
{
	short Length;
	short Code;
};

class PacketUtil
{
public:
	static size_t RequiredSize(const google::protobuf::Message& msg)
	{
		return sizeof(PacketHeader) + msg.ByteSizeLong();
	}

	static bool Serialize(const asio::mutable_buffer& buffer, const short packetCode, const google::protobuf::Message& msg)
	{
		const size_t requiredSize = RequiredSize(msg);
		if (buffer.size() < requiredSize)
			return false;

		PacketHeader header;
		header.Length = static_cast<short>(msg.ByteSizeLong());
		header.Code = packetCode;

		memcpy(buffer.data(), &header, sizeof(PacketHeader));

		// buffer.data() : 헤더의 시작점
		// buffer.data()) + sizeof(PacketHeader) : 헤더의 끝점 = payload 시작점
		char* payloadPtr = static_cast<char*>(buffer.data()) + sizeof(PacketHeader);
		if (!msg.SerializeToArray(payloadPtr, static_cast<int>(buffer.size()) - sizeof(PacketHeader)))
			return false;

		return true;
	}

	static bool ParseHeader(const asio::mutable_buffer& buffer, PacketHeader* header, int& offset)
	{
		// 읽고 있는 부분이 버퍼 바깥으로 나갈 경우
		if (buffer.size() <= offset)
			return false;

		// 남은 버퍼 데이터가 헤더 사이즈보다 작을 경우
		const size_t remainedSize = buffer.size() - offset;
		if (remainedSize < sizeof(PacketHeader))
			return false;

		const char* headerPtr = static_cast<char*>(buffer.data()) + offset;
		memcpy(header, headerPtr, sizeof(PacketHeader));
		offset += sizeof(PacketHeader);

		return true;
	}

	// payloadSize : 헤더를 제외한 사이즈
	// offset : 헤더 이후부터 시작되는 payload 시작점
	static bool Parse(google::protobuf::Message& msg, const asio::mutable_buffer& buffer, const int payloadSize, int& offset)
	{
		if (buffer.size() < sizeof(PacketHeader))
			return false;

		const char* payloadPtr = static_cast<char*>(buffer.data()) + offset;
		const size_t remainedSize = buffer.size() - offset;
		const bool parseResult = msg.ParseFromArray(payloadPtr, payloadSize);
		if (parseResult)
		{
			offset += static_cast<int>(msg.ByteSizeLong());
			return true;
		}
		else
		{
			return false;
		}
	}
};

//int main()
//{
//	// 직렬화
//	Protocol::TEST test;
//	test.set_hp(10);
//	test.set_id(100);
//
//	const size_t requiredSize = PacketUtil::RequiredSize(test);
//	char* rawBuffer = new char[requiredSize];
//	const auto buffer = asio::buffer(rawBuffer, requiredSize);
//	PacketUtil::Serialize(buffer, TEST_PACKET, test);
//
//	// 역직렬화
//	PacketHeader header;
//	char* PacketBuffer = new char[4];
//	int offset = 0;
//	memcpy(PacketBuffer, rawBuffer, 4);
//	auto combinedBuffer = asio::buffer(PacketBuffer, 4);
//	PacketUtil::ParseHeader(combinedBuffer, &header, offset);
//	cout << header.Length << endl;
//	cout << header.Code << endl;
//	delete[] PacketBuffer;
//
//	PacketBuffer = new char[header.Length + sizeof(PacketHeader)];
//	memcpy(PacketBuffer, rawBuffer, offset + header.Length);
//	combinedBuffer = asio::buffer(PacketBuffer, sizeof(PacketHeader));
//
//	Protocol::TEST test2;
//	PacketUtil::Parse(test2, combinedBuffer, header.Length, offset);
//
//	std::cout << test2.hp() << endl;
//
//	return 0;
//}