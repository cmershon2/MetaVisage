/* global React, ReactDOM */
const { useState, useEffect, useRef, useCallback } = React;

/* ------------------------------------------------------------------ */
/*  ICONS — Lucide-style 24px stroke 1.5                              */
/* ------------------------------------------------------------------ */
const Icon = ({ d, size = 18, sw = 1.5, children, viewBox = "0 0 24 24", ...rest }) => (
  <svg viewBox={viewBox} width={size} height={size} fill="none" stroke="currentColor"
       strokeWidth={sw} strokeLinecap="square" strokeLinejoin="miter" {...rest}>
    {d ? <path d={d} /> : children}
  </svg>
);

const IconAlign = (p) => <Icon {...p}><circle cx="9" cy="12" r="5"/><circle cx="15" cy="12" r="5"/><path d="M12 7v10"/></Icon>;
const IconPoints = (p) => <Icon {...p}><circle cx="6" cy="6" r="1.5"/><circle cx="18" cy="6" r="1.5"/><circle cx="6" cy="18" r="1.5"/><circle cx="18" cy="18" r="1.5"/><circle cx="12" cy="12" r="1.5"/><path d="M6 6L18 18M18 6L6 18"/></Icon>;
const IconMorph = (p) => <Icon {...p}><path d="M3 12c0-5 4-9 9-9s9 4 9 9-4 9-9 9"/><path d="M3 12h6M9 9l-3 3 3 3"/></Icon>;
const IconBrush = (p) => <Icon {...p}><path d="M14 4l6 6-9 9H5v-6z"/><path d="M14 4l3-3 3 3-3 3"/></Icon>;
const IconImport = (p) => <Icon {...p}><path d="M12 3v12M7 10l5 5 5-5M4 21h16"/></Icon>;
const IconReset = (p) => <Icon {...p}><path d="M3 12a9 9 0 109-9"/><path d="M3 4v5h5"/></Icon>;
const IconLock = (p) => <Icon {...p}><rect x="5" y="11" width="14" height="10" rx="1"/><path d="M8 11V7a4 4 0 018 0v4"/></Icon>;
const IconEye = (p) => <Icon {...p}><path d="M2 12s4-7 10-7 10 7 10 7-4 7-10 7-10-7-10-7z"/><circle cx="12" cy="12" r="3"/></Icon>;
const IconEyeOff = (p) => <Icon {...p}><path d="M3 3l18 18"/><path d="M10.5 6.5A10 10 0 0112 6c6 0 10 6 10 6a17 17 0 01-3.5 4.5"/><path d="M6.5 6.5C3.5 8.5 2 12 2 12s4 6 10 6c1.5 0 2.8-.3 4-.8"/></Icon>;
const IconHelp = (p) => <Icon {...p}><circle cx="12" cy="12" r="9"/><path d="M9.5 9a2.5 2.5 0 015 0c0 2-2.5 2-2.5 4"/><circle cx="12" cy="17" r=".5" fill="currentColor"/></Icon>;
const IconClose = (p) => <Icon {...p}><path d="M5 5l14 14M19 5L5 19"/></Icon>;
const IconMin = (p) => <Icon {...p}><path d="M5 19h14"/></Icon>;
const IconMax = (p) => <Icon {...p}><rect x="5" y="5" width="14" height="14"/></Icon>;
const IconChevR = (p) => <Icon {...p}><path d="M9 6l6 6-6 6"/></Icon>;
const IconChevL = (p) => <Icon {...p}><path d="M15 6l-6 6 6 6"/></Icon>;
const IconChevD = (p) => <Icon {...p}><path d="M6 9l6 6 6-6"/></Icon>;
const IconCheck = (p) => <Icon {...p}><path d="M5 12l5 5 9-11"/></Icon>;
const IconPlus = (p) => <Icon {...p}><path d="M12 5v14M5 12h14"/></Icon>;
const IconMove = (p) => <Icon {...p}><path d="M12 3v18M3 12h18M9 6l3-3 3 3M9 18l3 3 3-3M6 9l-3 3 3 3M18 9l3 3-3 3"/></Icon>;
const IconRotate = (p) => <Icon {...p}><path d="M21 12a9 9 0 11-3-6.7"/><path d="M21 4v5h-5"/></Icon>;
const IconScale = (p) => <Icon {...p}><path d="M3 21V11M3 21h10M3 21l8-8"/><path d="M21 3v10M21 3H11M21 3l-8 8"/></Icon>;
const IconSwap = (p) => <Icon {...p}><path d="M7 4l-4 4 4 4M3 8h14M17 12l4 4-4 4M21 16H7"/></Icon>;
const IconWand = (p) => <Icon {...p}><path d="M15 4l5 5L9 20l-5-5z"/><path d="M14 5l5 5"/><path d="M19 3l1 2 2 1-2 1-1 2-1-2-2-1 2-1z"/></Icon>;
const IconLayers = (p) => <Icon {...p}><path d="M12 3l9 5-9 5-9-5z"/><path d="M3 13l9 5 9-5"/><path d="M3 18l9 5 9-5"/></Icon>;
const IconFolder = (p) => <Icon {...p}><path d="M3 6a2 2 0 012-2h4l2 2h8a2 2 0 012 2v9a2 2 0 01-2 2H5a2 2 0 01-2-2z"/></Icon>;
const IconExport = (p) => <Icon {...p}><path d="M12 16V4M7 9l5-5 5 5M5 16v3a1 1 0 001 1h12a1 1 0 001-1v-3"/></Icon>;

/* ------------------------------------------------------------------ */
/*  STAGES                                                            */
/* ------------------------------------------------------------------ */
const STAGES = [
  { n: 1, key: "align",   label: "Alignment",        sub: "Roughly position the target",   Icon: IconAlign },
  { n: 2, key: "points",  label: "Point Reference",  sub: "Mark corresponding landmarks",  Icon: IconPoints },
  { n: 3, key: "morph",   label: "Morphing",         sub: "Wrap the source onto target",   Icon: IconMorph },
  { n: 4, key: "touchup", label: "Touch up & Export",sub: "Sculpt fixes, then ship",       Icon: IconBrush },
];

/* ------------------------------------------------------------------ */
/*  TWEAKABLE DEFAULTS                                                */
/* ------------------------------------------------------------------ */
const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "stepperStyle": "rail",
  "viewportTexture": "blueprint",
  "showHotkeyHints": true,
  "targetTint": "blue",
  "compactInspector": false,
  "loginState": "idle",
  "showLogin": true
}/*EDITMODE-END*/;

