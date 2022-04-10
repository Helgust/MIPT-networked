#include <enet/enet.h>
#include <iostream>
#include <cstring>

int main(int argc, const char **argv)
{
  bool is_started = false;
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10887;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        if (is_started)
        {
          const char *msg = "localhost 10007";
          ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, packet);
        }
          

        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        if (!strcmp((char *)event.packet->data, "start"))
        {
          is_started = true;
          enet_packet_destroy(event.packet);
          for (int i = 0; i < server->connectedPeers; ++i)
          {
            const char *msg = "localhost 10007";
            ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(&server->peers[i], 0, packet);
          }
        }
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}