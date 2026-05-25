/* global React */
const { useState, useRef, useEffect } = React;

/* ------------------------------------------------------------------ */
/*  PRESETS                                                           */
/* ------------------------------------------------------------------ */
const PRESETS = [
  {
    id: "quick",
    name: "Quick Wrap",
    sub: "Tight wrap for human-like meshes",
    icon: "human",
    params: {
      stiffnessSteps: 5,
      stiffnessStart: 100,
      stiffnessEnd: 70.4,
      iterations: 7,
      normalThreshold: 89,
      jacobianWeight: 10,
      controlDensityStart: 5.13,
      controlDensityEnd: 2.0,
      normalizeSampling: true,
      rigidityStart: 0,
      rigidityEnd: 0,
      stepDamping: 1.0,
      innerIterations: 1,
      convergenceThreshold: 0.0018,
      excludeBoundaries: true,
      boundaryHops: 3,
    },
  },
  {
    id: "fantasy",
    name: "Fantasy Wrap",
    sub: "Looser fit for dramatic features",
    icon: "fantasy",
    params: {
      stiffnessSteps: 8,
      stiffnessStart: 65,
      stiffnessEnd: 35,
      iterations: 12,
      normalThreshold: 65,
      jacobianWeight: 4,
      controlDensityStart: 3.2,
      controlDensityEnd: 1.4,
      normalizeSampling: true,
      rigidityStart: 0.2,
      rigidityEnd: 0.05,
      stepDamping: 0.85,
      innerIterations: 2,
      convergenceThreshold: 0.0012,
      excludeBoundaries: true,
      boundaryHops: 4,
    },
  },
  {
    id: "custom",
    name: "Custom",
    sub: "Your saved configuration",
    icon: "custom",
    params: null, // placeholder
  },
];

const PARAM_GROUPS = {
  basic: [
    { k: "stiffnessSteps",     label: "Stiffness steps",  unit: "steps", min: 1,  max: 20,  step: 1 },
    { k: "stiffnessStart",     label: "Stiffness (start)", unit: "",     min: 0,  max: 200, step: 0.1 },
    { k: "stiffnessEnd",       label: "Stiffness (end)",   unit: "",     min: 0,  max: 200, step: 0.1 },
    { k: "iterations",         label: "Iterations / step", unit: "",     min: 1,  max: 30,  step: 1 },
    { k: "normalThreshold",    label: "Normal threshold",  unit: "°",    min: 0,  max: 180, step: 1 },
    { k: "jacobianWeight",     label: "Jacobian weight",   unit: "",     min: 0,  max: 30,  step: 0.1 },
  ],
  advanced: [
    { k: "controlDensityStart", label: "Control density (start)", unit: "", min: 0, max: 10, step: 0.01 },
    { k: "controlDensityEnd",   label: "Control density (end)",   unit: "", min: 0, max: 10, step: 0.01 },
    { k: "rigidityStart",       label: "Rigidity (start)",        unit: "", min: 0, max: 1,  step: 0.01 },
    { k: "rigidityEnd",         label: "Rigidity (end)",          unit: "", min: 0, max: 1,  step: 0.01 },
    { k: "stepDamping",         label: "Step damping (start)",    unit: "", min: 0, max: 2,  step: 0.01 },
    { k: "stepDampingEnd",      label: "Step damping (end)",      unit: "", min: 0, max: 2,  step: 0.01, fallback: "stepDamping" },
    { k: "innerIterations",     label: "Inner iterations",        unit: "", min: 1, max: 10, step: 1 },
    { k: "convergenceThreshold",label: "Convergence threshold",   unit: "", min: 0, max: 0.01, step: 0.0001, precision: 4 },
    { k: "boundaryHops",        label: "Boundary hops",           unit: "", min: 0, max: 10, step: 1 },
  ],
};