/* ------------------------------------------------------------------ */
/*  HEADS — SVG renders                                               */
/* ------------------------------------------------------------------ */
function CustomHeadSculpt({ rot = 0 }) {
  // Goblin-ish source mesh — orange wireframe + warm tan fill
  return (
    <g transform={`rotate(${rot * 0.4})`}>
      <defs>
        <linearGradient id="srcfill" x1="0" y1="0" x2="1" y2="1">
          <stop offset="0%" stopColor="#E89A4A"/>
          <stop offset="55%" stopColor="#C97726"/>
          <stop offset="100%" stopColor="#7B4413"/>
        </linearGradient>
        <radialGradient id="srcrim" cx="0.3" cy="0.3" r="0.7">
          <stop offset="0%" stopColor="#FFD9A8" stopOpacity="0.6"/>
          <stop offset="60%" stopColor="#C97726" stopOpacity="0"/>
        </radialGradient>
      </defs>
      {/* Body */}
      <path d="M -110,-20
               C -150,-40 -150,-110 -90,-150
               C -50,-180 30,-200 80,-150
               C 130,-100 130,-30 110,40
               C 130,60 140,90 130,140
               C 90,200 -80,200 -110,150
               C -130,110 -130,40 -110,-20 Z"
            fill="url(#srcfill)"/>
      <path d="M -110,-20 C -150,-40 -150,-110 -90,-150 C -50,-180 30,-200 80,-150 C 130,-100 130,-30 110,40 Z"
            fill="url(#srcrim)"/>
      {/* Long ear */}
      <path d="M -100,-90 Q -180,-100 -200,-50 Q -190,-20 -130,-30 Z" fill="#A86223"/>
      <path d="M -100,-90 Q -180,-100 -200,-50 Q -190,-20 -130,-30 Z" fill="url(#srcrim)" opacity="0.5"/>
      {/* Brow */}
      <path d="M -70,-110 Q -30,-130 30,-110" stroke="#5A2F0F" strokeWidth="3" fill="none"/>
      {/* Eyes - sunken */}
      <ellipse cx="-50" cy="-80" rx="14" ry="8" fill="#2A1408"/>
      <ellipse cx="20" cy="-85" rx="14" ry="8" fill="#2A1408"/>
      <circle cx="-48" cy="-80" r="3" fill="#FFD9A8"/>
      <circle cx="22" cy="-85" r="3" fill="#FFD9A8"/>
      {/* Long crooked nose */}
      <path d="M -10,-90 Q -25,-50 -30,0 Q -20,30 0,40 Q 10,30 5,0 Q 5,-50 0,-90 Z"
            fill="#B86E1F" stroke="#5A2F0F" strokeWidth="1.5"/>
      {/* Mouth - downturned */}
      <path d="M -50,70 Q -20,90 30,70 Q 35,75 30,82 Q -20,100 -50,82 Z" fill="#3A1B08"/>
      <path d="M -45,75 Q -10,82 25,75" stroke="#FFD9A8" strokeWidth="1.5" fill="none"/>
      {/* Wrinkles */}
      <path d="M -90,-40 Q -70,-30 -50,-40" stroke="#5A2F0F" strokeWidth="1" fill="none" opacity="0.7"/>
      <path d="M 30,-50 Q 50,-40 70,-50" stroke="#5A2F0F" strokeWidth="1" fill="none" opacity="0.7"/>
      <path d="M -80,20 Q -50,30 -20,20" stroke="#5A2F0F" strokeWidth="1" fill="none" opacity="0.5"/>
      <path d="M -80,40 Q -40,50 0,40" stroke="#5A2F0F" strokeWidth="1" fill="none" opacity="0.4"/>
      {/* Cheekbone shading */}
      <ellipse cx="-70" cy="-20" rx="20" ry="10" fill="#5A2F0F" opacity="0.25"/>
      <ellipse cx="60" cy="-30" rx="20" ry="10" fill="#5A2F0F" opacity="0.2"/>
      {/* Origin gizmo */}
      <g>
        <circle r="3" fill="#FFD9A8"/>
        <line x1="0" y1="0" x2="40" y2="0" stroke="#FF5555" strokeWidth="2"/>
        <polygon points="40,-4 50,0 40,4" fill="#FF5555"/>
        <line x1="0" y1="0" x2="0" y2="-40" stroke="#55FF77" strokeWidth="2"/>
        <polygon points="-4,-40 0,-50 4,-40" fill="#55FF77"/>
        <line x1="0" y1="0" x2="-25" y2="20" stroke="#5599FF" strokeWidth="2"/>
      </g>
    </g>
  );
}

function MetaHumanTarget({ tint = "blue", showWire = true, rot = 0 }) {
  const fillId = "tgtfill_" + tint;
  const colors = {
    blue: ["#7BC2FA", "#3DA5F5", "#1572B8"],
    cyan: ["#A8E5FA", "#5DCAE0", "#1A7A95"],
    neutral: ["#A8BBD0", "#6F839A", "#2F4358"],
  }[tint] || ["#7BC2FA", "#3DA5F5", "#1572B8"];
  return (
    <g transform={`rotate(${rot * 0.4})`}>
      <defs>
        <linearGradient id={fillId} x1="0.3" y1="0" x2="0.7" y2="1">
          <stop offset="0%" stopColor={colors[0]}/>
          <stop offset="55%" stopColor={colors[1]}/>
          <stop offset="100%" stopColor={colors[2]}/>
        </linearGradient>
        <radialGradient id={fillId+"_rim"} cx="0.3" cy="0.3" r="0.7">
          <stop offset="0%" stopColor="#E8F1FB" stopOpacity="0.55"/>
          <stop offset="60%" stopColor={colors[1]} stopOpacity="0"/>
        </radialGradient>
      </defs>
      {/* Head silhouette */}
      <path d="M -100,-40
               C -110,-130 -60,-180 0,-180
               C 60,-180 110,-130 100,-40
               C 95,20 80,80 60,130
               C 90,150 100,180 90,210
               C 30,235 -30,235 -90,210
               C -100,180 -90,150 -60,130
               C -80,80 -95,20 -100,-40 Z"
            fill={`url(#${fillId})`}/>
      <path d="M -100,-40 C -110,-130 -60,-180 0,-180 C 60,-180 110,-130 100,-40 Z"
            fill={`url(#${fillId}_rim)`}/>
      {/* Eyes */}
      <ellipse cx="-38" cy="-75" rx="14" ry="7" fill="#08151F" opacity="0.7"/>
      <ellipse cx="38" cy="-75" rx="14" ry="7" fill="#08151F" opacity="0.7"/>
      <circle cx="-38" cy="-75" r="4" fill="#1B2A3A"/>
      <circle cx="38" cy="-75" r="4" fill="#1B2A3A"/>
      {/* Nose */}
      <path d="M 0,-60 Q -8,-20 -12,10 Q -8,18 0,20 Q 8,18 12,10 Q 8,-20 0,-60"
            fill="#1572B8" opacity="0.55"/>
      {/* Mouth */}
      <path d="M -30,55 Q 0,62 30,55 Q 30,58 0,62 Q -30,58 -30,55 Z" fill="#08151F" opacity="0.5"/>
      <path d="M -30,55 Q 0,62 30,55" stroke="#08151F" strokeWidth="1" fill="none" opacity="0.7"/>
      {/* Brow */}
      <path d="M -60,-100 Q -38,-108 -16,-100" stroke="#08151F" strokeWidth="2" fill="none" opacity="0.4"/>
      <path d="M 16,-100 Q 38,-108 60,-100" stroke="#08151F" strokeWidth="2" fill="none" opacity="0.4"/>
      {/* Wireframe overlay */}
      {showWire && (
        <g stroke={colors[0]} fill="none" strokeWidth="0.6" opacity="0.4">
          <path d="M -90,-60 Q 0,-70 90,-60"/>
          <path d="M -95,-20 Q 0,-30 95,-20"/>
          <path d="M -100,20 Q 0,15 100,20"/>
          <path d="M -90,60 Q 0,55 90,60"/>
          <path d="M -75,100 Q 0,95 75,100"/>
          <path d="M -45,140 Q 0,135 45,140"/>
          <path d="M -60,-160 Q 0,-180 60,-160"/>
          <path d="M -85,-110 Q 0,-130 85,-110"/>
          <path d="M -38,-180 Q -38,-40 -45,140"/>
          <path d="M 0,-180 Q 0,-40 0,150"/>
          <path d="M 38,-180 Q 38,-40 45,140"/>
          <path d="M -75,-150 Q -90,-40 -75,100"/>
          <path d="M 75,-150 Q 90,-40 75,100"/>
        </g>
      )}
    </g>
  );
}

