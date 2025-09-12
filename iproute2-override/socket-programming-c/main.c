int main() { return 0; }

/*

https://www.linuxhowtos.org/C_C++/socket.htm

SOCKETS:
- between two processes
- both sides can send/receive information
- **socket**: one end of an interprocess communication channel
- client side steps:
  - create a socket with `socket()` system call
  - connect the socket to the address of the server using the `connect()` system
call
  - send and recieve data (simplest ways is with `read()` and `write()`
syscalls)
- server side steps:
  - create a socket with `socket()` system call
  - bind socket to an address with `bind()` system call
  - listen for connections with `listen()` system call
  - accept connection with `accept()` system call (blocks until a client
connects with the server)
  - send and recieve data


SOCKET TYPES:
- to create a socket, two things are needed: address domain + socket type
- two processes can communicate if sockets are of same type within same domain
- two widely used address domains (both have different address formats)
  - unix domain: two processes in same file system
    - address is a character string (entry in filesystem)
  - Internet domain: two processes on any two hosts on the Internet
    - address is IP address + port on host
- two types of sockets (each uses its own communications protocol)
  - stream sockets:
    - communications are a continous stream of characters
    - use TCP (stream oriented, reliable)
  - datagram sockets:
    - have to read entire messages at once
    - use UDP (message oriented, unreliable)

*/
