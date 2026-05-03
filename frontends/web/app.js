let socket;
let musicInterval = null;
let currentProgMs = 0;
let durMs = 0;
let currentTrackId = "";

function connectWebSocket() {
    const WS_PORT = window.IWAKURA_CONFIG.WS_PORT;
    const wsUrl = `ws://${window.location.hostname}:${WS_PORT}`;
    socket = new WebSocket(wsUrl);
    socket.onmessage = (event) => processStream(event.data);
    socket.onclose = () => setTimeout(connectWebSocket, 5000);
}

function processStream(payload) {
    const IPC_SECRET = window.IWAKURA_CONFIG.IPC_SECRET;
    if (IPC_SECRET && !payload.startsWith(IPC_SECRET + "|")) return;
    payload = payload.substring(65);

    if (payload.startsWith("CLOCK|")) {
        updateClock(payload.substring(6));
    } else if (payload.startsWith("WEATHER|")) {
        updateWeather(payload.substring(8));
    } else if (payload.startsWith("NEWS|")) {
        updateNews(payload.substring(5));
    } else if (payload.startsWith("CALENDAR|")) {
        updateCalendar(payload.substring(9));
    } else if (payload.startsWith("GUESTBOOK|")) {
        updateGuestbook(payload.substring(10));
    } else if (payload.startsWith("MUSIC|")) {
        updateMusic(payload.substring(6));
    }
}

function updateClock(data) {
    const parts = data.split("|");
    if (parts.length >= 2) {
        document.getElementById('clock-time').innerText = parts[0];

        const dateParts = parts[1].split(", ");
        if (dateParts.length >= 2) {
            const weekday = dateParts[0];
            const dayMonth = dateParts[1].split(" ");

            document.getElementById('cal-header-title').innerText = weekday;

            document.getElementById('cal-leaf-day').innerText = dayMonth[0];
            document.getElementById('cal-leaf-month').innerText = dayMonth[1].toUpperCase();
        }
    }
}

function updateGuestbook(data) {
    const parts = data.split(" | ");
    if (parts.length < 2) return;
    const rawTime = parts[0];
    const content = parts[1].split(": ");
    const author = content[0];
    const message = content[1];
    const timeParts = rawTime.split(" ");
    const dateParts = timeParts[0].split("/");
    const months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
    const formattedDate = `${months[parseInt(dateParts[1]) - 1]} ${dateParts[0]}, 20${dateParts[2]}`;
    document.getElementById('guestbook-text').innerText = `"${message}"`;
    document.getElementById('guestbook-meta').innerText = `${formattedDate} • ${timeParts[1]} — ${author}`;
}

function updateWeather(data) {
    const p = data.split("|");
    if (p.length < 5) return;

    const temp = Math.round(p[0]);
    const feelsLike = p[1];
    const wind = p[2];
    const code = parseInt(p[3]);
    const isDay = p[4] === "1";

    let bgUrl = "";
    if (code >= 95) {
        bgUrl = "https://images.unsplash.com/photo-1594760467013-64ac2b80b7d3?q=80&w=1932&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D";
    } else if (code >= 51 && code <= 82) {
        bgUrl = "https://media.istockphoto.com/id/1757967583/photo/rain-on-umbrella-background-weather-forecast-and-environment-concept.jpg?s=2048x2048&w=is&k=20&c=pJwt7U2LnHf6242gNj517Bh09rM0Wmj6Ugagy4GQYBk=";
    } else if (code >= 1 && code <= 3) {
        bgUrl = isDay
            ? "https://images.unsplash.com/photo-1501630834273-4b5604d2ee31?auto=format&fit=crop&w=1000&q=80"
            : "https://images.unsplash.com/photo-1534088568595-a066f410bcda?auto=format&fit=crop&w=1000&q=80";
    } else {
        bgUrl = isDay
            ? "https://images.unsplash.com/photo-1601297183305-6df142704ea2?q=80&w=1974&auto=format&fit=crop&ixlib=rb-4.1.0&ixid=M3wxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHx8fA%3D%3D"
            : "https://media.licdn.com/dms/image/v2/D4D22AQGXHKyjr7BvBA/feedshare-shrink_1280/B4DZUv5TN5GkAk-/0/1740265311008?e=1779321600&v=beta&t=TYxqut4d11hR2k668QM0dWUJdnkNViMogDBIyNo7_V4";
    }

    const widget = document.querySelector('.weather-widget');
    if (widget) {
        widget.style.backgroundImage = `linear-gradient(rgba(0,0,0,0.3), rgba(0,0,0,0.3)), url('${bgUrl}')`;
    }

    let icon = "☁️";
    if (code === 0) icon = isDay ? "☀️" : "🌙";
    else if (code >= 1 && code <= 3) icon = isDay ? "🌤️" : "☁️";
    else if (code >= 51 && code <= 67) icon = "🌧️";
    else if (code >= 71 && code <= 77) icon = "❄️";
    else if (code >= 80 && code <= 82) icon = "🌦️";
    else if (code >= 95) icon = "⛈️";

    document.getElementById('weather-icon').innerText = icon;
    document.getElementById('weather-temp').innerText = temp;
    document.getElementById('weather-details').innerText = `Feels like ${feelsLike}°C  •  Wind ${wind} km/h`;
}