/* ------------------------------------------------------------------ */
/*  MORPHED HEAD — goblin features on metahuman body, blue tint       */
/* ------------------------------------------------------------------ */
function MorphedHead({ tint = "blue", strength = 1, showWire = true, rot = 0, deformed = true }) {
  const colors = {
    blue:    ["#7BC2FA", "#3DA5F5", "#1572B8"],
    cyan:    ["#A8E5FA", "#5DCAE0", "#1A7A95"],
    neutral: ["#A8BBD0", "#6F839A", "#2F4358"],
  }[tint] || ["#7BC2FA", "#3DA5F5", "#1572B8"];

  // strength dictates how goblin-y the result is
  const s = strength;
  const browDrop = 8 * s;
  const earOut = 25 * s;

  return (
    <g transform={`rotate(${rot * 0.4})`}>
      <defs>
        <linearGradient id="morphfill" x1="0.3" y1="0" x2="0.7" y2="1">
          <stop offset="0%" stopColor={colors[0]}/>
          <stop offset="55%" stopColor={colors[1]}/>
          <stop offset="100%" stopColor={colors[2]}/>
        </linearGradient>
        <radialGradient id="morphrim" cx="0.3" cy="0.3" r="0.7">
          <stop offset="0%" stopColor="#E8F1FB" stopOpacity="0.55"/>
          <stop offset="60%" stopColor={colors[1]} stopOpacity="0"/>
        </radialGradient>
      </defs>

      {/* Floor shadow */}
      <ellipse cx="0" cy="240" rx="130" ry="14" fill="#000" opacity="0.4"/>

      {/* Body — head with morphed silhouette: pulled-back ears, sunken cheeks */}
      <path d={`M -100,-40
               C -110,-130 -60,${-180 - 5*s} 0,${-180 - 5*s}
               C 60,${-180 - 5*s} 110,-130 100,-40
               C 95,${20 - 4*s} ${80 + 4*s},80 60,130
               C 90,150 100,180 90,210
               C 30,235 -30,235 -90,210
               C -100,180 -90,150 -60,130
               C ${-80 - 4*s},80 ${-95 - 4*s},20 -100,-40 Z`}
            fill="url(#morphfill)"/>
      <path d="M -100,-40 C -110,-130 -60,-180 0,-180 C 60,-180 110,-130 100,-40 Z"
            fill="url(#morphrim)"/>

      {/* Ear — pulled outward by morph */}
      <path d={`M -100,-90 Q ${-130 - earOut},-100 ${-145 - earOut * 1.2},-50 Q ${-135 - earOut},-25 -110,-30 Z`}
            fill={colors[1]} opacity="0.85"/>
      <path d={`M 100,-90 Q ${130 + earOut},-100 ${145 + earOut * 1.2},-50 Q ${135 + earOut},-25 110,-30 Z`}
            fill={colors[1]} opacity="0.85"/>

      {/* Brow — heavy, dropped */}
      <path d={`M -65,${-100 + browDrop} Q -38,${-115 + browDrop} -16,${-100 + browDrop}`}
            stroke="#08151F" strokeWidth="3" fill="none" opacity="0.5"/>
      <path d={`M 16,${-100 + browDrop} Q 38,${-115 + browDrop} 65,${-100 + browDrop}`}
            stroke="#08151F" strokeWidth="3" fill="none" opacity="0.5"/>
      {/* Brow ridge shading */}
      <path d={`M -75,${-95 + browDrop} Q -38,${-108 + browDrop} 0,${-100 + browDrop} Q 38,${-108 + browDrop} 75,${-95 + browDrop} L 75,${-80 + browDrop} L -75,${-80 + browDrop} Z`}
            fill="#08151F" opacity="0.18"/>

      {/* Sunken eyes */}
      <ellipse cx="-38" cy={-72 + browDrop * 0.5} rx="14" ry="7" fill="#08151F" opacity="0.85"/>
      <ellipse cx="38" cy={-72 + browDrop * 0.5} rx="14" ry="7" fill="#08151F" opacity="0.85"/>
      <circle cx="-38" cy={-72 + browDrop * 0.5} r="4" fill="#1B2A3A"/>
      <circle cx="38" cy={-72 + browDrop * 0.5} r="4" fill="#1B2A3A"/>

      {/* Long crooked nose */}
      <path d={`M 0,${-55 + browDrop * 0.5} Q -8,-15 -12,15 Q ${-12 - 4*s},${30 + 8*s} 0,${35 + 10*s} Q ${12 + 4*s},${30 + 8*s} 12,15 Q 8,-15 0,${-55 + browDrop * 0.5} Z`}
            fill={colors[2]} opacity="0.55"/>

      {/* Mouth — downturned */}
      <path d={`M -32,${55 + 6*s} Q 0,${52 + 6*s} 32,${55 + 6*s} Q 28,${65 + 8*s} 0,${68 + 8*s} Q -28,${65 + 8*s} -32,${55 + 6*s} Z`}
            fill="#08151F" opacity="0.6"/>
      <path d={`M -32,${55 + 6*s} Q 0,${52 + 6*s} 32,${55 + 6*s}`} stroke="#08151F" strokeWidth="1.2" fill="none" opacity="0.7"/>

      {/* Cheek shading */}
      <ellipse cx="-58" cy="0" rx="22" ry="14" fill="#08151F" opacity="0.18"/>
      <ellipse cx="58" cy="0" rx="22" ry="14" fill="#08151F" opacity="0.18"/>

      {/* Wireframe overlay — denser since this is the morphed metahuman topology */}
      {showWire && deformed && (
        <g stroke={colors[0]} fill="none" strokeWidth="0.5" opacity="0.5">
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
          {/* Diagonals for density */}
          <path d="M -75,-150 Q 0,-100 75,-50" opacity="0.4"/>
          <path d="M 75,-150 Q 0,-100 -75,-50" opacity="0.4"/>
          <path d="M -75,-50 Q 0,0 75,50" opacity="0.4"/>
          <path d="M 75,-50 Q 0,0 -75,50" opacity="0.4"/>
        </g>
      )}
    </g>
  );
}