/* ------------------------------------------------------------------ */
/*  TITLE BAR (Windows-style)                                         */
/* ------------------------------------------------------------------ */
function TitleBar({ stage, file }) {
  return (
    <div style={{
      height: 36, background: "var(--mv-bg-0)", display: "flex",
      alignItems: "center", padding: "0 0 0 12px",
      borderBottom: "1px solid var(--mv-line-2)",
      fontFamily: "var(--mv-font-mono)", fontSize: 11,
      color: "var(--mv-fg-3)", letterSpacing: "0.06em",
    }}>
      <img src="assets/metavisage-logo-mark.png" alt="" style={{ width: 18, height: 18, marginRight: 10, opacity: 0.9 }} />
      <span style={{ color: "var(--mv-fg-1)", fontWeight: 600 }}>MetaVisage</span>
      <span style={{ color: "var(--mv-line-3)", margin: "0 10px" }}>·</span>
      <span style={{ color: "var(--mv-fg-2)" }}>{file}</span>
      <span style={{ color: "var(--mv-line-3)", margin: "0 10px" }}>›</span>
      <span style={{ color: "var(--mv-blue-400)", textTransform: "uppercase", letterSpacing: "0.12em" }}>
        Stage {stage.n} / 4 · {stage.label}
      </span>
      <div style={{ flex: 1 }} />
      <div style={{ display: "flex", height: "100%" }}>
        {[IconMin, IconMax, IconClose].map((I, i) => (
          <button key={i} style={{
            width: 46, height: "100%", background: "transparent", border: 0,
            color: "var(--mv-fg-2)", cursor: "pointer",
            display: "inline-flex", alignItems: "center", justifyContent: "center",
            transition: "background .15s",
          }}
          onMouseEnter={(e) => e.currentTarget.style.background = i === 2 ? "#E81123" : "var(--mv-bg-2)"}
          onMouseLeave={(e) => e.currentTarget.style.background = "transparent"}>
            <I size={14} sw={1.6}/>
          </button>
        ))}
      </div>
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  MENU BAR                                                          */
/* ------------------------------------------------------------------ */
function MenuBar() {
  return (
    <div style={{
      height: 32, background: "var(--mv-bg-2)",
      borderBottom: "1px solid var(--mv-line-2)",
      display: "flex", alignItems: "center", padding: "0 8px", gap: 2,
    }}>
      {["File", "Edit", "View", "Mesh", "Help"].map((m) => (
        <span key={m} style={{
          padding: "5px 10px", fontSize: 13, color: "var(--mv-fg-2)",
          borderRadius: 4, cursor: "pointer",
        }}
        onMouseEnter={(e) => { e.currentTarget.style.background = "var(--mv-bg-3)"; e.currentTarget.style.color = "var(--mv-fg-1)"; }}
        onMouseLeave={(e) => { e.currentTarget.style.background = "transparent"; e.currentTarget.style.color = "var(--mv-fg-2)"; }}>
          {m}
        </span>
      ))}
      <div style={{ flex: 1 }} />
      <span style={{
        fontFamily: "var(--mv-font-mono)", fontSize: 11,
        color: "var(--mv-fg-3)", letterSpacing: "0.08em", marginRight: 14,
        whiteSpace: "nowrap",
      }}>
        Autosaved <span style={{ color: "var(--mv-fg-1)" }}>12s ago</span>
      </span>
      <span style={{
        display: "inline-flex", alignItems: "center", gap: 6,
        padding: "3px 10px", borderRadius: 4,
        border: "1px solid rgba(63,214,158,0.4)",
        background: "rgba(63,214,158,0.06)",
        color: "var(--mv-success)",
        fontFamily: "var(--mv-font-mono)", fontSize: 11, letterSpacing: "0.1em",
        whiteSpace: "nowrap", flexShrink: 0,
      }}>
        <svg width="8" height="8"><circle cx="4" cy="4" r="3" fill="currentColor"/></svg>
        UE 5.4 · LIVE LINK
      </span>
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  TOP STAGE STEPPER                                                 */
/* ------------------------------------------------------------------ */
function StageStepper({ active, onChange }) {
  return (
    <div style={{
      height: 64, background: "var(--mv-bg-1)",
      borderBottom: "1px solid var(--mv-line-2)",
      display: "flex", alignItems: "stretch", padding: "0 8px",
      position: "relative",
    }}>
      {STAGES.map((s, i) => {
        const isActive = s.n === active;
        const isDone = s.n < active;
        const tint = isActive ? "var(--mv-blue-500)"
                   : isDone   ? "var(--mv-success)"
                              : "var(--mv-fg-4)";
        return (
          <React.Fragment key={s.n}>
            <button onClick={() => onChange(s.n)} style={{
              flex: 1, background: "transparent", border: 0,
              padding: "8px 16px", cursor: "pointer", textAlign: "left",
              display: "flex", alignItems: "center", gap: 14,
              color: "var(--mv-fg-1)", position: "relative",
              borderBottom: isActive ? "2px solid var(--mv-blue-500)" : "2px solid transparent",
              transition: "background .15s",
            }}
            onMouseEnter={(e) => !isActive && (e.currentTarget.style.background = "var(--mv-bg-2)")}
            onMouseLeave={(e) => !isActive && (e.currentTarget.style.background = "transparent")}>
              {/* Number disc */}
              <div style={{
                width: 36, height: 36, borderRadius: 6,
                display: "flex", alignItems: "center", justifyContent: "center",
                background: isActive ? "var(--mv-blue-500)" : isDone ? "rgba(63,214,158,0.12)" : "var(--mv-bg-2)",
                border: isActive ? "1px solid var(--mv-blue-300)" : `1px solid ${isDone ? "rgba(63,214,158,0.4)" : "var(--mv-line-2)"}`,
                color: isActive ? "#051322" : tint,
                boxShadow: isActive ? "var(--mv-glow-soft)" : "none",
                flexShrink: 0,
              }}>
                {isDone ? <IconCheck size={16} sw={2.2}/> :
                  <span style={{
                    fontFamily: "var(--mv-font-mono)", fontSize: 13, fontWeight: 600,
                    letterSpacing: "0.04em",
                  }}>0{s.n}</span>}
              </div>
              <div style={{ minWidth: 0 }}>
                <div style={{
                  fontFamily: "var(--mv-font-mono)", fontSize: 10, letterSpacing: "0.16em",
                  textTransform: "uppercase", color: tint, marginBottom: 2,
                }}>
                  {isActive ? "In progress" : isDone ? "Complete" : "Upcoming"}
                </div>
                <div style={{
                  fontSize: 14, fontWeight: 600,
                  color: isActive ? "var(--mv-fg-1)" : isDone ? "var(--mv-fg-2)" : "var(--mv-fg-3)",
                }}>
                  {s.label}
                </div>
              </div>
            </button>
            {i < STAGES.length - 1 && (
              <div style={{
                display: "flex", alignItems: "center", padding: "0 4px",
                color: "var(--mv-fg-4)",
              }}>
                <IconChevR size={14}/>
              </div>
            )}
          </React.Fragment>
        );
      })}
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  VIEWPORT                                                          */
/* ------------------------------------------------------------------ */
function Viewport({ transform, hasTarget, sourceLoaded, viewportTexture, targetTint, onResetView }) {
  const svgRef = useRef(null);
  const [drag, setDrag] = useState(null);
  const [view, setView] = useState({ rot: 0, zoom: 1 });

  const onPointerDown = (e) => {
    setDrag({ x: e.clientX, y: e.clientY, startRot: view.rot });
    e.currentTarget.setPointerCapture(e.pointerId);
  };
  const onPointerMove = (e) => {
    if (!drag) return;
    const dx = e.clientX - drag.x;
    setView((v) => ({ ...v, rot: drag.startRot + dx * 0.4 }));
  };
  const onPointerUp = (e) => { setDrag(null); };

  const bg = viewportTexture === "blueprint" ? {
    backgroundColor: "var(--mv-bg-0)",
    backgroundImage: `
      linear-gradient(var(--mv-line-1) 1px, transparent 1px),
      linear-gradient(90deg, var(--mv-line-1) 1px, transparent 1px),
      linear-gradient(rgba(125,175,220,0.04) 1px, transparent 1px),
      linear-gradient(90deg, rgba(125,175,220,0.04) 1px, transparent 1px)`,
    backgroundSize: "96px 96px, 96px 96px, 16px 16px, 16px 16px",
  } : viewportTexture === "fine" ? {
    backgroundColor: "var(--mv-bg-0)",
    backgroundImage: `
      linear-gradient(rgba(125,175,220,0.05) 1px, transparent 1px),
      linear-gradient(90deg, rgba(125,175,220,0.05) 1px, transparent 1px)`,
    backgroundSize: "16px 16px, 16px 16px",
  } : { backgroundColor: "var(--mv-bg-0)" };

  return (
    <div style={{
      position: "relative", overflow: "hidden",
      ...bg,
      cursor: drag ? "grabbing" : "grab",
    }}
    onPointerDown={onPointerDown}
    onPointerMove={onPointerMove}
    onPointerUp={onPointerUp}>

      {/* TOP VIEWPORT TOOLBAR */}
      <div style={{
        position: "absolute", top: 14, left: "50%", transform: "translateX(-50%)",
        display: "flex", gap: 4, alignItems: "center",
        background: "rgba(11,22,34,0.85)", backdropFilter: "blur(12px)",
        border: "1px solid var(--mv-line-2)", borderRadius: 6, padding: 5,
        zIndex: 5,
      }}>
        <span style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 11,
          color: "var(--mv-blue-400)", padding: "0 10px",
          letterSpacing: "0.08em",
        }}>PERSP</span>
        <ToolbarSep/>
        <VPBtn label="Solid" active><svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor" stroke="none"><circle cx="12" cy="12" r="8"/></svg></VPBtn>
        <VPBtn label="Wireframe"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><circle cx="12" cy="12" r="8"/><path d="M4 12h16M12 4v16M6 6l12 12M18 6L6 18"/></svg></VPBtn>
        <VPBtn label="Textured"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><circle cx="12" cy="12" r="8"/><path d="M5 9c4 3 10 3 14 0M5 15c4 3 10 3 14 0"/></svg></VPBtn>
        <ToolbarSep/>
        <VPBtn label="Show source" active><IconEye size={14}/></VPBtn>
        <VPBtn label="Show target" active><IconEye size={14}/></VPBtn>
        <ToolbarSep/>
        <span style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 11,
          color: "var(--mv-fg-3)", padding: "0 10px",
          letterSpacing: "0.06em",
        }}>F · ⇧ axis · ⌥ inv</span>
      </div>

      {/* TOP-RIGHT META PILLS */}
      <div style={{ position: "absolute", top: 14, right: 14, display: "flex", gap: 6, zIndex: 5 }}>
        <Pill><span style={{ color: "var(--mv-blue-400)" }}>●</span> LIVE</Pill>
        <Pill>FPS <b style={{ color: "var(--mv-fg-1)" }}>120</b></Pill>
      </div>

      {/* SUBJECTS */}
      <div style={{ position: "absolute", inset: 0, display: "flex", alignItems: "center", justifyContent: "center", pointerEvents: "none" }}>
        <svg ref={svgRef} viewBox="-500 -360 1000 720" width="100%" height="100%" preserveAspectRatio="xMidYMid meet">
          {/* Floor grid lines for depth */}
          <g opacity="0.15" stroke="var(--mv-blue-500)" strokeWidth="0.5">
            <line x1="-500" y1="280" x2="500" y2="280"/>
            <line x1="-300" y1="320" x2="300" y2="320"/>
          </g>

          {/* Target metahuman (placed roughly center, base) */}
          {hasTarget && (
            <g transform={`translate(${100 + transform.tx * 0.6} ${transform.ty * 0.6}) scale(${0.85 * transform.scale * view.zoom}) rotate(${transform.ry})`}>
              <MetaHumanTarget tint={targetTint} rot={view.rot}/>
            </g>
          )}

          {/* Source mesh */}
          {sourceLoaded && (
            <g transform={`translate(-180 0) scale(0.95) rotate(${view.rot * 0.4})`}>
              <CustomHeadSculpt/>
            </g>
          )}

          {/* If neither loaded, show placeholder ghost */}
          {!sourceLoaded && (
            <g opacity="0.25">
              <circle r="120" cx="0" cy="0" fill="none" stroke="var(--mv-blue-500)" strokeWidth="1" strokeDasharray="6 6"/>
              <text textAnchor="middle" y="200" fill="var(--mv-fg-3)" fontFamily="JetBrains Mono" fontSize="14" letterSpacing="3">DROP MESH</text>
            </g>
          )}
        </svg>
      </div>

      {/* BOTTOM-LEFT INSTRUCTIONS HUD */}
      <div style={{
        position: "absolute", left: 16, bottom: 16, zIndex: 5, maxWidth: 320,
        background: "rgba(11,22,34,0.88)", backdropFilter: "blur(10px)",
        border: "1px solid var(--mv-line-2)", borderRadius: 6,
        padding: "12px 14px",
      }}>
        <div style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 10,
          letterSpacing: "0.16em", textTransform: "uppercase",
          color: "var(--mv-fg-3)", marginBottom: 6,
          display: "flex", justifyContent: "space-between",
        }}>
          <span>/ Stage 1 · Alignment</span>
          <span style={{ color: "var(--mv-blue-400)" }}>HINT</span>
        </div>
        <div style={{
          fontSize: 13, color: "var(--mv-fg-1)", lineHeight: 1.5, marginBottom: 8,
        }}>
          Position the source roughly over the MetaHuman target. Hit <Kbd>F</Kbd> to focus, drag the gizmo handles to translate.
        </div>
        <div style={{ display: "flex", gap: 6, flexWrap: "wrap", fontFamily: "var(--mv-font-mono)", fontSize: 10, color: "var(--mv-fg-3)", letterSpacing: "0.06em" }}>
          <Kbd>W</Kbd> move &nbsp; <Kbd>E</Kbd> rotate &nbsp; <Kbd>R</Kbd> scale
        </div>
      </div>

      {/* BOTTOM-RIGHT GIZMO */}
      <div style={{
        position: "absolute", right: 16, bottom: 16, zIndex: 5,
        width: 88, height: 88,
      }}>
        <svg viewBox="0 0 100 100" width="100%" height="100%">
          <circle cx="50" cy="50" r="44" fill="rgba(11,22,34,0.6)" stroke="var(--mv-line-2)" strokeWidth="1"/>
          <circle cx="50" cy="50" r="32" fill="none" stroke="var(--mv-blue-500)" strokeWidth="0.8" opacity="0.4"/>
          <line x1="50" y1="6" x2="50" y2="94" stroke="var(--mv-blue-500)" strokeWidth="0.5" opacity="0.4"/>
          <line x1="6" y1="50" x2="94" y2="50" stroke="var(--mv-blue-500)" strokeWidth="0.5" opacity="0.4"/>
          <text x="50" y="14" fill="#7BC2FA" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">Y</text>
          <text x="92" y="53" fill="#7BC2FA" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">X</text>
          <text x="50" y="92" fill="#7BC2FA" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">−Y</text>
          <text x="8" y="53" fill="#7BC2FA" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">−X</text>
          <circle cx="50" cy="50" r="3" fill="#3DA5F5"/>
        </svg>
      </div>
    </div>
  );
}

