//Hangboard Timer Javascript Code

function updateTimerSettings() {
  var protocol = document.getElementById('protocolSelect').value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var response = JSON.parse(this.responseText);
      repAmount = response.repAmount;
      repTime = response.repTime;
      repRest = response.repRest;
      setAmount = response.setAmount;
      setRest = response.setRest;
      currentRep = 0; currentSet = 0;
      isHanging = true;
      currentTime = repTime;
      updateTimerDisplay();
    }
  };
  xhr.open('GET', '/getTimerSettings?protocol=' + protocol, true);
  xhr.send();
}

function tareScale(event) {
  fetch('/tare')
    .then(response => response.text())
    .then(data => console.log(data));
}

//


let isGettingReady = false;
let timerInterval; let currentTime = 0;
let repAmount, repTime, repRest, setAmount, setRest;
let currentRep = 0; let currentSet = 0; let isHanging = true;
const audioCtx = new (window.AudioContext || window.webkitAudioContext)();

function beep(duration = 200, frequency = 520) {
  console.log('Beep called'); // Debugging
  let oscillator = audioCtx.createOscillator();
  let gainNode = audioCtx.createGain();
  oscillator.connect(gainNode);
  gainNode.connect(audioCtx.destination);
  oscillator.frequency.value = frequency;
  oscillator.start(audioCtx.currentTime);
  oscillator.stop(audioCtx.currentTime + duration * 0.001);
}

function updateTimerDisplay() {
  let displayText;
  if (isGettingReady) {
    displayText = 'Get Ready ' + currentTime; // Display "Get Ready" during the get-ready phase
  } else {
    displayText = isHanging ? 'Hang ' : 'Rest ';
    displayText += currentTime;
  }
  document.getElementById('timerDisplay').textContent = displayText;
}


function timerTick() {
  if (isGettingReady) {
    // Get Ready phase countdown
    if (currentTime <= 1) {
      beep(); // Beep at the end of Get Ready phase
      isGettingReady = false;
      // Start the actual timer for the exercise
      currentRep = 0;
      currentSet = 0;
      isHanging = true;
      currentTime = repTime;
    } else {
      currentTime--;
    }
  } else {
    // Regular timer phase
    if (currentTime <= 1) {
      beep(); // Call beep at every transition
      if (isHanging) {
        if (currentRep < repAmount) {
          isHanging = false;
          currentTime = (repRest > 0) ? repRest : setRest;
          currentRep++;
        } else if (currentSet < setAmount) {
          currentSet++;
          currentRep = 0;
          isHanging = false;
          currentTime = setRest;
        } else {
          stopTimer();
          return;
        }
      } else {
        isHanging = true;
        currentTime = repTime;
      }
    } else {
      currentTime--;
    }
  }
  updateTimerDisplay();
}

function startTimer() {
  console.log('Attempting to start timer'); // Debugging
  if (audioCtx.state === 'suspended') {
    audioCtx.resume();
  }
  fetch('/startTimer');
  
  // Start the Get Ready phase
  isGettingReady = true;
  currentTime = 10; // 10 seconds for the Get Ready phase
  if (!timerInterval) {
    console.log('Starting new timer'); // Debugging
    timerInterval = setInterval(timerTick, 1000);
    updateTimerDisplay();
  }
}


function stopTimer() {
  console.log('Stopping timer'); // Debugging
  fetch('/stopTimer');
  if(timerInterval) {
    clearInterval(timerInterval);
  }
  timerInterval = null; // Clear the interval variable
  currentTime = 0; isHanging = true;
  currentRep = 0; currentSet = 0; // Reset rep and set counters
  updateTimerDisplay();
}

function updateTimerSettings() {
  var protocolSelect = document.getElementById('protocolSelect');
  if (!protocolSelect) return; // Early exit if the element is not found
  var protocol = protocolSelect.value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4 && xhr.status == 200) {
      try {
        var response = JSON.parse(xhr.responseText);
        repAmount = response.repAmount;
        repTime = response.repTime;
        repRest = response.repRest;
        setAmount = response.setAmount;
        setRest = response.setRest;
        currentRep = 0;
        currentSet = 0;
        isHanging = true;
        currentTime = repTime;
        updateTimerDisplay();
      } catch (e) {
        console.error('Error parsing JSON:', e); // Log parsing errors
      }
    }
  };
  xhr.open('GET', '/getTimerSettings?protocol=' + protocol, true);
  xhr.send();
}