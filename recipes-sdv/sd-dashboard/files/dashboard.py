#!/usr/bin/env python3
"""Predictive-maintenance real-time dashboard — pure Python standard library (no Flask/pip needed).
Reads the RPi3 pm-receiver SQLite, visualizes it in real time with Chart.js, and toasts anomalies.

Usage: dashboard.py [db_path] [port]   (default /var/lib/pm-receiver/sensors.db, 8080)
Open: http://<RPi3>:8080/
"""
import json
import os
import sqlite3
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

DB_PATH = "/var/lib/pm-receiver/sensors.db"
PORT = 8080

# Chart.js 4 (CDN). Fetched by the browser (RPi3 needs no internet).
HTML = r"""<!DOCTYPE html><html lang="ko"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Predictive Maintenance Dashboard</title>
<script src="/chart.js"></script>
<style>
  *{box-sizing:border-box}
  body{font-family:system-ui,Segoe UI,sans-serif;margin:0;padding:14px;background:#0e1117;color:#e6e6e6}
  h1{font-size:17px;margin:0 0 10px}
  .stats{display:flex;gap:10px;flex-wrap:wrap;margin-bottom:10px}
  .stat{flex:1;min-width:110px;background:#161b22;padding:10px 12px;border-radius:8px;border:1px solid #21262d}
  .stat b{display:block;font-size:11px;color:#8b949e;margin-bottom:3px}
  .stat span{font-size:20px;font-weight:600}
  .card{background:#161b22;border:1px solid #21262d;border-radius:8px;padding:10px;margin:10px 0}
  canvas{max-height:200px}
  #toast{position:fixed;top:12px;right:12px;background:#da3633;color:#fff;padding:12px 16px;border-radius:8px;font-weight:600;box-shadow:0 4px 12px rgba(0,0,0,.5);display:none}
  #toast.show{display:block;animation:pulse .4s}
  @keyframes pulse{0%{transform:scale(.9);opacity:0}100%{transform:scale(1);opacity:1}}
  #anoms div{padding:3px 0;border-bottom:1px solid #21262d;font-size:13px;font-family:monospace}
  .ok{color:#3fb950}.bad{color:#f85149}
  #status{font-size:12px;font-weight:400}
</style></head><body>
<h1>🔮 Predictive Maintenance Real-time Vibration Dashboard <span id="status">Connecting…</span></h1>
<div id="toast"></div>
<div class="stats">
  <div class="stat"><b>RMS (mg)</b><span id="s_rms">-</span></div>
  <div class="stat"><b>Kurtosis</b><span id="s_kurt">-</span></div>
  <div class="stat"><b>Crest</b><span id="s_crest">-</span></div>
  <div class="stat"><b>Dominant freq (Hz)</b><span id="s_f0">-</span></div>
  <div class="stat"><b>Status</b><span id="s_state">-</span></div>
  <div class="stat"><b>Temperature (°C)</b><span id="s_temp">-</span></div>
</div>
<div class="card"><canvas id="chart"></canvas></div>
<div class="card"><b>Recent anomaly events</b><div id="anoms" style="margin-top:6px"></div></div>

<script>
const MAX = 60;
const chart = new Chart(document.getElementById('chart').getContext('2d'), {
  type:'line',
  data:{labels:[],datasets:[
    {label:'RMS (mg)', data:[], borderColor:'#58a6ff', backgroundColor:'#58a6ff33', yAxisID:'y', tension:.3, pointRadius:0, borderWidth:2, fill:true},
    {label:'Kurtosis', data:[], borderColor:'#f0883e', yAxisID:'y1', tension:.3, pointRadius:0, borderWidth:2}
  ]},
  options:{animation:false, scales:{
    x:{display:false},
    y:{position:'left', grid:{color:'#21262d'}, ticks:{color:'#8b949e'}},
    y1:{position:'right', grid:{drawOnChartArea:false}, ticks:{color:'#8b949e'}}
  }, plugins:{legend:{labels:{color:'#c9d1d9'}}}}
});
function fmtTime(ts){return new Date(ts*1000).toLocaleTimeString('ko-KR');}
function pushPoint(ts, rms, kurt){
  chart.data.labels.push(fmtTime(ts));
  chart.data.datasets[0].data.push(rms);
  chart.data.datasets[1].data.push(kurt);
  if (chart.data.labels.length > MAX){
    chart.data.labels.shift(); chart.data.datasets[0].data.shift(); chart.data.datasets[1].data.shift();
  }
  chart.update('none');
}
let lastAnomTs = null;
function toast(msg){
  const t = document.getElementById('toast');
  t.textContent = msg; t.classList.add('show');
  clearTimeout(t._tid); t._tid = setTimeout(()=>t.classList.remove('show'), 3500);
}
async function get(path){
  const r = await fetch(path); return r.ok ? r.json() : null;
}
async function init(){
  const hist = await get('/api/history?n=' + MAX) || [];
  hist.forEach(r => pushPoint(r.ts, r.rms, r.kurtosis));
  const anoms = await get('/api/anomalies?n=5') || [];
  if (anoms.length) lastAnomTs = anoms[0].ts;
  renderAnoms(anoms);
}
function renderAnoms(anoms){
  document.getElementById('anoms').innerHTML = anoms.slice(0,5)
    .map(r => `<div><span class="bad">⚠</span> ${fmtTime(r.ts)} rms=${r.rms} kurt=${r.kurt} (thr ${r.thr_kurt}) reason=${r.reason?'kurt':'rms'}</div>`)
    .join('') || '<div style="color:#8b949e">No anomalies</div>';
}
async function poll(){
  try{
    const L = await get('/api/latest');
    if (L){
      document.getElementById('s_rms').textContent  = L.rms;
      document.getElementById('s_kurt').textContent = L.kurtosis;
      document.getElementById('s_crest').textContent= L.crest;
      document.getElementById('s_f0').textContent   = L.f0;
      const st = document.getElementById('s_state');
      st.textContent = L.anomaly ? '⚠ ANOMALY' : '✓ Normal';
      st.className = L.anomaly ? 'bad' : 'ok';
      if (chart.data.labels.length) pushPoint(L.ts, L.rms, L.kurtosis);
    }
    const T = await get('/api/temp');
    if (T) document.getElementById('s_temp').textContent = T.temp;
    const A = await get('/api/anomalies?n=5') || [];
    if (A.length && A[0].ts !== lastAnomTs){
      lastAnomTs = A[0].ts;
      toast('⚠ Anomaly detected: kurt=' + A[0].kurt + ' (threshold ' + A[0].thr_kurt + ')');
    }
    renderAnoms(A);
    document.getElementById('status').textContent = '● live';
  }catch(e){
    document.getElementById('status').textContent = '● Disconnected';
  }
}
init().then(()=>{poll(); setInterval(poll, 1000);});
</script></body></html>"""


