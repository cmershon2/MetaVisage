/* global React */
const { useState, useRef, useEffect } = React;

/* Anatomical landmark presets — positions in SVG-space for each head */
const LANDMARK_PRESETS = [
  { id: 1,  name: "Right brow outer",  src: { x: -82, y: -106 }, tgt: { x: -68, y: -100 } },
  { id: 2,  name: "Right brow inner",  src: { x: -28, y: -118 }, tgt: { x: -16, y: -108 } },
  { id: 3,  name: "Left brow inner",   src: { x: 12,  y: -118 }, tgt: { x: 16,  y: -108 } },
  { id: 4,  name: "Left brow outer",   src: { x: 50,  y: -110 }, tgt: { x: 60,  y: -100 } },
  { id: 5,  name: "Right eye corner",  src: { x: -68, y: -82  }, tgt: { x: -52, y: -75  } },
  { id: 6,  name: "Right eye pupil",   src: { x: -48, y: -80  }, tgt: { x: -38, y: -75  } },
  { id: 7,  name: "Left eye pupil",    src: { x: 22,  y: -85  }, tgt: { x: 38,  y: -75  } },
  { id: 8,  name: "Left eye corner",   src: { x: 42,  y: -78  }, tgt: { x: 52,  y: -75  } },
  { id: 9,  name: "Nose bridge",       src: { x: -10, y: -55  }, tgt: { x: 0,   y: -50  } },
  { id: 10, name: "Nose tip",          src: { x: -8,  y: 30   }, tgt: { x: 0,   y: 12   } },
  { id: 11, name: "Right nostril",     src: { x: -25, y: 18   }, tgt: { x: -10, y: 8    } },
  { id: 12, name: "Left nostril",      src: { x: 14,  y: 18   }, tgt: { x: 10,  y: 8    } },
  { id: 13, name: "Mouth right",       src: { x: -45, y: 78   }, tgt: { x: -28, y: 58   } },
  { id: 14, name: "Mouth top",         src: { x: -10, y: 70   }, tgt: { x: 0,   y: 53   } },
  { id: 15, name: "Mouth left",        src: { x: 30,  y: 78   }, tgt: { x: 28,  y: 58   } },
  { id: 16, name: "Chin",              src: { x: -15, y: 165  }, tgt: { x: 0,   y: 145  } },
];

/* Color the points pleasingly so user can spot pairs */
function pointColor(idx) {
  const palette = ["#3DA5F5", "#7BC2FA", "#3FD69E", "#F2B547", "#F26A6A", "#C58CF7", "#5DCAE0"];
  return palette[idx % palette.length];
}

