*This project has been created as part of the 42 curriculum by omgorege, yozlu, musisman.*

This project is an Internet Relay Chat (IRC) server developed from scratch in accordance with the C++98 standard. It is designed to be fully compatible with actual IRC clients (such as Irssi, HexChat, etc.).

## 👨‍💻 Development Team & Division of Labor

This project was developed by a team of three using a modular architecture:

* **Ömer Faruk Göregen (`omgorege`) - Server Core & Network Architecture:** TCP socket connections, `poll()` based event loop, non-blocking I/O operations, and memory/signal management.
* **Yusuf Taha Özlü (`yozlu`) - Client & Channel Data Structures:** Object-oriented data encapsulation, client authentication states, buffering mechanisms for partial data packets, and initial extraction logic.
* **Mustafa Ersin Şişman (`musisman`) - Command Engine & Protocol Logic:** Parsing raw text from clients according to RFC 2812 standards, managing channel operator privileges, messaging algorithms, and handling numeric replies.

---

## ⚙️ Technical Specifications & Constraints

In accordance with the 42 curriculum PDF requirements, our project strictly adheres to the following rules:

* **Standard:** Written entirely in C++98.
* **Network Infrastructure:** Communicates over the TCP/IP protocol. All sockets are set to non-blocking mode using `fcntl()` with `O_NONBLOCK`.
* **Concurrency:** No threads or multiple processes (`fork`) are used. All I/O operations are managed asynchronously (multiplexing) on a single thread using the `poll()` macro.
* **Reliability:** Partial or fragmented network packets will not crash the server; incoming data is buffered per client until a `\n` character is received.

---

## 🚀 Build and Run

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

## 📡 Client Connection

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

## 🛠️ Supported Commands

Our server fully supports the following commands in accordance with the RFC 2812 standard:

* **Authentication:** `PASS`, `NICK`, `USER`
* **Communication:** `PRIVMSG`, `PING`
* **Channel Operations:** `JOIN`, `PART`
* **Operator Commands:** `KICK`, `INVITE`, `TOPIC`, `MODE`

---

## 🔐 Channel Modes

Our server implements the following channel modes, manageable by channel operators:

* `+i / -i`: Invite-only mode. Only users invited via the `/invite` command by an operator can join.
* `+t / -t`: Topic protection mode. Only channel operators can change the channel's `TOPIC`.
* `+k / -k`: Key (password) mode. Requires a specific password to join the channel. (Usage: `MODE #channel +k password`)
* `+l / -l`: User limit mode. Sets the maximum number of users allowed in the channel at one time.
* `+o / -o`: Operator privilege. Grants or revokes channel operator status for a specific user. (Usage: `MODE #channel +o <nick>`)