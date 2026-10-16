/* global React */
const { useState: useState4, useRef: useRef4 } = React;

/* ------------------------------------------------------------------ */
/*  FINAL TOUCH-UP HEAD — fully refined goblin                        */
/* ------------------------------------------------------------------ */
function FinalGoblin({ tint = "blue", showOverlay = false }) {
  const colors = {
    blue:    ["#7BC2FA", "#3DA5F5", "#1572B8"],
    cyan:    ["#A8E5FA", "#5DCAE0", "#1A7A95"],
    neutral: ["#A8BBD0", "#6F839A", "#2F4358"],
  }[tint] || ["#7BC2FA", "#3DA5F5", "#1572B8"];

  return (
    <g>
      <defs>
        <linearGradient id="finalfill" x1="0.3" y1="0" x2="0.7" y2="1">
          <stop offset="0%" stopColor={colors[0]}/>
          <stop offset="55%" stopColor={colors[1]}/>
          <stop offset="100%" stopColor={colors[2]}/>
        </linearGradient>
        <radialGradient id="finalrim" cx="0.3" cy="0.25" r="0.7">
          <stop offset="0%" stopColor="#E8F1FB" stopOpacity="0.6"/>
          <stop offset="55%" stopColor={colors[1]} stopOpacity="0"/>
        </radialGradient>
        <linearGradient id="finalshade" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor="#08151F" stopOpacity="0"/>
          <stop offset="100%" stopColor="#08151F" stopOpacity="0.55"/>
        </linearGradient>
      </defs>

      {/* Floor shadow */}
      <ellipse cx="0" cy="240" rx="140" ry="14" fill="#000" opacity="0.45"/>

      {/* Bust shoulder/chest */}
      <path d="M -160,250 C -150,180 -100,140 -65,135 C -20,180 20,180 65,135 C 100,140 150,180 160,250 Z"
            fill="url(#finalfill)" opacity="0.92"/>
      <path d="M -160,250 C -150,180 -100,140 -65,135 C -20,180 20,180 65,135 C 100,140 150,180 160,250 Z"
            fill="url(#finalshade)" opacity="0.6"/>

      {/* Neck */}
      <path d="M -38,90 Q -45,120 -65,135 L 65,135 Q 45,120 38,90 Z"
            fill={colors[2]} opacity="0.85"/>

      {/* HEAD — fully sculpted goblin */}
      <path d="M -98,-60 
               C -118,-145 -68,-195 -8,-198 
               C 58,-200 110,-145 100,-50 
               C 95,10 80,55 60,90 
               C 45,115 20,130 -8,130 
               C -38,130 -55,115 -68,90 
               C -88,55 -98,10 -98,-60 Z"
            fill="url(#finalfill)"/>
      <path d="M -98,-60 
               C -118,-145 -68,-195 -8,-198 
               C 58,-200 110,-145 100,-50 
               C 95,10 80,55 60,90 
               C 45,115 20,130 -8,130 
               C -38,130 -55,115 -68,90 
               C -88,55 -98,10 -98,-60 Z"
            fill="url(#finalrim)"/>

      {/* Pointed ears - swept back */}
      <path d="M -98,-90 Q -130,-130 -158,-95 Q -150,-50 -120,-30 Q -100,-40 -95,-55 Z"
            fill={colors[1]}/>
      <path d="M -98,-90 Q -130,-130 -158,-95 Q -150,-50 -120,-30 Q -100,-40 -95,-55 Z"
            fill="url(#finalrim)" opacity="0.6"/>
      <path d="M -120,-90 Q -135,-95 -140,-75" stroke={colors[2]} strokeWidth="1.5" fill="none" opacity="0.6"/>

      <path d="M 100,-90 Q 132,-130 160,-95 Q 152,-50 122,-30 Q 102,-40 97,-55 Z"
            fill={colors[1]}/>
      <path d="M 100,-90 Q 132,-130 160,-95 Q 152,-50 122,-30 Q 102,-40 97,-55 Z"
            fill="url(#finalrim)" opacity="0.6"/>
      <path d="M 122,-90 Q 137,-95 142,-75" stroke={colors[2]} strokeWidth="1.5" fill="none" opacity="0.6"/>

      {/* Heavy brow ridge */}
      <path d="M -82,-105 Q -45,-128 -8,-118 Q 30,-128 80,-110 L 78,-92 Q 30,-108 -8,-100 Q -45,-108 -82,-92 Z"
            fill="#08151F" opacity="0.32"/>

      {/* Brow hair tufts */}
      <path d="M -68,-118 Q -54,-130 -38,-122 M -65,-115 Q -50,-122 -36,-118" stroke="#08151F" strokeWidth="2" fill="none" opacity="0.6" strokeLinecap="round"/>
      <path d="M 28,-122 Q 44,-130 60,-118 M 30,-118 Q 46,-122 60,-115" stroke="#08151F" strokeWidth="2" fill="none" opacity="0.6" strokeLinecap="round"/>

      {/* Sunken eye sockets */}
      <ellipse cx="-46" cy="-72" rx="20" ry="12" fill="#08151F" opacity="0.45"/>
      <ellipse cx="40" cy="-72" rx="20" ry="12" fill="#08151F" opacity="0.45"/>

      {/* Eyes */}
      <ellipse cx="-46" cy="-72" rx="11" ry="6" fill="#1A2330"/>
      <circle cx="-44" cy="-72" r="3.5" fill="#F2B547"/>
      <circle cx="-43" cy="-72" r="1.5" fill="#08151F"/>
      <ellipse cx="40" cy="-72" rx="11" ry="6" fill="#1A2330"/>
      <circle cx="42" cy="-72" r="3.5" fill="#F2B547"/>
      <circle cx="43" cy="-72" r="1.5" fill="#08151F"/>

      {/* Long crooked nose */}
      <path d="M -2,-65 Q -14,-30 -16,5 Q -18,30 -8,42 Q 8,46 18,30 Q 18,0 14,-30 Q 8,-58 -2,-65 Z"
            fill={colors[2]} opacity="0.75"/>
      <path d="M -2,-65 Q -14,-30 -16,5 Q -18,30 -8,42 Q 8,46 18,30 Q 18,0 14,-30 Q 8,-58 -2,-65 Z"
            fill="url(#finalrim)" opacity="0.5"/>
      {/* Nostrils */}
      <ellipse cx="-7" cy="32" rx="3" ry="2" fill="#08151F" opacity="0.7"/>
      <ellipse cx="9" cy="32" rx="3" ry="2" fill="#08151F" opacity="0.7"/>

      {/* Cheekbones */}
      <ellipse cx="-65" cy="-10" rx="14" ry="22" fill="#08151F" opacity="0.18" transform="rotate(-15 -65 -10)"/>
      <ellipse cx="65" cy="-10" rx="14" ry="22" fill="#08151F" opacity="0.18" transform="rotate(15 65 -10)"/>

      {/* Mouth — downturned with snaggle tusks */}
      <path d="M -38,58 Q 0,52 38,58 Q 32,72 0,74 Q -32,72 -38,58 Z" fill="#08151F" opacity="0.7"/>
      <path d="M -38,58 Q 0,52 38,58" stroke="#08151F" strokeWidth="1.5" fill="none" opacity="0.85"/>
      {/* Tusks */}
      <path d="M -22,62 L -19,76 L -16,62 Z" fill="#E8F1FB" opacity="0.85"/>
      <path d="M 16,62 L 19,76 L 22,62 Z" fill="#E8F1FB" opacity="0.85"/>

      {/* Chin shading */}
      <path d="M -28,95 Q 0,108 28,95 Q 20,112 0,115 Q -20,112 -28,95 Z" fill="#08151F" opacity="0.22"/>

      {/* Forehead wrinkles */}
      <path d="M -50,-138 Q -8,-148 50,-138" stroke="#08151F" strokeWidth="1" fill="none" opacity="0.3"/>
      <path d="M -45,-128 Q -8,-138 45,-128" stroke="#08151F" strokeWidth="1" fill="none" opacity="0.25"/>

      {/* Show target overlay (translucent metahuman ghost) */}
      {showOverlay && (
        <g opacity="0.18" pointerEvents="none">
          <ellipse cx="0" cy="-30" rx="80" ry="115" fill="none" stroke="#A8BBD0" strokeWidth="1.5" strokeDasharray="3 3"/>
          <ellipse cx="-32" cy="-72" rx="10" ry="5" fill="none" stroke="#A8BBD0" strokeWidth="1"/>
          <ellipse cx="32" cy="-72" rx="10" ry="5" fill="none" stroke="#A8BBD0" strokeWidth="1"/>
        </g>
      )}
    </g>
  );
}

