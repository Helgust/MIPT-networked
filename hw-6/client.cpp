#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <string>

#include "./common.hpp"

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, matchmakerHost);
  address.port = matchmakerPort;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to matchmaker\n");
    return 1;
  }
  else
  {
    printf("Connecting to matchmaker\n");
  }

  uint32_t timeStart = enet_time_get();

  bool inMatchmaker = false;
  bool inGame = false;


  uint32_t lastUpdateSendTime = timeStart;
  bool connected = false;

  std::string myName;
  uint myId;
  message msg = {};

  std::string command;

  while (true)
  {
    command = getCommand(int (' '));

    if (command == roomsCommand)
    {
        printf("Refreshing room list\n");
        msg.type = CLIENT_TO_MATCHMAKER_GET_ROOM_LIST;
        msg.data = {};
        send_message(&msg,serverPeer,0,true);

    }
    else if (command == joinCommand)
    {
        int i;
        printf("Type desired room number from list:\n");
        scanf("%d",&i);
        printf("Joining room %d\n",i);
        msg.type = CLIENT_TO_MATCHMAKER_JOIN_ROOM;
        msg.data = {std::to_string(i)};
        send_message(&msg,serverPeer,0,true);
    }
    else if (command == leaveCommand)
    {
        
        printf("Leaving current room\n");
        msg.type = CLIENT_TO_MATCHMAKER_LEAVE_ROOM;
        msg.data = {};
        send_message(&msg,serverPeer,0,true);
    }


    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        if (event.peer->address.port == matchmakerPort)
        {
          inMatchmaker = true;
          printf("Connected to matchmaker\n");
          printf("~~~~~\n");
          printf("Command list:\n");
          printf("/rooms - Get room list\n");
          printf("/join - Select room\n");
          printf("/leave - Leave room\n");
          printf("\n");
          printf("Press SPACE to start writing command\n");
          printf("~~~~~\n");
          msg.type = CLIENT_TO_MATCHMAKER_GET_ROOM_LIST;
          msg.data = {};
          send_message(&msg,serverPeer,0,true);
        }
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
      	data_to_message(event.packet->data,msg);
      	if (inMatchmaker)
        {
            if (msg.type == MATCHMAKER_TO_CLIENT_HOST_LINK)
            {
      
                enet_address_set_host(&address, msg.data[0].c_str());
                address.port = std::atoi(msg.data[1].c_str());

                serverPeer = enet_host_connect(client, &address, 2, 0);
                if (!serverPeer)
                {
                  printf("Cannot connect to server\n");
                  return 1;
                }
                else
                {
                  printf("Connected to server\n");
                  inMatchmaker = false;
                  inGame = true;
                }
            }
            else if (msg.type == MATCHMAKER_TO_CLIENT_MESSAGE)
            {
                printf("MATCHMAKER: %s\n",msg.data[0].c_str());
            }
            else if (msg.type == MATCHMAKER_TO_CLIENT_ROOM_LIST)
            {
                int n = std::stoi(msg.data[0]);
                printf("ROOM LIST:\n");
                printf("name\t\t\t\tplayers\tdifficulty\n");
                for (int i=0;i<n;i++)
                {
                    std::string name = msg.data[1+i*4];
                    uint pl = std::stoi(msg.data[2+i*4]);
                    uint total = std::stoi(msg.data[3+i*4]);
                    uint diff = std::stoi(msg.data[4+i*4]);
                    printf("%s\t\t\t\t%u/%u\t%u\n",name.c_str(),pl,total,diff);
                }
            }

        }
        else if (inGame)
        {
            if (msg.type == SERVER_TO_CLIENT_INIT)
            {
                myId = std::stoi(msg.data[0]);
                myName = msg.data[1];
                printf("My ID: %u; My name: %s\n",myId,myName.c_str());
                printf("Other players (total %lu):\n",(msg.data.size()-2)/2);
                for (int i=0;i<(msg.data.size()-2)/2;i++)
                    printf("(%s) [%s]\n",msg.data[3+2*i].c_str(),msg.data[2+2*i].c_str());

            }
            else if (msg.type == SERVER_TO_CLIENT_NEW_CONNECTED)
            {
                printf("New Player connected (%s) [%s]\n",msg.data[1].c_str(),msg.data[0].c_str());
            }
            else if (msg.type == SERVER_TO_CLIENT_UPDATE)
            {
                printf("Server time (%s)\n",msg.data[0].c_str());
            }
            else if (msg.type == SERVER_TO_CLIENT_PING_LIST)
            {
                printf("PING LIST (id:ping): ");
                for (int i=0;i<(msg.data.size())/2;i++)
                    printf("(%s:%s)",msg.data[2*i].c_str(),msg.data[2*i+1].c_str());
                printf("\n");
            }
        }
        enet_packet_destroy(event.packet); 
        break;
      default:
        break;
      };
    }
    if (connected && inGame)
    {
      uint32_t curTime = enet_time_get();

      if (curTime - lastUpdateSendTime > 1000)
      {
        lastUpdateSendTime = curTime;
        msg = {.type = CLIENT_TO_SERVER_UPDATE,.data = {std::to_string(curTime-timeStart)}};
        printf("SENT MESSAGE TO SERVER\n");
        send_message(&msg,serverPeer,0,true);
      }
    }
  }
  return 0;
}