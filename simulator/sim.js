// =============================================================================
// Minibot OLED Simulator — sim.js
// Emulates the 128x64 SH1106 OLED display in a browser Canvas.
// Supports all pages: Eyes, Clock, Music, Pomodoro, Answer Book (with slot machine).
// =============================================================================

const canvas = document.getElementById("oled");
const ctx = canvas.getContext("2d");
const modeLabel = document.getElementById("modeLabel");

// --- Virtual Framebuffer (128x64, 1-bit) ---
const W = 128, H = 64;
const fb = new Uint8Array(W * H); // 0 = black, 1 = white

// --- Answer Pool (mirrors Globals.cpp) ---
const answersPool = [
    "Go for it", "Wait for it", "Too early", "It is decisive", "Unlikely",
    "Believe in it", "Trust yourself", "Follow your heart", "Ignore the doubt", "Count on it",
    "Let it go", "Keep going", "Take a break", "Try harder", "Look closer",
    "Move on", "Start now", "Be patient", "Don't worry", "Stay calm",
    "Act now", "Think twice", "Take the risk", "Play it safe", "Seek advice",
    "Listen carefully", "Speak up", "Forgive", "Forget it", "Hold on",
    "Let go", "Change direction", "Stay put", "Explore", "Simplify",
    "Work harder", "Rest now", "Enjoy it", "Be bold", "Be kind",
    "Smile", "Breath deep", "Why not?", "Definitely", "Of course",
    "Never", "Always", "Sometimes", "Once more", "Enough",
    "Big yes", "Small no", "Do it", "Don't do it", "Absolutely",
    "It's time", "Not yet", "Soon", "Later", "Tomorrow",
    "Today", "Right now", "In a year", "Forget that", "Remember this",
    "Yes, please", "No, thanks", "Maybe later", "Right away", "Eventually",
    "Within weeks", "Within months", "Years away", "Any moment", "Never ever",
    "Choose this", "Choose that", "Both", "Neither", "All in",
    "Step back", "Walk away", "Stop", "Look up", "Look down",
    "Inside you", "Ask a friend", "Ask family", "Trust luck", "Hard work pays",
    "Dream big", "Wake up", "Sleep on it", "It will pass", "Good things coming",
    "Be ready", "Watch out", "Safe choice", "Wild guess", "Sure thing",
    "No way", "Possible", "Impossible", "Worth it", "Waste of time",
    "Drink warm water", "Take some rest", "Wear a jacket", "Eat more!",
    "Treat yourself", "Take a sick day", "Go shopping!", "Take a deep breath",
    "You deserve a break", "Drink some coffee", "Have some tea", "Stop working!",
    "Time for a vacation", "Listen to music", "Close your laptop", "Protect your eyes",
    "Keep secret", "Tell everyone", "Help others", "Help yourself",
    "Be honest", "Stay true", "Fake it", "Make it",
    "Break it", "Fix it", "Leave it", "Take it", "Give it",
    "Save it", "Spend it", "Invest", "Wait and see", "Take charge",
    "Let others lead", "Cooperate", "Compete", "Win", "Learn",
    "Grow", "Shrink", "Expand", "Focus", "Distract",
    "Clear your mind", "Follow logic", "Follow instinct", "Be practical", "Be creative",
    "Serious", "Have fun", "Relax", "Hurry", "Slow down",
    "Speed up", "Turn left", "Turn right", "Go straight", "Go back",
    "New start", "End it", "Continue", "Pause", "Resume",
    "Accept it", "Reject it", "Negotiate", "Ask nicely", "Just take it", "Wait",
    "See the signs", "Ignore signs", "Trust fate", "Make fate", "Destiny",
    "Luck", "For sure", "I doubt it", "Who knows?", "Only you know", "Ask nature",
    "You got this", "Keep pushing", "Almost there", "One more try", "Stay strong",
    "Rise up", "Bounce back", "No regrets", "Own it", "Shine bright",
    "Stay hungry", "Think bigger", "Dig deeper", "Stand tall", "Press on",
    "Ask your cat", "Flip a coin", "Check the moon", "Roll a dice", "Read a book",
    "Dance first", "Sing it out", "Nap on it", "Eat snacks", "Touch grass",
    "Hug someone", "Count to ten", "Close your eyes", "Feel the wind", "Just vibes",
    "Look within", "Know yourself", "Find balance", "Seek peace", "Breathe out",
    "Let it flow", "Stay grounded", "Open your mind", "Free yourself", "Be still",
    "Trust the path", "Embrace change", "Accept chaos", "Find joy", "Stay curious",
    "Take the leap", "Build something", "Ship it now", "Break the mold", "Go big",
    "Say yes", "Say no", "Walk faster", "Run for it", "Jump in",
    "Make waves", "Be fearless", "Start fresh", "Level up", "Power through"
];

