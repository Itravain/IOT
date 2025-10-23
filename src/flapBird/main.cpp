#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>

// Configuração do Access Point
const char* ap_ssid = "FlappyBird_ESP32";

// Servidor na porta 80
WebServer server(80);

// Página HTML com menu e Flappy Bird interativo sensível ao toque
const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, maximum-scale=1.0">
<title>Flappy Bird ESP32</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { 
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    overflow: hidden;
    width: 100vw;
    height: 100vh;
  }
  #gameContainer {
    width: 100%;
    height: 100%;
    display: flex;
    justify-content: center;
    align-items: center;
    position: relative;
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
  }
  canvas { 
    background: linear-gradient(to bottom, #87CEEB 0%, #E0F6FF 100%);
    touch-action: none;
    display: none;
    width: 100vw !important;
    height: 100vh !important;
    position: absolute;
    top: 0;
    left: 0;
  }
  button { 
    font-size: 18px; 
    padding: 14px 32px; 
    margin: 8px; 
    cursor: pointer;
    border: none;
    border-radius: 50px;
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    color: white;
    font-weight: 600;
    box-shadow: 0 4px 15px rgba(79, 172, 254, 0.4);
    transition: all 0.3s ease;
    text-transform: uppercase;
    letter-spacing: 1px;
  }
  button:hover {
    transform: translateY(-2px);
    box-shadow: 0 6px 20px rgba(79, 172, 254, 0.6);
  }
  button:active {
    transform: translateY(0);
  }
  input { 
    font-size: 16px; 
    padding: 12px 20px; 
    margin: 15px auto; 
    width: 85%;
    max-width: 320px;
    border: none;
    border-radius: 50px;
    background: rgba(255, 255, 255, 0.9);
    box-shadow: 0 4px 15px rgba(0,0,0,0.1);
    text-align: center;
    font-weight: 500;
    transition: all 0.3s ease;
    display: block;
  }
  input:focus {
    outline: none;
    background: white;
    box-shadow: 0 6px 20px rgba(79, 172, 254, 0.3);
  }
  input::placeholder {
    color: #999;
  }
  #menu { 
    text-align: center;
    padding: 30px 20px;
    background: rgba(255, 255, 255, 0.95);
    border-radius: 20px;
    box-shadow: 0 20px 60px rgba(0,0,0,0.3);
    max-width: 500px;
    max-height: 90vh;
    overflow-y: auto;
    margin: auto;
  }
  #menu h1 {
    font-size: 36px;
    margin-bottom: 10px;
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    font-weight: 700;
  }
  #menu p {
    color: #666;
    margin-bottom: 20px;
    font-size: 14px;
  }
  #score { 
    position: absolute;
    top: 20px;
    left: 50%;
    transform: translateX(-50%);
    font-size: 28px;
    color: white;
    text-shadow: 0 2px 10px rgba(0,0,0,0.3);
    z-index: 10;
    display: none;
    font-weight: 700;
    background: rgba(79, 172, 254, 0.8);
    padding: 10px 25px;
    border-radius: 50px;
  }
  #leaderboard { 
    margin-top: 25px; 
    text-align: left; 
    display: inline-block;
    width: 100%;
    max-width: 400px;
  }
  #leaderboard h2 { 
    text-align: center; 
    margin-bottom: 15px;
    font-size: 24px;
    color: #4facfe;
    font-weight: 600;
  }
  #leaderboard ol { 
    background: linear-gradient(135deg, #e0f7ff 0%, #b3e5fc 100%);
    padding: 20px 30px; 
    border-radius: 15px;
    max-height: 280px;
    overflow-y: auto;
    box-shadow: inset 0 2px 10px rgba(0,0,0,0.1);
  }
  #leaderboard li { 
    margin: 8px 0; 
    font-size: 15px;
    color: #333;
    padding: 8px;
    border-radius: 8px;
    transition: all 0.2s ease;
    cursor: pointer;
    position: relative;
  }
  #leaderboard li:hover {
    background: rgba(255, 100, 100, 0.3);
    transform: translateX(5px);
  }
  #leaderboard li::after {
    content: '🗑️';
    position: absolute;
    right: 10px;
    opacity: 0;
    transition: opacity 0.2s;
  }
  #leaderboard li:hover::after {
    opacity: 0.5;
  }
  #leaderboard li strong {
    color: #4facfe;
    font-weight: 600;
  }
  
  /* Modal de senha */
  #passwordModal {
    display: none;
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: rgba(0, 0, 0, 0.7);
    z-index: 1000;
    justify-content: center;
    align-items: center;
  }
  #passwordModal.show {
    display: flex;
  }
  #passwordBox {
    background: white;
    padding: 30px;
    border-radius: 20px;
    text-align: center;
    max-width: 400px;
    width: 90%;
  }
  #passwordBox h3 {
    color: #4facfe;
    margin-bottom: 20px;
  }
  #passwordBox input {
    width: 100%;
    margin: 10px 0;
  }
  #passwordBox .buttons {
    display: flex;
    gap: 10px;
    justify-content: center;
    margin-top: 20px;
  }
  #passwordBox button {
    flex: 1;
    max-width: 150px;
  }
  #cancelBtn {
    background: linear-gradient(135deg, #ee0979 0%, #ff6a00 100%);
  }
  
  #leaderboard ol::-webkit-scrollbar {
    width: 6px;
  }
  #leaderboard ol::-webkit-scrollbar-track {
    background: rgba(255,255,255,0.3);
    border-radius: 10px;
  }
  #leaderboard ol::-webkit-scrollbar-thumb {
    background: #4facfe;
    border-radius: 10px;
  }
  
  @media (max-width: 600px) {
    #menu { 
      margin: 10px;
      padding: 20px 15px;
    }
    #menu h1 { font-size: 28px; }
    #menu p { font-size: 13px; }
    button { font-size: 16px; padding: 12px 28px; }
    #leaderboard h2 { font-size: 20px; }
    #leaderboard li { font-size: 14px; }
  }