/* ------------------------------------------------------------------ */
/*  STAGE 3 VIEW                                                      */
/* ------------------------------------------------------------------ */
function Stage3View({ tweaks }) {
  const [presetId, setPresetId] = useState("quick");
  const [params, setParams] = useState(() => ({ ...PRESETS[0].params }));
  const [advancedOpen, setAdvancedOpen] = useState(true);
  const [previewMode, setPreviewMode] = useState("deformed");
  const [processing, setProcessing] = useState(false);
  const [hasResult, setHasResult] = useState(true);
  const [maskMode, setMaskMode] = useState(false);
  const [maskRadius, setMaskRadius] = useState(10);
  const [showMask, setShowMask] = useState(true);

  const switchPreset = (id) => {
    setPresetId(id);
    const p = PRESETS.find((x) => x.id === id);
    if (p && p.params) setParams({ ...p.params });
  };

  const updateParam = (k, v) => setParams((prev) => ({ ...prev, [k]: v }));
  const resetDefaults = () => {
    const p = PRESETS.find((x) => x.id === presetId);
    if (p && p.params) setParams({ ...p.params });
  };

  const onProcess = () => {
    setProcessing(true);
    setTimeout(() => { setProcessing(false); setHasResult(true); }, 1200);
  };

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
    <div style={{ display: "grid", gridTemplateColumns: "1fr 380px", height: "100%", overflow: "hidden" }}>
      {/* Viewport */}
      <div style={{ position: "relative", overflow: "hidden", ...bgStyle }}>
        {/* Viewport top bar */}
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
          <div style={{ width: 1, height: 18, background: "var(--mv-line-2)", margin: "0 3px" }}/>
          <ViewModeBtn active={previewMode === "source"} onClick={() => setPreviewMode("source")}>Source</ViewModeBtn>
          <ViewModeBtn active={previewMode === "target"} onClick={() => setPreviewMode("target")}>Target</ViewModeBtn>
          <ViewModeBtn active={previewMode === "deformed"} onClick={() => setPreviewMode("deformed")}>Deformed</ViewModeBtn>
          <ViewModeBtn active={previewMode === "overlay"} onClick={() => setPreviewMode("overlay")}>Overlay</ViewModeBtn>
        </div>

        {/* Top-right pills */}
        <div style={{ position: "absolute", top: 14, right: 14, display: "flex", gap: 6, zIndex: 5 }}>
          <Pill><span style={{ color: "var(--mv-blue-400)" }}>●</span> {processing ? "WORKING" : hasResult ? "READY" : "IDLE"}</Pill>
          <Pill>FPS <b style={{ color: "var(--mv-fg-1)" }}>120</b></Pill>
        </div>

        {/* Subject */}
        <div style={{ position: "absolute", inset: 0, display: "flex", alignItems: "center", justifyContent: "center" }}>
          <svg viewBox="-300 -280 600 560" width="100%" height="100%" preserveAspectRatio="xMidYMid meet">
            {previewMode === "source" && (
              <g transform="scale(0.85)">
                <window.CustomHeadSculpt/>
              </g>
            )}
            {previewMode === "target" && (
              <g transform="scale(0.95)">
                <window.MetaHumanTarget tint={tweaks.targetTint} showWire={true} rot={0}/>
              </g>
            )}
            {previewMode === "deformed" && (
              <MorphedHead tint={tweaks.targetTint} strength={presetId === "fantasy" ? 1 : 0.5} showWire={true} deformed/>
            )}
            {previewMode === "overlay" && (
              <>
                <g opacity="0.4">
                  <window.MetaHumanTarget tint="neutral" showWire={true}/>
                </g>
                <MorphedHead tint={tweaks.targetTint} strength={presetId === "fantasy" ? 1 : 0.5} showWire={true} deformed/>
              </>
            )}

            {/* Vertex mask preview as red translucent dots */}
            {maskMode && showMask && (
              <g pointerEvents="none">
                {/* Sample mask dots on cheeks */}
                {[
                  [-58, 0],[-72,8],[-50,16],[-65,-12],
                  [58,0],[72,8],[50,16],[65,-12],
                  [-32,55],[32,55],[0,68],
                ].map(([x,y], i) => (
                  <circle key={i} cx={x} cy={y} r="6" fill="#F26A6A" opacity="0.45"/>
                ))}
              </g>
            )}
          </svg>
        </div>

        {/* Processing overlay */}
        {processing && (
          <div style={{
            position: "absolute", inset: 0, display: "flex",
            alignItems: "center", justifyContent: "center",
            background: "rgba(7,16,28,0.6)", backdropFilter: "blur(2px)",
            zIndex: 6,
          }}>
            <div style={{
              padding: "20px 28px", borderRadius: 8,
              background: "var(--mv-bg-2)", border: "1px solid var(--mv-line-3)",
              boxShadow: "var(--mv-shadow-3)", textAlign: "center", minWidth: 260,
            }}>
              <div style={{ fontFamily: "var(--mv-font-mono)", fontSize: 10, color: "var(--mv-blue-400)", letterSpacing: "0.16em", textTransform: "uppercase", marginBottom: 8 }}>Processing morph</div>
              <div style={{ fontSize: 14, color: "var(--mv-fg-1)", marginBottom: 12 }}>Stiffness pass 3 of {params.stiffnessSteps}…</div>
              <div style={{ height: 4, background: "var(--mv-bg-1)", borderRadius: 999, overflow: "hidden" }}>
                <div style={{ width: "42%", height: "100%", background: "linear-gradient(90deg, var(--mv-blue-600), var(--mv-blue-400))", borderRadius: "inherit" }}/>
              </div>
            </div>
          </div>
        )}

        {/* Info HUD */}
        {hasResult && !processing && (
          <div style={{
            position: "absolute", left: 16, bottom: 16, zIndex: 5, minWidth: 260,
            background: "rgba(11,22,34,0.88)", backdropFilter: "blur(10px)",
            border: "1px solid var(--mv-line-2)", borderRadius: 6,
            padding: "12px 14px",
          }}>
            <div style={{
              fontFamily: "var(--mv-font-mono)", fontSize: 10,
              letterSpacing: "0.16em", textTransform: "uppercase",
              color: "var(--mv-fg-3)", marginBottom: 8,
              display: "flex", justifyContent: "space-between",
            }}>
              <span>/ Result</span>
              <span style={{ color: "var(--mv-success)" }}>● CONVERGED</span>
            </div>
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: "6px 16px", fontSize: 12 }}>
              <span style={{ color: "var(--mv-fg-3)" }}>Max disp.</span>
              <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-fg-1)", textAlign: "right" }}>7.062 mm</span>
              <span style={{ color: "var(--mv-fg-3)" }}>Avg disp.</span>
              <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-fg-1)", textAlign: "right" }}>1.176 mm</span>
              <span style={{ color: "var(--mv-fg-3)" }}>Iterations</span>
              <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-fg-1)", textAlign: "right" }}>{params.stiffnessSteps * params.iterations}</span>
              <span style={{ color: "var(--mv-fg-3)" }}>Elapsed</span>
              <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-fg-1)", textAlign: "right" }}>2.4 s</span>
            </div>
          </div>
        )}

        {/* Gizmo */}
        <div style={{ position: "absolute", right: 16, bottom: 16, zIndex: 5, width: 88, height: 88 }}>
          <svg viewBox="0 0 100 100" width="100%" height="100%">
            <circle cx="50" cy="50" r="44" fill="rgba(11,22,34,0.6)" stroke="var(--mv-line-2)" strokeWidth="1"/>
            <circle cx="50" cy="50" r="32" fill="none" stroke="var(--mv-blue-500)" strokeWidth="0.8" opacity="0.4"/>
            <line x1="50" y1="6" x2="50" y2="94" stroke="var(--mv-blue-500)" strokeWidth="0.5" opacity="0.4"/>
            <line x1="6" y1="50" x2="94" y2="50" stroke="var(--mv-blue-500)" strokeWidth="0.5" opacity="0.4"/>
            <text x="50" y="14" fill="#7BC2FA" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">Y</text>
            <text x="92" y="53" fill="#7BC2FA" fontFamily="JetBrains Mono" fontSize="9" textAnchor="middle">X</text>
            <circle cx="50" cy="50" r="3" fill="#3DA5F5"/>
          </svg>
        </div>
      </div>

      {/* INSPECTOR */}
      <div style={{
        background: "var(--mv-bg-2)",
        borderLeft: "1px solid var(--mv-line-2)",
        display: "flex", flexDirection: "column", overflow: "hidden",
        minWidth: 0,
      }}>
        {/* Header */}
        <div style={{ padding: "18px 20px 16px", borderBottom: "1px solid var(--mv-line-2)" }}>
          <div style={{
            fontFamily: "var(--mv-font-mono)", fontSize: 11,
            color: "var(--mv-blue-400)", letterSpacing: "0.16em",
            textTransform: "uppercase", marginBottom: 6,
          }}>Stage 03 / 04</div>
          <div style={{
            fontFamily: "var(--mv-font-display)", fontStyle: "italic",
            fontWeight: 800, fontSize: 28, lineHeight: 1, color: "var(--mv-fg-1)",
            letterSpacing: "-0.005em", marginBottom: 8,
          }}>
            Morph<span style={{ color: "var(--mv-blue-500)" }}>ing</span>
          </div>
          <div style={{ fontSize: 13, color: "var(--mv-fg-2)", lineHeight: 1.5 }}>
            Tune parameters and process. Review before accepting.
          </div>
        </div>

        {/* Body */}
        <div style={{ flex: 1, overflowY: "auto", overflowX: "hidden", padding: "16px 20px 24px" }}>

          {/* Preset switcher */}
          <window.Section title="Preset">
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6 }}>
              {PRESETS.slice(0, 2).map((p) => (
                <PresetCard key={p.id} preset={p} active={presetId === p.id} onClick={() => switchPreset(p.id)}/>
              ))}
            </div>
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6, marginTop: 6 }}>
              <PresetCard preset={PRESETS[2]} active={presetId === "custom"} onClick={() => switchPreset("custom")}/>
              <button style={{ ...window.btnStyle("ghost"), justifyContent: "center", borderStyle: "dashed", color: "var(--mv-fg-3)" }}>
                <window.IconPlus size={13}/> Save preset
              </button>
            </div>
          </window.Section>

          {/* Parameters */}
          <window.Section title="Parameters">
            {PARAM_GROUPS.basic.map((p) => (
              <ParamRow key={p.k} param={p} value={params[p.k]} onChange={(v) => updateParam(p.k, v)}/>
            ))}
          </window.Section>

          {/* Advanced */}
          <div style={{ marginBottom: 24 }}>
            <button onClick={() => setAdvancedOpen((v) => !v)} style={{
              width: "100%", display: "flex", alignItems: "center", gap: 6,
              background: "transparent", border: 0, cursor: "pointer", padding: 0,
              fontFamily: "var(--mv-font-mono)", fontSize: 10,
              letterSpacing: "0.16em", textTransform: "uppercase",
              color: "var(--mv-fg-3)", marginBottom: 10,
            }}>
              <span style={{ width: 6, height: 6, background: "var(--mv-blue-500)", borderRadius: 1 }}/>
              <span style={{ display: "inline-flex", alignItems: "center", gap: 6, transform: advancedOpen ? "" : "rotate(-90deg)", transformOrigin: "center", transition: "transform .15s" }}>
                <window.IconChevD size={10}/>
              </span>
              <span>Advanced</span>
            </button>
            {advancedOpen && (
              <>
                <ToggleRow3 label="Normalize sampling" value={params.normalizeSampling}
                  onChange={(v) => updateParam("normalizeSampling", v)}/>
                {PARAM_GROUPS.advanced.map((p) => (
                  <ParamRow key={p.k} param={p} value={params[p.k] ?? params[p.fallback]} onChange={(v) => updateParam(p.k, v)}/>
                ))}
                <ToggleRow3 label="Exclude mesh boundaries" value={params.excludeBoundaries}
                  onChange={(v) => updateParam("excludeBoundaries", v)}/>
                <button onClick={resetDefaults} style={{
                  ...window.btnStyle("ghost"), width: "100%", marginTop: 8,
                  fontSize: 12, color: "var(--mv-fg-2)",
                }}>
                  <window.IconReset size={13}/> Reset to defaults
                </button>
              </>
            )}
          </div>

          {/* Process button */}
          <button onClick={onProcess} disabled={processing}
            style={{ ...window.btnStyle("primary"), width: "100%", padding: "12px",
                     opacity: processing ? 0.6 : 1,
                     fontSize: 14 }}>
            {processing ? "Processing…" : <><window.IconWand size={15}/> Process Morph</>}
          </button>

          {/* Vertex mask */}
          <window.Section title="Vertex mask">
            <div style={{ fontSize: 12, color: "var(--mv-fg-3)", marginBottom: 10, lineHeight: 1.5 }}>
              Paint to exclude vertices from the next process pass.
            </div>
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 6 }}>
              <ToggleChip label="Paint mask" value={maskMode} onChange={setMaskMode} icon={<window.IconBrush size={13}/>}/>
              <ToggleChip label="Show mask" value={showMask} onChange={setShowMask} icon={<window.IconEye size={13}/>}/>
            </div>
            {maskMode && (
              <>
                <div style={{
                  display: "grid", gridTemplateColumns: "100px 1fr 36px", alignItems: "center",
                  gap: 10, marginTop: 10,
                }}>
                  <span style={{ fontSize: 12, color: "var(--mv-fg-2)" }}>Mask radius</span>
                  <SliderTrack3 value={maskRadius} min={2} max={40} onChange={setMaskRadius}/>
                  <span style={{ fontFamily: "var(--mv-font-mono)", fontSize: 11, color: "var(--mv-fg-1)", textAlign: "right" }}>{maskRadius.toFixed(1)}</span>
                </div>
                <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 4, marginTop: 8 }}>
                  <button style={{ ...window.btnStyle("ghost"), padding: "6px 8px", fontSize: 11 }}>Erase</button>
                  <button style={{ ...window.btnStyle("ghost"), padding: "6px 8px", fontSize: 11 }}>Clear</button>
                  <button style={{ ...window.btnStyle("ghost"), padding: "6px 8px", fontSize: 11 }}>Invert</button>
                </div>
              </>
            )}
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
          }}>STEP <span style={{ color: "var(--mv-fg-1)" }}>03</span> OF 04</div>
          <button style={window.btnStyle("primary")}>Accept <window.IconChevR size={14}/></button>
        </div>
      </div>
    </div>
  );
}