// --- Modes ---
const MODES = ["EYES", "CLOCK", "MUSIC", "POMODORO", "ANSWER"];
let currentModeIndex = 4; // Start on ANSWER page

// --- Answer Book State ---
let answerState = "idle"; // "idle", "spinning", "revealed"
let answerSpinStart = 0;
let answerSpinOffset = 0;
let answerReelIndices = [0, 1, 2, 3, 4];
let currentAnswer = "";

// --- Clock State ---
let clockXOffset = 0, clockYOffset = 0;
let lastClockShift = 0;

// --- Music State ---
const composers = ["Mozart", "Beethoven", "Chopin", "Richard", "Bach", "Vivaldi"];
let musicTrack = 0;
let musicPlaying = false;
let musicSel = 0; // 0=play, 1=next, 2=vol, 3=prev

// --- Pomodoro State ---
let pomoSetupStep = 0; // 0=work, 1=short, 2=long
let pomoWork = 25, pomoShort = 5, pomoLong = 15;
let pomoRunning = false;
let pomoTimerSec = 0;
let pomoTotalSec = 0;

// --- Eyes State ---
let eyeLX = 20, eyeLY = 14, eyeRX = 70, eyeRY = 14;
let eyeW = 30, eyeH = 36, eyeR = 12;
let eyeTargetLX = 20, eyeTargetLY = 14;
let eyeBlinkTimer = 0, eyeBlinkH = 36;
let eyeIdleTimer = 0;

// =============================================================================
// Drawing Primitives (operate on framebuffer)
// =============================================================================
function clearFB() { fb.fill(0); }

function setPixel(x, y, color) {
    x = Math.round(x); y = Math.round(y);
    if (x >= 0 && x < W && y >= 0 && y < H) fb[y * W + x] = color ? 1 : 0;
}

function drawHLine(x, y, w, c) { for (let i = 0; i < w; i++) setPixel(x + i, y, c); }
function drawVLine(x, y, h, c) { for (let i = 0; i < h; i++) setPixel(x, y + i, c); }

function drawRect(x, y, w, h, c) {
    drawHLine(x, y, w, c); drawHLine(x, y + h - 1, w, c);
    drawVLine(x, y, h, c); drawVLine(x + w - 1, y, h, c);
}

function fillRect(x, y, w, h, c) {
    for (let j = 0; j < h; j++) drawHLine(x, y + j, w, c);
}

function fillRoundRect(x, y, w, h, r, c) {
    r = Math.min(r, Math.floor(w / 2), Math.floor(h / 2));
    fillRect(x + r, y, w - 2 * r, h, c);
    fillRect(x, y + r, r, h - 2 * r, c);
    fillRect(x + w - r, y + r, r, h - 2 * r, c);
    fillCircleQuadrants(x + r, y + r, r, c);
    fillCircleQuadrants(x + w - r - 1, y + r, r, c);
    fillCircleQuadrants(x + r, y + h - r - 1, r, c);
    fillCircleQuadrants(x + w - r - 1, y + h - r - 1, r, c);
}

function fillCircleQuadrants(cx, cy, r, c) {
    for (let dy = -r; dy <= r; dy++) {
        for (let dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) setPixel(cx + dx, cy + dy, c);
        }
    }
}

// Simple 5x7 bitmap font
const FONT_W = 6, FONT_H = 8;
const FONT_DATA = {};

// Generate basic font from canvas measurement (simplified monospace approximation)
function measureText(text, scale) {
    return text.length * FONT_W * scale;
}

function drawText(text, x, y, scale, color) {
    // Use a temporary canvas to render text, then sample pixels
    const tc = document.createElement("canvas");
    const fontSize = 7 * scale;
    tc.width = text.length * FONT_W * scale + 4;
    tc.height = FONT_H * scale + 4;
    const tctx = tc.getContext("2d");
    tctx.fillStyle = "white";
    tctx.font = `bold ${fontSize}px monospace`;
    tctx.textBaseline = "top";
    tctx.fillText(text, 0, 1);
    const imgData = tctx.getImageData(0, 0, tc.width, tc.height);

    for (let py = 0; py < tc.height && (y + py) < H; py++) {
        for (let px = 0; px < tc.width && (x + px) < W; px++) {
            const idx = (py * tc.width + px) * 4;
            if (imgData.data[idx] > 128) {
                setPixel(x + px, y + py, color);
            }
        }
    }
    return tc.width;
}