def db_query(sql, args=()):
    con = sqlite3.connect(DB_PATH, timeout=2)
    con.row_factory = sqlite3.Row
    try:
        rows = [dict(r) for r in con.execute(sql, args)]
    finally:
        con.close()
    return rows


_cleanup_counter = 0

def db_cleanup():
    """Keep only the last 200 feature_vector rows (prevent disk full). anomaly_event is preserved."""
    con = sqlite3.connect(DB_PATH, timeout=2)
    try:
        con.execute("DELETE FROM feature_vector WHERE rowid NOT IN "
                    "(SELECT rowid FROM feature_vector ORDER BY ts DESC LIMIT 200)")
        con.execute("DELETE FROM temperature WHERE rowid NOT IN "
                    "(SELECT rowid FROM temperature ORDER BY ts DESC LIMIT 200)")
        con.commit()
    except Exception:
        pass
    finally:
        con.close()


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def _send(self, body, ctype="application/json", code=200):
        data = body.encode() if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def _json(self, obj, code=200):
        self._send(json.dumps(obj), "application/json", code)

    def do_GET(self):
        u = urlparse(self.path)
        q = parse_qs(u.query)
        global _cleanup_counter
        _cleanup_counter += 1
        if _cleanup_counter % 60 == 0:
            db_cleanup()
        try:
            if u.path == "/":
                self._send(HTML, "text/html; charset=utf-8")
            elif u.path == "/chart.js":
                p = os.path.join(os.path.dirname(os.path.abspath(__file__)), "chart.js")
                try:
                    with open(p, "rb") as f:
                        self._send(f.read(), "application/javascript")
                except Exception:
                    self._send("/* chart.js not found */", "application/javascript", 404)
            elif u.path == "/api/history":
                n = min(int(q.get("n", ["60"])[0]), 600)
                self._json(db_query(
                    "SELECT ts,ts_src,rms,kurtosis,crest,f0,anomaly "
                    "FROM feature_vector ORDER BY ts DESC LIMIT ?", (n,))[::-1])
            elif u.path == "/api/latest":
                r = db_query("SELECT ts,ts_src,rms,kurtosis,crest,f0,anomaly "
                             "FROM feature_vector ORDER BY ts DESC LIMIT 1")
                self._json(r[0] if r else None)
            elif u.path == "/api/anomalies":
                n = min(int(q.get("n", ["20"])[0]), 200)
                self._json(db_query(
                    "SELECT ts,ts_src,rms,kurt,thr_rms,thr_kurt,reason "
                    "FROM anomaly_event ORDER BY ts DESC LIMIT ?", (n,)))
            elif u.path == "/api/temp":
                r = db_query("SELECT ts, ts_src, temp FROM temperature ORDER BY ts DESC LIMIT 1")
                self._json(r[0] if r else None)
            elif u.path == "/api/stats":
                r = db_query("SELECT COUNT(*) c, MAX(ts) t FROM feature_vector")
                a = db_query("SELECT COUNT(*) c FROM anomaly_event")
                self._json({"features": r[0]["c"] if r else 0,
                            "anomalies": a[0]["c"] if a else 0,
                            "latest_ts": r[0]["t"] if r else None})
            else:
                self._send("404", "text/plain", 404)
        except Exception as e:
            self._json({"error": str(e)}, 500)


def main():
    global DB_PATH, PORT
    if len(sys.argv) > 1:
        DB_PATH = sys.argv[1]
    if len(sys.argv) > 2:
        PORT = int(sys.argv[2])
    print(f"[dashboard] http://0.0.0.0:{PORT}  db={DB_PATH}", flush=True)
    # Initial DB check
    try:
        s = db_query("SELECT COUNT(*) c FROM feature_vector")
        print(f"[dashboard] feature rows: {s[0]['c'] if s else 0}", flush=True)
    except Exception as e:
        print(f"[dashboard] WARN db: {e}", flush=True)
    ThreadingHTTPServer(("0.0.0.0", PORT), Handler).serve_forever()


if __name__ == "__main__":
    main()
