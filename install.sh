#!/bin/bash
set -e

echo "======================================"
echo "        Iwakura Setup Wizard          "
echo "======================================"
echo ""
echo "Please select what you are installing on this machine:"
echo "1) Backend Core (Data Hubs & Servers)"
echo "2) Frontend UI (Terminal Dashboard)"
echo ""
read -p "Enter choice (1 or 2): " INSTALL_TYPE

echo "Checking dependencies..."
for pkg in make gcc; do
    if ! command -v $pkg &> /dev/null; then
        echo "Error: $pkg is not installed. Please install build-essential."
        exit 1
    fi
done

if [ "$INSTALL_TYPE" == "1" ]; then
    echo ""
    echo "--- BACKEND CORE INSTALLATION ---"
    
    for pkg in curl pkg-config; do
        if ! command -v $pkg &> /dev/null; then
            echo "Error: $pkg is not installed. The backend requires libcurl and libjson-c."
            exit 1
        fi
    done

    echo "Compiling Iwakura ecosystem..."
    make clean > /dev/null
    make > /dev/null
    
    mkdir -p "$HOME/.iwakura"

    echo ""
    echo "--- Core Configuration ---"
    read -p "Dashboard Language (en/es) [default: en]: " IWAKURA_LANG
    IWAKURA_LANG=${IWAKURA_LANG:-en}

    read -p "Listen Address (default: 0.0.0.0): " DASH_HOST
    DASH_HOST=${DASH_HOST:-0.0.0.0}

    read -p "Frontend Broadcast Port (default: 8891): " FRONTEND_PORT
    FRONTEND_PORT=${FRONTEND_PORT:-8891}

    echo ""
    echo "--- Module Selection ---"
    echo "Select which services you want to run:"
    read -p "Enable Weather? (y/n) [y]: " EN_WX
    read -p "Enable Music (Spotify/Last.fm)? (y/n) [y]: " EN_MUSIC
    read -p "Enable Calendar? (y/n) [y]: " EN_CAL
    read -p "Enable News/RSS? (y/n) [y]: " EN_NEWS
    read -p "Enable Guestbook? (y/n) [y]: " EN_GB

    GUESTBOOK_PORT=8890
    if [[ "$EN_GB" =~ ^[Yy]$ ]] || [ -z "$EN_GB" ]; then
        read -p "Guestbook Web Port (default: 8890): " GUESTBOOK_PORT
        GUESTBOOK_PORT=${GUESTBOOK_PORT:-8890}
        touch "$HOME/.iwakura/guestbook.txt"
    fi

    if [[ "$EN_NEWS" =~ ^[Yy]$ ]] || [ -z "$EN_NEWS" ]; then
        touch "$HOME/.iwakura/rss.txt"
    fi

    echo ""
    echo "--- Advanced Refresh Rates ---"
    read -p "Configure custom refresh rates? (y/N): " CONF_REFRESH
    REFRESH_WX=900
    REFRESH_MUSIC=6
    REFRESH_LFM=300
    REFRESH_CAL=600
    REFRESH_NEWS=300
    REFRESH_GB=15

    if [[ "$CONF_REFRESH" =~ ^[Yy]$ ]]; then
        read -p "Weather Refresh (seconds) [900]: " IN_WX
        REFRESH_WX=${IN_WX:-900}
        read -p "Spotify Sync (seconds) [6]: " IN_MUSIC
        REFRESH_MUSIC=${IN_MUSIC:-6}
        read -p "Last.fm Refresh (seconds) [300]: " IN_LFM
        REFRESH_LFM=${IN_LFM:-300}
        read -p "Calendar Refresh (seconds) [600]: " IN_CAL
        REFRESH_CAL=${IN_CAL:-600}
        read -p "News Refresh (seconds) [300]: " IN_NEWS
        REFRESH_NEWS=${IN_NEWS:-300}
        read -p "Guestbook Cycle (seconds) [15]: " IN_GB
        REFRESH_GB=${IN_GB:-15}
    fi

    echo ""

    echo "Generating secure IPC token..."
        IPC_SECRET=$(cat /dev/urandom | tr -dc 'a-f0-9' | fold -w 64 | head -n 1)

    echo "Generating .env configuration template..."

cat <<EOF > .env
DASH_PORT=8888
DASH_HOST=$DASH_HOST
ORCH_PORT=8889
FRONTEND_PORT=$FRONTEND_PORT
GUESTBOOK_PORT=$GUESTBOOK_PORT
IWAKURA_LANG="$IWAKURA_LANG"
IWAKURA_SECRET="$IPC_SECRET"