const ToolbarSep = () => <div style={{ width: 1, height: 18, background: "var(--mv-line-2)", margin: "0 3px" }}/>;
const VPBtn = ({ children, active, label }) => (
  <button title={label} style={{
    width: 28, height: 26, display: "inline-flex", alignItems: "center", justifyContent: "center",
    border: 0, borderRadius: 4, cursor: "pointer",
    background: active ? "var(--mv-blue-500)" : "transparent",
    color: active ? "#051322" : "var(--mv-fg-2)",
  }}>{children}</button>
);
const Pill = ({ children }) => (
  <span style={{
    background: "rgba(11,22,34,0.85)", backdropFilter: "blur(8px)",
    border: "1px solid var(--mv-line-2)", borderRadius: 4,
    padding: "5px 9px", fontFamily: "var(--mv-font-mono)", fontSize: 11,
    color: "var(--mv-fg-2)", letterSpacing: "0.06em",
    display: "inline-flex", alignItems: "center", gap: 6,
  }}>{children}</span>
);
const Kbd = ({ children }) => (
  <span style={{
    fontFamily: "var(--mv-font-mono)", fontSize: 10,
    padding: "2px 6px", borderRadius: 3,
    background: "var(--mv-bg-3)", color: "var(--mv-fg-1)",
    border: "1px solid var(--mv-line-2)",
    boxShadow: "0 1px 0 rgba(0,0,0,0.3)",
  }}>{children}</span>
);