/* ------------------------------------------------------------------ */
/*  STAGE 4 VIEW                                                      */
/* ------------------------------------------------------------------ */
function Stage4View({ tweaks }) {
  const [tool, setTool] = useState4("smooth");
  const [radius, setRadius] = useState4(18);
  const [strength, setStrength] = useState4(0.5);
  const [falloff, setFalloff] = useState4("smooth");
  const [symmetry, setSymmetry] = useState4("x");
  const [showOverlay, setShowOverlay] = useState4(false);
  const [showHud, setShowHud] = useState4(false);
  const [exportOpen, setExportOpen] = useState4(false);
  const [cursorPos, setCursorPos] = useState4(null);

  const bgStyle = tweaks.viewportTexture === "blueprint" ? {
    backgroundColor: "var(--mv-bg-0)",
    backgroundImage: `
      linear-gradient(var(--mv-line-1) 1px, transparent 1px),
      linear-gradient(90deg, var(--mv-line-1) 1px, transparent 1px),
      linear-gradient(rgba(125,175,220,0.04) 1px, transparent 1px),
      linear-gradient(90deg, rgba(125,175,220,0.04) 1px, transparent 1px)`,
    backgroundSize: "96px 96px, 96px 96px, 16px 16px, 16px 16px",
  } : tweaks.viewportTexture === "fine" ? {
    backgroundColor: "var(--mv-bg-0)",
    backgroundImage: `
      linear-gradient(rgba(125,175,220,0.05) 1px, transparent 1px),
      linear-gradient(90deg, rgba(125,175,220,0.05) 1px, transparent 1px)`,
    backgroundSize: "16px 16px, 16px 16px",
  } : { backgroundColor: "var(--mv-bg-0)" };

  return (
    <div style={{ display: "grid", gridTemplateColumns: "1fr 380px", height: "100%", overflow: "hidden", position: "relative" }}>
      {/* Viewport */}
      <div
        style={{ position: "relative", overflow: "hidden", cursor: "none", ...bgStyle }}
        onPointerMove={(e) => {
          const r = e.currentTarget.getBoundingClientRect();
          setCursorPos({ x: e.clientX - r.left, y: e.clientY - r.top });
        }}
        onPointerLeave={() => setCursorPos(null)}
      >
        {/* Viewport top bar */}
        <div style={{
          position: "absolute", top: 14, left: 14,
          display: "flex", gap: 4, alignItems: "center",
          background: "rgba(11,22,34,0.85)", backdropFilter: "blur(12px)",
          border: "1px solid var(--mv-line-2)", borderRadius: 6, padding: 5,
          zIndex: 5,
        }}>
          <span style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 11,
            color: "var(--mv-blue-400)", padding: "0 10px",
            letterSpacing: "0.08em",
          }}>SOLID</span>
          <div style={{ width: 1, height: 18, background: "var(--mv-line-2)", margin: "0 3px" }}/>
          <button onClick={() => setShowHud((v) => !v)} style={{
            padding: "5px 10px", border: 0, borderRadius: 4,
            background: showHud ? "rgba(61,165,245,0.15)" : "transparent",
            color: showHud ? "var(--mv-blue-400)" : "var(--mv-fg-2)",
            fontFamily: "var(--mv-font-body)", fontSize: 12,
            cursor: "pointer", display: "flex", alignItems: "center", gap: 6,
          }}>
            <window.IconChevD size={11} style={{ transform: showHud ? "" : "rotate(-90deg)", transition: "transform .15s" }}/>
            Debug HUD
          </button>
        </div>

        {/* Top-right pills */}
        <div style={{ position: "absolute", top: 14, right: 14, display: "flex", gap: 6, zIndex: 5 }}>
          <Pill4><span style={{ color: "var(--mv-success)" }}>●</span> COMPLETE</Pill4>
          <Pill4>{Math.round(strength * 100)}% strength</Pill4>
        </div>

        {/* Debug HUD */}
        {showHud && (
          <div style={{
            position: "absolute", top: 64, left: 14, zIndex: 5,
            background: "rgba(11,22,34,0.92)", backdropFilter: "blur(8px)",
            border: "1px solid var(--mv-line-2)", borderRadius: 4,
            padding: "10px 14px", minWidth: 240,
            fontFamily: "var(--mv-font-mono)", fontSize: 10,
            color: "var(--mv-fg-2)", lineHeight: 1.7,
            letterSpacing: "0.04em",
          }}>
            <div style={{ color: "var(--mv-blue-400)", fontSize: 9, letterSpacing: "0.16em", marginBottom: 6 }}>/ DEBUG HUD</div>
            <div>Frame: <span style={{ color: "var(--mv-fg-1)" }}>21.154 ms (47.3 FPS)</span></div>
            <div>Verts: <span style={{ color: "var(--mv-fg-1)" }}>24,316</span> · Tris: <span style={{ color: "var(--mv-fg-1)" }}>48,328</span></div>
            <div>GL : <span style={{ color: "var(--mv-fg-1)" }}>4.6.0 — Build 30.0.100.9866</span></div>
            <div>GLSL: <span style={{ color: "var(--mv-fg-1)" }}>4.60 — Build 30.0.100.9866</span></div>
            <div>GPU: <span style={{ color: "var(--mv-fg-1)" }}>RTX 4090 · 18%</span></div>
            <div>Mem: <span style={{ color: "var(--mv-fg-1)" }}>1.42 GB / 24 GB</span></div>
          </div>
        )}

        {/* Subject */}
        <div style={{ position: "absolute", inset: 0, display: "flex", alignItems: "center", justifyContent: "center" }}>
          <svg viewBox="-300 -280 600 580" width="100%" height="100%" preserveAspectRatio="xMidYMid meet">
            <FinalGoblin tint={tweaks.targetTint} showOverlay={showOverlay}/>
          </svg>
        </div>

        {/* Brush cursor */}
        {cursorPos && !exportOpen && (
          <div style={{
            position: "absolute", left: cursorPos.x, top: cursorPos.y,
            width: radius * 2, height: radius * 2,
            transform: "translate(-50%, -50%)",
            borderRadius: "50%",
            border: `1.5px solid ${tool === "smooth" ? "#3DA5F5" : tool === "grab" ? "#F2B547" : tool === "pushpull" ? "#7BC2FA" : "#5DCAE0"}`,
            background: `radial-gradient(circle, transparent 60%, ${tool === "smooth" ? "rgba(61,165,245,0.15)" : "rgba(242,181,71,0.12)"} 100%)`,
            pointerEvents: "none", zIndex: 4,
            mixBlendMode: "screen",
          }}>
            <div style={{
              position: "absolute", left: "50%", top: "50%",
              width: 4, height: 4, transform: "translate(-50%, -50%)",
              background: "currentColor", borderRadius: "50%", color: "#3DA5F5",
            }}/>
          </div>
        )}

        {/* Bottom info HUD */}
        <div style={{
          position: "absolute", left: 16, bottom: 16, zIndex: 5,
          background: "rgba(11,22,34,0.88)", backdropFilter: "blur(10px)",
          border: "1px solid var(--mv-line-2)", borderRadius: 6,
          padding: "10px 14px", minWidth: 260,
        }}>
          <div style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 10,
            letterSpacing: "0.16em", textTransform: "uppercase",
            color: "var(--mv-fg-3)", marginBottom: 6,
            display: "flex", justifyContent: "space-between",
          }}>
            <span>/ Mesh</span>
            <span style={{ color: "var(--mv-success)" }}>● READY TO EXPORT</span>
          </div>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: "4px 16px", fontSize: 12 }}>
            <span style={{ color: "var(--mv-fg-3)" }}>Vertices</span>
            <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-fg-1)", textAlign: "right" }}>24,316</span>
            <span style={{ color: "var(--mv-fg-3)" }}>Triangles</span>
            <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-fg-1)", textAlign: "right" }}>48,328</span>
            <span style={{ color: "var(--mv-fg-3)" }}>UV maps</span>
            <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-fg-1)", textAlign: "right" }}>1</span>
          </div>
        </div>

        {/* Gizmo */}
        <div style={{ position: "absolute", right: 16, bottom: 16, zIndex: 5, width: 88, height: 88 }}>
          <svg viewBox="0 0 100 100" width="100%" height="100%">
            <circle cx="50" cy="50" r="44" fill="rgba(11,22,34,0.6)" stroke="var(--mv-line-2)" strokeWidth="1"/>
            <circle cx="50" cy="50" r="32" fill="none" stroke="var(--mv-blue-500)" strokeWidth="0.8" opacity="0.4"/>
            <line x1="50" y1="6" x2="50" y2="94" stroke="#5BC58A" strokeWidth="0.8" opacity="0.6"/>
            <line x1="6" y1="50" x2="94" y2="50" stroke="#E8665D" strokeWidth="0.8" opacity="0.6"/>
            <text x="50" y="14" fill="#5BC58A" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">Y</text>
            <text x="92" y="53" fill="#E8665D" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">X</text>
            <circle cx="50" cy="50" r="3" fill="#3DA5F5"/>
          </svg>
        </div>
      </div>

      {/* INSPECTOR */}
      <div style={{
        background: "var(--mv-bg-2)",
        borderLeft: "1px solid var(--mv-line-2)",
        display: "flex", flexDirection: "column", overflow: "hidden", minWidth: 0,
      }}>
        {/* Header */}
        <div style={{ padding: "18px 20px 16px", borderBottom: "1px solid var(--mv-line-2)" }}>
          <div style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 11,
            color: "var(--mv-blue-400)", letterSpacing: "0.16em",
            textTransform: "uppercase", marginBottom: 6,
          }}>Stage 04 / 04</div>
          <div style={{
            fontFamily: "var(--mv-font-display)", fontStyle: "italic",
            fontWeight: 800, fontSize: 28, lineHeight: 1, color: "var(--mv-fg-1)",
            letterSpacing: "-0.005em", marginBottom: 8,
          }}>
            Touch <span style={{ color: "var(--mv-blue-500)" }}>up</span>
          </div>
          <div style={{ fontSize: 13, color: "var(--mv-fg-2)", lineHeight: 1.5 }}>
            Sculpt with brushes to refine the result, then export.
          </div>
        </div>

        {/* Body */}
        <div style={{ flex: 1, overflowY: "auto", overflowX: "hidden", padding: "16px 20px 24px" }}>
          {/* Brush */}
          <window.Section title="Brush">
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6 }}>
              <BrushBtn active={tool === "smooth"}    onClick={() => setTool("smooth")}    icon="smooth"   label="Smooth"   sub="Relax surface noise"/>
              <BrushBtn active={tool === "grab"}      onClick={() => setTool("grab")}      icon="grab"     label="Grab"     sub="Drag verts in plane"/>
              <BrushBtn active={tool === "pushpull"}  onClick={() => setTool("pushpull")}  icon="pushpull" label="Push/Pull" sub="Move along normal"/>
              <BrushBtn active={tool === "inflate"}   onClick={() => setTool("inflate")}   icon="inflate"  label="Inflate"  sub="Expand outward"/>
            </div>

            <div style={{ marginTop: 12 }}>
              <ParamRow4 label="Radius"   value={radius}   min={1}  max={80} step={0.1} unit=""    onChange={setRadius}/>
              <ParamRow4 label="Strength" value={strength} min={0}  max={1}  step={0.01} unit=""   onChange={setStrength} display={(v) => v.toFixed(2)}/>
            </div>

            <div style={{ display: "grid", gridTemplateColumns: "100px 1fr", gap: 10, marginTop: 8, alignItems: "center" }}>
              <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>Falloff</span>
              <Select4 value={falloff} options={[
                { v: "smooth", l: "Smooth" },
                { v: "linear", l: "Linear" },
                { v: "constant", l: "Constant" },
                { v: "sharp", l: "Sharp" },
              ]} onChange={setFalloff}/>
            </div>
          </window.Section>

          {/* Symmetry & overlay */}
          <window.Section title="Options">
            <div style={{ display: "grid", gridTemplateColumns: "100px 1fr", gap: 10, alignItems: "center", marginBottom: 10 }}>
              <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>Symmetry</span>
              <Select4 value={symmetry} options={[
                { v: "x", l: "X axis" },
                { v: "y", l: "Y axis" },
                { v: "z", l: "Z axis" },
                { v: "off", l: "Off" },
              ]} onChange={setSymmetry}/>
            </div>
            <ToggleRow4 label="Show target overlay" value={showOverlay} onChange={setShowOverlay}/>
          </window.Section>

          {/* Export button */}
          <button onClick={() => setExportOpen(true)}
            style={{ ...window.btnStyle("primary"), width: "100%", padding: "12px",
                     fontSize: 14, marginBottom: 12 }}>
            <window.IconExport size={15}/> Export Mesh…
          </button>
          <div style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 10, color: "var(--mv-fg-3)",
            letterSpacing: "0.06em", textAlign: "center", lineHeight: 1.5,
          }}>
            <div style={{ color: "var(--mv-success)" }}>● Ready · 24,316 verts</div>
            <div style={{ marginTop: 2 }}>Will write FBX + 2K PNG textures</div>
          </div>
        </div>

        {/* Footer */}
        <div style={{
          padding: "14px 20px", borderTop: "1px solid var(--mv-line-2)",
          background: "var(--mv-bg-1)",
          display: "grid", gridTemplateColumns: "auto 1fr auto", gap: 10, alignItems: "center",
        }}>
          <button style={window.btnStyle("ghost")}><window.IconChevL size={14}/> Previous</button>
          <div style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 10,
            color: "var(--mv-fg-3)", textAlign: "center",
            letterSpacing: "0.12em",
          }}>STEP <span style={{ color: "var(--mv-fg-1)" }}>04</span> OF 04</div>
          <button onClick={() => setExportOpen(true)} style={{ ...window.btnStyle("primary"), background: "var(--mv-success)", borderColor: "var(--mv-success)" }}>
            <window.IconCheck size={14}/> Finish
          </button>
        </div>
      </div>

      {/* EXPORT MODAL */}
      {exportOpen && <ExportModal onClose={() => setExportOpen(false)}/>}
    </div>
  );
}

