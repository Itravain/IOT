#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// Configuração do Access Point
const char* ap_ssid = "Agario_ESP32";

// Servidor na porta 80
WebServer server(80);

// Servidor WebSocket na porta 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Estrutura para armazenar dados dos jogadores
struct Player {
  String id;
  String name;
  float x;
  float y;
  float r;
  float mx;
  float my;
  String fillColor;
  String strokeColor;
  uint8_t clientNum;
  unsigned long lastUpdate;
  bool active;
};

// Mapa de jogadores (máximo 10 jogadores)
Player players[10];
int playerCount = 0;

// Pellets compartilhados
struct Pellet {
  float x;
  float y;
  float r;
  String color;
  int numPoints;
  float angle;
};

Pellet pellets[1000];
bool pelletsInitialized = false;

// Página HTML com o jogo Agar.io
const char* html = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, maximum-scale=1.0">
	<title>Canvas agar.io Clone - Multiplayer</title>
	<style type="text/css">
		body { margin: 0; padding: 0; overflow: hidden; }
		#screen { width: 100%; height: 100%; background-color: #f2fbff; }
		#debug { position: fixed; top: 10px; right: 10px; background: rgba(0,0,0,0.7); color: white; padding: 10px; font-family: monospace; font-size: 12px; z-index: 1000; }
	</style>