/* ------------------------------------------------------------------ */
/*  INSPECTOR — Stage 1 content                                       */
/* ------------------------------------------------------------------ */
function Stage1Inspector({
  sourceFile, targetFile, onPickSource, onPickTarget, onUseDefault,
  transform, setTransform, onReset, onAutoAlign, textures, addTexture,
}) {
  return (
    <div style={{ display: "flex", flexDirection: "column", height: "100%" }}>
      {/* Stage header */}
      <div style={{
        padding: "18px 20px 16px",
        borderBottom: "1px solid var(--mv-line-2)",
      }}>
        <div style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 11,
          color: "var(--mv-blue-400)", letterSpacing: "0.16em",
          textTransform: "uppercase", marginBottom: 6,
        }}>Stage 01 / 04</div>
        <div style={{
          fontFamily: "var(--mv-font-display)", fontStyle: "italic",
          fontWeight: 800, fontSize: 28, lineHeight: 1, color: "var(--mv-fg-1)",
          letterSpacing: "-0.005em", marginBottom: 8,
        }}>
          Align<span style={{ color: "var(--mv-blue-500)" }}>ment</span>
        </div>
        <div style={{ fontSize: 13, color: "var(--mv-fg-2)", lineHeight: 1.5 }}>
          Position the target mesh to roughly align with the source mesh.
        </div>
      </div>

      {/* Body */}
      <div style={{ flex: 1, overflowY: "auto", padding: "16px 20px 24px" }}>

        <Section title="Source mesh" hint="The mesh you want to convert.">
          <FileSlot
            file={sourceFile}
            placeholder="No source mesh"
            onPick={onPickSource}
            primary
            actions={[]}
          />
        </Section>

        <Section title="Target mesh" hint="The MetaHuman head you'll wrap onto.">
          <FileSlot
            file={targetFile}
            placeholder="No target mesh"
            onPick={onPickTarget}
            actions={[
              <button key="default" onClick={onUseDefault} style={btnStyle("ghost")}>
                Use Default Head
              </button>,
            ]}
          />
        </Section>

        <Section title="Target textures">
          <div style={{ marginBottom: 8 }}>
            <Disclosure label="Albedo" count="1 file"/>
            <div style={{ display: "grid", gridTemplateColumns: "1fr auto", gap: 6, marginTop: 6 }}>
              <Select value="albedo_neutral.png" options={["albedo_neutral.png", "albedo_warm.png"]}/>
              <button style={btnStyle("ghost")}><IconPlus size={13}/> Texture</button>
            </div>
          </div>
        </Section>

        <Section title="Target transform">
          <TransformGroup
            label="Position"
            unit="cm"
            values={[transform.tx, transform.ty, transform.tz]}
            onChange={(i, v) => setTransform({ ...transform, [["tx","ty","tz"][i]]: v })}
          />
          <TransformGroup
            label="Rotation"
            unit="°"
            values={[transform.rx, transform.ry, transform.rz]}
            onChange={(i, v) => setTransform({ ...transform, [["rx","ry","rz"][i]]: v })}
          />
          <TransformGroup
            label="Scale"
            unit="×"
            values={[transform.scale, transform.scale, transform.scale]}
            uniform
            onChange={(i, v) => setTransform({ ...transform, scale: v })}
          />

          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6, marginTop: 12 }}>
            <button onClick={onReset} style={btnStyle("ghost")}>
              <IconReset size={14}/> Reset
            </button>
            <button onClick={onAutoAlign} style={btnStyle("ghost")}>
              <IconWand size={14}/> Auto-align
            </button>
          </div>
        </Section>

        <Section title="Validation" compact>
          <ValidationRow ok label="Source loaded"      value="14,206 verts"/>
          <ValidationRow ok label="Target loaded"      value="MetaHuman v1.4"/>
          <ValidationRow ok label="Bounds intersect"   value="71% overlap"/>
          <ValidationRow warn label="Pivot offset"     value="rough · OK"/>
        </Section>
      </div>

      {/* Stage nav footer */}
      <div style={{
        padding: "14px 20px", borderTop: "1px solid var(--mv-line-2)",
        background: "var(--mv-bg-1)",
        display: "grid", gridTemplateColumns: "auto 1fr auto", gap: 10, alignItems: "center",
      }}>
        <button style={btnStyle("ghost")}><IconChevL size={14}/> Previous</button>
        <div style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 10,
          color: "var(--mv-fg-3)", textAlign: "center",
          letterSpacing: "0.12em",
        }}>STEP <span style={{ color: "var(--mv-fg-1)" }}>01</span> OF 04</div>
        <button style={btnStyle("primary")}>Continue <IconChevR size={14}/></button>
      </div>
    </div>
  );
}

