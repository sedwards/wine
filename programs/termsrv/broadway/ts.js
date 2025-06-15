<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>WineRDS Client</title>
  <style>
    canvas { border: 1px solid black; cursor: none; }
    #cursor {
      position: absolute;
      width: 10px;
      height: 10px;
      background: red;
      border-radius: 5px;
      pointer-events: none;
      z-index: 100;
    }
  </style>
</head>
<body>
  <canvas id="screen" width="800" height="600"></canvas>
  <div id="cursor"></div>

  <script>
    const canvas = document.getElementById("screen");
    const ctx = canvas.getContext("2d");
    const cursor = document.getElementById("cursor");
    const ws = new WebSocket("ws://localhost:8765");

    canvas.addEventListener("mousedown", e => {
      const rect = canvas.getBoundingClientRect();
      const x = Math.floor(e.clientX - rect.left);
      const y = Math.floor(e.clientY - rect.top);
      ws.send(JSON.stringify({ type: "mouse", action: "click", x, y }));
    });

    window.addEventListener("keydown", e => {
      if (e.key.length === 1) {
        ws.send(JSON.stringify({ type: "key", char: e.key }));
      }
    });

    ws.onmessage = msg => {
      try {
        const ack = JSON.parse(msg.data);
        if (ack.ack === "mouse") {
          cursor.style.left = ack.x + "px";
          cursor.style.top = ack.y + "px";
        }
      } catch (e) {
        console.log("Non-JSON message", msg.data);
      }
    };
  </script>
</body>
</html>



