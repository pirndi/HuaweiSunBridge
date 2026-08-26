#pragma once
#include <Arduino.h>

static const char PAGE_INDEX[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HuaweiSunBridge</title>
<style>
:root{
  --bg:#0f1318;--surface:#181e26;--surface2:#1e2630;--line:#252e3a;
  --fg:#dde3ea;--muted:#6b7a8d;--accent:#e8a020;
  --ok:#3ecf8e;--warn:#e8a020;--bad:#e5484d;
  --mono:ui-monospace,SFMono-Regular,"SF Mono",Menlo,Consolas,monospace;
  --r:10px;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--fg);
  font:14px/1.55 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  min-height:100vh}
.wrap{max-width:740px;margin:0 auto;padding:16px 14px 60px}

/* ── Header ── */
header{display:flex;align-items:center;gap:10px;
  padding:12px 0 14px;border-bottom:1px solid var(--line);margin-bottom:18px}
.logo{font-size:17px;font-weight:700;letter-spacing:.01em;color:var(--fg)}
.logo span{color:var(--accent)}
.spacer{flex:1}
.badge{display:inline-flex;align-items:center;gap:6px;font:12px var(--mono);
  padding:4px 10px;border-radius:20px;cursor:default;text-decoration:none}
.badge.ok{background:rgba(62,207,142,.1);color:var(--ok);border:1px solid rgba(62,207,142,.25)}
.badge.upd{background:rgba(232,160,32,.12);color:var(--warn);border:1px solid rgba(232,160,32,.3);
  cursor:pointer;text-decoration:none}
.badge .dot{width:7px;height:7px;border-radius:50%;background:currentColor}

/* ── Tabs ── */
.tabs{display:flex;gap:2px;margin-bottom:18px;
  border-bottom:2px solid var(--line)}
.tab{padding:8px 16px;font-size:13px;font-weight:500;color:var(--muted);
  cursor:pointer;border-bottom:2px solid transparent;margin-bottom:-2px;
  background:none;border-top:0;border-left:0;border-right:0;transition:color .15s}
.tab:hover{color:var(--fg)}
.tab.active{color:var(--accent);border-bottom-color:var(--accent)}
.tabpane{display:none}
.tabpane.active{display:block}

/* ── Signalweg ── */
.path{display:grid;grid-template-columns:1fr 32px 1fr 32px 1fr;
  align-items:center;gap:6px;background:var(--surface);
  border:1px solid var(--line);border-radius:var(--r);padding:14px 10px;margin-bottom:16px}
.node{text-align:center}
.pulse{width:10px;height:10px;border-radius:50%;margin:0 auto 7px;background:var(--muted)}
.node.on .pulse{background:var(--ok);animation:pulse 2.4s ease-out infinite}
.node.off .pulse{background:var(--bad)}
.nlabel{font-size:10px;letter-spacing:.1em;text-transform:uppercase;color:var(--muted)}
.nval{font:12px var(--mono);margin-top:3px;word-break:break-all;color:var(--fg)}
.arrow{text-align:center;color:var(--line);font-size:18px;line-height:1}
.arrow.on{color:var(--ok)}
@keyframes pulse{0%{box-shadow:0 0 0 0 rgba(62,207,142,.5)}
  70%{box-shadow:0 0 0 8px rgba(62,207,142,0)}100%{box-shadow:0 0 0 0 rgba(62,207,142,0)}}
@media(prefers-reduced-motion:reduce){.node.on .pulse{animation:none}}

/* ── Einklappbare Sektionen ── */
.card{background:var(--surface);border:1px solid var(--line);
  border-radius:var(--r);margin-bottom:12px;overflow:hidden}
.card-head{display:flex;align-items:center;gap:8px;padding:11px 14px;
  cursor:pointer;user-select:none}
.card-head h2{font-size:11px;letter-spacing:.12em;text-transform:uppercase;
  color:var(--accent);flex:1}
.chevron{color:var(--muted);font-size:14px;transition:transform .2s;line-height:1}
.card.open .chevron{transform:rotate(180deg)}
.card-body{padding:0 14px 14px;display:none}
.card.open .card-body{display:block}

/* ── Tabellen ── */
dl.rows{margin:0}
.row{display:flex;justify-content:space-between;gap:10px;
  padding:6px 0;border-bottom:1px solid var(--line)}
.row:last-child{border:0}
.row dt{color:var(--muted);font-size:13px;flex-shrink:0}
.row dd{font:13px var(--mono);text-align:right;word-break:break-all}