</style>
</head>
<body>

<div id="gameContainer">
  <div id="menu">
    <h1>🐦 Flappy Bird</h1>
    <p>Toque para voar, evite os obstáculos!</p>
    <input type="text" id="playerName" placeholder="Digite seu nome" maxlength="15">
    <button id="startBtn">▶ Jogar</button>
    
    <div id="leaderboard">
      <h2>🏆 Hall da Fama</h2>
      <ol id="scoreList"></ol>
    </div>
  </div>

  <canvas id="gameCanvas"></canvas>
  <div id="score"></div>
</div>

<div id="passwordModal">
  <div id="passwordBox">
    <h3>🔒 Deletar Placar</h3>
    <p id="deleteInfo"></p>
    <input type="password" id="passwordInput" placeholder="Digite a senha de administrador">
    <div class="buttons">
      <button id="cancelBtn">Cancelar</button>
      <button id="confirmBtn">Confirmar</button>
    </div>
  </div>
</div>

<script>
const canvas = document.getElementById('gameCanvas');
const ctx = canvas.getContext('2d');
const startBtn = document.getElementById('startBtn');
const menu = document.getElementById('menu');
const scoreDisplay = document.getElementById('score');
const playerNameInput = document.getElementById('playerName');
const scoreList = document.getElementById('scoreList');
const gameContainer = document.getElementById('gameContainer');

let bird = { x: 50, y: 240, width: 20, height: 20, velocity: 0 };
let gravity = 0.5;
let jumpStrength = -8;
let pipes = [];
let gameOver = false;
let score = 0;
let lastTime = 0;
let pipeTimer = 0;
let playerName = '';
let gamePaused = true;
const targetFPS = 60;
const frameTime = 1000 / targetFPS;

let canvasWidth, canvasHeight;
let scale;
const BASE_WIDTH = 320;
const BASE_HEIGHT = 480;

// Otimização: criar gradientes uma vez
let bgGradient;

function resizeCanvas() {
  canvasWidth = window.innerWidth;
  canvasHeight = window.innerHeight;
  
  canvas.width = canvasWidth;
  canvas.height = canvasHeight;
  
  // Usar escala uniforme para evitar distorção
  scale = Math.min(canvasWidth / BASE_WIDTH, canvasHeight / BASE_HEIGHT);
  
  bird.width = 20 * scale;
  bird.height = 20 * scale;
  bird.x = 50 * scale;
  
  if (!gamePaused && !gameOver) {
    // Manter posição Y durante o jogo
  } else {
    bird.y = canvasHeight / 2;
  }
  
  // Recriar gradientes com novo tamanho
  bgGradient = ctx.createLinearGradient(0, 0, 0, canvasHeight);
  bgGradient.addColorStop(0, '#87CEEB');
  bgGradient.addColorStop(1, '#E0F6FF');
}