</head>
<body>
	<canvas id="screen"></canvas>
	<div id="debug">Carregando...</div>
	
	<script type="text/javascript">
	var canvas,
		context,
		ws,
		wsConnected = false,
		can_render = true,
		can_player_move = true,
		window_width = 0, window_height = 0,
		window_halfWidth = 0, window_halfHeight = 0,
		camera_position = {},
		camera_scale = 1.0,
		camera_scale_multiplier = 0.8,
		mouse_screenPosition = {'x': 0, 'y': 0},
		fastest_cell_speed = 250,
		objects = {'cells': {}, 'pellets': {}},
		world_size = {'x': 5000, 'y': 5000},
		player_world_position = {'x': (world_size.x / 2), 'y': (world_size.y / 2)},
		player_speed_per_second = 30,
		player_object_id,
		angleFromPlayer = 0,
		distanceFromPlayer = 0,
		INPUT = {
			'SPACEBAR': 32,
			'ENTER': 13,
			'W': 119
		},
		input_active = {},
		game_fps = 0;
	
	canvas = document.getElementById('screen');
	context = canvas.getContext('2d');
	
	function updateDebug(msg) {
		document.getElementById('debug').innerHTML = msg;
		console.log(msg);
	}
	
	// Conectar ao WebSocket
	function connectWebSocket() {
		try {
			updateDebug('Tentando conectar WebSocket...');
			ws = new WebSocket('ws://' + window.location.hostname + ':81');
			
			ws.onopen = function() {
				wsConnected = true;
				updateDebug('WebSocket conectado!');
				console.log('WebSocket conectado!');
			};
			
			ws.onmessage = function(event) {
				try {
					var data = JSON.parse(event.data);
					
					if(data.type === 'init') {
						player_object_id = data.playerId;
						if(objects.cells[player_object_id]) {
							objects.cells[player_object_id].x = data.x;
							objects.cells[player_object_id].y = data.y;
							player_world_position.x = data.x;
							player_world_position.y = data.y;
							updateDebug('Jogador inicializado: ' + player_object_id);
						}
					} else if(data.type === 'players') {
						// Atualizar todos os jogadores
						for(var id in data.players) {
							if(id !== player_object_id) {
								if(!objects.cells[id]) {
									objects.cells[id] = new Cell(
										data.players[id].x,
										data.players[id].y,
										data.players[id].r,
										data.players[id].name
									);
									objects.cells[id].fillColor = data.players[id].fillColor;
									objects.cells[id].strokeColor = data.players[id].strokeColor;
								} else {
									objects.cells[id].x = data.players[id].x;
									objects.cells[id].y = data.players[id].y;
									objects.cells[id].r = data.players[id].r;
								}
							}
						}
						
						// Remover jogadores desconectados
						for(var id in objects.cells) {
							if(id !== player_object_id && !data.players[id]) {
								delete objects.cells[id];
							}
						}
					} else if(data.type === 'pellets') {
						// Atualizar pellets
						for(var i = 0; i < data.pellets.length; i++) {
							if(!objects.pellets[i]) {
								objects.pellets[i] = new Pellet();
							}
							objects.pellets[i].x = data.pellets[i].x;
							objects.pellets[i].y = data.pellets[i].y;
							objects.pellets[i].r = data.pellets[i].r;
							objects.pellets[i].color = data.pellets[i].color;
						}
					} else if(data.type === 'playerEaten') {
						if(data.eatenId === player_object_id) {
							alert('Você foi comido por ' + data.eaterName + '!");
							location.reload();
						}
					}
				} catch(e) {
					console.error('Erro ao processar mensagem:', e);
				}
			};
			
			ws.onclose = function() {
				wsConnected = false;
				updateDebug('WebSocket desconectado. Reconectando...');
				console.log('WebSocket desconectado');
				setTimeout(connectWebSocket, 2000);
			};
			
			ws.onerror = function(error) {
				wsConnected = false;
				updateDebug('Erro WebSocket');
				console.log('Erro WebSocket:', error);
			};
		} catch(e) {
			updateDebug('Erro ao criar WebSocket: ' + e.message);
			console.error('Erro ao criar WebSocket:', e);
		}
	}
	
	function sendPlayerUpdate() {
		if(ws && ws.readyState === WebSocket.OPEN && objects.cells[player_object_id]) {
			try {
				ws.send(JSON.stringify({
					type: 'update',
					playerId: player_object_id,
					x: objects.cells[player_object_id].x,
					y: objects.cells[player_object_id].y,
					r: objects.cells[player_object_id].r,
					mx: objects.cells[player_object_id].mx,
					my: objects.cells[player_object_id].my
				}));
			} catch(e) {
				console.error('Erro ao enviar update:', e);
			}
		}
	}
	
	function randomBetween(min, max) {
		return Math.floor(Math.random()*(max-min+1)+min);
	}
	
	function worldXToCameraX(x) {
		return (x - player_world_position.x);
	}
	
	function worldYToCameraY(y) {
		return (y - player_world_position.y);
	}
	
	function worldXYToCameraXY(x, y) {
		return {
			'x':	worldXToCameraX(x),
			'y':	worldYToCameraY(y)
		}
	}
	
	function calibrateCameraScale() {
		var player_radius = objects.cells[ player_object_id ].r,
			scaled_player_radius = (player_radius / camera_scale);
	}
	
	function calibrateCameraSize(cam_width, cam_height, offset_x, offset_y) {
		offset_x = offset_x || 0;
		offset_y = offset_y || 0;
		
		window_width = cam_width;
		window_height = cam_height;
		
		window_halfWidth = (window_width * 0.5);
		window_halfHeight = (window_height * 0.5);
		
		canvas.width = window_width;
		canvas.height = window_height;
	}
	
	var colors = [
		[255, 130,   7],
		[255,   7, 139],
		[254, 255,   0],
		[  7, 255, 171],
		[255,  14,   7],
		[ 81, 255,   7],
		[  7, 191, 255],
		[  7, 133, 255],
		[205,   7, 255]
	];
	
	function chooseRandomColor() {
		return colors[ Math.floor(Math.random() * colors.length) ];
	}
	
	function darkenColor(color) {
		var strokeDiff = 0.85;
		
		return [
			Math.max(0, Math.round( (color[0] * strokeDiff) )),
			Math.max(0, Math.round( (color[1] * strokeDiff) )),
			Math.max(0, Math.round( (color[2] * strokeDiff) ))
		];
	}
	
	function Pellet() {
		this.refreshPosition();
		this.refreshColor();
		this.refreshDisplay();
	}
	
	Pellet.prototype = {
		'x':			0,
		'y':			0,
		'r':			0,
		'color':		"",
		'lineWidth':	10,
		'angle':		0,
		'num_points':	6,
		'lastAngleChg':	0,
		'chgAngleAftr':	1,
		
		'refreshPosition': function(x, y, r) {
			this.r = r || 1;
			
			this.x = x || randomBetween((this.r + this.lineWidth), (world_size.x - (this.r + this.lineWidth)));
			this.y = r || randomBetween((this.r + this.lineWidth), (world_size.y - (this.r + this.lineWidth)));
		},
		
		'refreshDisplay': function(change_color) {
			this.angle = randomBetween(0, 359);
			this.num_points = randomBetween(8, 9);
			this.chgAngleAftr = randomBetween(1, 5);
		},
		
		'refreshColor': function() {
			var newColor = chooseRandomColor();
			
			this.color = "rgb("+ newColor.join(",") +")";
		},
		
		'update': function(dt) {
			// Pellets são gerenciados pelo servidor agora
			this.lastAngleChg += dt;
		},
		
		'draw': function() {
			context.save();
			
			context.translate(worldXToCameraX(this.x), worldYToCameraY(this.y));
			
			if(this.lastAngleChg >= this.chgAngleAftr) {
				this.refreshDisplay();
				
				this.lastAngleChg = 0;
			}
			
			context.rotate((this.angle * (Math.PI / 180)));
			
			var per_point = (360 / (this.num_points + 1));
			
			context.beginPath();
			context.moveTo((this.r + this.lineWidth), 0);
			
			var angle_rad, next_x, nexy_y;
			
			for(var i = 1; i <= this.num_points; i++) {
				angle_rad = (Math.PI * ((per_point * i) / 180));
				next_x = ((this.r + this.lineWidth) * Math.cos(angle_rad));
				next_y = ((this.r + this.lineWidth) * Math.sin(angle_rad));
				
				context.lineTo(next_x, next_y);
			}
			
			context.closePath();
			
			context.fillStyle = this.color;
			context.fill();
			
			context.restore();
		},
		
		'consume': function() {
			this.refreshPosition();
			this.refreshColor();
			this.refreshDisplay();
			
			return this.r;
		}
	};
	
	function Cell(x, y, radius, name) {
		var newFillColor = chooseRandomColor();
		var newStrokeColor = darkenColor(newFillColor);
		
		this.x = x;
		this.y = y;
		this.r = radius;
		this.fillColor = "rgb("+ newFillColor.join(",") +")";
		this.strokeColor = "rgb("+ newStrokeColor.join(",") +")";
		this.name = name;
	}
	
	Cell.prototype = {
		'x':				0,
		'mx':				0,
		'y':				0,
		'my':				0,
		'r':				0,
		'rMin':				15,
		'fillColor':		"",
		'strokeColor':		"",
		'lineWidth':		5,
		'name':				"",
		'decreaseTime':		0,
		'decreaseAfter':	5,
		'subcells':			[[100, 0, 0]],
		'maxSubcells':		16,
		
		'update': function(dt) {
			if((this.mx != 0) || (this.my != 0)) {
				var speed = (fastest_cell_speed * (20 / (this.r + this.lineWidth)));
				
				if(this.mx != 0) {
					this.x += (this.mx * speed * dt);
					
					this.x = Math.max(this.r, Math.min((world_size.x - this.r), this.x));
				}
				
				if(this.my != 0) {
					this.y += (this.my * speed * dt);
					
					this.y = Math.max(this.r, Math.min((world_size.y - this.r), this.y));
				}
				
				this.mx = 0;
				this.my = 0;
			}
			
			this.decreaseTime += dt;
			
			if(this.decreaseTime >= this.decreaseAfter) {
				var decreaseTimes = Math.floor( (this.decreaseTime / this.decreaseAfter) ),
					decreaseAmount = 1;
				
				if(this.r > 1000) {
					decreaseAmount = 5;
				} else if(this.r > 250) {
					decreaseAmount = 3;
				}
				
				this.decreaseTime %= this.decreaseAfter;
				
				for(var i = 1; i <= decreaseTimes; i++) {
					this.r = (this.r * ((100 - decreaseAmount) / 100));
					
					if(this.r < this.rMin) {
						this.r = this.rMin;
						
						break;
					}
				}
			}
		},
		
		'draw': function() {
			var numSubcells = this.subcells.length || 1;
			
			if(numSubcells > 1) {
				var cell_radius_one_percent = (this.r * (1 / 100));
				
				for(var j = 0, k = this.subcells.length; j < k; j++) {
					context.beginPath();
					
					context.arc(
						(worldXToCameraX(this.x) + ( Math.cos(this.subcells[ j ][1]) * this.subcells[ j ][2] )),
						(worldYToCameraY(this.y) + ( Math.sin(this.subcells[ j ][1]) * this.subcells[ j ][2] )),
						(cell_radius_one_percent * this.subcells[j][0]),
						0,
						(2 * Math.PI)
					);
					
					context.strokeStyle = this.strokeColor;
					context.lineWidth = this.lineWidth;
					context.stroke();
					context.fillStyle = this.fillColor;
					context.fill();
				}
			} else {
				context.beginPath();
				context.arc(worldXToCameraX(this.x), worldYToCameraY(this.y), this.r, 0, (2 * Math.PI));
				context.strokeStyle = this.strokeColor;
				context.lineWidth = this.lineWidth;
				context.stroke();
				context.fillStyle = this.fillColor;
				context.fill();
			}
		},
		
		'consumeMass': function(amount) {
			this.r += amount;
			
			return this.r;
		},
		
		'subcellConsumeMass': function(i, amount) {
			if(this.subcells[ i ] == undefined) {
				return this.consumeMass(amount);
			}
			
			var old_cell_r = this.r,
				new_cell_r = this.consumeMass(amount);
			
			var old_one_percent_r = (old_cell_r * (1 / 100)),
				new_one_percent_r = (new_cell_r * (1 / 100));
			
			for(var j = 0, k = this.subcells.length; j < k; j++) {
				subcell_mass_amount = (this.subcells[ j ][0] * old_one_percent_r);
				
				if(j == i) {
					subcell_mass_amount += amount;
				}
				
				this.subcells[ j ][0] = ((subcell_mass_amount / new_cell_r) * 100);
			}
			
			return this.r;
		},
		
		'shootAt': function(angle) {
			
		},
		
		'splitAt': function(angle) {
			var curNumSubcells = this.subcells.length || 1;
			
			if(this.r < (this.rMin * (curNumSubcells + 1))) {
				return;
			}
			
			var newNumSubcells = Math.min(this.maxSubcells, (curNumSubcells * 2));
			
			for(var j = 0, k = this.subcells.length; j < k; j++) {
				if((this.r * (this.subcells[ j ][0] / 100)) < (this.rMin * 2)) {
					continue;
				}
				
				var reduced_subcell_percent = (this.subcells[ j ][0] / 2);
				
				this.subcells[ j ][0] = reduced_subcell_percent;
				
				var new_subcell_distance = 100;
				this.subcells[ this.subcells.length ] = [ reduced_subcell_percent, angle, new_subcell_distance ];
				
				if(this.subcells.length >= this.maxSubcells) {
					break;
				}
			}
		}
	};
	
	function draw_grid() {
		var grid_size = 20;
		
		context.save();
		
		context.translate(((window_halfWidth / camera_scale) * -1), ((window_halfHeight / camera_scale) * -1));
		
		var scaled_canvas_width = (window_width / camera_scale);
		var scaled_canvas_height = (window_height / camera_scale);
		
		var grid_start_x = 0;
		var grid_start_y = 0;
		
		grid_start_x -= ( ( (window_halfWidth / camera_scale) * -1 ) % grid_size );
		grid_start_y -= ( ( (window_halfHeight / camera_scale) * -1 ) % grid_size );
		
		grid_start_x -= ( ( worldXToCameraX(camera_position.x) * -1 ) % grid_size );
		grid_start_y -= ( ( worldYToCameraY(camera_position.y) * -1 ) % grid_size );
		
		context.beginPath();
		context.lineWidth = 1;
		context.strokeStyle = "#dee6ea";
		
		for(var grid_x = grid_start_x; grid_x <= scaled_canvas_width; grid_x += grid_size) {
			context.moveTo(grid_x, 0);
			context.lineTo(grid_x, scaled_canvas_height);
		}
		
		for(var grid_y = grid_start_y; grid_y <= scaled_canvas_height; grid_y += grid_size) {
			context.moveTo(0, grid_y);
			context.lineTo(scaled_canvas_width, grid_y);
		}
		
		context.stroke();
		
		context.restore();
	}
	
	function draw_leaderboard() {
		
	}
	
	function draw_player_score() {
		var playerScoreText = "Score: "+ Math.floor(objects.cells[ player_object_id ].r),
			fontSize = 18 / camera_scale,
			windowPadding = 12 / camera_scale,
			boxPadding = 6 / camera_scale;
		
		context.save();
		
		context.scale(1, 1);
		context.translate( (( (window_halfWidth / camera_scale) * -1 ) + windowPadding ), ((window_halfHeight / camera_scale) - (windowPadding + boxPadding + fontSize + boxPadding)) );
		
		context.font = "normal "+ fontSize +"pt Verdana";
		
		var textMetrics = context.measureText(playerScoreText);
		
		context.beginPath();
		
		context.rect(0, 0, (boxPadding + textMetrics.width + boxPadding), (boxPadding + fontSize + boxPadding));
		context.fillStyle = "rgba(0, 0, 0, 0.3)";
		context.fill();
		
		context.fillStyle = "#ffffff";
		context.textAlign = "left";
		context.textBaseline = "hanging";
		context.fillText(playerScoreText, boxPadding, boxPadding);
		
		context.restore();
	}
	
	function draw_game_fps() {
		var playerScoreText = Math.floor(game_fps) +" FPS",
			fontSize = 12 / camera_scale,
			windowPadding = 6 / camera_scale,
			boxPadding = 6 / camera_scale;
		
		context.save();
		
		context.scale(1, 1);
		context.translate( (( (window_halfWidth / camera_scale) * -1 ) + windowPadding ), (((window_halfHeight / camera_scale) * -1) + windowPadding ) );
		
		context.font = "normal "+ fontSize +"pt Verdana";
		
		var textMetrics = context.measureText(playerScoreText);
		
		context.beginPath();
		
		context.rect(0, 0, (boxPadding + textMetrics.width + boxPadding), (boxPadding + fontSize + boxPadding));
		context.fillStyle = "rgba(0, 0, 0, 0.3)";
		context.fill();
		
		context.fillStyle = "#ffffff";
		context.textAlign = "left";
		context.textBaseline = "hanging";
		context.fillText(playerScoreText, boxPadding, boxPadding);
		
		context.restore();
	}
	
	function draw_world_border() {
		context.save();
		
		context.translate(
			worldXToCameraX(0),
			worldYToCameraY(0)
		);
		
		context.beginPath();
		context.lineWidth = 4;
		context.strokeStyle = "#000000";
		context.moveTo(0, 0);
		context.lineTo(0, world_size.y);
		context.lineTo(world_size.x, world_size.y);
		context.lineTo(world_size.x, 0);
		context.closePath();
		context.stroke();
		
		context.restore();
	}
	
	function update(dt) {
		if(objects.cells[player_object_id]) {
			if(can_player_move && (distanceFromPlayer >= (objects.cells[ player_object_id ].r * 0.75))) {
				objects.cells[ player_object_id ].mx = Math.cos(angleFromPlayer);
				objects.cells[ player_object_id ].my = Math.sin(angleFromPlayer);
			}
			
			for(var obj in objects.cells) {
				objects.cells[ obj ].update(dt);
			}
			
			for(var obj in objects.pellets) {
				objects.pellets[ obj ].update(dt);
			}
			
			player_world_position.x = objects.cells[ player_object_id ].x;
			player_world_position.y = objects.cells[ player_object_id ].y;
			
			camera_position = worldXYToCameraXY(player_world_position.x, player_world_position.y);
			
			// Enviar atualização do jogador
			sendPlayerUpdate();
		}
	}
	
	function draw() {
		context.clearRect(0, 0, canvas.width, canvas.height);
		
		context.save();
		
		context.translate((camera_position.x + window_halfWidth), (camera_position.y + window_halfHeight));
		context.scale(camera_scale, camera_scale);
		
		draw_grid();
		draw_world_border();
		
		for(var obj in objects.pellets) {
			objects.pellets[ obj ].draw();
		}
		
		for(var obj in objects.cells) {
			objects.cells[ obj ].draw();
		}
		
		draw_leaderboard();
		
		if(objects.cells[player_object_id]) {
			draw_player_score();
		}
		
		draw_game_fps();
		
		context.restore();
	}
	
	var loop__time = new Date().getTime();
	var fps = 0, last_fps = 0;
	
	function game_loop() {
		var now = new Date().getTime(),
			dt = ((now - loop__time) / 1000);
		
		window.requestAnimationFrame(game_loop);
		
		fps += 1;
		last_fps += dt;
		
		if(last_fps > 1) {
			game_fps = fps;
			
			fps = 0;
			last_fps %= 1;
		}
		
		loop__time = now;
		
		if(can_render) {
			update(dt);
			draw();
		}
	}
	
	window.onload = function() {
		try {
			updateDebug('Inicializando jogo...');
			
			player_object_id = Math.floor((Math.random() * 10000));
			player_object_id = player_object_id.toString();
			
			var playerName = prompt("Digite seu nome:") || "Player_"+ player_object_id;
			
			updateDebug('Criando jogador: ' + playerName);
			
			objects.cells[ player_object_id ] = new Cell(player_world_position.x, player_world_position.y, 15, playerName);
			
			camera_position = worldXYToCameraXY(player_world_position.x, player_world_position.y);
			
			// Criar alguns pellets localmente para visualização inicial
			for(var i = 0; i < 100; i++) {
				objects.pellets[i] = new Pellet();
			}
			
			updateDebug('Conectando ao servidor...');
			
			// Conectar ao servidor WebSocket
			connectWebSocket();
			
			// Enviar nome do jogador
			setTimeout(function() {
				if(ws && ws.readyState === WebSocket.OPEN) {
					try {
						ws.send(JSON.stringify({
							type: 'join',
							playerId: player_object_id,
							name: playerName,
							fillColor: objects.cells[player_object_id].fillColor,
							strokeColor: objects.cells[player_object_id].strokeColor
						}));
						updateDebug('Dados enviados ao servidor');
					} catch(e) {
						updateDebug('Erro ao enviar join: ' + e.message);
					}
				} else {
					updateDebug('WebSocket não conectado ainda');
				}
			}, 1000);
			
			calibrateCameraSize(window.innerWidth, window.innerHeight);
			
			window.addEventListener('resize', function() {
				calibrateCameraSize(window.innerWidth, window.innerHeight);
			});
			
			window.addEventListener('focus', function() {
				input_active = {};
			});
			
			document.addEventListener('mousemove', function(e) {
				mouse_screenPosition.x = e.pageX;
				mouse_screenPosition.y = e.pageY;
				
				if(objects.cells[player_object_id]) {
					var playerPos = worldXYToCameraXY(objects.cells[ player_object_id ].x, objects.cells[ player_object_id ].y);
					
					angleFromPlayer = Math.atan2(
						(mouse_screenPosition.y - window_halfHeight),
						(mouse_screenPosition.x - window_halfWidth)
					);
					
					var dx = (window_halfWidth - mouse_screenPosition.x);
					var dy = (window_halfHeight - mouse_screenPosition.y);
					
					distanceFromPlayer = (Math.sqrt( ((dx * dx) + (dy * dy)) ) / camera_scale);
				}
			});
			
			document.addEventListener('touchmove', function(e) {
				e.preventDefault();
				
				if(e.touches.length > 0) {
					mouse_screenPosition.x = e.touches[0].pageX;
					mouse_screenPosition.y = e.touches[0].pageY;
					
					angleFromPlayer = Math.atan2(
						(mouse_screenPosition.y - window_halfHeight),
						(mouse_screenPosition.x - window_halfWidth)
					);
					
					var dx = (window_halfWidth - mouse_screenPosition.x);
					var dy = (window_halfHeight - mouse_screenPosition.y);
					
					distanceFromPlayer = (Math.sqrt( ((dx * dx) + (dy * dy)) ) / camera_scale);
				}
			}, { passive: false });
			
			document.addEventListener('mousedown', function(e) {
				if(e.which == 2) {
					e.preventDefault();
					camera_scale = 1;
				}
			});
			
			document.addEventListener('keydown', function(e) {
				if(input_active[ e.which ] === false) {
					return;
				}
				
				input_active[ e.which ] = false;
				
				if(objects.cells[player_object_id]) {
					if(e.which == INPUT.W) {
						objects.cells[ player_object_id ].shootAt(angleFromPlayer);
					} else if(e.which == INPUT.SPACEBAR) {
						objects.cells[ player_object_id ].splitAt(angleFromPlayer);
					} else if(e.which == INPUT.ENTER) {
						can_player_move = !can_player_move;
					}
				}
			});
			
			document.addEventListener('keyup', function(e) {
				input_active[ e.which ] = true;
			});
			
			document.body.addEventListener('wheel', function(e) {
				if(e.deltaY > 0) {
					camera_scale *= camera_scale_multiplier;
				} else {
					camera_scale /= camera_scale_multiplier;
				}
				
				calibrateCameraScale();
			});
			
			updateDebug('Jogo iniciado! FPS: 0');
			
			game_loop();
		} catch(e) {
			updateDebug('ERRO: ' + e.message);
			console.error('Erro na inicialização:', e);
		}
	};
	</script>
</body>
</html>
)rawliteral";