/* ------------------------------------------------------------------ */
/*  POINT VIEWPORT — single side                                      */
/* ------------------------------------------------------------------ */
function PointViewport({
  side, label, sublabel, points, activeId, onAddPoint, onSelect,
  HeadComponent, headProps, bgStyle, viewportTexture,
  pointSize, mirrorAxis, mirror,
}) {
  const [hovered, setHovered] = useState(null);
  const svgRef = useRef(null);
  const [drag, setDrag] = useState(null);
  const [view, setView] = useState({ rot: 0, ox: 0, oy: 0 });

  const onSvgClick = (e) => {
    // Translate browser coords → SVG world coords
    const rect = svgRef.current.getBoundingClientRect();
    const px = e.clientX - rect.left;
    const py = e.clientY - rect.top;
    // viewBox is "-260 -260 520 520"
    const wx = (px / rect.width) * 520 - 260 - view.ox;
    const wy = (py / rect.height) * 520 - 260 - view.oy;
    onAddPoint({ x: Math.round(wx), y: Math.round(wy) });
  };

  return (
    <div style={{
      flex: 1, position: "relative", overflow: "hidden", minWidth: 0,
      ...bgStyle,
      borderRight: side === "left" ? "1px solid var(--mv-line-2)" : 0,
    }}>
      {/* Local toolbar */}
      <div style={{
        position: "absolute", top: 0, left: 0, right: 0, height: 36,
        background: "rgba(11,22,34,0.7)", backdropFilter: "blur(10px)",
        borderBottom: "1px solid var(--mv-line-2)",
        display: "flex", alignItems: "center", padding: "0 12px", gap: 10,
        zIndex: 4, overflow: "hidden",
      }}>
        <span style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 10,
          color: side === "left" ? "var(--mv-warning)" : "var(--mv-blue-400)",
          letterSpacing: "0.16em", textTransform: "uppercase",
          padding: "3px 8px", borderRadius: 3, flexShrink: 0,
          border: `1px solid ${side === "left" ? "rgba(242,181,71,0.4)" : "rgba(61,165,245,0.4)"}`,
          background: side === "left" ? "rgba(242,181,71,0.06)" : "rgba(61,165,245,0.06)",
        }}>{side === "left" ? "Source" : "Target"}</span>
        <span style={{ fontSize: 13, fontWeight: 600, color: "var(--mv-fg-1)", whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis", minWidth: 0 }}>{label}</span>
        <div style={{ flex: 1 }}/>
        <span style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 10,
          color: "var(--mv-fg-2)", letterSpacing: "0.06em", whiteSpace: "nowrap", flexShrink: 0,
        }}>
          <span style={{ color: "var(--mv-blue-400)" }}>●</span> {points.length} pts
        </span>
      </div>

      {/* SVG canvas */}
      <svg ref={svgRef}
        viewBox="-260 -260 520 520"
        width="100%" height="100%"
        style={{ display: "block", cursor: "crosshair" }}
        onClick={onSvgClick}>

        {/* Floor shadow */}
        <ellipse cx="0" cy="220" rx="120" ry="14" fill="#000" opacity="0.4"/>

        {/* Head */}
        <g>
          <HeadComponent {...headProps}/>
        </g>

        {/* Symmetry guide */}
        {mirror && (
          <line
            x1={mirrorAxis === "y" ? "-260" : "0"}
            y1={mirrorAxis === "y" ? "0" : "-260"}
            x2={mirrorAxis === "y" ? "260" : "0"}
            y2={mirrorAxis === "y" ? "0" : "260"}
            stroke="var(--mv-blue-500)"
            strokeWidth="0.6"
            strokeDasharray="4 5"
            opacity="0.5"
            pointerEvents="none"
          />
        )}

        {/* Points */}
        {points.map((p, i) => {
          const isActive = p.id === activeId;
          const isHover = p.id === hovered;
          const c = pointColor(i);
          return (
            <g key={p.id}
               onMouseEnter={() => setHovered(p.id)}
               onMouseLeave={() => setHovered(null)}
               onClick={(e) => { e.stopPropagation(); onSelect(p.id); }}
               style={{ cursor: "pointer" }}>
              {(isActive || isHover) && (
                <circle cx={p.pos.x} cy={p.pos.y} r={pointSize * 1.8} fill={c} opacity="0.18"/>
              )}
              <circle cx={p.pos.x} cy={p.pos.y} r={pointSize}
                fill="#0B1622" stroke={c} strokeWidth={isActive ? 2.2 : 1.5}/>
              <circle cx={p.pos.x} cy={p.pos.y} r={pointSize * 0.45} fill={c}/>
              <text x={p.pos.x + pointSize + 4} y={p.pos.y + 3}
                fill={c} fontFamily="JetBrains Mono" fontSize="9"
                fontWeight="600" letterSpacing="0.04em">
                {String(p.id).padStart(2, "0")}
              </text>
            </g>
          );
        })}
      </svg>

      {/* Bottom-left mini gizmo */}
      <div style={{
        position: "absolute", left: 12, bottom: 12, zIndex: 3,
        width: 60, height: 60, opacity: 0.85,
      }}>
        <svg viewBox="0 0 100 100" width="100%" height="100%">
          <circle cx="50" cy="50" r="34" fill="rgba(11,22,34,0.6)" stroke="var(--mv-line-2)" strokeWidth="1"/>
          <line x1="50" y1="14" x2="50" y2="86" stroke="#3FD69E" strokeWidth="1" opacity="0.7"/>
          <line x1="14" y1="50" x2="86" y2="50" stroke="#FF6B6B" strokeWidth="1" opacity="0.7"/>
          <text x="86" y="53" fill="#FF6B6B" fontFamily="JetBrains Mono" fontSize="10" textAnchor="middle">X</text>
          <text x="50" y="14" fill="#3FD69E" fontFamily="JetBrains Mono" fontSize="10" textAnchor="middle">Y</text>
          <circle cx="50" cy="50" r="2" fill="#3DA5F5"/>
        </svg>
      </div>
    </div>
  );
}

