#pragma once
#include <vector>
#include <enet/enet.h>

#include <termios.h>
#include <unistd.h>

int khbit();
void nonblock(int state);
bool keyState(int key);
std::string getCommand(int pauseKey);

const char* startRoomCommand = "/start";
const char* createRoomCommand = "/create";
const char* roomsCommand = "/rooms";
const char* joinCommand = "/join";
const char* leaveCommand = "/leave";
const char* gameServerHost = "localhost";
const char* matchmakerHost = "localhost";
const int matchmakerPort = 10887;


struct addrinfo;



enum MessageType { 
    CLIENT_TO_MATCHMAKER_GET_ROOM_LIST,
    CLIENT_TO_MATCHMAKER_JOIN_ROOM,
    CLIENT_TO_MATCHMAKER_LEAVE_ROOM,
    
    MATCHMAKER_TO_CLIENT_ROOM_LIST,
    MATCHMAKER_TO_CLIENT_HOST_LINK,
    MATCHMAKER_TO_CLIENT_MESSAGE,

    SERVER_TO_MATCHMAKER_READY,
    MATCHMAKER_TO_SERVER_START,

    SERVER_TO_CLIENT_INIT,
    SERVER_TO_CLIENT_NEW_CONNECTED,

    CLIENT_TO_SERVER_UPDATE,

    SERVER_TO_CLIENT_UPDATE,
    SERVER_TO_CLIENT_PING_LIST
     };

struct message
{
  MessageType type;
  std::vector<std::string> data;

};

void send_message(message * msg, ENetPeer * peer,uint channel,bool isReliable);
void data_to_message(enet_uint8 * data, message &msg);

int khbit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

void nonblock(int state)
{
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);

    if ( state == 1)
    {
        ttystate.c_lflag &= (~ICANON & ~ECHO); 
        ttystate.c_cc[VMIN] = 1;
    }
    else if (state == 0)
    {
        ttystate.c_lflag |= ICANON | ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

bool keyState(int key)
{
    bool pressed = false;
    int i = khbit();
    if (i != 0)
    {
        char c = fgetc(stdin);
        if (c == (char) key)
        {
            pressed = true;
        }
        else
        {
            pressed = false;
        }
    }

    return pressed;
}

std::string getCommand(int pauseKey)
{

    std::string command = "";
    bool isInput = false;
    nonblock(1);
    if (keyState(pauseKey))
        isInput = true;
    nonblock(0);
    if (isInput)
    {
        char* ss;
        printf("Type command=> $");
        scanf("%s",ss);
        command = ss;
    }

    return command;
}

void send_message(message * msg, ENetPeer * peer,uint channel,bool isReliable)
{   

    uint sz = 2;

    for (int i=0;i<msg->data.size();i++)
        sz+=msg->data[i].length()+1;
    
    enet_uint8 *arr = new enet_uint8[sz];
    arr[0] = msg->type;
    arr[1] = msg->data.size();
    uint k = 2;

    for (int i=0;i<msg->data.size();i++)
    {
        arr[k] = msg->data[i].length();
        k+=1;

        for (int j=0;j<msg->data[i].length();j++)
        {
            arr[k] = msg->data[i][j];
            k+=1;
        }

    }

    ENetPacket *packet = enet_packet_create(arr, sz, isReliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(peer, channel, packet);
}

void data_to_message(enet_uint8 * data, message &msg)
{

    msg.type = (MessageType)data[0];
    msg.data = {};

    uint vectorSize = data[1];
    uint k = 2;
    while (vectorSize>0)
    {
        uint strLen = data[k];
        msg.data.push_back(std::string(&data[k+1], &data[k+strLen+1]));
        k+=strLen+1;
        vectorSize-=1;
    }

}
