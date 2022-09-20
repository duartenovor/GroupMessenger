# ERSG_class - Group menseger
This project implements a "Broadcasting Chat service" using some features, such as:
* TCP-IP Client/Server; 
* Posix threads;
* Message Queues;
* Daemons;
* Device Drivers.

## How it works?

Each Client is connected to the Server via TCP/IP. After the connection is established, each client can send to the server a character string passed by argument via command line. The server after receiving the message, it forwards the received messages to all connected Clients, and identifies the client that has sent the message. In order to check the status of each connected client, every 5 seconds the Server checks if each client is still ONLINE or AFK. (it is not shown on the terminal).

#### Interconnection

![alt text](https://github.com/JoaoMiranda-88237-UM/ESRG_class/blob/main/Images/Interconnection.png?raw=true)

## How to use?
### 1. Installation

#### 1.1 Clone this repository into your local machine.
```shell
$ git clone https://github.com/JoaoMiranda-88237-UM/ESRG_class
```
#### 1.2. Change working directory.
```shell
$ cd ESRG_class/
```
#### 1.3 Compile and create all object files
In project folder:
```Shell
$ make all CC_C=<client compiler> CC_S=<server compiler> 
```
**Example:**
* Compile server with **gcc**;
* Compile client to **Raspberry Pi (arm-linux-gcc)**.
```Shell
$ make all CC_C=arm-linux-gcc CC_S=gcc
```
#### 1.4 Copy to raspberry Pi
##### - Client 
To copy the client executable files (transfer_client):
```Shell
$ make transfer_client IP=<ip connection> FOLDER=<destination folder>
```
**Example:**
* IP = 10.42.0.174
* FOLDER = etc
```Shell
$ make transfer_client IP=10.42.0.174 FOLDER=etc
```
##### - Server
To copy the server executable files (transfer_server):
```Shell
$ make transfer_server IP=<ip connection> FOLDER=<destination folder>
```
**Example:**
* IP = 10.42.0.174
* FOLDER = etc
```Shell
$ make transfer_server IP=10.42.0.174 FOLDER=etc
```
##### - Server and Client
To copy both executable files (transfer_both): 
```Shell
$ make transfer_both IP=<ip connection> FOLDER=<destination folder>
```
**Example:**
* IP = 10.42.0.174
* FOLDER = etc
```Shell
$ make transfer_both IP=10.42.0.174 FOLDER=etc
```
#### 1.5 Remove object files
To remove all created object files (clean): 
```Shell
$ make clean
```
### 2. Execution 
#### 2.1 Check that you are in the right directory
```shell
$ pwd
/.../ESRG_class
```
#### 2.2 Start TCP server
Starts a TCP server on a given port.
```Shell
$ ./server <port>
```
#### 2.3 Start Daemon client
Starts a TCP client connected to a given server name on a given port.
```Shell
$ ./daemon <servername> <port> 
```
When this is running, a led (led0 - green led in Raspberry Pi) is light up. This is done via a device driver, developed in previous classes.
#### 2.4 Client - Send message / see received messages
Send message to server or to see messages that have been send to the client since last time. Everytime the user wants to send a message he must use client with the wanted message to be sent. 
```shell
$ ./client <msg[0}> <msg[1}> ... <msg[n]>
```
#### 2.5 Close connection
##### 2.5.1 On client
Type 'close', or use Ctrl+C.
```shell
$ ./tcpclient_send close
```
When this happens, the daemon terminates, and the led that was previously light up (led0) is turned off.

##### 2.5.2  On server
Type 'close', or use Ctrl+C.

## Done by
João Miranda, a88237 \
Duarte Rodrigues, a88259 \
Embedded Systems students @ Universidade do Minho, 2021