/* ------------------------------------------------------------------ */
/*  POINT REFERENCE PAGE                                              */
/* ------------------------------------------------------------------ */
function Stage2View({ tweaks, sourceFile, targetFile }) {
  // Pre-seed a few placed points to show off the design
  const [points, setPoints] = useState([
    { id: 1, name: "Right brow",       src: { x: -82, y: -106 }, tgt: { x: -52, y: -100 }, status: "matched" },
    { id: 2, name: "Left brow",        src: { x: 50,  y: -110 }, tgt: { x: 50,  y: -100 }, status: "matched" },
    { id: 3, name: "Right eye",        src: { x: -50, y: -80  }, tgt: { x: -38, y: -75  }, status: "matched" },
    { id: 4, name: "Left eye",         src: { x: 22,  y: -85  }, tgt: { x: 38,  y: -75  }, status: "matched" },
    { id: 5, name: "Nose tip",         src: { x: -8,  y: 28   }, tgt: { x: 0,   y: 12   }, status: "matched" },
    { id: 6, name: "Mouth right",      src: { x: -45, y: 78   }, tgt: null,                 status: "source-only" },
    { id: 7, name: "Mouth left",       src: { x: 30,  y: 78   }, tgt: null,                 status: "source-only" },
  ]);
  const [activeId, setActiveId] = useState(6);
  const [pointSize, setPointSize] = useState(6);
  const [mirror, setMirror] = useState(true);
  const [mirrorAxis, setMirrorAxis] = useState("x");

  const matched = points.filter((p) => p.status === "matched").length;
  const sourceOnly = points.filter((p) => p.status === "source-only").length;
  const targetOnly = points.filter((p) => p.status === "target-only").length;

  // Sync click-add: clicking the source side either adds a new pair (activates it)
  // or fills the source slot of the active point if it's missing.
  const addOnSide = (side, pos) => {
    setPoints((prev) => {
      const active = prev.find((p) => p.id === activeId);
      if (active && ((side === "src" && !active.src) || (side === "tgt" && !active.tgt))) {
        return prev.map((p) => p.id === activeId
          ? { ...p,
              [side]: pos,
              status: (side === "src" ? p.tgt : p.src) ? "matched"
                    : side === "src" ? "source-only" : "target-only" }
          : p);
      }
      const nextId = Math.max(0, ...prev.map((p) => p.id)) + 1;
      const newP = { id: nextId, name: `Point ${nextId}`,
        src: side === "src" ? pos : null,
        tgt: side === "tgt" ? pos : null,
        status: side === "src" ? "source-only" : "target-only" };
      setActiveId(nextId);
      return [...prev, newP];
    });
  };

  const removePoint = (id) => {
    setPoints((prev) => prev.filter((p) => p.id !== id));
  };

  const clearAll = () => setPoints([]);

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
    <div style={{ display: "grid", gridTemplateColumns: "1fr 360px", height: "100%", overflow: "hidden" }}>
      {/* Dual viewport */}
      <div style={{ display: "flex", height: "100%", overflow: "hidden" }}>
        <PointViewport
          side="left"
          label={sourceFile}
          sublabel="14,206 verts · OBJ"
          bgStyle={bgStyle}
          viewportTexture={tweaks.viewportTexture}
          HeadComponent={window.CustomHeadSculpt}
          headProps={{}}
          points={points.filter((p) => p.src).map((p) => ({ ...p, pos: p.src }))}
          activeId={activeId}
          onSelect={setActiveId}
          onAddPoint={(pos) => addOnSide("src", pos)}
          pointSize={pointSize}
          mirror={mirror}
          mirrorAxis={mirrorAxis}
        />
        <PointViewport
          side="right"
          label="MetaHuman_default.obj"
          sublabel="24,316 verts · MetaHuman 1.4"
          bgStyle={bgStyle}
          viewportTexture={tweaks.viewportTexture}
          HeadComponent={window.MetaHumanTarget}
          headProps={{ tint: tweaks.targetTint, showWire: false }}
          points={points.filter((p) => p.tgt).map((p) => ({ ...p, pos: p.tgt }))}
          activeId={activeId}
          onSelect={setActiveId}
          onAddPoint={(pos) => addOnSide("tgt", pos)}
          pointSize={pointSize}
          mirror={mirror}
          mirrorAxis={mirrorAxis}
        />
      </div>

      {/* Inspector */}
      <div style={{
        background: "var(--mv-bg-2)",
        borderLeft: "1px solid var(--mv-line-2)",
        display: "flex", flexDirection: "column", overflow: "hidden",
      }}>
        {/* Header */}
        <div style={{ padding: "18px 20px 16px", borderBottom: "1px solid var(--mv-line-2)" }}>
          <div style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 11,
            color: "var(--mv-blue-400)", letterSpacing: "0.16em",
            textTransform: "uppercase", marginBottom: 6,
          }}>Stage 02 / 04</div>
          <div style={{
            fontFamily: "var(--mv-font-display)", fontStyle: "italic",
            fontWeight: 800, fontSize: 28, lineHeight: 1, color: "var(--mv-fg-1)",
            letterSpacing: "-0.005em", marginBottom: 8,
          }}>
            Point <span style={{ color: "var(--mv-blue-500)" }}>reference</span>
          </div>
          <div style={{ fontSize: 13, color: "var(--mv-fg-2)", lineHeight: 1.5 }}>
            Place matching landmarks on each mesh. Pairs guide the morph deformation.
          </div>
        </div>

        {/* Body */}
        <div style={{ flex: 1, overflowY: "auto", padding: "16px 20px 24px" }}>

          {/* Stat trio */}
          <div style={{
            display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 8,
            marginBottom: 22,
          }}>
            <StatCard label="Paired" value={matched} accent="var(--mv-success)" big/>
            <StatCard label="Source only" value={sourceOnly} accent="var(--mv-warning)"/>
            <StatCard label="Target only" value={targetOnly} accent="var(--mv-warning)"/>
          </div>

          {/* Tools */}
          <window.Section title="Placement tools">
            <div style={{
              display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6,
              marginBottom: 8,
            }}>
              <button style={window.btnStyle("ghost")}>
                <window.IconWand size={14}/> Auto-detect
              </button>
              <button style={window.btnStyle("ghost")}>
                <window.IconLayers size={14}/> Load preset
              </button>
            </div>
            <ToggleRow
              label="Mirror across symmetry axis"
              value={mirror}
              onChange={setMirror}
            />
            <div style={{
              display: "grid", gridTemplateColumns: "100px 1fr", alignItems: "center",
              gap: 10, marginTop: 10,
            }}>
              <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>Symmetry axis</span>
              <Segmented value={mirrorAxis} onChange={setMirrorAxis}
                options={[{label:"X",value:"x"},{label:"Y",value:"y"},{label:"Z",value:"z"}]}/>
            </div>
            <div style={{
              display: "grid", gridTemplateColumns: "100px 1fr 36px", alignItems: "center",
              gap: 10, marginTop: 10,
            }}>
              <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>Point size</span>
              <SliderTrack value={pointSize} min={3} max={14} onChange={setPointSize}/>
              <span style={{ fontFamily: "var(--mv-font-mono)", fontSize: 11, color: "var(--mv-fg-1)", textAlign: "right" }}>{pointSize}px</span>
            </div>
          </window.Section>

          {/* Points list */}
          <window.Section
            title={`Points · ${points.length}`}
            actions={
              <button onClick={clearAll} style={{
                background: "transparent", border: 0,
                color: "var(--mv-danger)", cursor: "pointer",
                fontFamily: "var(--mv-font-mono)", fontSize: 10,
                letterSpacing: "0.08em", padding: 0,
              }}>CLEAR ALL</button>
            }>
            <div style={{ display: "flex", flexDirection: "column", gap: 3 }}>
              {points.map((p, i) => (
                <PointRow
                  key={p.id}
                  point={p}
                  index={i}
                  active={activeId === p.id}
                  onClick={() => setActiveId(p.id)}
                  onRemove={() => removePoint(p.id)}
                />
              ))}
              <button onClick={() => {
                const nextId = Math.max(0, ...points.map((p) => p.id)) + 1;
                setPoints([...points, { id: nextId, name: `Point ${nextId}`, src: null, tgt: null, status: "source-only" }]);
                setActiveId(nextId);
              }} style={{
                marginTop: 6, ...window.btnStyle("ghost"), justifyContent: "flex-start",
                color: "var(--mv-fg-3)", borderStyle: "dashed",
              }}>
                <window.IconPlus size={13}/> Add point
              </button>
            </div>
          </window.Section>

          <window.Section title="Tips" compact>
            <div style={{ fontSize: 12, color: "var(--mv-fg-2)", lineHeight: 1.55 }}>
              Click in either viewport to drop a point. Click a point in the list to fill its missing pair on the other mesh.
            </div>
            <div style={{ marginTop: 8, fontSize: 12, color: "var(--mv-fg-3)", lineHeight: 1.55 }}>
              At least <span style={{ color: "var(--mv-fg-1)", fontFamily: "var(--mv-font-mono)" }}>5</span> matched pairs recommended before continuing.
            </div>
          </window.Section>
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
          }}>STEP <span style={{ color: "var(--mv-fg-1)" }}>02</span> OF 04</div>
          <button style={window.btnStyle("primary")}>Continue <window.IconChevR size={14}/></button>
        </div>
      </div>
    </div>
  );
}

