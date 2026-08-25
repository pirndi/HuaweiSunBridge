#pragma once
#include <Arduino.h>

// Einseitige Oberflaeche, komplett offline lauffaehig (keine externen Fonts,
// kein CDN). Aufbau folgt dem Signalweg: Heimnetz -> Bridge -> Wechselrichter.
static const char PAGE_INDEX[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SunBridge</title>
<style>
:root{
  --bg:#12151a; --surface:#1b2027; --line:#2c333d; --line-soft:#232a33;
  --fg:#e4e8ed; --muted:#8a94a2; --accent:#f0a830;
  --ok:#4fd1a5; --warn:#f0a830; --bad:#e5484d;
  --mono:ui-monospace,SFMono-Regular,"SF Mono",Menlo,Consolas,monospace;
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);
  font:15px/1.5 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
.wrap{max-width:720px;margin:0 auto;padding:20px 16px 56px}

header{display:flex;align-items:baseline;gap:10px;
  border-bottom:2px solid var(--accent);padding-bottom:10px;margin-bottom:22px}
header h1{font-size:19px;margin:0;letter-spacing:.02em}
header .ver{font:12px var(--mono);color:var(--muted);margin-left:auto}

/* Signalweg-Leiste: die eine Sache, die man beim Debuggen zuerst sucht */
.path{display:grid;grid-template-columns:1fr auto 1fr auto 1fr;align-items:center;
  gap:8px;background:var(--surface);border:1px solid var(--line);
  border-radius:10px;padding:16px 12px;margin-bottom:20px}
.node{text-align:center;min-width:0}
.node .dot{width:11px;height:11px;border-radius:50%;margin:0 auto 8px;
  background:var(--muted);box-shadow:0 0 0 0 rgba(79,209,165,.5)}
.node.on .dot{background:var(--ok);animation:pulse 2.4s ease-out infinite}
.node.off .dot{background:var(--bad)}
.node .name{font-size:11px;letter-spacing:.09em;text-transform:uppercase;color:var(--muted)}
.node .val{font:13px var(--mono);margin-top:3px;word-break:break-all}
.link{width:100%;height:2px;background:var(--line);position:relative;border-radius:2px}
.link.on{background:linear-gradient(90deg,var(--line),var(--ok),var(--line))}
@keyframes pulse{0%{box-shadow:0 0 0 0 rgba(79,209,165,.45)}
  70%{box-shadow:0 0 0 9px rgba(79,209,165,0)}100%{box-shadow:0 0 0 0 rgba(79,209,165,0)}}
@media (prefers-reduced-motion:reduce){.node.on .dot{animation:none}}

section{background:var(--surface);border:1px solid var(--line);
  border-radius:10px;padding:16px;margin-bottom:16px}
section h2{font-size:11px;letter-spacing:.12em;text-transform:uppercase;
  color:var(--accent);margin:0 0 14px}

.row{display:flex;justify-content:space-between;gap:12px;padding:7px 0;
  border-bottom:1px solid var(--line-soft)}
.row:last-child{border-bottom:0}
.row dt{color:var(--muted);font-size:13px}
.row dd{margin:0;font:13px var(--mono);text-align:right;word-break:break-all}
dl{margin:0}

label{display:block;font-size:12px;letter-spacing:.05em;text-transform:uppercase;
  color:var(--muted);margin:14px 0 5px}
input,select{width:100%;padding:9px 10px;background:#0d1015;color:var(--fg);
  border:1px solid var(--line);border-radius:6px;font:14px var(--mono)}
input:focus,select:focus{outline:2px solid var(--accent);outline-offset:1px;border-color:transparent}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:0 12px}
.check{display:flex;align-items:center;gap:9px;margin-top:16px}
.check input{width:auto}
.check label{margin:0;text-transform:none;letter-spacing:0;font-size:14px;color:var(--fg)}

