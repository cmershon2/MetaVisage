/* global React */
const { useState: useStateW, useRef: useRefW } = React;

function WelcomeModal({ onClose, onNew, onOpen, onLoadProject }) {
  const recents = [
    { name: "goblin_v3_step4.mvproj",   when: "2 hours ago",  size: "184 MB" },
    { name: "goblin_v3_step3.mvproj",   when: "today, 10:42",  size: "172 MB" },
    { name: "goblin_v3_step2.mvproj",   when: "yesterday",     size: "96 MB"  },
    { name: "goblin_v3_step1.mvproj",   when: "yesterday",     size: "48 MB"  },
    { name: "Untitled Project.mvproj",  when: "3 days ago",    size: "12 MB"  },
  ];
  const [hover, setHover] = useStateW(null);

  return (
    <div style={{
      position: "absolute", inset: 0, zIndex: 100,
      background: "rgba(7,16,28,0.55)", backdropFilter: "blur(2px)",
      display: "flex", alignItems: "center", justifyContent: "center",
    }}>
      <div style={{
        width: 560, maxHeight: "88%",
        background: "var(--mv-bg-2)",
        border: "1px solid var(--mv-line-3)",
        borderRadius: 8, overflow: "hidden",
        boxShadow: "var(--mv-shadow-3), 0 0 80px rgba(61,165,245,0.06)",
        display: "flex", flexDirection: "column",
        position: "relative",
      }}>
        {/* Decorative corner */}
        <div aria-hidden style={{
          position: "absolute", top: 0, right: 0, width: 220, height: 220,
          background: "radial-gradient(circle at 100% 0%, rgba(61,165,245,0.10), transparent 60%)",
          pointerEvents: "none",
        }}/>

        {/* Hero */}
        <div style={{
          padding: "28px 32px 22px",
          borderBottom: "1px solid var(--mv-line-2)",
          background: "linear-gradient(180deg, rgba(15,30,46,0.6), transparent)",
          display: "flex", alignItems: "center", gap: 18,
          position: "relative",
        }}>
          {/* App mark */}
          <div style={{
            width: 56, height: 56, borderRadius: 10, flexShrink: 0,
            background: "linear-gradient(135deg, var(--mv-blue-500), var(--mv-blue-700))",
            border: "1px solid var(--mv-blue-400)",
            display: "flex", alignItems: "center", justifyContent: "center",
            boxShadow: "0 0 20px rgba(61,165,245,0.25)",
          }}>
            <svg width="30" height="30" viewBox="0 0 24 24" fill="none" stroke="#E8F1FB" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round">
              <circle cx="12" cy="9" r="4"/>
              <path d="M5 21c0-4 3-7 7-7s7 3 7 7"/>
              <path d="M9 9c1-1 2-1 3 0M12 9c1-1 2-1 3 0" opacity="0.6"/>
            </svg>
          </div>
          <div style={{ minWidth: 0, flex: 1 }}>
            <div style={{
              fontFamily: "var(--mv-font-mono)", fontSize: 11,
              color: "var(--mv-blue-400)", letterSpacing: "0.16em",
              textTransform: "uppercase", marginBottom: 6,
            }}>v 0.9.4 · early access</div>
            <div style={{
              fontFamily: "var(--mv-font-display)", fontStyle: "italic",
              fontWeight: 800, fontSize: 32, lineHeight: 1, color: "var(--mv-fg-1)",
              letterSpacing: "-0.005em", marginBottom: 6,
            }}>
              Welcome to <span style={{ color: "var(--mv-blue-400)" }}>MetaVisage</span>
            </div>
            <div style={{ fontSize: 14, color: "var(--mv-fg-2)", lineHeight: 1.45 }}>
              MetaHuman mesh morphing for indie teams.
            </div>
          </div>
          <button onClick={onClose} style={{
            position: "absolute", top: 14, right: 14,
            width: 26, height: 26, padding: 0, border: 0,
            background: "transparent", color: "var(--mv-fg-3)", cursor: "pointer",
            display: "flex", alignItems: "center", justifyContent: "center",
            borderRadius: 4,
          }}>
            <window.IconClose size={14}/>
          </button>
        </div>

        {/* Actions */}
        <div style={{ padding: "18px 32px", borderBottom: "1px solid var(--mv-line-2)" }}>
          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 8 }}>
            <BigAction
              icon={
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round">
                  <path d="M14 3H6a2 2 0 00-2 2v14a2 2 0 002 2h12a2 2 0 002-2V9z"/>
                  <path d="M14 3v6h6M12 12v6M9 15h6"/>
                </svg>
              }
              label="New project"
              sub="Start a fresh morph from scratch"
              primary
              onClick={onNew}
            />
            <BigAction
              icon={<window.IconFolder size={18}/>}
              label="Open project…"
              sub="Continue from a .mvproj file"
              onClick={onOpen}
            />
          </div>
        </div>

        {/* Recents */}
        <div style={{ padding: "16px 32px 18px", overflowY: "auto", overflowX: "hidden", flex: 1, minHeight: 0 }}>
          <div style={{
            display: "flex", alignItems: "center", justifyContent: "space-between",
            marginBottom: 10,
          }}>
            <div style={{
              fontFamily: "var(--mv-font-mono)", fontSize: 10,
              color: "var(--mv-fg-3)", letterSpacing: "0.16em",
              textTransform: "uppercase",
              display: "flex", alignItems: "center", gap: 8,
            }}>
              <span style={{ width: 6, height: 6, background: "var(--mv-blue-500)", borderRadius: 1 }}/>
              Recent projects
            </div>
            <span style={{ fontSize: 11, color: "var(--mv-fg-3)" }}>{recents.length} files</span>
          </div>

          <div style={{
            background: "var(--mv-bg-1)",
            border: "1px solid var(--mv-line-2)",
            borderRadius: 6, overflow: "hidden",
            maxWidth: "100%",
          }}>
            {recents.map((r, i) => (
              <button
                key={r.name}
                onClick={() => onLoadProject?.(r)}
                onMouseEnter={() => setHover(i)}
                onMouseLeave={() => setHover(null)}
                style={{
                  width: "100%", textAlign: "left",
                  padding: "10px 14px", border: 0,
                  borderTop: i === 0 ? "0" : "1px solid var(--mv-line-2)",
                  background: hover === i ? "rgba(61,165,245,0.06)" : "transparent",
                  cursor: "pointer", display: "flex", alignItems: "center", gap: 12,
                  color: "var(--mv-fg-1)",
                }}
              >
                <div style={{
                  width: 28, height: 28, flexShrink: 0, borderRadius: 4,
                  background: hover === i ? "rgba(61,165,245,0.12)" : "var(--mv-bg-2)",
                  border: "1px solid var(--mv-line-2)",
                  display: "flex", alignItems: "center", justifyContent: "center",
                  color: hover === i ? "var(--mv-blue-400)" : "var(--mv-fg-3)",
                }}>
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round">
                    <path d="M14 3H6a2 2 0 00-2 2v14a2 2 0 002 2h12a2 2 0 002-2V9z"/>
                    <path d="M14 3v6h6"/>
                  </svg>
                </div>
                <div style={{ minWidth: 0, flex: 1 }}>
                  <div style={{
                    fontFamily: "var(--mv-font-mono)", fontSize: 12,
                    color: "var(--mv-fg-1)", marginBottom: 2,
                    overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
                  }}>{r.name}</div>
                  <div style={{ fontSize: 11, color: "var(--mv-fg-3)" }}>
                    {r.when} · {r.size}
                  </div>
                </div>
                <window.IconChevR size={14} style={{ color: "var(--mv-fg-3)", opacity: hover === i ? 1 : 0.5 }}/>
              </button>
            ))}
          </div>
        </div>

        {/* Footer */}
        <div style={{
          padding: "12px 32px",
          borderTop: "1px solid var(--mv-line-2)",
          background: "var(--mv-bg-1)",
          display: "flex", alignItems: "center", justifyContent: "space-between",
        }}>
          <label style={{ display: "flex", alignItems: "center", gap: 8, cursor: "pointer", fontSize: 12, color: "var(--mv-fg-2)" }}>
            <input type="checkbox" defaultChecked style={{ accentColor: "var(--mv-blue-500)" }}/>
            Show this on startup
          </label>
          <button onClick={onClose} style={window.btnStyle("ghost")}>Close</button>
        </div>
      </div>
    </div>
  );
}

