/**
 * ==============================================================================
 * KeyPill Hardware Token Test Bench - Application Logic
 * Open-Source USB HID Evaluation Bench for STM32F103C8T6
 * ==============================================================================
 */

(() => {
  'use strict';

  // Internal State
  const state = {
    username: 'admin@keypill.local',
    currentSession: null,
    auditLogs: [],
    telemetry: {
      timestamps: [],
      intervals: [],
      charsTyped: 0,
      startTime: 0,
      lastKeyTime: 0,
      isHardwareBurst: false,
      enterTriggered: false
    }
  };

  // DOM Elements
  const el = {
    // Navbar
    hwStatusDot: document.getElementById('hw-status-dot'),
    hwStatusText: document.getElementById('hw-status-text'),

    // Form & Left Panel
    authSuccessBanner: document.getElementById('auth-success-banner'),
    authSuccessDetails: document.getElementById('auth-success-details'),
    btnRearmAfterSuccess: document.getElementById('btn-rearm-after-success'),
    btnFocusPassword: document.getElementById('btn-focus-password'),
    focusButtonLabel: document.getElementById('focus-button-label'),
    loginForm: document.getElementById('login-form'),
    inputUsername: document.getElementById('input-username'),
    inputPassword: document.getElementById('input-password'),
    passwordFieldContainer: document.getElementById('password-field-container'),
    btnTogglePwd: document.getElementById('btn-toggle-pwd'),
    eyeLabelShow: document.getElementById('eye-label-show'),
    eyeLabelHide: document.getElementById('eye-label-hide'),
    passwordErrorAlert: document.getElementById('password-error-alert'),
    errorMessageText: document.getElementById('error-message-text'),
    btnClearInput: document.getElementById('btn-clear-input'),

    // Complexity Tags
    ruleLen: document.getElementById('rule-len'),
    ruleUpper: document.getElementById('rule-upper'),
    ruleLower: document.getElementById('rule-lower'),
    ruleDigit: document.getElementById('rule-digit'),
    ruleSym: document.getElementById('rule-sym'),

    // Telemetry Panel
    telemetryLiveBadge: document.getElementById('telemetry-live-badge'),
    metricChars: document.getElementById('metric-chars'),
    metricCharsExpected: document.getElementById('metric-chars-expected'),
    metricDelay: document.getElementById('metric-delay'),
    metricSpeed: document.getElementById('metric-speed'),
    metricEntropy: document.getElementById('metric-entropy'),
    metricEntropyRating: document.getElementById('metric-entropy-rating'),
    metricSource: document.getElementById('metric-source'),
    metricSourceSub: document.getElementById('metric-source-sub'),
    statJitter: document.getElementById('stat-jitter'),
    streamEnterBadge: document.getElementById('stream-enter-badge'),
    latencyBarsContainer: document.getElementById('latency-bars-container'),
    bufferByteCount: document.getElementById('buffer-byte-count'),
    streamDisplay: document.getElementById('stream-display'),

    // Charset Counters
    countUpper: document.getElementById('count-upper'),
    countLower: document.getElementById('count-lower'),
    countDigit: document.getElementById('count-digit'),
    countSymbol: document.getElementById('count-symbol'),

    // Terminal
    eventLogTerminal: document.getElementById('event-log-terminal'),
    btnExportCsv: document.getElementById('btn-export-csv'),
    btnExportJson: document.getElementById('btn-export-json'),
    btnClearLogs: document.getElementById('btn-clear-logs')
  };

  // ============================================================================
  // Terminal Event Logger
  // ============================================================================
  function logEvent(level, message) {
    const now = new Date();
    const timeStr = now.toTimeString().split(' ')[0] + '.' + String(now.getMilliseconds()).padStart(3, '0');

    let levelClass = 'log-info';
    if (level === 'SUCCESS') levelClass = 'log-success';
    else if (level === 'STREAM') levelClass = 'log-stream';
    else if (level === 'WARN') levelClass = 'log-warn';
    else if (level === 'ERROR') levelClass = 'log-err';

    const line = document.createElement('div');
    line.className = 'terminal-line';
    line.innerHTML = `<span class="log-ts">[${timeStr}]</span> <span class="${levelClass}">[${level}]</span> <span>${escapeHtml(message)}</span>`;

    el.eventLogTerminal.appendChild(line);
    el.eventLogTerminal.scrollTop = el.eventLogTerminal.scrollHeight;
  }

  // ============================================================================
  // Shannon Entropy & Complexity Calculation
  // ============================================================================
  function calculateMetrics(pwd) {
    if (!pwd || pwd.length === 0) {
      return {
        length: 0,
        hasUpper: false,
        hasLower: false,
        hasDigit: false,
        hasSymbol: false,
        counts: { upper: 0, lower: 0, digit: 0, symbol: 0 },
        entropyBits: 0,
        entropyRating: 'Awaiting payload',
        isKeyPillCompliant: false
      };
    }

    let upper = 0, lower = 0, digit = 0, symbol = 0;
    const len = pwd.length;

    for (let i = 0; i < len; i++) {
      const code = pwd.charCodeAt(i);
      if (code >= 65 && code <= 90) upper++;
      else if (code >= 97 && code <= 122) lower++;
      else if (code >= 48 && code <= 57) digit++;
      else symbol++;
    }

    let pool = 0;
    if (upper > 0) pool += 26;
    if (lower > 0) pool += 26;
    if (digit > 0) pool += 10;
    if (symbol > 0) pool += 32;

    const entropyBits = pool > 0 ? (len * (Math.log(pool) / Math.LN2)).toFixed(1) : 0;
    
    let rating = 'Sub-optimal (<40 bits)';
    if (entropyBits >= 80) rating = 'NIST SP 800-63B Compliant (80+ bits)';
    else if (entropyBits >= 64) rating = 'Strong (64+ bits)';
    else if (entropyBits >= 40) rating = 'Adequate (40-64 bits)';

    const isKeyPillLen = (len === 12 || len === 14 || len === 16);
    const hasFullMix = (upper > 0 && lower > 0 && digit > 0 && symbol > 0);
    const isKeyPillCompliant = isKeyPillLen && hasFullMix;

    return {
      length: len,
      hasUpper: upper > 0,
      hasLower: lower > 0,
      hasDigit: digit > 0,
      hasSymbol: symbol > 0,
      counts: { upper, lower, digit, symbol },
      entropyBits: Number(entropyBits),
      entropyRating: rating,
      isKeyPillCompliant
    };
  }

  // ============================================================================
  // Telemetry Engine & Keystroke Listener
  // ============================================================================
  function resetTelemetry() {
    state.telemetry = {
      timestamps: [],
      intervals: [],
      charsTyped: 0,
      startTime: 0,
      lastKeyTime: 0,
      isHardwareBurst: false,
      enterTriggered: false
    };

    el.metricChars.textContent = '0';
    el.metricDelay.textContent = '0.0';
    el.metricSpeed.textContent = '0.0 chars/sec';
    el.metricEntropy.textContent = '0.0';
    el.metricEntropyRating.textContent = 'Awaiting payload';
    el.metricSource.textContent = 'IDLE';
    el.metricSource.className = 'metric-num font-mono';
    el.metricSourceSub.textContent = 'Waiting for input stream';

    el.statJitter.textContent = 'Jitter: \u00B10.0 ms';
    el.streamEnterBadge.textContent = 'Return: Awaiting';
    el.streamEnterBadge.className = 'stat-pill font-mono';

    el.latencyBarsContainer.innerHTML = '<div class="empty-hist-hint">Keystroke timing profile will plot here in real time as characters stream in.</div>';
    el.bufferByteCount.textContent = '0 bytes';
    el.streamDisplay.innerHTML = '<span class="buffer-placeholder">[Buffer Empty &bull; Ready to capture USB HID keystrokes]</span>';

    el.countUpper.textContent = '0';
    el.countLower.textContent = '0';
    el.countDigit.textContent = '0';
    el.countSymbol.textContent = '0';

    el.telemetryLiveBadge.textContent = 'LISTENING';
    el.telemetryLiveBadge.className = 'status-indicator-badge';
  }

  function handleKeydown(e) {
    const now = performance.now();

    // Check for Return / ENTER key (Auto-submit from KeyPill hardware)
    if (e.key === 'Enter') {
      state.telemetry.enterTriggered = true;
      el.streamEnterBadge.textContent = 'Return: Captured \u2713';
      el.streamEnterBadge.className = 'stat-pill font-mono submitted';
      logEvent('STREAM', 'Return (0x0D) auto-submit key received from hardware.');
      return;
    }

    // Ignore navigation / modifier keys
    if (e.key.length > 1) return;

    if (state.telemetry.timestamps.length === 0) {
      state.telemetry.startTime = now;
      state.telemetry.lastKeyTime = now;
      el.telemetryLiveBadge.textContent = 'RECEIVING STREAM';
      el.telemetryLiveBadge.className = 'status-indicator-badge receiving';
      logEvent('STREAM', `Keystroke stream began. First character: '${e.key}'`);
    } else {
      const deltaT = now - state.telemetry.lastKeyTime;
      state.telemetry.intervals.push(deltaT);
      state.telemetry.lastKeyTime = now;
      renderLatencyBar(deltaT, state.telemetry.intervals.length);
    }

    state.telemetry.timestamps.push(now);
    state.telemetry.charsTyped++;

    analyzeCadence();
  }

  function renderLatencyBar(deltaT, index) {
    const placeholder = el.latencyBarsContainer.querySelector('.empty-hist-hint');
    if (placeholder) placeholder.remove();

    const bar = document.createElement('div');
    bar.className = 'hist-bar';

    // Normalize height (max 75px for ~150ms)
    const clampedDelta = Math.min(Math.max(deltaT, 5), 200);
    const heightPercent = Math.max(12, Math.min(100, (clampedDelta / 150) * 100));
    bar.style.height = `${heightPercent}%`;
    bar.title = `Char #${index}: \u0394t = ${Math.round(deltaT)} ms`;

    // Color code fast (< 50ms) vs manual (> 100ms)
    if (deltaT < 50) {
      bar.classList.add('fast');
    } else if (deltaT > 100) {
      bar.classList.add('slow');
    }

    el.latencyBarsContainer.appendChild(bar);
    el.latencyBarsContainer.scrollLeft = el.latencyBarsContainer.scrollWidth;
  }

  function analyzeCadence() {
    const intervals = state.telemetry.intervals;
    if (intervals.length === 0) return;

    const sum = intervals.reduce((a, b) => a + b, 0);
    const mean = sum / intervals.length;

    const variance = intervals.reduce((acc, val) => acc + Math.pow(val - mean, 2), 0) / intervals.length;
    const stdDev = Math.sqrt(variance);

    const charsPerSec = mean > 0 ? (1000 / mean).toFixed(1) : '0.0';
    el.metricDelay.textContent = mean.toFixed(1);
    el.metricSpeed.textContent = `${charsPerSec} chars/sec`;
    el.statJitter.textContent = `Jitter: \u00B1${stdDev.toFixed(1)} ms`;

    // KeyPill hardware typing (USB HID or xdotool companion daemon) has rapid uniform cadence (<65ms, stdDev <40ms)
    if (intervals.length >= 3) {
      if (mean < 65 && stdDev < 40) {
        state.telemetry.isHardwareBurst = true;
        el.metricSource.textContent = 'STM32 HID';
        el.metricSource.className = 'metric-num font-mono source-hardware';
        el.metricSourceSub.textContent = 'Hardware Token Injection';
      } else {
        state.telemetry.isHardwareBurst = false;
        el.metricSource.textContent = 'MANUAL';
        el.metricSource.className = 'metric-num font-mono source-human';
        el.metricSourceSub.textContent = 'Human Typist Entry';
      }
    }
  }

  function handlePasswordInput() {
    const val = el.inputPassword.value;
    const metrics = calculateMetrics(val);

    // Update Metrics
    el.metricChars.textContent = metrics.length;
    el.metricEntropy.textContent = metrics.entropyBits;
    el.metricEntropyRating.textContent = metrics.entropyRating;

    // Buffer Preview & Byte Count
    el.bufferByteCount.textContent = `${val.length} bytes`;
    if (val.length > 0) {
      const masked = val.replace(/./g, '\u25CF ');
      el.streamDisplay.innerHTML = `<span>${escapeHtml(masked)}</span>`;
    } else {
      el.streamDisplay.innerHTML = '<span class="buffer-placeholder">[Buffer Empty &bull; Ready to capture USB HID keystrokes]</span>';
    }

    // Charset Counters
    el.countUpper.textContent = metrics.counts.upper;
    el.countLower.textContent = metrics.counts.lower;
    el.countDigit.textContent = metrics.counts.digit;
    el.countSymbol.textContent = metrics.counts.symbol;

    // Rules Badges
    updateChip(el.ruleLen, metrics.length === 12 || metrics.length === 14 || metrics.length === 16);
    updateChip(el.ruleUpper, metrics.hasUpper);
    updateChip(el.ruleLower, metrics.hasLower);
    updateChip(el.ruleDigit, metrics.hasDigit);
    updateChip(el.ruleSym, metrics.hasSymbol);

    // Hide error alert once user/hardware corrects
    if (!el.passwordErrorAlert.classList.contains('hidden')) {
      el.passwordErrorAlert.classList.add('hidden');
      el.inputPassword.removeAttribute('aria-invalid');
    }
  }

  function updateChip(chipEl, isValid) {
    if (isValid) {
      chipEl.classList.add('valid');
    } else {
      chipEl.classList.remove('valid');
    }
  }

  // ============================================================================
  // Focus / Arming Logic
  // ============================================================================
  function focusForHardware() {
    el.btnFocusPassword.classList.add('armed');
    el.focusButtonLabel.textContent = 'Password Field Focused \u2022 Awaiting KeyPill...';
    el.passwordFieldContainer.classList.add('listening');

    el.inputPassword.focus();
    el.inputPassword.select();

    el.telemetryLiveBadge.textContent = 'READY (FOCUSED)';
    el.telemetryLiveBadge.className = 'status-indicator-badge receiving';

    logEvent('INFO', 'Password buffer armed and focused. Ready for KeyPill rotary knob selection.');

    const blurHandler = () => {
      setTimeout(() => {
        if (document.activeElement !== el.inputPassword) {
          disarmFocus();
        }
      }, 300);
      el.inputPassword.removeEventListener('blur', blurHandler);
    };
    el.inputPassword.addEventListener('blur', blurHandler);
  }

  function disarmFocus() {
    el.btnFocusPassword.classList.remove('armed');
    el.focusButtonLabel.textContent = 'Focus Password Field for Hardware Input';
    el.passwordFieldContainer.classList.remove('listening');
  }

  // ============================================================================
  // Form Submission & Inline Verification
  // ============================================================================
  function handleLoginSubmit(e) {
    e.preventDefault();
    const username = el.inputUsername.value.trim() || 'admin@keypill.local';
    const password = el.inputPassword.value;
    const metrics = calculateMetrics(password);

    if (password.length === 0) {
      showError('Please enter or inject a password.');
      logEvent('WARN', 'Sign-in attempted with empty password buffer.');
      return;
    }

    const sessionDuration = state.telemetry.timestamps.length > 1 
      ? Math.round(state.telemetry.timestamps[state.telemetry.timestamps.length - 1] - state.telemetry.timestamps[0])
      : 0;

    const sourceName = state.telemetry.isHardwareBurst ? 'STM32 Blue Pill (USB HID)' : 'Manual Entry';

    const logEntry = {
      timestamp: new Date().toLocaleTimeString(),
      username,
      passwordMasked: password.length > 4 ? password.slice(0, 2) + '\u2022'.repeat(password.length - 4) + password.slice(-2) : '\u2022'.repeat(password.length),
      passwordRaw: password,
      length: password.length,
      durationMs: sessionDuration,
      entropy: metrics.entropyBits,
      isHardware: state.telemetry.isHardwareBurst,
      source: sourceName,
      status: metrics.isKeyPillCompliant ? 'VALIDATED' : 'ACCEPTED (CUSTOM)'
    };

    state.auditLogs.unshift(logEntry);
    state.currentSession = logEntry;

    // Show inline success banner without jumping away
    el.authSuccessDetails.innerHTML = `Authenticated as <strong>${escapeHtml(username)}</strong> &bull; Length: ${password.length} chars &bull; Entropy: ${metrics.entropyBits} bits &bull; ${escapeHtml(sourceName)}`;
    el.authSuccessBanner.classList.remove('hidden');

    logEvent('SUCCESS', `Authenticated user '${username}' [${password.length} chars, ${metrics.entropyBits} bits entropy, ${sessionDuration}ms duration]`);
  }

  function showError(msg) {
    el.errorMessageText.textContent = msg;
    el.passwordErrorAlert.classList.remove('hidden');
    el.inputPassword.setAttribute('aria-invalid', 'true');
  }

  function resetForNextTest() {
    el.authSuccessBanner.classList.add('hidden');
    el.inputPassword.value = '';
    resetTelemetry();
    handlePasswordInput();
    focusForHardware();
    logEvent('INFO', 'Test bench reset. Focus placed on password field for next input.');
  }

  // ============================================================================
  // Synthetic Hardware Simulation (Instant demo without physical STM32)
  // ============================================================================
  function simulateKeyPillBurst(length = 14) {
    resetForNextTest();

    const uppers = 'ABCDEFGHJKLMNPQRSTUVWXYZ';
    const lowers = 'abcdefghijkmnopqrstuvwxyz';
    const digits = '23456789';
    const syms = '!@#$%^&*()-_=+';
    const all = uppers + lowers + digits + syms;

    let chars = [
      uppers[Math.floor(Math.random() * uppers.length)],
      lowers[Math.floor(Math.random() * lowers.length)],
      digits[Math.floor(Math.random() * digits.length)],
      syms[Math.floor(Math.random() * syms.length)]
    ];

    while (chars.length < length) {
      chars.push(all[Math.floor(Math.random() * all.length)]);
    }

    // Shuffle characters
    for (let i = chars.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [chars[i], chars[j]] = [chars[j], chars[i]];
    }

    logEvent('STREAM', `Simulating ${length}-character KeyPill hardware keystroke burst (~18ms cadence)...`);

    let i = 0;
    function injectNext() {
      if (i < chars.length) {
        const char = chars[i];
        el.inputPassword.value += char;

        const evt = new KeyboardEvent('keydown', { key: char, bubbles: true });
        handleKeydown(evt);
        handlePasswordInput();

        i++;
        const jitter = Math.floor(Math.random() * 8) - 4; // 18ms +/- 4ms
        setTimeout(injectNext, Math.max(10, 18 + jitter));
      } else {
        setTimeout(() => {
          const enterEvt = new KeyboardEvent('keydown', { key: 'Enter', bubbles: true });
          handleKeydown(enterEvt);
          el.loginForm.requestSubmit();
        }, 35);
      }
    }

    setTimeout(injectNext, 80);
  }

  // ============================================================================
  // Export Data (JSON & CSV)
  // ============================================================================
  function exportAuditLogs(format = 'csv') {
    if (state.auditLogs.length === 0) {
      alert('No test observations recorded yet. Trigger a password injection first.');
      return;
    }

    let dataStr = '';
    let filename = `keypill-telemetry-${Date.now()}`;

    if (format === 'json') {
      dataStr = 'data:text/json;charset=utf-8,' + encodeURIComponent(JSON.stringify(state.auditLogs, null, 2));
      filename += '.json';
    } else {
      const headers = ['Timestamp', 'Username', 'Password', 'Length', 'Duration_ms', 'Entropy_bits', 'Hardware_Source', 'Status'];
      const rows = state.auditLogs.map(l => [
        `"${l.timestamp}"`,
        `"${l.username}"`,
        `"${l.passwordRaw.replace(/"/g, '""')}"`,
        l.length,
        l.durationMs,
        l.entropy,
        `"${l.source}"`,
        `"${l.status}"`
      ]);
      const csvContent = [headers.join(','), ...rows.map(r => r.join(','))].join('\n');
      dataStr = 'data:text/csv;charset=utf-8,' + encodeURIComponent(csvContent);
      filename += '.csv';
    }

    const a = document.createElement('a');
    a.href = dataStr;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    a.remove();

    logEvent('INFO', `Exported dataset (${state.auditLogs.length} records) as ${format.toUpperCase()}`);
  }

  function escapeHtml(str) {
    if (!str) return '';
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#039;');
  }

  // ============================================================================
  // Initialization & Event Bindings
  // ============================================================================
  function init() {
    // Focus button
    el.btnFocusPassword.addEventListener('click', focusForHardware);

    // Keystroke listeners
    el.inputPassword.addEventListener('keydown', handleKeydown);
    el.inputPassword.addEventListener('input', handlePasswordInput);

    // Password reveal toggle
    el.btnTogglePwd.addEventListener('click', () => {
      const isPwd = el.inputPassword.type === 'password';
      el.inputPassword.type = isPwd ? 'text' : 'password';
      el.eyeLabelShow.classList.toggle('hidden', isPwd);
      el.eyeLabelHide.classList.toggle('hidden', !isPwd);
    });

    // Clear input
    el.btnClearInput.addEventListener('click', () => {
      el.inputPassword.value = '';
      resetTelemetry();
      handlePasswordInput();
      el.inputPassword.focus();
      logEvent('INFO', 'Password field cleared.');
    });

    // Reset after success
    el.btnRearmAfterSuccess.addEventListener('click', resetForNextTest);

    // Form submit
    el.loginForm.addEventListener('submit', handleLoginSubmit);

    // Simulator shortcuts in navbar
    document.querySelectorAll('.btn-nav-sim').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const len = parseInt(e.target.getAttribute('data-sim'), 10) || 14;
        simulateKeyPillBurst(len);
      });
    });

    // Log actions
    el.btnExportCsv.addEventListener('click', () => exportAuditLogs('csv'));
    el.btnExportJson.addEventListener('click', () => exportAuditLogs('json'));
    el.btnClearLogs.addEventListener('click', () => {
      state.auditLogs = [];
      el.eventLogTerminal.innerHTML = '<div class="terminal-line"><span class="log-ts">[LOG]</span> <span class="log-info">Event log cleared.</span></div>';
    });

    // Global shortcut: Press ESC to focus password field
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        focusForHardware();
      }
    });

    resetTelemetry();
    setTimeout(focusForHardware, 300);
  }

  document.addEventListener('DOMContentLoaded', init);

})();