function drawTextCentered(text, y, scale, color) {
    const tc = document.createElement("canvas");
    const fontSize = 7 * scale;
    tc.width = 256;
    tc.height = FONT_H * scale + 4;
    const tctx = tc.getContext("2d");
    tctx.font = `bold ${fontSize}px monospace`;
    const m = tctx.measureText(text);
    const tw = Math.ceil(m.width);
    const x = Math.floor((W - tw) / 2);
    drawText(text, x, y, scale, color);
}

// Proportional font rendering (for Helvetica-like appearance)
function drawTextProp(text, x, y, fontSize, bold, color) {
    const tc = document.createElement("canvas");
    tc.width = 256;
    tc.height = fontSize + 6;
    const tctx = tc.getContext("2d");
    tctx.fillStyle = "white";
    const weight = bold ? "bold" : "normal";
    tctx.font = `${weight} ${fontSize}px "Helvetica Neue", Helvetica, Arial, sans-serif`;
    tctx.textBaseline = "top";
    tctx.fillText(text, 0, 1);
    const imgData = tctx.getImageData(0, 0, tc.width, tc.height);
    for (let py = 0; py < tc.height && (y + py) < H; py++) {
        for (let px = 0; px < tc.width && (x + px) < W; px++) {
            const idx = (py * tc.width + px) * 4;
            if (imgData.data[idx] > 100) setPixel(x + px, y + py, color);
        }
    }
    return Math.ceil(tctx.measureText(text).width);
}

function drawTextPropCentered(text, y, fontSize, bold, color) {
    const tc = document.createElement("canvas");
    tc.width = 1;
    tc.height = 1;
    const tctx = tc.getContext("2d");
    const weight = bold ? "bold" : "normal";
    tctx.font = `${weight} ${fontSize}px "Helvetica Neue", Helvetica, Arial, sans-serif`;
    const tw = Math.ceil(tctx.measureText(text).width);
    const x = Math.floor((W - tw) / 2);
    drawTextProp(text, x, y, fontSize, bold, color);
}

function measureTextProp(text, fontSize, bold) {
    const tc = document.createElement("canvas");
    tc.width = 1; tc.height = 1;
    const tctx = tc.getContext("2d");
    const weight = bold ? "bold" : "normal";
    tctx.font = `${weight} ${fontSize}px "Helvetica Neue", Helvetica, Arial, sans-serif`;
    return Math.ceil(tctx.measureText(text).width);
}

// =============================================================================
// Page Renderers
// =============================================================================

function drawEyesPage(now) {
    // Idle movement
    if (now - eyeIdleTimer > 2000) {
        eyeIdleTimer = now;
        eyeTargetLX = Math.floor(Math.random() * 30);
        eyeTargetLY = Math.floor(Math.random() * 20);
    }
    eyeLX += (eyeTargetLX - eyeLX) * 0.1;
    eyeLY += (eyeTargetLY - eyeLY) * 0.1;

    // Blink
    if (now - eyeBlinkTimer > 3000 + Math.random() * 3000) {
        eyeBlinkTimer = now;
        eyeBlinkH = 2;
    }
    if (eyeBlinkH < eyeH) eyeBlinkH = Math.min(eyeH, eyeBlinkH + 4);

    const lx = Math.round(eyeLX);
    const ly = Math.round(eyeLY);
    const space = 18;
    fillRoundRect(lx, ly + Math.floor((eyeH - eyeBlinkH) / 2), eyeW, eyeBlinkH, eyeR, 1);
    fillRoundRect(lx + eyeW + space, ly + Math.floor((eyeH - eyeBlinkH) / 2), eyeW, eyeBlinkH, eyeR, 1);
}

