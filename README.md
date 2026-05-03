# iwakura

Iwakura is a headless, decoupled dashboard architecture written in pure C.

In plain english, it's a dashboard focused on being super easy to customize both in the back and the frontend.
It comes with the news, a music and music stats visualizer, the weather, a clock, a calendar and a small guestbook.


The goal of this project is to offer a super easy to install dashboard for the casual users, while also being a scaffold for builders. You can use it as-is, but it's really meant to be a playground where you can let your creativity shine by writing your own custom data modules or building much cooler frontends than I ever could since I am definitely not a UX designer.

## Architecture

Iwakura is essentially a local Pub/Sub system. It is broken down into five distinct layers that communicate over local sockets:

1. **Servers (`srv_*`)**: These are the fetchers. They do the heavy lifting of talking to external APIs and push the raw data forward.
2. **Data Hub (`srv_hub`)**: The central cache. Its only job is to hold the last known good state of your data. If your internet drops or an API rate-limits you, the dashboard doesn't crash, the hub just serves the cached data until things are back.
3. **Panels (`pan_*`)**: The logic layer. Panels pull raw data from the Hub, calculate what needs to be displayed, and format it into a universal pipe-delimited language (for example `CLOCK|18:00|Monday`).
4. **Panel Hub (`pan_hub`)**: The broadcast tower. It catches the strings from the panels via UDP and broadcasts them over a TCP socket to any connected frontend client.
5. **Frontends (`front_*`)**: The actual image the user sees. Frontends are incredibly dumb by design. They connect to the panel hub, listen to the pipe string, and draw it. This is where I hope the cooler designs have a chance to shine.

Because the backend just sendds text over a TCP port, you can write a frontend in any language. For now I only wrote a terminal dashboard at `front_tui`, but you can easily build a web dashboard, a mobile app, or a desktop GUI that plugs right into the same stream. I am currently working on making a web based one, so translating the panel's stream to json is also an objective.

## Getting Started

### Prerequisites
You need a C compiler, `make`, `libcurl` and `libjson-c` for the API servers.

### Building
Clone the repository and compile the ecosystem:

```bash
make clean && make
```

Configure your environment variables by copying the example file:
```bash
cp .env.example .env
```
Edit `.env` with your API keys, coordinates, and preferred ports. (For now this also customizes the guestbook webpage at a VERY basic level, I will add a separate html doc just so this is more customizable)

### Running the Core
Boot the master daemon:
```bash
./iwakura
```
This launches the hub, servers, panels, and broker in the background. It will keep running quietly until you kill it.

### Connecting a Frontend
To view the dashboard in your terminal, open a new window and run:
```bash
./front_tui
```

If you just want to see the raw data matrix flowing in real-time, you don't even need a frontend. Just listen to the TCP broadcast:
```bash
nc 127.0.0.1 8891
```

## Roadmap

This is my favorite project, and I plan to maintain and expand it. Current priorities include:

* **Installation helper**: A script to let you selectively compile and boot only the services you actually want to use, instead of launching the whole suite.
* **Protocol standardization**: PLEASE critique my API structure for the pipe language, I want it to be super easy so anyone can seamlessly drop in new modules and frontends while keeping consistency in naming conventions and whatnot.
* **Security hardening**: This was originally designed to run safely on a local home network (for context I used a 2015 hp notebook as server, and a bunch of Raspberry Pis for the frontends). I understand people might want to host this on VMs or public-facing servers, so tightening socket security and data validation is on the list.
* **More frontends**: I will be building a web-based client next, and I highly encourage others to build and share their own.
* **True server-frontend decoupling**: Making it easier for casual users to install just the server side on machine X and then deploy as many frontends as they want on multiple other machines.
## License
MIT License. Do whatever you want with it.


## Support & Contact
If you find any bugs, have ideas to improve this, or want to chat about programming in general, feel free to reach out. Be patient though, I am not that good outside the C world.

Email: ticuette@gmail.com 
Other tools I have released: willmanstoolbox.com

If you want to support the time and passion I pour into this, feel free to got to my sponsor button or https://www.willmanstoolbox.com/donate/ to support this and my other projects directly. Thank you for making it this far!