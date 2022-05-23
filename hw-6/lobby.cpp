#include <enet/enet.h>
#include <iostream>
#include <cstring>

#include "common.hpp"

struct room 
{
    std::string name;
    uint maxPlayers;
    uint difficulty;
    std::vector<uint> players;
};

struct gameServer
{
    uint port;
    ENetPeer* peer;
};

room initRoom(std::string name, uint maxPlayers, uint difficulty)
{
  room result;
  return room {
    .name = name,
    .maxPlayers = maxPlayers,
    .difficulty = difficulty};

}

message genRoomListMessage(std::vector<room> & rooms)
{
    message msg;
    msg.type = MATCHMAKER_TO_CLIENT_ROOM_LIST;
    msg.data = {std::to_string(rooms.size())};

    for (int i=0;i<rooms.size();i++)
    {
        msg.data.push_back(rooms[i].name);
        msg.data.push_back(std::to_string(rooms[i].players.size()));
        msg.data.push_back(std::to_string(rooms[i].maxPlayers));
        msg.data.push_back(std::to_string(rooms[i].difficulty));
    }
    return msg;
}

void printRooms(std::vector<room> & rooms)
{
    printf("\n");
    printf("ROOMS:\n");
    printf("name\t\t\tplayers\tdifficulty\n");
    for (int i=0;i<rooms.size();i++)
    {
      printf("%s\t\t\t%d/%u\t%d\n",rooms[i].name.c_str(),(int)rooms[i].players.size(),rooms[i].maxPlayers,rooms[i].difficulty);
    }        
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;


  address.host = ENET_HOST_ANY;
  address.port = matchmakerPort;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }
  else
  {
    printf("Lobby withMatchmaker started\n");

    printf("#######\n");
    printf("Command list:\n");
    printf("/start - Start room\n");
    printf("/create - Create room\n");
    printf("\n");
    printf("#######\n");
    printf("Press SPACE to start writing command\n");
    printf("\n");
  }

  std::vector<room> rooms;
  rooms.push_back(initRoom("Podpivas Gaming",3,2));
  rooms.push_back(initRoom("PRODAM GARAZH",4,5));
  rooms.push_back(initRoom("Casual for all",8,0));
  rooms.push_back(initRoom("My Server",3,3));

  std::vector<gameServer> readyServers;

  message msg;
  uint userId;

  std::string command;

  printRooms(rooms);

  while (true)
  {
    command = getCommand(int (' '));
    if (command == startRoomCommand)
    {
      int roomN;
      printf("Type desired room number from list:\n");
      scanf("%d",&roomN);
      printf("Starting room %d\n",roomN);

      if (roomN>=rooms.size())
      {

        printf("Room doesn't exist\n");
      }
      else if (readyServers.size()==0)
      {
        printf("ERROR: There are no ready servers. Please start a new server.\n");
      }
      else
      {
        gameServer gameServer = readyServers.back();
        readyServers.pop_back();
        msg.type = MATCHMAKER_TO_SERVER_START;
        msg.data = {std::to_string(rooms[roomN].difficulty)};
        send_message(&msg,gameServer.peer,0,true);

        std::vector<ENetPeer*> peers;

        for (int i=0;i<rooms[roomN].players.size();i++)
        {
          for (int j=0;j< server->connectedPeers; j++)
          if (server->peers[j].connectID == rooms[roomN].players[i])
            {
              peers.push_back(&server->peers[j]);
            }
        }

        for (int i=0;i<peers.size();i++)
        {
          msg.type = MATCHMAKER_TO_CLIENT_HOST_LINK;
          msg.data = {gameServerHost, std::to_string(gameServer.port)};
          
          
          send_message(&msg,peers[i],0,true);
        }

        rooms.erase(rooms.begin()+roomN);
        printf("ROOM STARTED\n");
        printRooms(rooms);
      }
    }
    if (command == createRoomCommand)
    {
      std::string name;
      int pl, d;
      char* ss = new char[12];
      printf("Type desired room name:\n");
      scanf("%128s",ss);
      name = ss;
      printf("Type max players number:\n");
      scanf("%d",&pl);
      printf("Type gamemode difficulty (0-8):\n");
      scanf("%d",&d);

      rooms.push_back(initRoom(name,pl,d));
      printf("ROOM ADDED\n");
      printRooms(rooms);
    }

    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);          

        break;
      case ENET_EVENT_TYPE_RECEIVE:
        userId = event.peer->connectID;

        data_to_message(event.packet->data,msg);

        if (msg.type == SERVER_TO_MATCHMAKER_READY)
        {
          uint newPort = stoi(msg.data[0]);
          gameServer ns;
          ns.peer = event.peer;
          ns.port = newPort;
          readyServers.push_back(ns);
          printf("New game server ready: %u\n",newPort);
        }
        if (msg.type == CLIENT_TO_MATCHMAKER_GET_ROOM_LIST)
        {
          msg = genRoomListMessage(rooms);
          send_message(&msg,event.peer,0,true);
        }
        else if (msg.type == CLIENT_TO_MATCHMAKER_JOIN_ROOM)
        {
          bool inRoom = false;
          for (int i=0;i<rooms.size();i++)
          {
            for (int j=0;j<rooms[i].players.size();j++)
            {
              if (rooms[i].players[j]==userId)
              {
                inRoom = true;
              }
            }
          }
          uint roomN = stoi(msg.data[0]);
          printf("Player %u trying to join room %u\n",userId,roomN);
          msg.type = MATCHMAKER_TO_CLIENT_MESSAGE;
          if (inRoom)
            {
              printf("Player is already in some room\n");
              msg.data = {"You're already in some room"};

            }
          else if (roomN>=rooms.size())
          {
            printf("Room doesn't exist\n");
            msg.data = {"Room doesn't exist"};
          }
          else if (rooms[roomN].players.size()>=rooms[roomN].maxPlayers)
          {
            printf("Room is full\n");
            msg.data = {"Room is full"};
          }
          else
          {                    
            printf("Player is added to the room\n");
            rooms[roomN].players.push_back(userId);
            msg.data = {"You're added to the room"};
            printRooms(rooms);
          }
          send_message(&msg,event.peer,0,true);
        }
        else if (msg.type == CLIENT_TO_MATCHMAKER_LEAVE_ROOM)
        {
          msg.type = MATCHMAKER_TO_CLIENT_MESSAGE;
          msg.data = {"You're not in any room"};
          for (int i=0;i<rooms.size();i++)
          {
            int pos = -1;
            for (int j=0;j<rooms[i].players.size();j++)
            { 
              if (rooms[i].players[j]==userId)
              {
                pos = j;
              }
            }
            if (pos!=-1)
            {
              rooms[i].players.erase(rooms[i].players.begin()+pos);
              msg.data = {"You left the room"};
            }
          }
          send_message(&msg,event.peer,0,true);
        }
        enet_packet_destroy(event.packet);
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