function BigAction({ icon, label, sub, primary, onClick }) {
  const [hover, setHover] = useStateW(false);
  return (
    <button
      onClick={onClick}
      onMouseEnter={() => setHover(true)}
      onMouseLeave={() => setHover(false)}
      style={{
        background: primary
          ? (hover ? "rgba(61,165,245,0.16)" : "rgba(61,165,245,0.10)")
          : (hover ? "var(--mv-bg-3)" : "var(--mv-bg-1)"),
        border: `1px solid ${primary ? "var(--mv-blue-500)" : "var(--mv-line-2)"}`,
        borderRadius: 6, padding: "14px 16px",
        cursor: "pointer", textAlign: "left",
        boxShadow: primary ? "var(--mv-glow-soft)" : "none",
        transition: "all .15s",
        display: "flex", alignItems: "center", gap: 12,
        minWidth: 0,
      }}
    >
      <div style={{
        width: 36, height: 36, flexShrink: 0, borderRadius: 6,
        background: primary ? "rgba(61,165,245,0.18)" : "var(--mv-bg-2)",
        border: `1px solid ${primary ? "var(--mv-blue-400)" : "var(--mv-line-2)"}`,
        display: "flex", alignItems: "center", justifyContent: "center",
        color: primary ? "var(--mv-blue-400)" : "var(--mv-fg-2)",
      }}>{icon}</div>
      <div style={{ minWidth: 0, flex: 1, overflow: "hidden" }}>
        <div style={{
          fontSize: 14, fontWeight: 600,
          color: primary ? "var(--mv-fg-1)" : "var(--mv-fg-1)",
          marginBottom: 2,
        }}>{label}</div>
        <div style={{ fontSize: 11, color: "var(--mv-fg-3)", lineHeight: 1.4, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
          {sub}
        </div>
      </div>
      <window.IconChevR size={14} style={{ color: primary ? "var(--mv-blue-400)" : "var(--mv-fg-3)", opacity: hover ? 1 : 0.6, flexShrink: 0 }}/>
    </button>
  );
}

window.WelcomeModal = WelcomeModal;