/* ---------- subcomponents ---------- */

function ViewModeBtn({ children, active, onClick }) {
  return (
    <button onClick={onClick} style={{
      padding: "5px 12px", border: 0, borderRadius: 4,
      background: active ? "var(--mv-blue-500)" : "transparent",
      color: active ? "#051322" : "var(--mv-fg-2)",
      fontFamily: "var(--mv-font-body)", fontSize: 12, fontWeight: active ? 600 : 500,
      cursor: "pointer",
    }}>{children}</button>
  );
}

function Pill({ children }) {
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

function PresetCard({ preset, active, onClick }) {
  return (
    <button onClick={onClick} style={{
      background: active ? "rgba(61,165,245,0.08)" : "var(--mv-bg-1)",
      border: `1px solid ${active ? "var(--mv-blue-500)" : "var(--mv-line-2)"}`,
      borderRadius: 6, padding: "10px 12px",
      cursor: "pointer", textAlign: "left",
      boxShadow: active ? "var(--mv-glow-soft)" : "none",
      transition: "all .15s",
      display: "flex", alignItems: "center", gap: 10,
      minWidth: 0, overflow: "hidden",
    }}>
      <PresetIcon kind={preset.icon} active={active}/>
      <div style={{ minWidth: 0, flex: 1, overflow: "hidden" }}>
        <div style={{ fontSize: 13, fontWeight: 600, color: active ? "var(--mv-fg-1)" : "var(--mv-fg-2)", marginBottom: 2, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
          {preset.name}
        </div>
        <div style={{ fontSize: 11, color: "var(--mv-fg-3)", lineHeight: 1.3, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
          {preset.sub}
        </div>
      </div>
    </button>
  );
}

function PresetIcon({ kind, active }) {
  const c = active ? "var(--mv-blue-400)" : "var(--mv-fg-3)";
  return (
    <div style={{
      width: 32, height: 32, borderRadius: 4, flexShrink: 0,
      background: active ? "rgba(61,165,245,0.12)" : "var(--mv-bg-2)",
      display: "flex", alignItems: "center", justifyContent: "center",
      color: c,
    }}>
      {kind === "human" && (
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
          <circle cx="12" cy="9" r="4"/>
          <path d="M5 21c0-4 3-7 7-7s7 3 7 7"/>
        </svg>
      )}
      {kind === "fantasy" && (
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
          <path d="M12 4l-5 6c-1 1-1 3 0 4l4 4c0.5 0.5 1.5 0.5 2 0l4-4c1-1 1-3 0-4z"/>
          <path d="M7 10l-3-2M17 10l3-2M9 17l-3 3M15 17l3 3"/>
        </svg>
      )}
      {kind === "custom" && (
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
          <path d="M12 3l3 6 6 1-4 4 1 6-6-3-6 3 1-6-4-4 6-1z"/>
        </svg>
      )}
    </div>
  );
}

function ParamRow({ param, value, onChange }) {
  const ref = useRef(null);
  const [drag, setDrag] = useState(false);
  const update = (e) => {
    const r = ref.current.getBoundingClientRect();
    const p = Math.max(0, Math.min(1, (e.clientX - r.left) / r.width));
    let v = param.min + p * (param.max - param.min);
    if (param.step >= 1) v = Math.round(v / param.step) * param.step;
    else v = +(Math.round(v / param.step) * param.step).toFixed(param.precision || 4);
    onChange(v);
  };
  const pct = ((value - param.min) / (param.max - param.min)) * 100;
  const display = param.precision ? Number(value).toFixed(param.precision)
                : param.step >= 1 ? Math.round(value)
                : Number(value).toFixed(2);
  return (
    <div style={{
      display: "grid", gridTemplateColumns: "120px 1fr", gap: 10,
      alignItems: "center", padding: "5px 0",
    }}>
      <span style={{ fontSize: 12, color: "var(--mv-fg-2)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{param.label}</span>
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
          style={{
            position: "absolute", inset: 0, cursor: "ew-resize",
          }}>
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
        }}>{display}{param.unit && param.unit !== "" ? ` ${param.unit}` : ""}</span>
      </div>
    </div>
  );
}

function ToggleRow3({ label, value, onChange }) {
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

function ToggleChip({ label, value, onChange, icon }) {
  return (
    <button onClick={() => onChange(!value)} style={{
      ...window.btnStyle("ghost"),
      background: value ? "rgba(61,165,245,0.12)" : "var(--mv-bg-2)",
      borderColor: value ? "var(--mv-blue-500)" : "var(--mv-line-2)",
      color: value ? "var(--mv-blue-400)" : "var(--mv-fg-2)",
      justifyContent: "flex-start",
    }}>
      {icon} {label}
    </button>
  );
}

function SliderTrack3({ value, min, max, onChange }) {
  const pct = ((value - min) / (max - min)) * 100;
  const ref = useRef(null);
  const [drag, setDrag] = useState(false);
  const update = (e) => {
    const r = ref.current.getBoundingClientRect();
    const p = Math.max(0, Math.min(1, (e.clientX - r.left) / r.width));
    onChange(+(min + p * (max - min)).toFixed(1));
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

window.Stage3View = Stage3View;