function updateNews(data) {
    const items = data.split("|");
    const container = document.getElementById('news-list');
    container.innerHTML = "";
    items.forEach(item => {
        if (!item.trim()) return;
        const li = document.createElement("li");
        li.innerText = item;
        container.appendChild(li);
    });
}

function updateCalendar(data) {
    const items = data.split(" | ");
    const container = document.getElementById('calendar-list');
    container.innerHTML = "";
    items.forEach(item => {
        if (!item.trim()) return;
        const block = document.createElement("div");
        const isTodo = item.startsWith("[-] ");
        block.className = `cal-block ${isTodo ? 'cal-todo' : 'cal-event'}`;
        const timeText = isTodo ? "ALL DAY" : item.substring(1, 6);
        const summary = isTodo ? item.substring(4) : item.substring(8);
        block.innerHTML = `<div style="font-size: 0.75rem; font-weight: bold; opacity: 0.8;">${timeText}</div><div>${summary}</div>`;
        container.appendChild(block);
    });
}

function updateMusic(data) {
    const p = data.split("|");
    if (p.length < 9) return;

    const serverIsPlaying = p[0] === "1";
    const trackId = p[1] + p[2];
    const albumArt = p[5];

    document.getElementById('music-status').innerText = serverIsPlaying ? "NOW PLAYING" : "PAUSED";
    document.getElementById('music-artist').innerText = p[1];
    document.getElementById('music-track').innerText = p[2];

    const card = document.getElementById('music-card');
    if (albumArt !== "none") {
        card.style.backgroundImage = `url('${albumArt}')`;
    } else {
        card.style.backgroundImage = "none";
        card.style.backgroundColor = "#222";
    }

    durMs = parseInt(p[4]);
    document.getElementById('music-total').innerText = formatTime(durMs);

    if (currentTrackId !== trackId || Math.abs(currentProgMs - parseInt(p[3])) > 4000) {
        currentProgMs = parseInt(p[3]);
        currentTrackId = trackId;
    }

    renderMusicProgress();

    if (musicInterval) clearInterval(musicInterval);
    if (serverIsPlaying) {
        musicInterval = setInterval(() => {
            currentProgMs += 1000;
            if (currentProgMs > durMs) currentProgMs = durMs;
            renderMusicProgress();
        }, 1000);
    }

    let topArtist = p[7];
    let scrobbles = p[8];

    if (topArtist.startsWith("Top Artist: ")) {
        topArtist = topArtist.replace("Top Artist: ", "");
    }
    if (scrobbles.startsWith("Scrobbles: ")) {
        scrobbles = scrobbles.replace("Scrobbles: ", "");
    }

    document.getElementById('lfm-top').innerText = topArtist;
    document.getElementById('lfm-scrob').innerText = scrobbles;
}

function renderMusicProgress() {
    const pct = (durMs > 0) ? (currentProgMs / durMs) * 100 : 0;
    document.getElementById('music-progress').style.width = `${pct}%`;
    document.getElementById('music-current').innerText = formatTime(currentProgMs);
}

function formatTime(ms) {
    const totalSec = Math.floor(ms / 1000);
    const m = Math.floor(totalSec / 60);
    const s = (totalSec % 60).toString().padStart(2, '0');
    return `${m}:${s}`;
}

window.onload = connectWebSocket;