/* ---------- inspector primitives ---------- */
function Section({ title, hint, children, compact }) {
  return (
    <div style={{ marginBottom: compact ? 16 : 24 }}>
      <div style={{
        fontFamily: "var(--mv-font-mono)", fontSize: 10,
        letterSpacing: "0.16em", textTransform: "uppercase",
        color: "var(--mv-fg-3)", marginBottom: hint ? 4 : 10,
        display: "flex", alignItems: "center", gap: 8,
      }}>
        <span style={{ width: 6, height: 6, background: "var(--mv-blue-500)", borderRadius: 1 }}/>
        {title}
      </div>
      {hint && <div style={{ fontSize: 12, color: "var(--mv-fg-3)", marginBottom: 10, lineHeight: 1.45 }}>{hint}</div>}
      {children}
    </div>
  );
}

function FileSlot({ file, placeholder, onPick, primary, actions }) {
  return (
    <div>
      <div style={{
        background: "var(--mv-bg-1)", border: "1px solid var(--mv-line-2)",
        borderRadius: 6, padding: "10px 12px",
        display: "flex", alignItems: "center", gap: 10,
        marginBottom: 6,
      }}>
        <div style={{
          width: 30, height: 30, borderRadius: 4,
          background: file ? "rgba(61,165,245,0.12)" : "var(--mv-bg-2)",
          color: file ? "var(--mv-blue-400)" : "var(--mv-fg-4)",
          display: "flex", alignItems: "center", justifyContent: "center",
          flexShrink: 0,
        }}>
          <IconFolder size={16}/>
        </div>
        <div style={{ flex: 1, minWidth: 0 }}>
          <div style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 12,
            color: file ? "var(--mv-fg-1)" : "var(--mv-fg-3)",
            overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
          }}>
            {file || placeholder}
          </div>
          {file && (
            <div style={{ fontFamily: "var(--mv-font-mono)", fontSize: 10, color: "var(--mv-fg-3)", letterSpacing: "0.06em", marginTop: 2 }}>
              .OBJ · 1.4 MB
            </div>
          )}
        </div>
      </div>
      <div style={{ display: "flex", gap: 6 }}>
        <button onClick={onPick} style={{ ...btnStyle(primary ? "primary" : "ghost"), flex: 1 }}>
          <IconImport size={14}/> Import
        </button>
        {actions}
      </div>
    </div>
  );
}

function TransformGroup({ label, unit, values, onChange, uniform }) {
  const axes = ["X", "Y", "Z"];
  const axisColors = ["#FF6B6B", "#3FD69E", "#3DA5F5"];
  return (
    <div style={{ marginBottom: 10 }}>
      <div style={{
        display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 6,
      }}>
        <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>{label}</span>
        <span style={{ fontFamily: "var(--mv-font-mono)", fontSize: 10, color: "var(--mv-fg-4)", letterSpacing: "0.06em" }}>
          {unit}{uniform ? " · uniform" : ""}
        </span>
      </div>
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 4 }}>
        {axes.map((ax, i) => (
          <NumInput
            key={ax}
            axis={ax}
            axisColor={axisColors[i]}
            value={values[i]}
            disabled={uniform && i > 0}
            onChange={(v) => onChange(i, v)}
          />
        ))}
      </div>
    </div>
  );
}

function NumInput({ axis, axisColor, value, onChange, disabled }) {
  const [drag, setDrag] = useState(null);
  const onPointerDown = (e) => {
    if (disabled) return;
    setDrag({ x: e.clientX, start: value });
    e.currentTarget.setPointerCapture(e.pointerId);
  };
  const onPointerMove = (e) => {
    if (!drag) return;
    const dx = e.clientX - drag.x;
    onChange(+(drag.start + dx * 0.5).toFixed(3));
  };
  const onPointerUp = (e) => setDrag(null);
  return (
    <div
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      onPointerUp={onPointerUp}
      style={{
        background: "var(--mv-bg-1)",
        border: "1px solid var(--mv-line-2)",
        borderRadius: 4,
        padding: "6px 8px",
        display: "flex", alignItems: "center", justifyContent: "space-between",
        cursor: disabled ? "default" : "ew-resize",
        opacity: disabled ? 0.5 : 1,
        userSelect: "none",
      }}>
      <span style={{ fontFamily: "var(--mv-font-mono)", fontSize: 9, color: axisColor, letterSpacing: "0.1em", fontWeight: 700 }}>{axis}</span>
      <input
        value={Number(value).toFixed(2)}
        readOnly
        style={{
          background: "transparent", border: 0, color: "var(--mv-fg-1)",
          fontFamily: "var(--mv-font-mono)", fontSize: 11,
          width: "100%", textAlign: "right", outline: "none",
          fontVariantNumeric: "tabular-nums",
        }}
      />
    </div>
  );
}

function ValidationRow({ ok, warn, label, value }) {
  const color = warn ? "var(--mv-warning)" : ok ? "var(--mv-success)" : "var(--mv-danger)";
  return (
    <div style={{
      display: "grid", gridTemplateColumns: "auto 1fr auto", gap: 8, alignItems: "center",
      padding: "5px 0", fontSize: 12,
    }}>
      <span style={{
        width: 14, height: 14, borderRadius: "50%",
        border: `1px solid ${color}`, color,
        display: "inline-flex", alignItems: "center", justifyContent: "center",
        fontSize: 10, fontFamily: "var(--mv-font-mono)",
      }}>{warn ? "!" : ok ? "✓" : "×"}</span>
      <span style={{ color: "var(--mv-fg-2)" }}>{label}</span>
      <span style={{ fontFamily: "var(--mv-font-mono)", fontSize: 11, color: "var(--mv-fg-1)", letterSpacing: "0.04em" }}>{value}</span>
    </div>
  );
}