function drawClockPage(now) {
    if (now - lastClockShift > 60000) {
        clockXOffset = Math.floor(Math.random() * 3);
        clockYOffset = Math.floor(Math.random() * 3);
        lastClockShift = now;
    }
    const d = new Date();
    let h = d.getHours();
    if (h === 0) h = 12; else if (h > 12) h -= 12;
    const hStr = h.toString().padStart(2, "0");
    const mStr = d.getMinutes().toString().padStart(2, "0");
    const days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
    const dateStr = `${(d.getMonth() + 1).toString().padStart(2, "0")}/${d.getDate().toString().padStart(2, "0")}`;

    // Date line
    drawTextProp(dateStr, 5 + clockXOffset, 2 + clockYOffset, 10, false, 1);
    drawTextProp(days[d.getDay()], 56 + clockXOffset, 2 + clockYOffset, 10, false, 1);
    drawTextProp(d.getFullYear().toString(), 99 + clockXOffset, 2 + clockYOffset, 10, false, 1);

    // Time
    const timeY = 24 + clockYOffset;
    let cx = 15 + clockXOffset;
    cx += drawTextProp(hStr, cx, timeY, 24, true, 1);
    cx += 4;
    if (Date.now() % 1000 < 500) drawTextProp(":", cx, timeY - 2, 24, true, 1);
    cx += drawTextProp(":", cx, timeY, 24, true, 1) + 2;
    drawTextProp(mStr, cx, timeY, 24, true, 1);
}

function drawMusicPage() {
    const name = composers[musicTrack] || "Track ?";
    drawTextPropCentered(name, 16, 14, true, 1);

    // Simple button indicators
    drawTextProp("<<", 14, 44, 10, true, 1);
    drawTextPropCentered(musicPlaying ? "||" : ">", 44, 12, true, 1);
    drawTextProp(">>", 102, 44, 10, true, 1);

    // Selection bracket
    const positions = [[52, 40, 24, 18], [10, 40, 24, 18], [98, 40, 24, 18], [0, 0, 16, 10]];
    if (musicSel < positions.length) {
        const [rx, ry, rw, rh] = positions[musicSel];
        drawRect(rx, ry, rw, rh, 1);
    }
}

function drawPomodoroPage() {
    if (!pomoRunning) {
        // Setup phase
        drawTextPropCentered("Pomodoro", 1, 12, true, 1);
        drawHLine(21, 15, 83, 1);

        const labels = ["Focus Time", "Short Break", "Long Break"];
        const values = [pomoWork, pomoShort, pomoLong];
        drawTextProp(labels[pomoSetupStep], 7, 28, 10, false, 1);
        drawTextPropCentered(values[pomoSetupStep].toString(), 26, 18, true, 1);
        drawTextProp("min", 102, 30, 10, false, 1);
        drawTextProp("[L]", 15, 54, 8, false, 1);
        drawTextProp("[R]", 95, 54, 8, false, 1);
    } else {
        // Running phase
        drawTextProp("WORK", 4, 1, 10, true, 1);
        const m = Math.floor(pomoTimerSec / 60);
        const s = pomoTimerSec % 60;
        const tStr = `${m.toString().padStart(2, "0")}:${s.toString().padStart(2, "0")}`;
        drawTextPropCentered(tStr, 16, 22, true, 1);

        // Progress bar
        const barX = 4, barY = 48, barW = 120, barH = 10;
        drawRect(barX, barY, barW, barH, 1);
        const progress = pomoTotalSec > 0 ? 1 - pomoTimerSec / pomoTotalSec : 0;
        const fillW = Math.floor(barW * progress);
        if (fillW > 0) fillRect(barX, barY, fillW, barH, 1);
    }
}

function drawAnswerPage(now) {
    if (answerState === "idle") {
        drawTextPropCentered("Book of Answers", 1, 9, false, 1);
        // Simple book icon placeholder
        drawRect(44, 16, 40, 32, 1);
        drawVLine(64, 16, 32, 1);
        drawTextPropCentered("Press [R]", 53, 9, false, 1);
    }
    else if (answerState === "spinning") {
        const elapsed = now - answerSpinStart;
        const totalDuration = 2500;
        let progress = elapsed / totalDuration;
        if (progress > 1) progress = 1;
        const easedSpeed = 12 * (1 - progress) * (1 - progress);

        answerSpinOffset += easedSpeed;
        const lineHeight = 20;
        while (answerSpinOffset >= lineHeight) {
            answerSpinOffset -= lineHeight;
            for (let i = 0; i < 4; i++) answerReelIndices[i] = answerReelIndices[i + 1];
            answerReelIndices[4] = Math.floor(Math.random() * answersPool.length);
        }

        drawTextPropCentered("Book of Answers", 1, 9, false, 1);
        drawHLine(0, 13, 128, 1);

        const yBase = 22 - Math.floor(answerSpinOffset);
        for (let i = 0; i < 5; i++) {
            const y = yBase + i * lineHeight;
            if (y < 8 || y > 62) continue;
            drawTextPropCentered(answersPool[answerReelIndices[i]], y, 10, true, 1);
        }

        // Focus bracket
        drawRect(2, 22, 124, 18, 1);

        if (elapsed >= totalDuration) {
            answerState = "revealed";
            currentAnswer = answersPool[answerReelIndices[2]];
        }
    }
    else if (answerState === "revealed") {
        drawTextPropCentered(currentAnswer, 24, 13, true, 1);
        drawTextProp("[L]", 15, 54, 8, false, 1);
    }
}