window.addEventListener('resize', resizeCanvas);
resizeCanvas();

// Carregar placar do servidor ESP32
async function loadLeaderboard() {
  try {
    const response = await fetch('/leaderboard');
    const leaderboard = await response.json();
    return leaderboard;
  } catch(error) {
    console.error('Erro ao carregar placar:', error);
    return [];
  }
}

// Salvar pontuação no servidor ESP32
async function addScore(name, points) {
  try {
    await fetch('/score', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ name: name, score: points })
    });
    await displayLeaderboard();
  } catch(error) {
    console.error('Erro ao salvar pontuação:', error);
  }
}

let selectedPosition = null;

// Exibir placar na tela
async function displayLeaderboard() {
  let leaderboard = await loadLeaderboard();
  scoreList.innerHTML = '';
  
  if(leaderboard.length === 0) {
    scoreList.innerHTML = '<li>Nenhum recorde ainda!</li>';
  } else {
    leaderboard.forEach((entry, index) => {
      let li = document.createElement('li');
      li.innerHTML = `<strong>${index + 1}. ${entry.name}</strong> - ${entry.score} pontos`;
      li.dataset.position = index + 1;
      li.dataset.name = entry.name;
      li.dataset.score = entry.score;
      li.addEventListener('click', () => showPasswordModal(index + 1, entry.name, entry.score));
      scoreList.appendChild(li);
    });
  }
}

// Mostrar modal de senha
function showPasswordModal(position, name, score) {
  selectedPosition = position;
  document.getElementById('deleteInfo').innerText = `Deletar: ${position}. ${name} - ${score} pontos`;
  document.getElementById('passwordModal').classList.add('show');
  document.getElementById('passwordInput').value = '';
  document.getElementById('passwordInput').focus();
}

// Fechar modal
function closePasswordModal() {
  document.getElementById('passwordModal').classList.remove('show');
  selectedPosition = null;
}

// Deletar placar
async function deleteScore(position, password) {
  try {
    const response = await fetch('/delete', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ position: position, password: password })
    });
    
    const result = await response.json();
    
    if(response.ok && result.success) {
      alert('Placar deletado com sucesso!');
      await displayLeaderboard();
      closePasswordModal();
    } else {
      alert(result.error || 'Senha incorreta!');
    }
  } catch(error) {
    console.error('Erro ao deletar placar:', error);
    alert('Erro ao deletar placar!');
  }
}

// Event listeners para modal
document.getElementById('cancelBtn').addEventListener('click', closePasswordModal);
document.getElementById('confirmBtn').addEventListener('click', () => {
  const password = document.getElementById('passwordInput').value;
  if(password && selectedPosition) {
    deleteScore(selectedPosition, password);
  } else {
    alert('Digite a senha!');
  }
});

document.getElementById('passwordInput').addEventListener('keypress', (e) => {
  if(e.key === 'Enter') {
    document.getElementById('confirmBtn').click();
  }
});

// Fechar modal ao clicar fora
document.getElementById('passwordModal').addEventListener('click', (e) => {
  if(e.target.id === 'passwordModal') {
    closePasswordModal();
  }
});

startBtn.onclick = function() {
  playerName = playerNameInput.value.trim();
  if(playerName === '') {
    alert('Por favor, digite seu nome!');
    return;
  }
  
  resizeCanvas();
  menu.style.display = 'none';
  canvas.style.display = 'block';
  scoreDisplay.style.display = 'block';
  gameOver = false;
  gamePaused = true;
  score = 0;
  bird.y = canvasHeight / 2;
  bird.velocity = 0;
  pipes = [];
  pipeTimer = 0;
  lastTime = performance.now();
  scoreDisplay.innerText = '';
  requestAnimationFrame(gameLoop);
}

function generatePipe() {
  let gap = 120 * scale;
  let minHeight = 50 * scale;
  let maxHeight = canvasHeight - gap - 50 * scale;
  let height = Math.floor(Math.random() * (maxHeight - minHeight + 1) + minHeight);
  let pipeWidth = 40 * scale;
  
  pipes.push({ x: canvasWidth, y: 0, width: pipeWidth, height: height, scored: false });
  pipes.push({ x: canvasWidth, y: height + gap, width: pipeWidth, height: canvasHeight - height - gap, scored: true });
}

function checkCollision(pipe) {
  return (bird.x < pipe.x + pipe.width &&
          bird.x + bird.width > pipe.x &&
          bird.y < pipe.y + pipe.height &&
          bird.y + bird.height > pipe.y);
}