/* ---------- subcomponents ---------- */

function Pill4({ children }) {
  return (
    <span style={{
      background: "rgba(11,22,34,0.85)", backdropFilter: "blur(8px)",
      border: "1px solid var(--mv-line-2)", borderRadius: 4,
      padding: "5px 9px", fontFamily: "var(--mv-font-mono)", fontSize: 11,
      color: "var(--mv-fg-2)", letterSpacing: "0.06em",
      display: "inline-flex", alignItems: "center", gap: 6,
    }}>{children}</span>
  );
}

function BrushBtn({ icon, label, sub, active, onClick }) {
  const c = active ? "var(--mv-blue-400)" : "var(--mv-fg-3)";
  return (
    <button onClick={onClick} style={{
      background: active ? "rgba(61,165,245,0.08)" : "var(--mv-bg-1)",
      border: `1px solid ${active ? "var(--mv-blue-500)" : "var(--mv-line-2)"}`,
      borderRadius: 6, padding: "10px",
      cursor: "pointer", textAlign: "left",
      boxShadow: active ? "var(--mv-glow-soft)" : "none",
      transition: "all .15s",
      display: "flex", alignItems: "center", gap: 10, minWidth: 0,
    }}>
      <div style={{
        width: 28, height: 28, borderRadius: 4, flexShrink: 0,
        background: active ? "rgba(61,165,245,0.15)" : "var(--mv-bg-2)",
        display: "flex", alignItems: "center", justifyContent: "center", color: c,
      }}>
        <BrushIcon kind={icon}/>
      </div>
      <div style={{ minWidth: 0, flex: 1, overflow: "hidden" }}>
        <div style={{ fontSize: 12, fontWeight: 600, color: active ? "var(--mv-fg-1)" : "var(--mv-fg-2)" }}>
          {label}
        </div>
        <div style={{ fontSize: 10, color: "var(--mv-fg-3)", lineHeight: 1.3, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
          {sub}
        </div>
      </div>
    </button>
  );
}

function BrushIcon({ kind }) {
  const props = { width: 14, height: 14, viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: 1.5, strokeLinecap: "round", strokeLinejoin: "round" };
  if (kind === "smooth")   return <svg {...props}><path d="M3 12c2-3 4-3 6 0s4 3 6 0 4-3 6 0"/></svg>;
  if (kind === "grab")     return <svg {...props}><path d="M9 3v8M9 11l-3 3v5h12v-5l-3-3V8a2 2 0 0 0-2-2"/></svg>;
  if (kind === "pushpull") return <svg {...props}><path d="M12 4v16M6 10l6-6 6 6M6 14l6 6 6-6"/></svg>;
  if (kind === "inflate")  return <svg {...props}><circle cx="12" cy="12" r="3"/><path d="M12 4v3M12 17v3M4 12h3M17 12h3M6 6l2 2M16 16l2 2M18 6l-2 2M8 16l-2 2"/></svg>;
  return null;
}

function ParamRow4({ label, value, min, max, step, unit, onChange, display }) {
  const ref = useRef4(null);
  const [drag, setDrag] = useState4(false);
  const update = (e) => {
    const r = ref.current.getBoundingClientRect();
    const p = Math.max(0, Math.min(1, (e.clientX - r.left) / r.width));
    let v = min + p * (max - min);
    if (step >= 1) v = Math.round(v / step) * step;
    else v = +(Math.round(v / step) * step).toFixed(2);
    onChange(v);
  };
  const pct = ((value - min) / (max - min)) * 100;
  const text = display ? display(value) : (step >= 1 ? Math.round(value) : Number(value).toFixed(1));
  return (
    <div style={{
      display: "grid", gridTemplateColumns: "100px 1fr", gap: 10,
      alignItems: "center", padding: "5px 0",
    }}>
      <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>{label}</span>
      <div style={{
        background: "var(--mv-bg-1)", border: "1px solid var(--mv-line-2)",
        borderRadius: 4, padding: "0 10px", height: 26,
        display: "flex", alignItems: "center", justifyContent: "space-between",
        position: "relative", overflow: "hidden",
      }}>
        <div ref={ref}
          onPointerDown={(e) => { setDrag(true); update(e); e.currentTarget.setPointerCapture(e.pointerId); }}
          onPointerMove={(e) => drag && update(e)}
          onPointerUp={() => setDrag(false)}
          style={{ position: "absolute", inset: 0, cursor: "ew-resize" }}>
          <div style={{
            position: "absolute", left: 0, top: 0, bottom: 0,
            width: pct + "%",
            background: "linear-gradient(90deg, rgba(30,143,224,0.35), rgba(77,174,247,0.4))",
          }}/>
        </div>
        <span style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 11,
          color: "var(--mv-fg-1)", letterSpacing: "0.04em",
          fontVariantNumeric: "tabular-nums",
          marginLeft: "auto", position: "relative", pointerEvents: "none",
        }}>{text}{unit ? ` ${unit}` : ""}</span>
      </div>
    </div>
  );
}

