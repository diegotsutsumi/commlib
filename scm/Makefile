CXXFLAGS += -std=c++11
exec: p2p_client2 p2p_client p2p_server snitch_notifier scm_server scm_client
debug: CXXFLAGS += -g -ggdb -O0 -Wall
debug: exec

p2p_client_objects = p2p_client.o P2PMessage.o Socket.o
p2p_client2_objects = p2p_client2.o P2PMessage.o Socket.o
p2p_server_objects = p2p_server.o P2PRequestServer.o P2PConnection.o P2PMessage.o Socket.o
snitch_notifier_objects = snitch_notifier.o SnitchNotifier.o NotificationSender.o NotificationReceiver.o SnitchNotification.o Socket.o
scm_server_objects = scm_server.o ServerSCM.o ActiveSnitch.o SCMRouter.o SCMPacket.o Socket.o
scm_client_objects = scm_client.o ClientSCM.o SCMRouter.o SCMPacket.o Socket.o

all : exec

p2p_client: $(p2p_client_objects)
	g++ -o p2p_client $(p2p_client_objects) -pthread

p2p_client2: $(p2p_client2_objects)
	g++ -o p2p_client2 $(p2p_client2_objects) -pthread

p2p_server: $(p2p_server_objects)
	g++ -o p2p_server $(p2p_server_objects) -pthread

snitch_notifier: $(snitch_notifier_objects)
	g++ -o snitch_notifier $(snitch_notifier_objects) -pthread

scm_server: $(scm_server_objects)
	g++ -o scm_server $(scm_server_objects) -pthread

scm_client: $(scm_client_objects)
	g++ -o scm_client $(scm_client_objects) -pthread

Socket: Socket.cpp
ServerSCM: ServerSCM.cpp
ActiveSnitch: ActiveSnitch.cpp
SCMPacket: SCKPacket.cpp
ClientSCM: ClientSCM.cpp
SCMRouter: SCMRouter.cpp
SnitchNotifier: SnitchNotifier.cpp
NotificationSender: NotificationSender.cpp
NotificationReceiver: NotificationReceiver.cpp
SnitchNotification: SnitchNotification.cpp
P2PConnection: P2PConnection.cpp
P2PMessage: P2PMessage.cpp
P2PRequestServer: P2PRequestServer.cpp
p2p_clint: p2p_client.cpp
p2p_clint: p2p_client2.cpp
p2p_server: p2p_server.cpp
snitch_notifier: snitch_notifier.cpp
scm_server: scm_server.cpp
scm_client: scm_client.cpp

clean:
	rm -f *.o p2p_client p2p_client2 p2p_server snitch_notifier scm_server scm_client