// Função que responde ao cliente
void handleRoot() {
  server.send(200, "text/html", html);
}

// Inicializar pellets
void initPellets() {
  if (!pelletsInitialized) {
    for (int i = 0; i < 1000; i++) {
      pellets[i].x = random(10, 4990);
      pellets[i].y = random(10, 4990);
      pellets[i].r = 1;
      pellets[i].numPoints = random(8, 10);
      pellets[i].angle = random(0, 360);
      
      int colorIdx = random(0, 9);
      int colors[9][3] = {
        {255, 130, 7}, {255, 7, 139}, {254, 255, 0},
        {7, 255, 171}, {255, 14, 7}, {81, 255, 7},
        {7, 191, 255}, {7, 133, 255}, {205, 7, 255}
      };
      pellets[i].color = "rgb(" + String(colors[colorIdx][0]) + "," + 
                         String(colors[colorIdx][1]) + "," + 
                         String(colors[colorIdx][2]) + ")";
    }
    pelletsInitialized = true;
  }
}

// Evento WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Desconectado!\n", num);
      // Remover jogador
      for (int i = 0; i < 10; i++) {
        if (players[i].active && players[i].clientNum == num) {
          players[i].active = false;
          playerCount--;
          break;
        }
      }
      break;
      
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Conectado de %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      }
      break;
      
    case WStype_TEXT:
      {
        StaticJsonDocument<512> doc;
        deserializeJson(doc, payload);
        
        String type = doc["type"];
        
        if (type == "join") {
          String playerId = doc["playerId"];
          String name = doc["name"];
          String fillColor = doc["fillColor"];
          String strokeColor = doc["strokeColor"];
          
          // Adicionar novo jogador
          for (int i = 0; i < 10; i++) {
            if (!players[i].active) {
              players[i].id = playerId;
              players[i].name = name;
              players[i].x = random(100, 4900);
              players[i].y = random(100, 4900);
              players[i].r = 15;
              players[i].fillColor = fillColor;
              players[i].strokeColor = strokeColor;
              players[i].clientNum = num;
              players[i].active = true;
              players[i].lastUpdate = millis();
              playerCount++;
              
              // Enviar posição inicial
              StaticJsonDocument<200> initDoc;
              initDoc["type"] = "init";
              initDoc["playerId"] = playerId;
              initDoc["x"] = players[i].x;
              initDoc["y"] = players[i].y;
              
              String initMsg;
              serializeJson(initDoc, initMsg);
              webSocket.sendTXT(num, initMsg);
              
              break;
            }
          }
        } else if (type == "update") {
          String playerId = doc["playerId"];
          float x = doc["x"];
          float y = doc["y"];
          float r = doc["r"];
          
          // Atualizar jogador
          for (int i = 0; i < 10; i++) {
            if (players[i].active && players[i].id == playerId) {
              players[i].x = x;
              players[i].y = y;
              players[i].r = r;
              players[i].lastUpdate = millis();
              
              // Verificar colisões com pellets
              for (int p = 0; p < 1000; p++) {
                float dx = players[i].x - pellets[p].x;
                float dy = players[i].y - pellets[p].y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if (distance < (players[i].r + 5)) {
                  players[i].r += pellets[p].r;
                  // Reposicionar pellet
                  pellets[p].x = random(10, 4990);
                  pellets[p].y = random(10, 4990);
                }
              }
              
              // Verificar colisões com outros jogadores
              for (int j = 0; j < 10; j++) {
                if (i != j && players[j].active) {
                  float dx = players[i].x - players[j].x;
                  float dy = players[i].y - players[j].y;
                  float distance = sqrt(dx*dx + dy*dy);
                  
                  // Se um jogador é 10% maior, pode comer o outro
                  if (players[i].r > players[j].r * 1.1 && distance < players[i].r) {
                    // Jogador i come jogador j
                    players[i].r += (players[j].r * 0.8);
                    
                    // Notificar que o jogador foi comido
                    StaticJsonDocument<200> eatenDoc;
                    eatenDoc["type"] = "playerEaten";
                    eatenDoc["eatenId"] = players[j].id;
                    eatenDoc["eaterId"] = players[i].id;
                    eatenDoc["eaterName"] = players[i].name;
                    
                    String eatenMsg;
                    serializeJson(eatenDoc, eatenMsg);
                    webSocket.broadcastTXT(eatenMsg);
                    
                    // Resetar jogador comido
                    players[j].x = random(100, 4900);
                    players[j].y = random(100, 4900);
                    players[j].r = 15;
                  } else if (players[j].r > players[i].r * 1.1 && distance < players[j].r) {
                    // Jogador j come jogador i
                    players[j].r += (players[i].r * 0.8);
                    
                    StaticJsonDocument<200> eatenDoc;
                    eatenDoc["type"] = "playerEaten";
                    eatenDoc["eatenId"] = players[i].id;
                    eatenDoc["eaterId"] = players[j].id;
                    eatenDoc["eaterName"] = players[j].name;
                    
                    String eatenMsg;
                    serializeJson(eatenDoc, eatenMsg);
                    webSocket.broadcastTXT(eatenMsg);
                    
                    players[i].x = random(100, 4900);
                    players[i].y = random(100, 4900);
                    players[i].r = 15;
                  }
                }
              }
              
              break;
            }
          }
        }
      }
      break;
  }
}