/* ── Formular ── */
label.lbl{display:block;font-size:11px;letter-spacing:.07em;text-transform:uppercase;
  color:var(--muted);margin:12px 0 4px}
input,select{width:100%;padding:8px 10px;background:#0b0e12;color:var(--fg);
  border:1px solid var(--line);border-radius:6px;font:13px var(--mono)}
input:focus,select:focus{outline:2px solid var(--accent);outline-offset:1px;border-color:transparent}
.g2{display:grid;grid-template-columns:1fr 1fr;gap:0 12px}
.chk{display:flex;align-items:center;gap:8px;margin-top:12px}
.chk input{width:auto}
.chk label{font-size:13px;color:var(--fg);letter-spacing:0;text-transform:none;margin:0}
.hide{display:none!important}

/* ── Buttons ── */
button{font:600 13px system-ui;padding:8px 14px;border-radius:6px;cursor:pointer;
  border:1px solid var(--line);background:var(--surface2);color:var(--fg);transition:border-color .15s}
button:hover{border-color:var(--muted)}
button.primary{background:var(--accent);color:#160e00;border-color:var(--accent)}
button.danger{color:var(--bad)}
button:disabled{opacity:.45;cursor:default}
.acts{display:flex;gap:8px;flex-wrap:wrap;margin-top:14px}

/* ── Log-Konsole ── */
.log-box{background:#090c10;border:1px solid var(--line);border-radius:6px;
  height:160px;overflow-y:auto;padding:8px 10px;font:12px/1.6 var(--mono);
  color:#8fbe9f;margin-top:4px}
.log-box .err{color:var(--bad)}
.log-box .warn{color:var(--warn)}

/* ── Meldungen ── */
.msg{margin-top:12px;padding:9px 12px;border-radius:6px;font-size:13px;display:none}
.msg.ok{display:block;background:rgba(62,207,142,.1);border:1px solid var(--ok)}
.msg.err{display:block;background:rgba(229,72,77,.1);border:1px solid var(--bad)}

/* ── Fortschrittsbalken OTA ── */
.prog{height:4px;border-radius:2px;background:var(--line);margin-top:10px;overflow:hidden}
.prog-bar{height:100%;width:0;background:var(--accent);transition:width .3s}
</style>
</head>
<body>
<div class="wrap">

<!-- Header -->
<header>
  <div class="logo">Huawei<span>Sun</span>Bridge</div>
  <div class="spacer"></div>
  <a id="fwBadge" class="badge ok" href="#" target="_blank">
    <span class="dot"></span><span id="fwLabel">...</span>
  </a>
</header>

<!-- Tabs -->
<div class="tabs">
  <button class="tab active" onclick="showTab('main',this)">Übersicht</button>
  <button class="tab" onclick="showTab('net',this)">Netzwerk</button>
</div>

<!-- ══════════════ TAB: ÜBERSICHT ══════════════ -->
<div id="tab-main" class="tabpane active">

  <!-- Signalweg -->
  <div class="path">
    <div class="node" id="nEth">
      <div class="pulse"></div>
      <div class="nlabel">Heimnetz</div>
      <div class="nval" id="ethIp">-</div>
    </div>
    <div class="arrow" id="aEth">›</div>
    <div class="node" id="nBr">
      <div class="pulse"></div>
      <div class="nlabel">Bridge</div>
      <div class="nval" id="brPort">-</div>
    </div>
    <div class="arrow" id="aWr">›</div>
    <div class="node" id="nWr">
      <div class="pulse"></div>
      <div class="nlabel">Wechselrichter</div>
      <div class="nval" id="wrIp">-</div>
    </div>
  </div>

  <!-- Wechselrichter-WLAN -->
  <div class="card open" id="cWr">
    <div class="card-head" onclick="toggle('cWr')">
      <h2>Wechselrichter</h2><span class="chevron">▾</span>
    </div>
    <div class="card-body">
      <label class="lbl">WLAN-Netzwerk</label>
      <div style="display:flex;gap:8px;align-items:flex-end">
        <select id="ssid" style="flex:1"></select>
        <button type="button" id="btnScan" style="white-space:nowrap">Suchen</button>
      </div>
      <label class="lbl">Passwort</label>
      <input id="pass" type="password" autocomplete="off" placeholder="unverändert lassen">
      <div class="chk">
        <input type="checkbox" id="tauto" checked>
        <label for="tauto">Ziel automatisch (Gateway des WR-WLANs)</label>
      </div>
      <div id="tmanual" class="g2 hide">
        <div><label class="lbl">Ziel-IP</label><input id="tip" placeholder="192.168.200.1"></div>
        <div><label class="lbl">Port</label><input id="tport" value="6607"></div>
      </div>
      <div class="acts">
        <button class="primary" onclick="saveWr()">Speichern & neu starten</button>
      </div>
      <div class="msg" id="msgWr"></div>
    </div>
  </div>

  <!-- Status -->
  <div class="card open" id="cStat">
    <div class="card-head" onclick="toggle('cStat')">
      <h2>Status</h2><span class="chevron">▾</span>
    </div>
    <div class="card-body">
      <dl class="rows">
        <div class="row"><dt>WR-AP</dt><dd id="sSsid">-</dd></div>
        <div class="row"><dt>Signal</dt><dd id="sRssi">-</dd></div>
        <div class="row"><dt>Sitzungen</dt><dd id="sSess">-</dd></div>
        <div class="row"><dt>→ Wechselrichter</dt><dd id="sTx">-</dd></div>
        <div class="row"><dt>← Wechselrichter</dt><dd id="sRx">-</dd></div>
        <div class="row"><dt>Letzte Aktivität</dt><dd id="sLast">-</dd></div>
        <div class="row"><dt>Laufzeit</dt><dd id="sUp">-</dd></div>
        <div class="row"><dt>Letzter Fehler</dt><dd id="sErr">-</dd></div>
      </dl>
    </div>
  </div>

  <!-- Log -->
  <div class="card" id="cLog">
    <div class="card-head" onclick="toggle('cLog')">
      <h2>Log</h2><span class="chevron">▾</span>
    </div>
    <div class="card-body">
      <div class="log-box" id="logBox"></div>
    </div>
  </div>

</div><!-- /tab-main -->

<!-- ══════════════ TAB: NETZWERK ══════════════ -->
<div id="tab-net" class="tabpane">

  <!-- Ethernet -->
  <div class="card open" id="cEth">
    <div class="card-head" onclick="toggle('cEth')">
      <h2>Ethernet (Heimnetz)</h2><span class="chevron">▾</span>
    </div>
    <div class="card-body">
      <div class="chk" style="margin-top:0">
        <input type="checkbox" id="dhcp" checked>
        <label for="dhcp">Adresse per DHCP</label>
      </div>
      <div id="static" class="g2 hide">
        <div><label class="lbl">IP-Adresse</label><input id="eip"></div>
        <div><label class="lbl">Subnetzmaske</label><input id="esn" value="255.255.255.0"></div>
        <div><label class="lbl">Gateway</label><input id="egw"></div>
        <div><label class="lbl">DNS</label><input id="edns"></div>
      </div>
      <div class="g2">
        <div><label class="lbl">Server-Port</label><input id="lport" value="6607"></div>
        <div><label class="lbl">Gerätename</label><input id="hname" placeholder="automatisch"></div>
      </div>
      <div class="acts">
        <button class="primary" onclick="saveNet()">Speichern & neu starten</button>
        <button onclick="reboot()">Neu starten</button>
        <button class="danger" onclick="factoryReset()">Zurücksetzen</button>
      </div>
      <div class="msg" id="msgNet"></div>
    </div>
  </div>

  <!-- Firmware-Update -->
  <div class="card open" id="cFw">
    <div class="card-head" onclick="toggle('cFw')">
      <h2>Firmware-Update</h2><span class="chevron">▾</span>
    </div>
    <div class="card-body">
      <p style="font-size:13px;color:var(--muted);margin-bottom:10px">
        Aktuelle Firmware: <span id="fwCur" style="font-family:var(--mono);color:var(--fg)">-</span>
        &nbsp;·&nbsp; Neueste: <span id="fwNew" style="font-family:var(--mono);color:var(--fg)">-</span>
      </p>
      <label class="lbl">Firmware-Datei (.bin)</label>
      <input type="file" id="fwFile" accept=".bin">
      <div class="prog"><div class="prog-bar" id="progBar"></div></div>
      <div class="acts">
        <button id="btnFw" disabled onclick="flashFw()">Firmware übertragen</button>
      </div>
      <div class="msg" id="msgFw"></div>
    </div>
  </div>

</div><!-- /tab-net -->

</div><!-- /wrap -->
<script>
// ─── Hilfsfunktionen ────────────────────────────────────────────────────────
const $=id=>document.getElementById(id);
const fmtB=n=>{if(n<1024)return n+' B';if(n<1048576)return(n/1024).toFixed(1)+' kB';return(n/1048576).toFixed(2)+' MB';};
const fmtS=s=>{const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+'d ':'')+( h?h+'h ':'')+m+'m';};
function node(el,on,off){el.classList.toggle('on',on);el.classList.toggle('off',off&&!on);}
function showMsg(el,txt,ok){el.textContent=txt;el.className='msg '+(ok?'ok':'err');}

// ─── Tabs ────────────────────────────────────────────────────────────────────
function showTab(name,btn){
  document.querySelectorAll('.tabpane').forEach(p=>p.classList.remove('active'));
  document.querySelectorAll('.tab').forEach(b=>b.classList.remove('active'));
  $('tab-'+name).classList.add('active');
  btn.classList.add('active');
}

// ─── Einklappen ──────────────────────────────────────────────────────────────
function toggle(id){$( id).classList.toggle('open');}

// ─── Status-Refresh ──────────────────────────────────────────────────────────
async function refresh(){
  let s;
  try{s=await(await fetch('/api/status')).json();}catch(e){return;}

  // Signalweg
  node($('nEth'),s.eth.up,!s.eth.up);
  node($('nBr'),s.relay.listening,!s.relay.listening);
  node($('nWr'),s.wifi.up,!s.wifi.up);
  $('aEth').className='arrow'+(s.eth.up?' on':'');
  $('aWr').className='arrow'+(s.wifi.up?' on':'');
  $('ethIp').textContent=s.eth.ip||'kein Link';
  $('brPort').textContent=':'+s.relay.port;
  $('wrIp').textContent=s.target||'nicht verbunden';

  // Status-Tabelle
  $('sSsid').textContent=s.wifi.ssid||(s.wifi.up?'verbunden':'nicht eingerichtet');
  $('sRssi').textContent=s.wifi.up?(s.wifi.rssi+' dBm'):'-';
  $('sSess').textContent=s.relay.active+' / '+s.relay.max+
    '  (gesamt '+s.relay.total+', abgelehnt '+s.relay.rejected+')';
  $('sTx').textContent=fmtB(s.relay.toInv);
  $('sRx').textContent=fmtB(s.relay.toCli);
  $('sLast').textContent=s.relay.lastAgo<0?'noch keine':(s.relay.lastAgo+'s her');
  $('sUp').textContent=fmtS(s.uptime);
  $('sErr').textContent=s.relay.err||'—';

  // FW-Badge
  $('fwCur').textContent=s.fw;
  updateFwBadge(s.fw,s.fwLatest);
}

// ─── Firmware-Badge ──────────────────────────────────────────────────────────
function updateFwBadge(cur,latest){
  const badge=$('fwBadge'),lbl=$('fwLabel');
  if(!latest||latest==='?'||latest===cur){
    badge.className='badge ok';badge.removeAttribute('href');
    lbl.textContent='v'+cur+' · aktuell';
  } else {
    badge.className='badge upd';
    badge.href='https://github.com/pirndi/HuaweiSunBridge/releases/latest';
    lbl.textContent='Update: '+latest;
  }
}

// ─── Konfiguration laden ──────────────────────────────────────────────────────
async function loadCfg(){
  let c;try{c=await(await fetch('/api/config')).json();}catch(e){return;}
  if(c.ssid){const o=document.createElement('option');o.value=c.ssid;
    o.textContent=c.ssid+'  (gespeichert)';o.selected=true;$('ssid').append(o);}
  $('tauto').checked=c.targetAuto;$('tip').value=c.targetIp||'';$('tport').value=c.targetPort;
  $('dhcp').checked=c.dhcp;$('eip').value=c.ip||'';$('esn').value=c.subnet||'255.255.255.0';
  $('egw').value=c.gw||'';$('edns').value=c.dns||'';
  $('lport').value=c.listenPort;$('hname').value=c.hostname||'';
  toggleStatic();
}
function toggleStatic(){
  $('tmanual').classList.toggle('hide',$('tauto').checked);
  $('static').classList.toggle('hide',$('dhcp').checked);
}
$('tauto').onchange=toggleStatic;$('dhcp').onchange=toggleStatic;

// ─── WLAN-Scan ───────────────────────────────────────────────────────────────
$('btnScan').onclick=async()=>{
  const b=$('btnScan');b.disabled=true;b.textContent='Suche...';
  await fetch('/api/scan',{method:'POST'});
  const poll=setInterval(async()=>{
    let r;try{r=await(await fetch('/api/scan')).json();}catch{return;}
    if(r.running)return;
    clearInterval(poll);
    const sel=$('ssid'),cur=sel.value;sel.innerHTML='';
    if(!r.nets.length){sel.innerHTML='<option value="">— nichts gefunden —</option>';}
    r.nets.forEach(n=>{const o=document.createElement('option');
      o.value=n.ssid;o.textContent=n.ssid+'  '+n.rssi+' dBm'+(n.open?' 🔓':'');
      if(n.ssid===cur)o.selected=true;sel.append(o);});
    b.disabled=false;b.textContent='Suchen';
  },900);
};

// ─── Speichern WR ────────────────────────────────────────────────────────────
async function saveWr(){
  const body={ssid:$('ssid').value,targetAuto:$('tauto').checked,
    targetIp:$('tip').value,targetPort:+$('tport').value};
  if($('pass').value)body.pass=$('pass').value;
  const r=await fetch('/api/config',{method:'POST',
    headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
  const j=await r.json();
  showMsg($('msgWr'),j.ok?'Gespeichert. Gerät startet neu.':('Fehler: '+j.err),j.ok);
}

// ─── Speichern Netzwerk ───────────────────────────────────────────────────────
async function saveNet(){
  const body={dhcp:$('dhcp').checked,ip:$('eip').value,subnet:$('esn').value,
    gw:$('egw').value,dns:$('edns').value,
    listenPort:+$('lport').value,hostname:$('hname').value};
  const r=await fetch('/api/config',{method:'POST',
    headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
  const j=await r.json();
  showMsg($('msgNet'),j.ok?'Gespeichert. Gerät startet neu.':('Fehler: '+j.err),j.ok);
}

async function reboot(){
  if(!confirm('Gerät neu starten?'))return;
  await fetch('/api/reboot',{method:'POST'});
  showMsg($('msgNet'),'Neustart läuft…',true);
}
async function factoryReset(){
  if(!confirm('Alle Einstellungen löschen?'))return;
  await fetch('/api/factory-reset',{method:'POST'});
  showMsg($('msgNet'),'Zurückgesetzt. Gerät startet neu.',true);
}

// ─── OTA ─────────────────────────────────────────────────────────────────────
$('fwFile').onchange=()=>{$('btnFw').disabled=!$('fwFile').files.length;};
function flashFw(){
  const f=$('fwFile').files[0];if(!f)return;
  const fd=new FormData();fd.append('firmware',f,f.name);
  const x=new XMLHttpRequest();x.open('POST','/update');
  x.upload.onprogress=e=>{if(e.lengthComputable){
    const p=Math.round(e.loaded/e.total*100);
    $('progBar').style.width=p+'%';
    showMsg($('msgFw'),'Übertragung '+p+' %',true);}};
  x.onload=()=>{const ok=x.status===200;
    showMsg($('msgFw'),ok?'Firmware übernommen. Gerät startet neu.':('Fehler: '+x.responseText),ok);
    $('progBar').style.width='0';};
  x.onerror=()=>{showMsg($('msgFw'),'Übertragung unterbrochen.',false);$('progBar').style.width='0';};
  $('btnFw').disabled=true;x.send(fd);
}

// ─── WebSocket Log ────────────────────────────────────────────────────────────
function appendLog(msg){
  const box=$('logBox');
  const line=document.createElement('div');
  const low=msg.toLowerCase();
  if(low.includes('fehler')||low.includes('error'))line.className='err';
  else if(low.includes('warn'))line.className='warn';
  line.textContent=msg;
  box.append(line);
  // max 60 Zeilen im DOM behalten
  while(box.children.length>60)box.removeChild(box.firstChild);
  box.scrollTop=box.scrollHeight;
}
function initWs(){
  const ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen=()=>appendLog('— WebSocket verbunden —');
  ws.onmessage=e=>{
    try{
      const d=JSON.parse(e.data);
      if(d.t==='log')appendLog(d.m);
      else if(d.t==='logbuf'&&Array.isArray(d.buf))d.buf.forEach(appendLog);
    }catch{appendLog(e.data);}
  };
  ws.onclose=()=>{appendLog('— WebSocket getrennt, Wiederverbindung in 5s —');
    setTimeout(initWs,5000);};
  ws.onerror=()=>ws.close();
}

// ─── Start ────────────────────────────────────────────────────────────────────
loadCfg();
refresh();
setInterval(refresh,2500);
initWs();
</script>
</body>
</html>)HTML";