/* ---------- helpers ---------- */
function StatCard({ label, value, accent, big }) {
  return (
    <div style={{
      background: "var(--mv-bg-1)", border: "1px solid var(--mv-line-2)",
      borderRadius: 6, padding: "10px 12px",
      borderTop: `2px solid ${accent}`,
    }}>
      <div style={{
        fontFamily: "var(--mv-font-mono)", fontSize: 9,
        letterSpacing: "0.14em", textTransform: "uppercase",
        color: "var(--mv-fg-3)", marginBottom: 4,
      }}>{label}</div>
      <div style={{
        fontFamily: "var(--mv-font-mono)", fontSize: big ? 22 : 18,
        color: accent, fontWeight: 600,
        fontVariantNumeric: "tabular-nums",
      }}>{String(value).padStart(2, "0")}</div>
    </div>
  );
}

function ToggleRow({ label, value, onChange }) {
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
          background: "#E8F1FB",
          transition: "left .15s",
        }}/>
      </div>
    </button>
  );
}

function Segmented({ value, onChange, options }) {
  return (
    <div style={{
      display: "flex", background: "var(--mv-bg-1)",
      border: "1px solid var(--mv-line-2)", borderRadius: 4, padding: 2,
    }}>
      {options.map((o) => (
        <button key={o.value} onClick={() => onChange(o.value)} style={{
          flex: 1, padding: "5px 0", background: value === o.value ? "var(--mv-bg-3)" : "transparent",
          border: 0, borderRadius: 3,
          fontFamily: "var(--mv-font-mono)", fontSize: 11,
          color: value === o.value ? "var(--mv-fg-1)" : "var(--mv-fg-3)",
          letterSpacing: "0.08em", cursor: "pointer", fontWeight: 500,
        }}>{o.label}</button>
      ))}
    </div>
  );
}