function gameLoop(currentTime) {
  const deltaTime = (currentTime - lastTime) / frameTime;
  lastTime = currentTime;

  ctx.fillStyle = bgGradient;
  ctx.fillRect(0, 0, canvasWidth, canvasHeight);

  if(!gamePaused) {
    bird.velocity += gravity * deltaTime * scale;
    bird.y += bird.velocity * deltaTime;
  }
  
  // Desenhar pássaro com raio uniforme (círculo perfeito)
  ctx.fillStyle = '#FFD700';
  ctx.beginPath();
  ctx.arc(bird.x + bird.width/2, bird.y + bird.height/2, bird.width/2, 0, Math.PI * 2);
  ctx.fill();

  if(!gamePaused) {
    pipeTimer += deltaTime;
    if(pipeTimer >= 90) {
      generatePipe();
      pipeTimer = 0;
    }
  }

  ctx.fillStyle = '#2ecc71';
  ctx.strokeStyle = '#27ae60';
  ctx.lineWidth = 2 * scale;
  
  for(let i = pipes.length-1; i >= 0; i--) {
    if(!gamePaused) {
      pipes[i].x -= 2 * deltaTime * scale;
    }
    
    ctx.fillRect(pipes[i].x, pipes[i].y, pipes[i].width, pipes[i].height);
    ctx.strokeRect(pipes[i].x, pipes[i].y, pipes[i].width, pipes[i].height);

    if(checkCollision(pipes[i])) gameOver = true;
    
    if(!pipes[i].scored && pipes[i].x + pipes[i].width < bird.x) {
      score++;
      pipes[i].scored = true;
    }
    
    if(pipes[i].x + pipes[i].width < 0) pipes.splice(i,1);
  }

  if(gamePaused) {
    ctx.fillStyle = 'rgba(0, 0, 0, 0.4)';
    ctx.fillRect(0, 0, canvasWidth, canvasHeight);
    
    ctx.fillStyle = 'white';
    ctx.font = `bold ${Math.floor(24 * scale)}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.fillText('Toque para iniciar', canvasWidth/2, canvasHeight/2);
  }

  scoreDisplay.innerText = '⭐ ' + score;

  if(bird.y + bird.height > canvasHeight || bird.y < 0) gameOver = true;

  if(gameOver) {
    addScore(playerName, score);
    
    ctx.fillStyle = 'rgba(0, 0, 0, 0.6)';
    ctx.fillRect(0, 0, canvasWidth, canvasHeight);
    
    ctx.fillStyle = 'white';
    ctx.font = `bold ${Math.floor(32 * scale)}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.fillText('Game Over!', canvasWidth/2, canvasHeight/2 - 30 * scale);
    ctx.font = `bold ${Math.floor(24 * scale)}px sans-serif`;
    ctx.fillText('Pontuação: ' + score, canvasWidth/2, canvasHeight/2 + 20 * scale);
    
    setTimeout(() => {
      menu.style.display = '';
      canvas.style.display = 'none';
      scoreDisplay.style.display = 'none';
    }, 2000);
  } else {
    requestAnimationFrame(gameLoop);
  }
}

// Carregar placar ao iniciar a página
displayLeaderboard();

// Eventos desktop
document.addEventListener('keydown', (e) => { 
  if(canvas.style.display === 'block') {
    if(gamePaused) {
      gamePaused = false;
    } else if(!gameOver) {
      bird.velocity = jumpStrength * scale;
    }
  }
});

canvas.addEventListener('click', (e) => { 
  if(gamePaused) {
    gamePaused = false;
  } else if(!gameOver) {
    bird.velocity = jumpStrength * scale;
  }
});

canvas.addEventListener('touchstart', (e) => {
  e.preventDefault();
  if(gamePaused) {
    gamePaused = false;
  } else if(!gameOver) {
    bird.velocity = jumpStrength * scale;
  }
}, { passive: false });
</script>

</body>
</html>
)rawliteral";

// Função que responde ao cliente
void handleRoot() {
  server.send(200, "text/html", html);
}

// Carregar placar do arquivo
void handleGetLeaderboard() {
  File file = SPIFFS.open("/leaderboard.json", "r");
  if (!file) {
    server.send(200, "application/json", "[]");
    return;
  }
  
  String content = file.readString();
  file.close();
  server.send(200, "application/json", content);
}

