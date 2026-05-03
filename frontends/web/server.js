const net = require('net');
const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');

const envPath = path.join(__dirname, '.env');
if (fs.existsSync(envPath)) {
    const envFile = fs.readFileSync(envPath, 'utf8');
    envFile.split('\n').forEach(line => {
        const match = line.match(/^\s*([\w.-]+)\s*=\s*(.*)?\s*$/);
        if (match) {
            let key = match[1];
            let value = match[2] ? match[2].trim() : '';
            if (value.startsWith('"') && value.endsWith('"')) {
                value = value.substring(1, value.length - 1);
            }
            process.env[key] = value;
        }
    });
}

const DASH_HOST = process.env.DASH_HOST || '127.0.0.1';
const FRONTEND_PORT = parseInt(process.env.FRONTEND_PORT) || 8891;
const HTTP_PORT = parseInt(process.env.HTTP_PORT) || 8080;
const WS_PORT = 8892;
const IPC_SECRET = process.env.IWAKURA_SECRET || '';

const server = http.createServer((req, res) => {
    let filePath = path.join(__dirname, req.url === '/' ? 'index.html' : req.url);
    const extname = String(path.extname(filePath)).toLowerCase();

    if (req.url === '/config.js') {
        res.writeHead(200, { 'Content-Type': 'application/javascript' });
        res.end(`
            window.IWAKURA_CONFIG = {
                WS_PORT: ${WS_PORT},
                IPC_SECRET: "${IPC_SECRET}"
            };
        `);
        return;
    }

    const mimeTypes = {
        '.html': 'text/html',
        '.js': 'text/javascript',
        '.css': 'text/css',
        '.png': 'image/png',
        '.jpg': 'image/jpg'
    };

    const contentType = mimeTypes[extname] || 'application/octet-stream';

    fs.readFile(filePath, (error, content) => {
        if (error) {
            if(error.code == 'ENOENT'){
                res.writeHead(404);
                res.end('File Not Found');
            } else {
                res.writeHead(500);
                res.end('Server Error: '+error.code+' ..\n');
            }
        } else {
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(content, 'utf-8');
        }
    });
});

server.listen(HTTP_PORT, () => {
    console.log(`HTTP Server running at http://localhost:${HTTP_PORT}/`);
});

const wss = new WebSocket.Server({ port: WS_PORT });

wss.on('connection', (ws) => {
    console.log('Web client connected');
    ws.on('close', () => console.log('Web client disconnected'));
});

const tcpClient = new net.Socket();

function connectToBackend() {
    console.log(`Connecting to C Backend at ${DASH_HOST}:${FRONTEND_PORT}...`);
    tcpClient.connect(FRONTEND_PORT, DASH_HOST, () => {
        console.log('Connected to C Backend successfully');
    });
}

let buffer = '';

tcpClient.on('data', (data) => {
    buffer += data.toString();
    let nIndex;
    while ((nIndex = buffer.indexOf('\n')) !== -1) {
        const line = buffer.substring(0, nIndex);
        buffer = buffer.substring(nIndex + 1);
        if (line) {
            wss.clients.forEach((client) => {
                if (client.readyState === WebSocket.OPEN) {
                    client.send(line);
                }
            });
        }
    }
});

tcpClient.on('close', () => {
    console.log('Connection to C Backend lost. Retrying in 5 seconds...');
    setTimeout(connectToBackend, 5000);
});

tcpClient.on('error', (err) => {
    console.error(`TCP Error: ${err.message}`);
});

connectToBackend();