function SliderTrack({ value, min, max, onChange }) {
  const pct = ((value - min) / (max - min)) * 100;
  const ref = useRef(null);
  const [drag, setDrag] = useState(false);
  const update = (e) => {
    const r = ref.current.getBoundingClientRect();
    const p = Math.max(0, Math.min(1, (e.clientX - r.left) / r.width));
    onChange(Math.round(min + p * (max - min)));
  };
  return (
    <div ref={ref}
      onPointerDown={(e) => { setDrag(true); update(e); e.currentTarget.setPointerCapture(e.pointerId); }}
      onPointerMove={(e) => drag && update(e)}
      onPointerUp={() => setDrag(false)}
      style={{ height: 18, position: "relative", cursor: "pointer", display: "flex", alignItems: "center" }}>
      <div style={{ position: "absolute", left: 0, right: 0, height: 4, background: "var(--mv-bg-1)", borderRadius: 999 }}>
        <div style={{ position: "absolute", left: 0, top: 0, bottom: 0, width: pct + "%",
          background: "linear-gradient(90deg, var(--mv-blue-600), var(--mv-blue-400))", borderRadius: 999 }}/>
      </div>
      <div style={{
        position: "absolute", left: pct + "%", transform: "translateX(-50%)",
        width: 12, height: 12, borderRadius: "50%",
        background: "var(--mv-fg-1)", border: "2px solid var(--mv-blue-500)",
      }}/>
    </div>
  );
}