ENABLE_WEATHER=$([[ "$EN_WX" =~ ^[Nn]$ ]] && echo "0" || echo "1")
ENABLE_MUSIC=$([[ "$EN_MUSIC" =~ ^[Nn]$ ]] && echo "0" || echo "1")
ENABLE_CALENDAR=$([[ "$EN_CAL" =~ ^[Nn]$ ]] && echo "0" || echo "1")
ENABLE_NEWS=$([[ "$EN_NEWS" =~ ^[Nn]$ ]] && echo "0" || echo "1")
ENABLE_GUESTBOOK=$([[ "$EN_GB" =~ ^[Nn]$ ]] && echo "0" || echo "1")

REFRESH_WX=$REFRESH_WX
REFRESH_MUSIC=$REFRESH_MUSIC
REFRESH_LFM=$REFRESH_LFM
REFRESH_CAL=$REFRESH_CAL
REFRESH_NEWS=$REFRESH_NEWS
REFRESH_GB=$REFRESH_GB

GB_TITLE="Iwakura Guestbook"
GB_HEADING="Leave a message"
GB_BG_COLOR="#050505"
GB_FG_COLOR="#00ff00"
GB_BTN_COLOR="#008800"

WX_LAT=""
WX_LON=""

SPOTIFY_CLIENT_ID=""
SPOTIFY_CLIENT_SECRET=""
SPOTIFY_REFRESH_TOKEN=""

LASTFM_API_KEY=""
LASTFM_USER=""
LASTFM_DISPLAY_NAME=""

CAL_URL="http://ip:port/username/calname/"
CAL_USER=""
CAL_PASS=""

RSS_LIST_PATH="$HOME/.iwakura/rss.txt"
EOF

    echo "Setting up systemd user service..."
    SERVICE_DIR="$HOME/.config/systemd/user"
    mkdir -p "$SERVICE_DIR"

cat <<EOF > "$SERVICE_DIR/iwakura.service"
[Unit]
Description=Iwakura Headless Dashboard Core
After=network.target

[Service]
Type=simple
WorkingDirectory=$PWD
ExecStart=$PWD/iwakura
Restart=always
RestartSec=5
StandardOutput=append:$HOME/.iwakura/systemd_out.log
StandardError=append:$HOME/.iwakura/systemd_err.log

[Install]
WantedBy=default.target
EOF

    systemctl --user daemon-reload
    systemctl --user enable iwakura.service
    systemctl --user restart iwakura.service
    loginctl enable-linger "$USER"

    echo "======================================"
    echo "Backend Installation Complete!"
    echo "The Core is running in the background."
    echo ""
echo "NEXT STEPS:"
    echo "1. Edit the .env file in this directory to add your API keys, coordinates, and credentials."
    if [[ "$EN_NEWS" =~ ^[Yy]$ ]] || [ -z "$EN_NEWS" ]; then
        echo "2. Add your RSS feed URLs to $HOME/.iwakura/rss.txt"
    fi
    echo "3. Run 'systemctl --user restart iwakura.service' to apply changes."
    echo ""
    echo "IMPORTANT: Copy this IPC Secret. You will need it to install frontends!"
    echo "IWAKURA_SECRET: $IPC_SECRET"
    echo "======================================"

elif [ "$INSTALL_TYPE" == "2" ]; then
    echo ""
    echo "--- FRONTEND UI INSTALLATION ---"
    echo "NOTE: You must have already installed the Backend Core on a machine"
    echo "before proceeding, as you will need its IP address."
    echo ""
    read -p "Press Enter to continue or Ctrl+C to abort..."

    echo "Compiling Frontend..."
    make clean > /dev/null
    make front_tui > /dev/null

    echo ""
    echo "--- Frontend Configuration ---"
    read -p "Dashboard Language (en/es) [default: en]: " IWAKURA_LANG
    IWAKURA_LANG=${IWAKURA_LANG:-en}

    read -p "Enter the IP address of your Iwakura Backend server: " DASH_HOST
    if [ -z "$DASH_HOST" ]; then
        echo "Error: Backend IP is required for the frontend to connect."
        exit 1
    fi

read -p "Enter the backend broadcast port (default: 8891): " FRONTEND_PORT
    FRONTEND_PORT=${FRONTEND_PORT:-8891}

    echo ""
    read -p "Enter the 64-character IPC Secret generated by the backend: " IPC_SECRET
    if [ -z "$IPC_SECRET" ]; then
        echo "Warning: No secret provided. Please edit your env and add your secret before starting the frontend."
    fi

cat <<EOF > .env
DASH_HOST="$DASH_HOST"
FRONTEND_PORT=$FRONTEND_PORT
IWAKURA_LANG="$IWAKURA_LANG"
IWAKURA_SECRET="$IPC_SECRET"
EOF

    echo "======================================"
    echo "Frontend Installation Complete!"
    echo "Run the dashboard by typing: ./front_tui"
    echo "======================================"
else
    echo "Invalid choice. Exiting."
    exit 1
fi