function Select4({ value, options, onChange }) {
  const [open, setOpen] = useState4(false);
  const current = options.find((o) => o.v === value);
  return (
    <div style={{ position: "relative" }}>
      <button onClick={() => setOpen((v) => !v)} style={{
        width: "100%", height: 26, padding: "0 10px",
        background: "var(--mv-bg-1)", border: "1px solid var(--mv-line-2)",
        borderRadius: 4, cursor: "pointer",
        display: "flex", alignItems: "center", justifyContent: "space-between",
        color: "var(--mv-fg-1)", fontSize: 12,
      }}>
        <span>{current?.l}</span>
        <window.IconChevD size={11}/>
      </button>
      {open && (
        <div style={{
          position: "absolute", top: "100%", left: 0, right: 0, marginTop: 2,
          background: "var(--mv-bg-2)", border: "1px solid var(--mv-line-3)",
          borderRadius: 4, padding: 4, zIndex: 30,
          boxShadow: "var(--mv-shadow-2)",
        }}>
          {options.map((o) => (
            <button key={o.v} onClick={() => { onChange(o.v); setOpen(false); }} style={{
              width: "100%", padding: "6px 8px", textAlign: "left",
              background: value === o.v ? "rgba(61,165,245,0.12)" : "transparent",
              border: 0, borderRadius: 3, cursor: "pointer",
              color: "var(--mv-fg-1)", fontSize: 12,
            }}>{o.l}</button>
          ))}
        </div>
      )}
    </div>
  );
}