function PointRow({ point, index, active, onClick, onRemove }) {
  const c = pointColor(index);
  const status = point.status;
  const statusInfo = {
    "matched":     { label: "PAIRED",  color: "var(--mv-success)" },
    "source-only": { label: "NEEDS B", color: "var(--mv-warning)" },
    "target-only": { label: "NEEDS A", color: "var(--mv-warning)" },
  }[status];

  return (
    <div onClick={onClick} style={{
      display: "grid", gridTemplateColumns: "auto 1fr auto auto", gap: 8, alignItems: "center",
      padding: "7px 9px", borderRadius: 4,
      background: active ? "var(--mv-bg-4)" : "transparent",
      border: `1px solid ${active ? "var(--mv-blue-500)" : "transparent"}`,
      cursor: "pointer",
      transition: "background .12s",
    }}
    onMouseEnter={(e) => !active && (e.currentTarget.style.background = "var(--mv-bg-3)")}
    onMouseLeave={(e) => !active && (e.currentTarget.style.background = "transparent")}>
      <div style={{
        width: 22, height: 22, borderRadius: 4,
        background: "var(--mv-bg-1)", border: `1.5px solid ${c}`,
        display: "flex", alignItems: "center", justifyContent: "center",
        fontFamily: "var(--mv-font-mono)", fontSize: 10, fontWeight: 600,
        color: c, flexShrink: 0,
      }}>{String(point.id).padStart(2, "0")}</div>

      <div style={{ minWidth: 0 }}>
        <div style={{
          fontSize: 12.5, color: "var(--mv-fg-1)",
          overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
        }}>{point.name}</div>
        <div style={{
          fontFamily: "var(--mv-font-mono)", fontSize: 9,
          color: statusInfo.color, letterSpacing: "0.1em", marginTop: 1,
        }}>{statusInfo.label}</div>
      </div>

      {/* Pair indicator */}
      <div style={{ display: "flex", gap: 3 }}>
        <div style={{
          width: 8, height: 8, borderRadius: 1,
          background: point.src ? c : "transparent",
          border: `1px solid ${c}`,
          opacity: point.src ? 1 : 0.4,
        }}/>
        <div style={{
          width: 8, height: 8, borderRadius: 1,
          background: point.tgt ? c : "transparent",
          border: `1px solid ${c}`,
          opacity: point.tgt ? 1 : 0.4,
        }}/>
      </div>

      <button onClick={(e) => { e.stopPropagation(); onRemove(); }}
        style={{
          background: "transparent", border: 0,
          color: "var(--mv-fg-4)", cursor: "pointer",
          width: 18, height: 18, padding: 0, borderRadius: 3,
          display: "flex", alignItems: "center", justifyContent: "center",
        }}
        onMouseEnter={(e) => { e.currentTarget.style.color = "var(--mv-danger)"; e.currentTarget.style.background = "var(--mv-bg-3)"; }}
        onMouseLeave={(e) => { e.currentTarget.style.color = "var(--mv-fg-4)"; e.currentTarget.style.background = "transparent"; }}>
        <window.IconClose size={12}/>
      </button>
    </div>
  );
}

window.Stage2View = Stage2View;
