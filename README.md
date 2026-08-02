*This project has been created as part of the 42 curriculum by omgorege, yozlu, musisman.*

This project is an Internet Relay Chat (IRC) server developed from scratch in accordance with the C++98 standard. It is designed to be fully compatible with actual IRC clients (such as Irssi, HexChat, etc.).

# Description

## Development Team & Division of Labor

This project was developed by a team of three using a modular architecture:

* **Ömer Faruk Göregen (`omgorege`) - Server Core & Network Architecture:** TCP socket connections, `poll()` based event loop, non-blocking I/O operations, and memory/signal management.
* **Yusuf Taha Özlü (`yozlu`) - Client & Channel Data Structures:** Object-oriented data encapsulation, client authentication states, buffering mechanisms for partial data packets, and initial extraction logic.
* **Mustafa Ersin Şişman (`musisman`) - Command Engine & Protocol Logic:** Parsing raw text from clients according to RFC 2812 standards, managing channel operator privileges, messaging algorithms, and handling numeric replies.

---

## Technical Specifications & Constraints

In accordance with the 42 curriculum PDF requirements, our project strictly adheres to the following rules:

* **Standard:** Written entirely in C++98.
* **Network Infrastructure:** Communicates over the TCP/IP protocol. All sockets are set to non-blocking mode using `fcntl()` with `O_NONBLOCK`.
* **Concurrency:** No threads or multiple processes (`fork`) are used. All I/O operations are managed asynchronously (multiplexing) on a single thread using the `poll()` macro.
* **Reliability:** Partial or fragmented network packets will not crash the server; incoming data is buffered per client until a `\n` character is received.

---


# Instructions

## Build and Run

To compile the project, you must have `make` and a C++ compiler (`c++` or `clang++`) installed on your system.

**1. Clone the repository and compile:**

```bash
make

```

**2. Start the server:**

```bash
./ircserv <port> <password>

```

* `<port>`: The port number the server will listen on (e.g., `6667`).
* `<password>`: The connection password required for clients to join your network.

*Example:*

```bash
./ircserv 6667 12345

```

---

## Client Connection

Once the server is running, you can connect using a standard IRC client or Netcat.

**Connecting with Irssi:**

```bash
irssi
/connect 127.0.0.1 6667 12345

```

**Raw Connection with Netcat:**

```bash
nc 127.0.0.1 6667
PASS 12345
NICK <nickname>
USER <username> 0 * :<realname>

```

---

## Supported Commands

Our server fully supports the following commands in accordance with the RFC 2812 standard:

* **Authentication:** `PASS`, `NICK`, `USER`
* **Communication:** `PRIVMSG`, `PING`
* **Channel Operations:** `JOIN`, `PART`
* **Operator Commands:** `KICK`, `INVITE`, `TOPIC`, `MODE`

---

## Channel Modes

Our server implements the following channel modes, manageable by channel operators:

* `+i / -i`: Invite-only mode. Only users invited via the `/invite` command by an operator can join.
* `+t / -t`: Topic protection mode. Only channel operators can change the channel's `TOPIC`.
* `+k / -k`: Key (password) mode. Requires a specific password to join the channel. (Usage: `MODE #channel +k password`)
* `+l / -l`: User limit mode. Sets the maximum number of users allowed in the channel at one time.
* `+o / -o`: Operator privilege. Grants or revokes channel operator status for a specific user. (Usage: `MODE #channel +o <nick>`)

# 📚 References

During the development process of this project, the following official documentation, network programming guides, and standard documents (RFCs) were utilized:

## 1. IRC Protocol and RFC Standards
For the core architecture, command structures, and numeric replies of the IRC server, official standards were directly referenced:
*   **[RFC 2812 - Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812):** Our primary guide for the messaging format between the server and the client, mandatory commands (JOIN, PRIVMSG, KICK, etc.), and error codes.
*   **[Modern IRC Docs](https://modern.ircdocs.horse/):** A modern compilation explaining how classic RFCs work in harmony with today's IRC clients (like Irssi and HexChat) and listing numeric error codes much more clearly.

## 2. Network & Socket Programming
Resources we utilized while establishing the non-blocking socket structure and I/O Multiplexing (poll) logic within C++98 standards:
*   **[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/):** The essential reference explaining how sockets (socket, bind, listen, accept) work and the TCP/IP architecture.
*   **[Jacob Sorber - Network Programming Tutorials](https://youtube.com/playlist?list=PL9IEJIKnBJjH_zM5LnovnoaKlXML5qh17&si=k4gueCy-ucw2laQq):** A video series consulted specifically to understand the basics of sockets, secure data read/write (recv/send) operations over the network, and to build the TCP client-server model.
*   **Linux Man Pages:** Official documentation for the POSIX standard uses of system calls such as `man 2 socket`, `man 2 poll`, `man 2 fcntl`, `man 2 recv`, and `man 2 send`.

## 3. C++98 and Development Tools
*   **[cplusplus.com](https://cplusplus.com/) / [cppreference.com](https://en.cppreference.com/):** Referenced documentation for STL containers (Map, Vector) and C++98 compatibility.
*   **[Irssi Documentation](https://irssi.org/documentation/):** Documentation used to test our server's communication with a real IRC client, its format expectations (Full Hostmask), and synchronization.