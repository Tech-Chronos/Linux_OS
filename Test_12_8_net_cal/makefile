.PHONY:all
all:TcpServer TcpClient
TcpServer:TcpEchoServer.cc
	g++ -o $@ $^ -std=c++14 -ljsoncpp

TcpClient:TcpEchoClient.cc
	g++ -o $@ $^ -std=c++14 -ljsoncpp

.PHONY:clean
clean:
	rm -f TcpServer TcpClient