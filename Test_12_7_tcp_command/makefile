.PHONY:all
all:TcpServer TcpClient
TcpServer:TcpEchoServer.cc
	g++ -o $@ $^ -std=c++14

TcpClient:TcpEchoClient.cc
	g++ -o $@ $^ -std=c++14

.PHONY:clean
clean:
	rm -f TcpServer TcpClient