// Broadcast estado do jogo
void broadcastGameState() {
  static unsigned long lastBroadcast = 0;
  
  if (millis() - lastBroadcast > 50) { // 20 updates por segundo
    // Enviar informações dos jogadores
    StaticJsonDocument<2048> doc;
    doc["type"] = "players";
    JsonObject playersObj = doc.createNestedObject("players");
    
    for (int i = 0; i < 10; i++) {
      if (players[i].active) {
        JsonObject player = playersObj.createNestedObject(players[i].id);
        player["x"] = players[i].x;
        player["y"] = players[i].y;
        player["r"] = players[i].r;
        player["name"] = players[i].name;
        player["fillColor"] = players[i].fillColor;
        player["strokeColor"] = players[i].strokeColor;
      }
    }
    
    String msg;
    serializeJson(doc, msg);
    webSocket.broadcastTXT(msg);
    
    // Enviar informações dos pellets (menos frequente)
    static int pelletUpdateCounter = 0;
    if (pelletUpdateCounter % 10 == 0) {
      StaticJsonDocument<4096> pelletDoc;
      pelletDoc["type"] = "pellets";
      JsonArray pelletsArray = pelletDoc.createNestedArray("pellets");
      
      for (int i = 0; i < 1000; i++) {
        JsonObject pellet = pelletsArray.createNestedObject();
        pellet["x"] = pellets[i].x;
        pellet["y"] = pellets[i].y;
        pellet["r"] = pellets[i].r;
        pellet["color"] = pellets[i].color;
      }
      
      String pelletMsg;
      serializeJson(pelletDoc, pelletMsg);
      webSocket.broadcastTXT(pelletMsg);
    }
    pelletUpdateCounter++;
    
    lastBroadcast = millis();
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
  
  // Inicializar array de jogadores
  for (int i = 0; i < 10; i++) {
    players[i].active = false;
  }
  
  // Inicializar pellets
  initPellets();
  
  // Configurar ESP32 como Access Point
  Serial.println("Configurando Access Point...");
  
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_ip, gateway, subnet);
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
  
  server.begin();
  Serial.println("Servidor HTTP iniciado!");
  
  // Iniciar WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Servidor WebSocket iniciado na porta 81!");
}

void loop() {
  server.handleClient();
  webSocket.loop();
  broadcastGameState();
}