// Salvar nova pontuação
void handlePostScore() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Carregar placar existente
    DynamicJsonDocument leaderboard(4096);
    File file = SPIFFS.open("/leaderboard.json", "r");
    if (file) {
      deserializeJson(leaderboard, file);
      file.close();
    }
    
    // Adicionar nova pontuação
    DynamicJsonDocument newScore(256);
    deserializeJson(newScore, body);
    
    JsonArray array = leaderboard.as<JsonArray>();
    array.add(newScore);
    
    // Estrutura para armazenar pontuações
    struct ScoreEntry {
      String name;
      int score;
    };
    
    // Usar vector ao invés de array de tamanho variável
    std::vector<ScoreEntry> entries;
    entries.reserve(array.size());
    
    for (size_t i = 0; i < array.size(); i++) {
      ScoreEntry entry;
      entry.name = array[i]["name"].as<String>();
      entry.score = array[i]["score"].as<int>();
      entries.push_back(entry);
    }
    
    // Ordenar usando std::sort
    std::sort(entries.begin(), entries.end(), [](const ScoreEntry &a, const ScoreEntry &b) {
      return a.score > b.score;
    });
    
    // Criar novo array ordenado (top 10)
    DynamicJsonDocument sortedLeaderboard(4096);
    JsonArray sortedArray = sortedLeaderboard.to<JsonArray>();
    
    int limit = min((int)entries.size(), 10);
    for (int i = 0; i < limit; i++) {
      JsonObject obj = sortedArray.createNestedObject();
      obj["name"] = entries[i].name;
      obj["score"] = entries[i].score;
    }
    
    // Salvar no arquivo
    file = SPIFFS.open("/leaderboard.json", "w");
    if (file) {
      serializeJson(sortedLeaderboard, file);
      file.close();
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(500, "application/json", "{\"error\":\"Failed to save\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

// Senha de administrador (altere conforme necessário)
const char* ADMIN_PASSWORD = "admin123";

// Apagar placar por posição com senha
void handleDeleteScore() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, body);
    int position = doc["position"];
    String password = doc["password"].as<String>();
    
    // Verificar senha
    if (password != ADMIN_PASSWORD) {
      server.send(401, "application/json", "{\"success\":false,\"error\":\"Senha incorreta\"}");
      Serial.println("Tentativa de deletar com senha incorreta!");
      return;
    }
    
    // Carregar placar existente
    DynamicJsonDocument leaderboard(4096);
    File file = SPIFFS.open("/leaderboard.json", "r");
    if (file) {
      deserializeJson(leaderboard, file);
      file.close();
    }
    
    JsonArray array = leaderboard.as<JsonArray>();
    
    if (position >= 1 && position <= (int)array.size()) {
      String deletedName = array[position - 1]["name"].as<String>();
      int deletedScore = array[position - 1]["score"].as<int>();
      
      array.remove(position - 1);
      
      file = SPIFFS.open("/leaderboard.json", "w");
      if (file) {
        serializeJson(leaderboard, file);
        file.close();
        Serial.print("Placar removido: ");
        Serial.print(position);
        Serial.print(". ");
        Serial.print(deletedName);
        Serial.print(" - ");
        Serial.println(deletedScore);
        server.send(200, "application/json", "{\"success\":true}");
      } else {
        server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save\"}");
      }
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid position\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"No data\"}");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
    return;
  }
  Serial.println("SPIFFS montado com sucesso");
  
  // Configurar ESP32 como Access Point
  Serial.println("Configurando Access Point...");
  
  // Configurar IP fixo (opcional)
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_ip, gateway, subnet);
  
  // Iniciar Access Point sem senha (rede aberta)
  WiFi.softAP(ap_ssid);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point iniciado!");
  Serial.print("Nome da rede: ");
  Serial.println(ap_ssid);
  Serial.println("Rede ABERTA (sem senha)");
  Serial.print("IP do servidor: ");
  Serial.println(IP);
  Serial.println("Conecte-se à rede e acesse: http://192.168.4.1");

  server.on("/", handleRoot);
  server.on("/leaderboard", HTTP_GET, handleGetLeaderboard);
  server.on("/score", HTTP_POST, handlePostScore);
  server.on("/delete", HTTP_POST, handleDeleteScore);
  
  server.begin();
  Serial.println("Servidor iniciado!");
  Serial.println("Use o endpoint /delete para remover placares");
}

void loop() {
  server.handleClient();
}