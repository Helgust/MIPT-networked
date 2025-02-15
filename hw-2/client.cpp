#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <string>

int main(int argc, const char **argv)
{
  srand(time(nullptr));
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  uint32_t startMatchTime = 0;
  if (argc == 2)
  {
    printf("Start session after %s seconds\n", argv[1]);
    startMatchTime = atoi(argv[1]) * 1000;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &address, 2, 0);
  ENetPeer *serverPeer = nullptr;
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  bool connected = false;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        if (!serverPeer && event.peer->address.port == address.port) //==
        {
          char *buffer = (char *)event.packet->data;
          char *save;
          char *host = strtok_r(buffer, " ", &save);
          char *port = strtok_r(nullptr, " ", &save);
          ENetAddress serverAddress;
          enet_address_set_host(&serverAddress, host);
          serverAddress.port = atoi(port);
          serverPeer = enet_host_connect(client, &serverAddress, 2, 0);
          startMatchTime = 0;
          if (!serverPeer)
          {
            printf("Cannot connect to server\n");
            return 1;
          }
        }
        printf("Packet received '%s' From:%x\n", event.packet->data, event.peer->address.host);
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (connected)
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastMicroSendTime > 1000 && serverPeer)
      {
        printf("Send time\n");
        lastMicroSendTime = curTime;
        const std::string msg = std::to_string(curTime + rand() % 200);
        ENetPacket *packet = enet_packet_create(msg.c_str(), strlen(msg.c_str()), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(serverPeer, 1, packet);

      }
      if (curTime - timeStart > startMatchTime && startMatchTime > 0)
      {
        printf("Send start\n");
        const char *msg = "start";
        ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(lobbyPeer, 0, packet);
        startMatchTime = 0;
      }
    }
  }
  return 0;
}