function Disclosure({ label, count }) {
  return (
    <div style={{
      display: "flex", alignItems: "center", justifyContent: "space-between",
      padding: "6px 8px", borderRadius: 4,
      background: "var(--mv-bg-1)", border: "1px solid var(--mv-line-2)",
      cursor: "pointer",
    }}>
      <span style={{ display: "inline-flex", alignItems: "center", gap: 6, fontSize: 12, color: "var(--mv-fg-1)" }}>
        <IconChevD size={12}/>
        {label}
      </span>
      <span style={{ fontFamily: "var(--mv-font-mono)", fontSize: 10, color: "var(--mv-fg-3)", letterSpacing: "0.06em" }}>{count}</span>
    </div>
  );
}

function Select({ value, options }) {
  return (
    <div style={{
      background: "var(--mv-bg-1)", border: "1px solid var(--mv-line-2)",
      borderRadius: 4, padding: "6px 10px",
      display: "flex", alignItems: "center", justifyContent: "space-between",
      fontSize: 12, color: "var(--mv-fg-1)", cursor: "pointer",
      fontFamily: "var(--mv-font-mono)",
    }}>
      <span>{value}</span>
      <IconChevD size={12}/>
    </div>
  );
}

/* ---------- Buttons ---------- */
function btnStyle(kind = "ghost") {
  const base = {
    display: "inline-flex", alignItems: "center", justifyContent: "center",
    gap: 6, padding: "8px 12px", borderRadius: 4,
    fontFamily: "var(--mv-font-body)", fontSize: 13, fontWeight: 500,
    cursor: "pointer", border: "1px solid var(--mv-line-2)",
    transition: "background .15s, border-color .15s",
  };
  if (kind === "primary") return {
    ...base, background: "var(--mv-blue-500)", color: "#051322",
    border: "1px solid var(--mv-blue-300)", boxShadow: "var(--mv-glow-soft)",
    fontWeight: 600,
  };
  if (kind === "ghost") return {
    ...base, background: "var(--mv-bg-2)", color: "var(--mv-fg-1)",
  };
  return base;
}

