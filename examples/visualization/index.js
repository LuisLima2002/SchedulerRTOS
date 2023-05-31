const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline');
const port = new SerialPort('/dev/ttyACM0', { baudRate: 115200 });
const parser = port.pipe(new Readline({ delimiter: '\n' }));
// Read the port data
port.on("open", () => {
  console.log('serial port open');
});
var data ="T1"
lastTime = Date.now()
parser.on('data', res =>{
  data=res.slice(0,-1);
  if(data ==="Criando queue"||data==="Adicionando queue" || data ==="Queue mais de 1 item" ||data ==="Queue nao tem item" ||data ==="Queue tem item" || data ==="Queue apenas 1 item"){
    console.log(data)
  }else if(Date.now()>lastTime+500){
    lastTime= Date.now()
    wss1.clients.forEach(function each(client){
      if (client.readyState === WebSocket.OPEN) {
        client.send(data);
      }
    })
  }
});

const WebSocket = require('ws');
const wss1 = new WebSocket.Server({ port: 3000 });

wss1.on('connection', function connection(ws) {
});


/*

setInterval(()=>{
  wss1.clients.forEach(function each(client){
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  })
},500)*/