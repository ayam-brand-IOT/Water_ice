const char* INDEX_HTML = R"rawliteral(
    <!DOCTYPE html>
    <html lang="es">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width,initial-scale=1.0"/>
      <title>Tote Scanner - Loadcell</title>
      <style>
        body {
          margin: 0;
          padding: 0;
          font-family: 'Segoe UI', 'Arial', sans-serif;
          background: #f4f6fa;
          min-height: 100vh;
          display: flex;
        }
        .sidebar {
          width: 56px;
          background: #fff;
          box-shadow: 2px 0 6px #0001;
          display: flex;
          flex-direction: column;
          align-items: center;
          padding: 30px 0;
        }
        .sidebar .icon {
          font-size: 2rem;
          color: #2563eb;
          margin-bottom: 1rem;
        }
        .container {
          flex: 1;
          display: flex;
          align-items: center;
          justify-content: center;
          padding: 1rem;
        }
        .card {
          background: #fff;
          border-radius: 20px;
          box-shadow: 0 4px 24px #0001;
          padding: 2rem 2.5rem 2.5rem 2.5rem;
          min-width: 320px;
          max-width: 380px;
          width: 100%;
        }
        .title {
          font-size: 2rem;
          color: #1a202c;
          text-align: center;
          margin-bottom: 1.5rem;
          font-weight: 600;
        }
        .weight-label {
          color: #64748b;
          text-align: center;
          margin-bottom: 0.5rem;
          font-size: 1rem;
        }
        .weight-value {
          text-align: center;
          font-size: 3.5rem;
          font-weight: bold;
          color: #2563eb;
          margin-bottom: 1.5rem;
          letter-spacing: -2px;
        }
        .button {
          width: 100%;
          background: #2563eb;
          color: #fff;
          border: none;
          border-radius: 12px;
          font-size: 1.3rem;
          font-weight: 600;
          padding: 1.2rem 0;
          cursor: pointer;
          transition: background 0.18s, transform 0.1s;
          box-shadow: 0 4px 12px #2563eb44;
          margin-bottom: 1rem;
        }
        .button:active {
          background: #1740b6;
          transform: scale(0.98);
        }
        .button-icon {
          font-size: 1.5rem;
          margin-right: 0.5rem;
        }
        .status {
          margin-top: 1rem;
          padding: 0.7rem 0.5rem;
          text-align: center;
          border-radius: 8px;
          font-weight: 500;
          font-size: 1.05rem;
        }
        .status-success {
          background: #bbf7d0;
          color: #166534;
          border: 1.2px solid #16653444;
        }
        .status-error {
          background: #fecaca;
          color: #991b1b;
          border: 1.2px solid #991b1b44;
        }
        .socket-bar {
          position: fixed;
          bottom: 0;
          left: 0;
          width: 100%;
          text-align: center;
          padding: 0.5rem 0;
          font-weight: 500;
        }
        .socket-bar.connected {
          background: #bbf7d0;
          color: #166534;
        }
        .socket-bar.disconnected {
          background: #fecaca;
          color: #991b1b;
        }
        /* QR Scanner Modal */
        .modal {
          display: none;
          position: fixed;
          z-index: 1000;
          left: 0;
          top: 0;
          width: 100%;
          height: 100%;
          background-color: rgba(0,0,0,0.95);
        }
        .modal.active {
          display: flex;
          align-items: center;
          justify-content: center;
          flex-direction: column;
        }
        .modal-content {
          position: relative;
          max-width: 500px;
          width: 90%;
        }
        #qr-video {
          width: 100%;
          border-radius: 12px;
          box-shadow: 0 0 20px rgba(37, 99, 235, 0.5);
        }
        .close-btn {
          position: absolute;
          top: -50px;
          right: 0;
          color: #fff;
          font-size: 2.5rem;
          cursor: pointer;
          background: none;
          border: none;
          z-index: 10;
        }
        .qr-status {
          color: #fff;
          text-align: center;
          margin-top: 1.5rem;
          font-size: 1.1rem;
          background: rgba(0,0,0,0.7);
          padding: 1rem;
          border-radius: 8px;
        }
        .recent-item {
          padding: 0.5rem;
          margin: 0.3rem 0;
          background: #f1f5f9;
          border-radius: 6px;
          font-size: 0.95rem;
          color: #334155;
        }
      </style>
    </head>
    <body>
      <div class="sidebar">
        <div class="icon">⚖️</div>
        <a href="/settings" style="color:#2563eb;font-size:1.5rem;text-decoration:none;margin-top:0.5rem;" title="Settings">⚙️</a>
      </div>
      <div class="container">
        <div class="card">
          <div class="title">Ice Tote 🖕🖕</div>
          <div class="weight-label">Actual weight (kg)</div>
          <div id="weight" class="weight-value">{{WEIGHT}}</div>
          
          <input type="file" id="qrInput" accept="image/*" capture="environment" style="display:none;">
          <button type="button" id="qrBtn" class="button">
            <span class="button-icon">📷</span>
            Scan Tote QR Code
          </button>
          
          <div id="statusMsg"></div>
          <canvas id="qrCanvas" style="display:none;"></canvas>
          
          <div id="recentContainer">
            <div class="weight-label" style="margin-top:1.5rem;">Recent Scans</div>
            <div id="recentIds"></div>
          </div>
        </div>
      </div>
      
      <div id="socketStatus" class="socket-bar disconnected">Socket: disconnected</div>
      
      <!-- jsQR Library -->
      <script src="https://cdn.jsdelivr.net/npm/jsqr@1.4.0/dist/jsQR.min.js"></script>
      
      <script>
        // === WebSocket para peso en tiempo real ===
        let ws;
        let reconnectDelay = 1000;
        function updateSocketStatus(isConnected) {
          const el = document.getElementById('socketStatus');
          if (isConnected) {
            el.textContent = 'Socket: connected';
            el.className = 'socket-bar connected';
          } else {
            el.textContent = 'Socket: disconnected';
            el.className = 'socket-bar disconnected';
          }
        }
        function connectWS() {
          ws = new WebSocket(`ws://${location.host}/ws`);
          ws.onopen = () => {
            reconnectDelay = 1000;
            updateSocketStatus(true);
          };
          ws.onmessage = (event) => {
            document.getElementById('weight').textContent = event.data || '-';
          };
          ws.onclose = () => {
            document.getElementById('weight').textContent = '-';
            updateSocketStatus(false);
            setTimeout(connectWS, reconnectDelay);
            reconnectDelay = Math.min(reconnectDelay * 2, 10000);
          };
          ws.onerror = () => ws.close();
        }
        connectWS();
    
        // === Enviar Tote ID ===
        function submitToteId(palletId) {
          const statusMsg = document.getElementById('statusMsg');
          statusMsg.textContent = 'Registering...';
          statusMsg.className = 'status';
          
          fetch('/register_pallet', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: new URLSearchParams({ id: palletId })
          })
          .then(async res => {
            const text = await res.text();
            if (!res.ok) throw new Error(text || 'Error registering Tote.');
            return text || 'Tote registered successfully.';
          })
          .then(msg => {
            statusMsg.textContent = '✓ ' + msg;
            statusMsg.className = 'status status-success';
            addRecentId(palletId);
          })
          .catch(err => {
            statusMsg.textContent = '✗ ' + (err.message || 'Error registering Tote.');
            statusMsg.className = 'status status-error';
          });
        }

        // === Recent IDs ===
        const recentIds = [];
        const maxIds = 5;
        function addRecentId(id) {
          recentIds.unshift(id);
          if (recentIds.length > maxIds) recentIds.pop();
          const container = document.getElementById('recentIds');
          container.innerHTML = '';
          recentIds.forEach(i => {
            const div = document.createElement('div');
            div.className = 'recent-item';
            div.textContent = i;
            container.appendChild(div);
          });
        }

        // === QR Scanner (usando File Input) ===
        const qrBtn = document.getElementById('qrBtn');
        const qrInput = document.getElementById('qrInput');
        const qrCanvas = document.getElementById('qrCanvas');
        const statusMsg = document.getElementById('statusMsg');

        qrBtn.addEventListener('click', () => {
          qrInput.click();
        });

        qrInput.addEventListener('change', (e) => {
          const file = e.target.files[0];
          if (!file) return;

          statusMsg.textContent = 'Scanning QR code...';
          statusMsg.className = 'status';

          const reader = new FileReader();
          reader.onload = (event) => {
            const img = new Image();
            img.onload = () => {
              const canvas = qrCanvas;
              const context = canvas.getContext('2d');
              
              canvas.width = img.width;
              canvas.height = img.height;
              context.drawImage(img, 0, 0);
              
              const imageData = context.getImageData(0, 0, canvas.width, canvas.height);
              const code = jsQR(imageData.data, imageData.width, imageData.height);

              if (code) {
                statusMsg.textContent = '✓ QR Detected: ' + code.data;
                statusMsg.className = 'status status-success';
                submitToteId(code.data);
              } else {
                statusMsg.textContent = '✗ No QR code found. Try again with better lighting.';
                statusMsg.className = 'status status-error';
              }
              
              qrInput.value = '';
            };
            img.src = event.target.result;
          };
          reader.readAsDataURL(file);
        });
      </script>
    </body>
    </html>
    )rawliteral";

