// Create WebSocket connection
const socket = new WebSocket(WS_URL);

socket.onopen = () => {
  console.log("WebSocket connected.");
  wsStatus.textContent = "WS: connected";
};
socket.onclose = () => {
  console.log("WebSocket connection closed.");
  wsStatus.textContent = "WS: disconnected";
};
socket.onerror = (e) => {
  console.error("WebSocket error", e);
  wsStatus.textContent = "WS: error";
};

// send raw movement or JSON
function sendCmd(cmd) {
  if (socket.readyState === WebSocket.OPEN) {
    socket.send(cmd);
    console.log("Sent:", cmd);
  }
}

// structured blinker
function sendLight(cmd) {
  if (socket.readyState === WebSocket.OPEN) {
    const json = JSON.stringify({ action: 'light', command: cmd });
    socket.send(json);
    console.log("Sent light:", json);
  }
}

// joystick init
const manager = nipplejs.create({
  zone: document.getElementById("joystick"),
  mode: "static",
  position: { left: "50%", top: "50%" },
  color: "white"
});

let leftOn = false, rightOn = false;
let assistedMode = false;

const btnL = document.getElementById('btnLeft');
const btnR = document.getElementById('btnRight');
const btnAssiste = document.getElementById('btnAssiste');
const assistStatus = document.getElementById('assistStatus');
const brake = document.getElementById('brake');

// manual blinkers
btnL.onclick = () => {
  if (!leftOn) {
    leftOn = true; rightOn = false;
    btnL.classList.add('active'); btnR.classList.remove('active');
    sendLight('gauche');
  } else {
    leftOn = false;
    btnL.classList.remove('active');
    sendLight('stop');
  }
};
btnR.onclick = () => {
  if (!rightOn) {
    rightOn = true; leftOn = false;
    btnR.classList.add('active'); btnL.classList.remove('active');
    sendLight('droite');
  } else {
    rightOn = false;
    btnR.classList.remove('active');
    sendLight('stop');
  }
};

// assisted-mode toggle
btnAssiste.addEventListener('click', () => {
  assistedMode = !assistedMode;
  btnAssiste.classList.toggle('active', assistedMode);
  assistStatus.textContent = assistedMode
    ? "Mode assisté : ✅ ON"
    : "Mode assisté : ❌ OFF";
  if (socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ action: 'assisted', enabled: assistedMode }));
  }
});

manager.on("move", (evt, data) => {
  if (!data || !data.vector) return;
  const x = data.vector.x;

  if (assistedMode) {
    // auto-activate
    if (x > 0.5 && !rightOn) {
      rightOn = true; leftOn = false;
      btnR.classList.add('active'); btnL.classList.remove('active');
      sendLight('droite');
    } else if (x < -0.5 && !leftOn) {
      leftOn = true; rightOn = false;
      btnL.classList.add('active'); btnR.classList.remove('active');
      sendLight('gauche');
    }
    // auto-cancel when joystick crosses center
    else if (leftOn && x >= 0) {
      leftOn = false;
      btnL.classList.remove('active');
      sendLight('stop');
    } else if (rightOn && x <= 0) {
      rightOn = false;
      btnR.classList.remove('active');
      sendLight('stop');
    }
  } else {
    // manual: only cancel opposite
    if (x > 0.5 && leftOn) {
      leftOn = false;
      btnL.classList.remove('active');
      sendLight('stop');
    } else if (x < -0.5 && rightOn) {
      rightOn = false;
      btnR.classList.remove('active');
      sendLight('stop');
    }
  }

  // send movement
  const angle = data.angle ? data.angle.degree : 0;
  const distance = data.distance || 0;
  sendCmd(JSON.stringify({ action: 'move', angle, distance }));
});

// **New: on joystick release also cancel blinkers in assisted mode**
manager.on("end", () => {
  // stop movement
  sendCmd("x");

  if (assistedMode && (leftOn || rightOn)) {
    leftOn = rightOn = false;
    btnL.classList.remove('active');
    btnR.classList.remove('active');
    sendLight('stop');
  }
});

// brake
brake.addEventListener("click", () => sendCmd("x"));

// incoming brake only
socket.onmessage = evt => {
  let m;
  try { m = JSON.parse(evt.data); }
  catch { return; }
  if (m.action === 'brake') brake.classList.add('active');
  else brake.classList.remove('active');
};
