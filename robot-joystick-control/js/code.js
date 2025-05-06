// This file contains additional JavaScript code that handles user interactions, joystick controls, and communication with the robot.

const VIDEO_URL = 'your_video_feed_url_here'; // Replace with your actual video feed URL

document.addEventListener('DOMContentLoaded', () => {
    const wsStatus = document.getElementById('wsStatus');
    const btnLeft = document.getElementById('btnLeft');
    const btnRight = document.getElementById('btnRight');
    const brakeButton = document.getElementById('brake');
    const assistButton = document.getElementById('btnAssiste');
    const assistStatus = document.getElementById('assistStatus');

    // WebSocket connection
    const socket = new WebSocket('ws://your_websocket_url_here'); // Replace with your actual WebSocket URL

    socket.onopen = () => {
        wsStatus.textContent = 'WS: Connected';
    };

    socket.onclose = () => {
        wsStatus.textContent = 'WS: Disconnected';
    };

    // Button event listeners
    btnLeft.addEventListener('click', () => {
        socket.send(JSON.stringify({ action: 'move', direction: 'left' }));
    });

    btnRight.addEventListener('click', () => {
        socket.send(JSON.stringify({ action: 'move', direction: 'right' }));
    });

    brakeButton.addEventListener('click', () => {
        socket.send(JSON.stringify({ action: 'brake' }));
    });

    assistButton.addEventListener('click', () => {
        const isAssisted = assistStatus.textContent === 'Assisted';
        assistStatus.textContent = isAssisted ? 'Not Assisted' : 'Assisted';
        socket.send(JSON.stringify({ action: 'toggleAssist', status: !isAssisted }));
    });

    // Joystick setup
    const joystickZone = document.getElementById('joystick');
    const joystick = nipplejs.create({
        zone: joystickZone,
        mode: 'static',
        position: { left: '50%', bottom: '50%' },
        color: 'blue',
    });

    joystick.on('move', (evt, data) => {
        const direction = {
            x: data.position.x,
            y: data.position.y,
        };
        socket.send(JSON.stringify({ action: 'move', direction }));
    });

    joystick.on('end', () => {
        socket.send(JSON.stringify({ action: 'stop' }));
    });
});