// ============================================================
// Settings page — served at GET /settings
// Placeholders: {{LOCATION}} {{VERSION}} {{ICE_KG}} {{WATER_KG}} {{MIN_WEIGHT}}
// ============================================================
const char* SETTINGS_HTML = R"rawliteral(
    <!DOCTYPE html>
    <html lang="es">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width,initial-scale=1.0"/>
      <title>Settings — {{LOCATION}}</title>
      <style>
        body {
          margin: 0; padding: 0;
          font-family: 'Segoe UI', 'Arial', sans-serif;
          background: #f4f6fa;
          min-height: 100vh;
          display: flex;
          align-items: center;
          justify-content: center;
        }
        .card {
          background: #fff;
          border-radius: 20px;
          box-shadow: 0 4px 24px #0001;
          padding: 2rem 2.5rem 2.5rem 2.5rem;
          min-width: 320px;
          max-width: 400px;
          width: 100%;
        }
        .title  { font-size: 1.6rem; color: #1a202c; text-align: center; margin-bottom: 0.3rem; font-weight: 600; }
        .subtitle { color: #64748b; text-align: center; font-size: 0.9rem; margin-bottom: 1.5rem; }
        .form-label { color: #1e293b; font-weight: 500; margin-bottom: 0.4rem; display: block; }
        .input {
          width: 100%; font-size: 1.1rem; border-radius: 8px;
          border: 1px solid #d1d5db; padding: 0.6rem 1rem;
          margin-bottom: 1.2rem; outline: none;
          box-sizing: border-box; transition: border 0.2s;
        }
        .input:focus { border: 1.7px solid #2563eb; }
        .button {
          width: 100%; background: #2563eb; color: #fff;
          border: none; border-radius: 8px; font-size: 1.1rem;
          font-weight: 600; padding: 0.75rem 0; cursor: pointer;
          transition: background 0.18s; box-shadow: 0 2px 8px #2563eb22;
        }
        .button:active { background: #1740b6; }
        .back-link {
          display: block; text-align: center; color: #2563eb;
          text-decoration: none; font-size: 0.95rem; margin-top: 1rem;
        }
        .status { margin-top: 1rem; padding: 0.7rem 0.5rem; text-align: center; border-radius: 8px; font-weight: 500; }
        .status-success { background: #bbf7d0; color: #166534; border: 1.2px solid #16653444; }
        .status-error   { background: #fecaca; color: #991b1b; border: 1.2px solid #991b1b44; }
        .hint { color: #94a3b8; font-size: 0.82rem; margin-top: -0.8rem; margin-bottom: 1.2rem; }
      </style>
    </head>
    <body>
      <div class="card">
        <div class="title">⚙️ Settings</div>
        <div class="subtitle">{{LOCATION}} &mdash; v{{VERSION}}</div>
        <form id="settingsForm" autocomplete="off">
          <label class="form-label" for="ice_kg">Target Ice (kg)</label>
          <input id="ice_kg" name="ice_kg" class="input" type="number" step="0.1" min="0" required value="{{ICE_KG}}" />
          <label class="form-label" for="water_kg">Target Water (kg)</label>
          <input id="water_kg" name="water_kg" class="input" type="number" step="0.1" min="0" required value="{{WATER_KG}}" />
          <label class="form-label" for="min_w">Minimum start weight (kg)</label>
          <input id="min_w" name="min_w" class="input" type="number" step="0.1" min="0" required value="{{MIN_WEIGHT}}" />
          <p class="hint">Changes take effect immediately and persist after reboot.</p>
          <button type="submit" class="button">Save Settings</button>
        </form>
        <div id="statusMsg"></div>
        <a class="back-link" href="/">&larr; Back to main page</a>
      </div>
      <script>
        document.getElementById('settingsForm').addEventListener('submit', function(e) {
          e.preventDefault();
          const statusMsg = document.getElementById('statusMsg');
          statusMsg.textContent = '';
          statusMsg.className = '';
          const data = new URLSearchParams({
            ice_kg:   document.getElementById('ice_kg').value,
            water_kg: document.getElementById('water_kg').value,
            min_w:    document.getElementById('min_w').value
          });
          fetch('/update_settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: data
          })
          .then(async res => {
            const text = await res.text();
            if (!res.ok) throw new Error(text || 'Error saving settings.');
            return text;
          })
          .then(msg => {
            statusMsg.textContent = msg;
            statusMsg.className = 'status status-success';
          })
          .catch(err => {
            statusMsg.textContent = err.message || 'Error saving settings.';
            statusMsg.className = 'status status-error';
          });
        });
      </script>
    </body>
    </html>
    )rawliteral";
    