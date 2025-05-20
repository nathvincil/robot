const VIDEO_URL = "http://192.168.2.19:4747/video";
const WS_URL = "ws://192.168.1.141:81";


var websocket = new WebSocket(WS_URL);

websocket.onopen = function() {
  console.log("WebSocket connected!");
};

websocket.onerror = function(error) {
  console.error("WebSocket error: " + error);
};

// Function to send messages only when WebSocket is open
function sendData(data) {
  if (websocket.readyState === WebSocket.OPEN) {
    websocket.send(data);
  } else {
    console.log("WebSocket is not open yet. Waiting...");
  }
}

function showPage(pageId) {
    document.querySelectorAll('.container').forEach(el => el.style.display = 'none');
    let element = document.getElementById(pageId);
    if (element) {
        element.style.display = 'flex';
    }
}
// Show the selected mode and hide the others
function selectMode(modeId) {
    // Hide all modes
    const modes = document.querySelectorAll('.container');
    modes.forEach(mode => mode.style.display = 'none');

    // Show the selected mode
    const selectedMode = document.getElementById(modeId);
    if (selectedMode) {
        selectedMode.style.display = 'block';
    }
}

// This function is called when the back button is clicked on pages like Mode Vagabond, Mode Taxi, and Mode Explorateur
function goBackToAutoMode() {
    // Navigate back to the automode.html page
    window.location.href = 'automode.html';
}

function goBack() {
    window.history.back(); // Go back to the previous page in history
}

function envoyerTaxi() {
    const dB = document.getElementById("departBureau").value;
    const dR = document.getElementById("departRangee").value;
    const aB = document.getElementById("destBureau").value;
    const aR = document.getElementById("destRangee").value;

    // Create the command to send
    const message = `TAXI:${dR},${dB},${aR},${aB}`;



    // Call existing function
    sendCommand(message);

    // Display the result page
    selectMode('taxi-result'); // You can create a taxi result section similar to the others if needed
}

function sendCommand(command) {
   
    // Send the command to the robot/system via WebSocket
    if (websocket.readyState === WebSocket.OPEN) {
        websocket.send(command);
        console.log("Commande envoyée: ", command);
    } else {
        console.error("WebSocket is not open. Command not sent.");
    }
}

function sendBrakeCommand() {
    if (typeof websocket !== 'undefined' && websocket.readyState === WebSocket.OPEN) {
        websocket.send("BRAKE");
        console.log("🚨 Commande de frein envoyée");
    } else {
        console.warn("WebSocket non connecté !");
    }
}