button{font:600 14px system-ui;padding:10px 16px;border-radius:6px;cursor:pointer;
  border:1px solid var(--line);background:#232a33;color:var(--fg)}
button:hover{border-color:var(--muted)}
button.primary{background:var(--accent);color:#1a1200;border-color:var(--accent)}
button.danger{color:var(--bad)}
button:disabled{opacity:.5;cursor:default}
.actions{display:flex;gap:10px;flex-wrap:wrap;margin-top:18px}

.hint{font-size:12px;color:var(--muted);margin-top:8px}
.msg{margin-top:14px;padding:10px 12px;border-radius:6px;font-size:13px;display:none}
.msg.ok{display:block;background:rgba(79,209,165,.12);border:1px solid var(--ok)}
.msg.err{display:block;background:rgba(229,72,77,.12);border:1px solid var(--bad)}
.hide{display:none}
</style>
</head>
<body>
<div class="wrap">

<header>
  <h1>SunBridge</h1>
  <span class="ver" id="host">-</span>
</header>

<div class="path">
  <div class="node" id="nEth">
    <div class="dot"></div><div class="name">Heimnetz</div><div class="val" id="ethIp">-</div>
  </div>
  <div class="link" id="lEth" style="width:28px"></div>
  <div class="node" id="nBr">
    <div class="dot"></div><div class="name">Bridge</div><div class="val" id="brPort">-</div>
  </div>
  <div class="link" id="lWr" style="width:28px"></div>
  <div class="node" id="nWr">
    <div class="dot"></div><div class="name">Wechselrichter</div><div class="val" id="wrIp">-</div>
  </div>
</div>

<section>
  <h2>Status</h2>
  <dl>
    <div class="row"><dt>WR-AP</dt><dd id="sSsid">-</dd></div>
    <div class="row"><dt>Signal</dt><dd id="sRssi">-</dd></div>
    <div class="row"><dt>Ethernet</dt><dd id="sEth">-</dd></div>
    <div class="row"><dt>Aktive Sitzungen</dt><dd id="sSess">-</dd></div>
    <div class="row"><dt>Uebertragen</dt><dd id="sBytes">-</dd></div>
    <div class="row"><dt>Letzte Daten</dt><dd id="sLast">-</dd></div>
    <div class="row"><dt>Laufzeit</dt><dd id="sUp">-</dd></div>
    <div class="row"><dt>Letzter Fehler</dt><dd id="sErr">-</dd></div>
  </dl>
</section>

<section>
  <h2>Wechselrichter-WLAN</h2>
  <label for="ssid">Netzwerk</label>
  <select id="ssid"><option value="">- Netzwerke suchen -</option></select>
  <div class="actions" style="margin-top:10px">
    <button type="button" id="btnScan">Netzwerke suchen</button>
  </div>
  <p class="hint">Der SUN2000 spannt ein eigenes WLAN auf, meist <code>SUN2000-&lt;Seriennummer&gt;</code>.
     Das Passwort steht im WLAN-Menue des Wechselrichters.</p>

  <label for="pass">Passwort</label>
  <input id="pass" type="password" autocomplete="off" placeholder="unveraendert lassen">

  <div class="check">
    <input type="checkbox" id="tauto" checked>
    <label for="tauto">Ziel automatisch bestimmen (Gateway des WR-WLANs)</label>
  </div>
  <div id="tmanual" class="grid2 hide">
    <div><label for="tip">Ziel-IP</label><input id="tip" placeholder="192.168.200.1"></div>
    <div><label for="tport">Ziel-Port</label><input id="tport" value="6607"></div>
  </div>
</section>

<section>
  <h2>Heimnetz-Seite</h2>
  <div class="check" style="margin-top:0">
    <input type="checkbox" id="dhcp" checked>
    <label for="dhcp">Adresse per DHCP beziehen</label>
  </div>
  <div id="static" class="hide">
    <div class="grid2">
      <div><label for="eip">IP-Adresse</label><input id="eip"></div>
      <div><label for="esn">Subnetzmaske</label><input id="esn" value="255.255.255.0"></div>
      <div><label for="egw">Gateway</label><input id="egw"></div>
      <div><label for="edns">DNS</label><input id="edns"></div>
    </div>
  </div>
  <div class="grid2">
    <div><label for="lport">Server-Port</label><input id="lport" value="6607"></div>
    <div><label for="hname">Geraetename</label><input id="hname" placeholder="automatisch"></div>
  </div>
  <p class="hint">Jede Bridge bekommt eine eigene Adresse im Heimnetz. Fuer mehrere
     Wechselrichter einfach mehrere Bridges betreiben - Home Assistant spricht
     dann jede unter ihrer eigenen IP an.</p>

  <div class="actions">
    <button type="button" class="primary" id="btnSave">Speichern und neu starten</button>
    <button type="button" id="btnReboot">Neu starten</button>
    <button type="button" class="danger" id="btnReset">Zuruecksetzen</button>
  </div>
  <div class="msg" id="msg"></div>
</section>

<section>
  <h2>Firmware</h2>
  <input type="file" id="fw" accept=".bin">
  <div class="actions">
    <button type="button" id="btnFw" disabled>Firmware uebertragen</button>
  </div>
  <div class="msg" id="fwmsg"></div>
</section>

</div>
<script>
const $=id=>document.getElementById(id);
const fmtBytes=n=>{if(n<1024)return n+" B";if(n<1048576)return (n/1024).toFixed(1)+" kB";
  return (n/1048576).toFixed(2)+" MB";};
const fmtSec=s=>{const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),
  m=Math.floor(s%3600/60);return (d?d+"d ":"")+(h?h+"h ":"")+m+"m";};

function node(el,on,off){el.classList.toggle("on",on);el.classList.toggle("off",off);}

async function refresh(){
  try{
    const s=await (await fetch("/api/status")).json();
    $("host").textContent=s.host+"  v"+s.fw;

    node($("nEth"),s.eth.up,!s.eth.up);
    node($("nBr"),s.relay.listening,!s.relay.listening);
    node($("nWr"),s.wifi.up,!s.wifi.up);
    $("lEth").className="link"+(s.eth.up?" on":"");
    $("lWr").className="link"+(s.wifi.up?" on":"");

    $("ethIp").textContent=s.eth.ip||"kein Link";
    $("brPort").textContent=":"+s.relay.port;
    $("wrIp").textContent=s.target||"nicht verbunden";

    $("sSsid").textContent=s.wifi.up?s.wifi.ssid:(s.wifi.ssid||"nicht eingerichtet");
    $("sRssi").textContent=s.wifi.up?(s.wifi.rssi+" dBm"):"-";
    $("sEth").textContent=s.eth.up?(s.eth.ip+"  "+s.eth.speed+" Mbit/s"):"kein Link";
    $("sSess").textContent=s.relay.active+" / "+s.relay.max+
      "  (gesamt "+s.relay.total+", abgelehnt "+s.relay.rejected+")";
    $("sBytes").textContent="\u2192 WR "+fmtBytes(s.relay.toInv)+
      "   \u2190 WR "+fmtBytes(s.relay.toCli);
    $("sLast").textContent=s.relay.lastAgo<0?"noch keine":(s.relay.lastAgo+" s her");
    $("sUp").textContent=fmtSec(s.uptime);
    $("sErr").textContent=s.relay.err||"-";
  }catch(e){}
}

async function loadCfg(){
  const c=await (await fetch("/api/config")).json();
  if(c.ssid){const o=document.createElement("option");o.value=c.ssid;
    o.textContent=c.ssid+"  (gespeichert)";o.selected=true;$("ssid").appendChild(o);}
  $("tauto").checked=c.targetAuto; $("tip").value=c.targetIp||"";
  $("tport").value=c.targetPort; $("dhcp").checked=c.dhcp;
  $("eip").value=c.ip||""; $("esn").value=c.subnet||"255.255.255.0";
  $("egw").value=c.gw||""; $("edns").value=c.dns||"";
  $("lport").value=c.listenPort; $("hname").value=c.hostname||"";
  toggles();
}
function toggles(){
  $("tmanual").classList.toggle("hide",$("tauto").checked);
  $("static").classList.toggle("hide",$("dhcp").checked);
}
$("tauto").onchange=toggles; $("dhcp").onchange=toggles;

$("btnScan").onclick=async()=>{
  const b=$("btnScan");b.disabled=true;b.textContent="Suche laeuft...";
  await fetch("/api/scan",{method:"POST"});
  const poll=setInterval(async()=>{
    const r=await (await fetch("/api/scan")).json();
    if(r.running)return;
    clearInterval(poll);
    const sel=$("ssid"),cur=sel.value;sel.innerHTML="";
    if(!r.nets.length){sel.innerHTML='<option value="">- nichts gefunden -</option>';}
    r.nets.forEach(n=>{const o=document.createElement("option");
      o.value=n.ssid;o.textContent=n.ssid+"   "+n.rssi+" dBm"+(n.open?"  offen":"");
      if(n.ssid===cur)o.selected=true;sel.appendChild(o);});
    b.disabled=false;b.textContent="Netzwerke suchen";
  },900);
};

function show(el,txt,ok){el.textContent=txt;el.className="msg "+(ok?"ok":"err");}

$("btnSave").onclick=async()=>{
  const body={ssid:$("ssid").value,targetAuto:$("tauto").checked,
    targetIp:$("tip").value,targetPort:+$("tport").value,dhcp:$("dhcp").checked,
    ip:$("eip").value,subnet:$("esn").value,gw:$("egw").value,dns:$("edns").value,
    listenPort:+$("lport").value,hostname:$("hname").value};
  if($("pass").value)body.pass=$("pass").value;
  const r=await fetch("/api/config",{method:"POST",
    headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  const j=await r.json();
  show($("msg"),j.ok?"Gespeichert. Geraet startet neu.":("Fehler: "+j.err),j.ok);
};
$("btnReboot").onclick=async()=>{await fetch("/api/reboot",{method:"POST"});
  show($("msg"),"Neustart laeuft.",true);};
$("btnReset").onclick=async()=>{
  if(!confirm("Alle Einstellungen loeschen?"))return;
  await fetch("/api/factory-reset",{method:"POST"});
  show($("msg"),"Zurueckgesetzt. Geraet startet neu.",true);};

$("fw").onchange=()=>{$("btnFw").disabled=!$("fw").files.length;};
$("btnFw").onclick=()=>{
  const f=$("fw").files[0];if(!f)return;
  const fd=new FormData();fd.append("firmware",f,f.name);
  const x=new XMLHttpRequest();x.open("POST","/update");
  x.upload.onprogress=e=>{if(e.lengthComputable)
    show($("fwmsg"),"Uebertragung "+Math.round(e.loaded/e.total*100)+" %",true);};
  x.onload=()=>show($("fwmsg"),x.status===200?
    "Firmware uebernommen. Geraet startet neu.":("Fehlgeschlagen: "+x.responseText),
    x.status===200);
  x.onerror=()=>show($("fwmsg"),"Uebertragung abgebrochen.",false);
  $("btnFw").disabled=true;x.send(fd);
};

loadCfg();refresh();setInterval(refresh,2000);
</script>
</body>
</html>)HTML";