function ToggleRow4({ label, value, onChange }) {
  return (
    <button onClick={() => onChange(!value)} style={{
      width: "100%", display: "flex", alignItems: "center", justifyContent: "space-between",
      background: "transparent", border: 0, padding: "6px 0",
      cursor: "pointer", color: "var(--mv-fg-1)",
    }}>
      <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>{label}</span>
      <div style={{
        width: 32, height: 18, borderRadius: 999,
        background: value ? "var(--mv-blue-500)" : "var(--mv-bg-3)",
        border: "1px solid " + (value ? "var(--mv-blue-300)" : "var(--mv-line-2)"),
        position: "relative", transition: "background .15s",
      }}>
        <div style={{
          position: "absolute", top: 1, left: value ? 15 : 1,
          width: 14, height: 14, borderRadius: "50%",
          background: "#E8F1FB", transition: "left .15s",
        }}/>
      </div>
    </button>
  );
}

/* ------------------------------------------------------------------ */
/*  EXPORT MODAL                                                      */
/* ------------------------------------------------------------------ */
function ExportModal({ onClose }) {
  const [path, setPath] = useState4("C:/Users/Artist/Documents/Exports/goblin_metahuman.fbx");
  const [format, setFormat] = useState4("fbx");
  const [includeMaterials, setIncludeMaterials] = useState4(true);
  const [bakeTransform, setBakeTransform] = useState4(true);
  const [yUp, setYUp] = useState4(true);
  const [mhCompat, setMhCompat] = useState4(true);
  const [scale, setScale] = useState4(1.0);
  const [uePrefix, setUePrefix] = useState4("SK_");
  const [bakeTextures, setBakeTextures] = useState4(true);
  const [resolution, setResolution] = useState4("2048");
  const [textureFormat, setTextureFormat] = useState4("png");
  const [exporting, setExporting] = useState4(false);

  const doExport = () => {
    setExporting(true);
    setTimeout(() => { setExporting(false); onClose(); }, 1400);
  };

  return (
    <div style={{
      position: "absolute", inset: 0, zIndex: 50,
      background: "rgba(7,16,28,0.65)", backdropFilter: "blur(3px)",
      display: "flex", alignItems: "center", justifyContent: "center",
    }} onClick={onClose}>
      <div onClick={(e) => e.stopPropagation()} style={{
        width: 480, maxHeight: "92%",
        background: "var(--mv-bg-2)",
        border: "1px solid var(--mv-line-3)",
        borderRadius: 6,
        boxShadow: "var(--mv-shadow-3)",
        display: "flex", flexDirection: "column", overflow: "hidden",
      }}>
        {/* Title bar */}
        <div style={{
          padding: "10px 14px",
          background: "var(--mv-bg-3)",
          borderBottom: "1px solid var(--mv-line-2)",
          display: "flex", alignItems: "center", justifyContent: "space-between",
        }}>
          <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
            <window.IconExport size={14}/>
            <span style={{ fontSize: 13, fontWeight: 600, color: "var(--mv-fg-1)" }}>Export mesh</span>
          </div>
          <button onClick={onClose} style={{
            width: 22, height: 22, padding: 0, border: 0,
            background: "transparent", color: "var(--mv-fg-3)", cursor: "pointer",
            display: "flex", alignItems: "center", justifyContent: "center",
            borderRadius: 3,
          }}>
            <window.IconClose size={14}/>
          </button>
        </div>

        {/* Body */}
        <div style={{ padding: "16px 18px", overflowY: "auto", overflowX: "hidden", flex: 1 }}>
          {/* Output file */}
          <FieldGroup label="Output file">
            <div style={{ display: "grid", gridTemplateColumns: "1fr auto", gap: 6 }}>
              <input value={path} onChange={(e) => setPath(e.target.value)} style={inputStyle}/>
              <button style={{ ...window.btnStyle("ghost"), padding: "0 14px" }}><window.IconFolder size={13}/> Browse…</button>
            </div>
          </FieldGroup>

          {/* Export options */}
          <SectionLabel>Export options</SectionLabel>
          <FieldRow label="Format">
            <Select4 value={format} options={[
              { v: "fbx", l: "FBX" },
              { v: "obj", l: "OBJ" },
              { v: "usd", l: "USD" },
              { v: "gltf", l: "glTF 2.0" },
            ]} onChange={setFormat}/>
          </FieldRow>

          <CheckRow label="Include materials" value={includeMaterials} onChange={setIncludeMaterials}/>
          <CheckRow label="Bake transform into vertices" value={bakeTransform} onChange={setBakeTransform}/>
          <CheckRow label="Y-up coordinate system (Unreal Engine)" value={yUp} onChange={setYUp}/>
          <CheckRow label="MetaHuman compatible (preserves original mesh topology)" value={mhCompat} onChange={setMhCompat}/>

          <FieldRow label="Scale factor">
            <NumberInput value={scale} step={0.01} onChange={setScale}/>
          </FieldRow>
          <FieldRow label="UE name prefix">
            <input value={uePrefix} onChange={(e) => setUePrefix(e.target.value)} style={inputStyle}/>
          </FieldRow>

          {/* Texture baking */}
          <SectionLabel>Texture baking</SectionLabel>
          <CheckRow label="Bake and export textures" value={bakeTextures} onChange={setBakeTextures}/>

          <div style={{ opacity: bakeTextures ? 1 : 0.4, pointerEvents: bakeTextures ? "auto" : "none" }}>
            <FieldRow label="Resolution">
              <Select4 value={resolution} options={[
                { v: "512",  l: "512 × 512" },
                { v: "1024", l: "1024 × 1024" },
                { v: "2048", l: "2048 × 2048" },
                { v: "4096", l: "4096 × 4096" },
              ]} onChange={setResolution}/>
            </FieldRow>
            <FieldRow label="Texture format">
              <Select4 value={textureFormat} options={[
                { v: "png", l: "PNG" },
                { v: "jpg", l: "JPEG" },
                { v: "exr", l: "EXR" },
                { v: "tga", l: "TGA" },
              ]} onChange={setTextureFormat}/>
            </FieldRow>
          </div>
        </div>

        {/* Footer */}
        <div style={{
          padding: "12px 18px",
          borderTop: "1px solid var(--mv-line-2)",
          background: "var(--mv-bg-1)",
          display: "flex", justifyContent: "flex-end", gap: 8,
        }}>
          <button onClick={onClose} style={window.btnStyle("ghost")}>Cancel</button>
          <button onClick={doExport} disabled={exporting} style={{
            ...window.btnStyle("primary"),
            opacity: exporting ? 0.6 : 1,
          }}>
            {exporting ? "Exporting…" : <><window.IconExport size={13}/> Export</>}
          </button>
        </div>
      </div>
    </div>
  );
}

