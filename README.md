# iwakura

A headless dashboard core written in C with the capability to talk to multiple frontends, in any language and for any target.

I built this because I wanted a decoupled system where the backend doesn't care about the UI, and the UI doesn't have to be a resource heavy web browser. It’s essentially a local pub/sub system designed to run 24/7 on low-powered hardware without bloating the process list.

Instead of a single binary, every module is its own independent one. If a fetcher crashes, the rest of the dashboard stays up. It handles all the fetching and caching in the background and streams the final state over a TCP socket, so it is actually super fast and you can translate that into any other data structure.


## Architecture

Iwakura is essentially a local Pub/Sub system. It is broken down into five distinct layers that communicate over local sockets:

1. **Servers (`srv_*`)**: These are the fetchers. They do the heavy lifting of talking to external APIs and push the raw data forward.
2. **Data Hub (`srv_hub`)**: The central cache. Its only job is to hold the last known good state of your data. If your internet drops or an API rate-limits you, the dashboard doesn't crash, the hub just serves the cached data until things are back.
3. **Panels (`pan_*`)**: The logic layer. Panels pull raw data from the Hub, calculate what needs to be displayed, and format it into a universal pipe-delimited language (for example `CLOCK|18:00|Monday`).
4. **Panel Hub (`pan_hub`)**: The broadcast tower. It catches the strings from the panels via UDP and broadcasts them over a TCP socket to any connected frontend client.
5. **Frontends (`front_*`)**: The actual image the user sees. Frontends are incredibly dumb by design. They connect to the panel hub, listen to the pipe string, and draw it. This is where I hope the cooler designs have a chance to shine.

Because the backend just sendds text over a TCP port, you can write a frontend in any language. For now I only wrote a terminal dashboard at `front_tui`, but you can easily build a web dashboard, a mobile app, or a desktop GUI that plugs right into the same stream. I am currently working on making a web based one, so translating the panel's stream to json is also an objective.

## Setup
You need a C compiler, `make`, `libcurl` and `libjson-c` for the API servers.

```bash
git clone https://github.com/willmanduran/iwakura
cd iwakura
chmod +x install.sh
./install.sh
```

The installer will ask if you want the backend or the frontend installed in that particular machine. In the backend it will setup the `systemd` units for you.


**In the backend:**
Once installed, edit `.env` to add your API keys, credentials, and preferences. I added a really detailed explanation on .env.example so you can follow along.


After updating .env, just run `systemctl --user restart iwakura.service`


You can check the logs at `tail -f ~/.iwakura/iwakura.log`


**In the frontend:**

Run `./front_tui`. It’ll snap to your terminal size and wait for the stream. I am working on adding the web interface now :)

## Roadmap
This is my favorite project, and probably the most fun I've had in a while, and I plan to keep maintaining and expanding it since I actually use it daily. Short term goals are:

* **Installation helper (Done!)**: A script to let you selectively compile and boot only the services you actually want to use, instead of launching the whole suite.
* **Protocol standardization**: PLEASE critique my API structure for the pipe language, I want it to be super easy so anyone can seamlessly drop in new modules and frontends while keeping consistency in naming conventions and whatnot. I will go through this anyway as I build the web frontend.
* **Security hardening**: This was originally designed to run safely on a local home network (for context I used a 2015 hp notebook as server, and a bunch of Raspberry Pis for the frontends). I understand people might want to host this on VMs or public facing servers, so keep in mind I have not tested yet the network security of this so think twice about making a public dashboard yet, keep it in local network unless you know exactly what you are doing.
* **More frontends**: I will be building a web-based client next, and I highly encourage others to build and share their own.
* **True server-frontend decoupling (Done!)**: Making it easier for casual users to install just the server side on machine X and then deploy as many frontends as they want on multiple other machines.


## Support & Contact
If you find any bugs, have ideas to improve this, or want to chat about programming in general, feel free to reach out. Be patient though, I am not that good outside the C world.

Email: ticuette@gmail.com

Other tools I have released: [willmanstoolbox.com](https://willmanstoolbox.com)


If you want to support the time and passion I pour into this, feel free to got to my sponsor button or https://www.willmanstoolbox.com/donate/ to support this and my other projects directly. Thank you for making it this far!