/* ------------------------------------------------------------------ */
/*  STATUS BAR                                                        */
/* ------------------------------------------------------------------ */
function StatusBar() {
  const sep = <span style={{ color: "var(--mv-line-3)" }}>·</span>;
  return (
    <div style={{
      height: 28, background: "var(--mv-bg-0)",
      borderTop: "1px solid var(--mv-line-2)",
      display: "flex", alignItems: "center", padding: "0 14px", gap: 14,
      fontFamily: "var(--mv-font-mono)", fontSize: 11,
      color: "var(--mv-fg-3)", letterSpacing: "0.06em",
    }}>
      <span><span style={{ color: "var(--mv-fg-1)" }}>●</span> Ready</span>
      {sep}
      <span>Stage <span style={{ color: "var(--mv-blue-400)" }}>01 ALIGN</span></span>
      {sep}
      <span>Source <span style={{ color: "var(--mv-fg-1)" }}>goblin_bust.obj</span></span>
      {sep}
      <span>Target <span style={{ color: "var(--mv-fg-1)" }}>MetaHuman 1.4</span></span>
      <div style={{ flex: 1 }}/>
      <span>FPS <span style={{ color: "var(--mv-fg-1)" }}>120</span></span>
      {sep}
      <span>GPU <span style={{ color: "var(--mv-fg-1)" }}>RTX 4090</span> · 18%</span>
      {sep}
      <span>Live link <span style={{ color: "var(--mv-success)" }}>UE 5.4 ●</span></span>
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  TOOL RAIL (left)                                                  */
/* ------------------------------------------------------------------ */
function ToolRail({ active, setActive }) {
  const tools = [
    { id: "move",   I: IconMove,   k: "W" },
    { id: "rotate", I: IconRotate, k: "E" },
    { id: "scale",  I: IconScale,  k: "R" },
  ];
  return (
    <div style={{
      width: 56, background: "var(--mv-bg-2)",
      borderRight: "1px solid var(--mv-line-2)",
      display: "flex", flexDirection: "column", alignItems: "center",
      padding: "12px 0", gap: 4,
    }}>
      <div style={{
        fontFamily: "var(--mv-font-mono)", fontSize: 9,
        color: "var(--mv-fg-4)", letterSpacing: "0.18em",
        textTransform: "uppercase", marginBottom: 6,
      }}>Xform</div>
      {tools.map((t) => (
        <button key={t.id}
          onClick={() => setActive(t.id)}
          title={t.id + " (" + t.k + ")"}
          style={{
            width: 40, height: 40, position: "relative",
            display: "flex", alignItems: "center", justifyContent: "center",
            border: 0, borderRadius: 6, cursor: "pointer",
            background: active === t.id ? "var(--mv-blue-500)" : "transparent",
            color: active === t.id ? "#051322" : "var(--mv-fg-2)",
            boxShadow: active === t.id ? "var(--mv-glow-soft)" : "none",
            transition: "all .15s",
          }}
          onMouseEnter={(e) => active !== t.id && (e.currentTarget.style.background = "var(--mv-bg-3)")}
          onMouseLeave={(e) => active !== t.id && (e.currentTarget.style.background = "transparent")}
        >
          <t.I size={20}/>
          <span style={{
            position: "absolute", right: 2, bottom: 2,
            fontFamily: "var(--mv-font-mono)", fontSize: 8,
            color: active === t.id ? "#051322" : "var(--mv-fg-3)",
            background: active === t.id ? "transparent" : "var(--mv-bg-1)",
            padding: "1px 3px", borderRadius: 2,
            border: active === t.id ? 0 : "1px solid var(--mv-line-2)",
          }}>{t.k}</span>
        </button>
      ))}
      <div style={{ width: 24, height: 1, background: "var(--mv-line-2)", margin: "10px 0" }}/>
      <button title="Symmetry X" style={railBtn(false)}>
        <Icon size={20}><path d="M12 4v16M5 8l4 4-4 4M19 8l-4 4 4 4"/></Icon>
      </button>
      <button title="Pivot snap" style={railBtn(false)}>
        <Icon size={20}><circle cx="12" cy="12" r="3"/><path d="M12 4v4M12 16v4M4 12h4M16 12h4"/></Icon>
      </button>
      <div style={{ flex: 1 }}/>
      <button title="Help" style={railBtn(false)}>
        <IconHelp size={20}/>
      </button>
    </div>
  );
}
function railBtn(active) {
  return {
    width: 40, height: 40, border: 0, borderRadius: 6, cursor: "pointer",
    display: "flex", alignItems: "center", justifyContent: "center",
    background: active ? "var(--mv-blue-500)" : "transparent",
    color: active ? "#051322" : "var(--mv-fg-2)",
  };
}

/* ------------------------------------------------------------------ */
/*  APP                                                               */
/* ------------------------------------------------------------------ */
function App() {
  const [tweaks, setTweak] = window.useTweaks(TWEAK_DEFAULTS);
  const [stage, setStage] = useState(1);
  const [sourceFile, setSourceFile] = useState("goblin_bust.obj");
  const [targetFile, setTargetFile] = useState("MetaHuman_default.obj");
  const [activeTool, setActiveTool] = useState("move");
  const [welcomeOpen, setWelcomeOpen] = useState(true);
  const [loginState, setLoginState] = useState("idle"); // "idle" | "waiting" | null
  const [transform, setTransform] = useState({
    tx: 25.99, ty: 8.00, tz: 76.17,
    rx: 145.45, ry: -89.99, rz: 76.17,
    scale: 1.000,
  });
  const onReset = () => setTransform({ tx: 0, ty: 0, tz: 0, rx: 0, ry: 0, rz: 0, scale: 1 });
  const onAutoAlign = () => setTransform({ tx: 0, ty: 0, tz: 0, rx: 0, ry: -5, rz: 0, scale: 1.05 });

  return (
    <div style={{
      height: "100vh", width: "100vw",
      display: "grid", gridTemplateRows: "36px 32px 64px 1fr 28px",
      background: "var(--mv-bg-1)",
      color: "var(--mv-fg-1)",
      fontFamily: "var(--mv-font-body)",
    }}>
      <TitleBar stage={STAGES[stage - 1]} file={sourceFile}/>
      <MenuBar/>
      <StageStepper active={stage} onChange={setStage}/>

      {/* Work area */}
      <div style={{ overflow: "hidden", borderTop: "1px solid var(--mv-line-2)" }}>
        <StageRouter
          stage={stage}
          tweaks={tweaks}
          sourceFile={sourceFile}
          setSourceFile={setSourceFile}
          targetFile={targetFile}
          setTargetFile={setTargetFile}
          activeTool={activeTool}
          setActiveTool={setActiveTool}
          transform={transform}
          setTransform={setTransform}
          onReset={onReset}
          onAutoAlign={onAutoAlign}
        />
      </div>

      <StatusBar/>

      {/* Welcome modal — shown after login on app open */}
      {welcomeOpen && !tweaks.showLogin && window.WelcomeModal && (
        <window.WelcomeModal
          onClose={() => setWelcomeOpen(false)}
          onNew={() => setWelcomeOpen(false)}
          onOpen={() => setWelcomeOpen(false)}
          onLoadProject={() => setWelcomeOpen(false)}
        />
      )}

      {/* Login screen — first-launch authentication, sits on top of everything */}
      {tweaks.showLogin && window.LoginScreen && (
        <window.LoginScreen
          state={tweaks.loginState}
          onSignIn={() => setTweak("loginState", "waiting")}
          onCancel={() => setTweak("loginState", "idle")}
          onRetry={() => setTweak("loginState", "waiting")}
          onOffline={() => setTweak("showLogin", false)}
        />
      )}

      {/* Tweaks */}
      <window.TweaksPanel title="Tweaks">
        <window.TweakSection title="Stepper">
          <window.TweakRadio
            label="Layout"
            value={tweaks.stepperStyle}
            onChange={(v) => setTweak("stepperStyle", v)}
            options={[
              { label: "Top rail", value: "rail" },
              { label: "Compact",  value: "compact" },
            ]}
          />
        </window.TweakSection>
        <window.TweakSection title="Viewport">
          <window.TweakRadio
            label="Background"
            value={tweaks.viewportTexture}
            onChange={(v) => setTweak("viewportTexture", v)}
            options={[
              { label: "Blueprint",   value: "blueprint" },
              { label: "Fine grid",   value: "fine" },
              { label: "Plain",       value: "plain" },
            ]}
          />
          <window.TweakRadio
            label="Target tint"
            value={tweaks.targetTint}
            onChange={(v) => setTweak("targetTint", v)}
            options={[
              { label: "Blue",    value: "blue" },
              { label: "Cyan",    value: "cyan" },
              { label: "Neutral", value: "neutral" },
            ]}
          />
          <window.TweakToggle
            label="Hotkey hints in HUD"
            value={tweaks.showHotkeyHints}
            onChange={(v) => setTweak("showHotkeyHints", v)}
          />
        </window.TweakSection>
        <window.TweakSection title="First launch">
          <window.TweakToggle
            label="Show login screen"
            value={tweaks.showLogin}
            onChange={(v) => setTweak("showLogin", v)}
          />
          <window.TweakRadio
            label="Login state"
            value={tweaks.loginState}
            onChange={(v) => setTweak("loginState", v)}
            options={[
              { label: "Idle",    value: "idle" },
              { label: "Waiting", value: "waiting" },
            ]}
          />
        </window.TweakSection>
        <window.TweakSection title="Demo">
          <window.TweakButton onClick={() => setWelcomeOpen(true)}>Show welcome screen</window.TweakButton>
        </window.TweakSection>
      </window.TweaksPanel>
    </div>
  );
}

/* Expose to stage2.jsx and other shared scopes */
Object.assign(window, {
  CustomHeadSculpt, MetaHumanTarget,
  Section, btnStyle,
  IconWand, IconLayers, IconPlus, IconClose, IconChevL, IconChevR,
  IconChevD, IconReset, IconBrush, IconEye, IconEyeOff, IconCheck, IconLock,
  IconExport, IconFolder,
});

/* ------------------------------------------------------------------ */
/*  ROUTER                                                            */
/* ------------------------------------------------------------------ */
function StageRouter({ stage, ...rest }) {
  if (stage === 2 && window.Stage2View) {
    return <window.Stage2View {...rest}/>;
  }
  if (stage === 3 && window.Stage3View) {
    return <window.Stage3View {...rest}/>;
  }
  if (stage === 4 && window.Stage4View) {
    return <window.Stage4View {...rest}/>;
  }
  return <Stage1Body {...rest} stage={stage}/>;
}

function Stage1Body({ tweaks, sourceFile, setSourceFile, targetFile, setTargetFile,
                     activeTool, setActiveTool, transform, setTransform,
                     onReset, onAutoAlign }) {
  return (
    <div style={{
      display: "grid",
      gridTemplateColumns: "56px 1fr 360px",
      overflow: "hidden",
      height: "100%",
    }}>
      <ToolRail active={activeTool} setActive={setActiveTool}/>
      <Viewport
        transform={transform}
        hasTarget={!!targetFile}
        sourceLoaded={!!sourceFile}
        viewportTexture={tweaks.viewportTexture}
        targetTint={tweaks.targetTint}
      />
      <div style={{
        background: "var(--mv-bg-2)",
        borderLeft: "1px solid var(--mv-line-2)",
        overflow: "hidden",
      }}>
        <Stage1Inspector
          sourceFile={sourceFile}
          targetFile={targetFile}
          onPickSource={() => setSourceFile("goblin_bust.obj")}
          onPickTarget={() => setTargetFile("MetaHuman_default.obj")}
          onUseDefault={() => setTargetFile("MetaHuman_default.obj")}
          transform={transform}
          setTransform={setTransform}
          onReset={onReset}
          onAutoAlign={onAutoAlign}
        />
      </div>
    </div>
  );
}

window.StageRouter = StageRouter;

ReactDOM.createRoot(document.getElementById("root")).render(<App/>);