const inputStyle = {
  width: "100%", height: 28, padding: "0 10px",
  background: "var(--mv-bg-1)", border: "1px solid var(--mv-line-2)",
  borderRadius: 4, color: "var(--mv-fg-1)",
  fontFamily: "var(--mv-font-mono)", fontSize: 12,
  outline: "none", boxSizing: "border-box",
};

function NumberInput({ value, step, onChange }) {
  return (
    <div style={{ display: "grid", gridTemplateColumns: "1fr auto auto", gap: 0,
      border: "1px solid var(--mv-line-2)", borderRadius: 4,
      background: "var(--mv-bg-1)", overflow: "hidden", height: 28,
    }}>
      <input value={value.toFixed(3)} readOnly style={{
        ...inputStyle, border: 0, background: "transparent", height: 26,
      }}/>
      <button onClick={() => onChange(+(value - step).toFixed(3))} style={{
        width: 22, border: 0, borderLeft: "1px solid var(--mv-line-2)",
        background: "var(--mv-bg-2)", color: "var(--mv-fg-2)", cursor: "pointer",
      }}>−</button>
      <button onClick={() => onChange(+(value + step).toFixed(3))} style={{
        width: 22, border: 0, borderLeft: "1px solid var(--mv-line-2)",
        background: "var(--mv-bg-2)", color: "var(--mv-fg-2)", cursor: "pointer",
      }}>+</button>
    </div>
  );
}