// =============================================================================
// Render Loop
// =============================================================================
function render() {
    const now = performance.now();
    clearFB();

    switch (MODES[currentModeIndex]) {
        case "EYES":     drawEyesPage(now); break;
        case "CLOCK":    drawClockPage(now); break;
        case "MUSIC":    drawMusicPage(); break;
        case "POMODORO": drawPomodoroPage(); break;
        case "ANSWER":   drawAnswerPage(now); break;
    }

    // Blit framebuffer to canvas
    const imgData = ctx.createImageData(W, H);
    for (let i = 0; i < W * H; i++) {
        const v = fb[i] ? 255 : 0;
        imgData.data[i * 4] = v;
        imgData.data[i * 4 + 1] = v;
        imgData.data[i * 4 + 2] = v;
        imgData.data[i * 4 + 3] = 255;
    }
    ctx.putImageData(imgData, 0, 0);

    modeLabel.textContent = MODES[currentModeIndex];
    requestAnimationFrame(render);
}

// =============================================================================
// Input Handling
// =============================================================================
document.addEventListener("keydown", (e) => {
    const mode = MODES[currentModeIndex];

    if (e.key === "ArrowLeft") {
        // Left button (L)
        e.preventDefault();
        if (mode === "ANSWER" && answerState === "revealed") {
            answerState = "idle";
            return;
        }
        // Cycle mode backward
        currentModeIndex = (currentModeIndex + 1) % MODES.length;
        if (MODES[currentModeIndex] === "ANSWER") answerState = "idle";
        if (MODES[currentModeIndex] === "POMODORO") { pomoRunning = false; pomoSetupStep = 0; }
    }
    else if (e.key === "ArrowRight") {
        // Right button (R)
        e.preventDefault();
        if (mode === "ANSWER") {
            if (answerState === "idle") {
                // Start spinning
                for (let i = 0; i < 5; i++) {
                    answerReelIndices[i] = Math.floor(Math.random() * answersPool.length);
                }
                answerSpinOffset = 0;
                answerSpinStart = performance.now();
                answerState = "spinning";
            }
        }
        else if (mode === "MUSIC") {
            if (musicSel === 0) musicPlaying = !musicPlaying;
            else if (musicSel === 1) musicTrack = (musicTrack + 1) % composers.length;
            else if (musicSel === 3) musicTrack = (musicTrack - 1 + composers.length) % composers.length;
        }
        else if (mode === "POMODORO") {
            if (!pomoRunning) {
                pomoSetupStep++;
                if (pomoSetupStep > 2) {
                    pomoRunning = true;
                    pomoTimerSec = pomoWork * 60;
                    pomoTotalSec = pomoTimerSec;
                    pomoSetupStep = 0;
                }
            }
        }
    }
    else if (e.key === "ArrowUp" || e.key === "ArrowDown") {
        e.preventDefault();
        const dir = e.key === "ArrowUp" ? 1 : -1;
        if (mode === "MUSIC") {
            musicSel = (musicSel + dir + 4) % 4;
        }
        else if (mode === "POMODORO" && !pomoRunning) {
            const vals = [pomoWork, pomoShort, pomoLong];
            vals[pomoSetupStep] = Math.max(1, Math.min(60, vals[pomoSetupStep] + dir * 5));
            [pomoWork, pomoShort, pomoLong] = vals;
        }
    }
});

// Pomodoro timer tick
setInterval(() => {
    if (pomoRunning && pomoTimerSec > 0) pomoTimerSec--;
}, 1000);

// Start
requestAnimationFrame(render);