function FieldGroup({ label, children }) {
  return (
    <div style={{ marginBottom: 14 }}>
      <div style={{ fontSize: 11, color: "var(--mv-fg-3)", marginBottom: 6 }}>{label}</div>
      {children}
    </div>
  );
}

function FieldRow({ label, children }) {
  return (
    <div style={{
      display: "grid", gridTemplateColumns: "1fr 200px", gap: 10,
      alignItems: "center", marginBottom: 8,
    }}>
      <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>{label}</span>
      <div>{children}</div>
    </div>
  );
}

function CheckRow({ label, value, onChange }) {
  return (
    <button onClick={() => onChange(!value)} style={{
      display: "flex", alignItems: "center", gap: 8,
      width: "100%", background: "transparent", border: 0,
      padding: "5px 0", cursor: "pointer", textAlign: "left",
    }}>
      <div style={{
        width: 16, height: 16, borderRadius: 3, flexShrink: 0,
        background: value ? "var(--mv-blue-500)" : "var(--mv-bg-1)",
        border: `1px solid ${value ? "var(--mv-blue-400)" : "var(--mv-line-2)"}`,
        display: "flex", alignItems: "center", justifyContent: "center",
      }}>
        {value && <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="#051322" strokeWidth="3" strokeLinecap="round" strokeLinejoin="round"><path d="M5 12l5 5 9-11"/></svg>}
      </div>
      <span style={{ fontSize: 12, color: "var(--mv-fg-1)" }}>{label}</span>
    </button>
  );
}

function SectionLabel({ children }) {
  return (
    <div style={{
      fontFamily: "var(--mv-font-mono)", fontSize: 10,
      color: "var(--mv-fg-3)", letterSpacing: "0.16em",
      textTransform: "uppercase", marginTop: 16, marginBottom: 8,
      paddingTop: 12, borderTop: "1px solid var(--mv-line-2)",
    }}>{children}</div>
  );
}

window.Stage4View = Stage4View;
