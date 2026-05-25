/* global React */
const { useState: useStateL, useEffect: useEffectL, useRef: useRefL } = React;

/* ------------------------------------------------------------------ */
/*  LoginScreen — first-launch authentication                         */
/*  Two states:                                                       */
/*    "idle"    → user has not yet clicked Sign In                    */
/*    "waiting" → browser opened, app is awaiting OS deep-link cb     */
/* ------------------------------------------------------------------ */
function LoginScreen({ state = "idle", onSignIn, onCancel, onRetry, onOffline }) {
  const isWaiting = state === "waiting";

  // Animate the rotating spinner manually so it works without CSS keyframes file
  const [tick, setTick] = useStateL(0);
  useEffectL(() => {
    if (!isWaiting) return;
    let raf;
    const loop = () => { setTick((t) => t + 1); raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [isWaiting]);

  const callbackUri = "metavisage://auth/callback?session=8f3a…d21c";

  return (
    <div style={{
      position: "absolute", inset: 0, zIndex: 200,
      background: "var(--mv-bg-0)",
      backgroundImage: `
        linear-gradient(var(--mv-line-1) 1px, transparent 1px),
        linear-gradient(90deg, var(--mv-line-1) 1px, transparent 1px),
        linear-gradient(rgba(125,175,220,0.04) 1px, transparent 1px),
        linear-gradient(90deg, rgba(125,175,220,0.04) 1px, transparent 1px),
        radial-gradient(ellipse at 50% 35%, rgba(61,165,245,0.10), transparent 60%)`,
      backgroundSize: "96px 96px, 96px 96px, 16px 16px, 16px 16px, 100% 100%",
      display: "flex", flexDirection: "column",
      overflow: "hidden",
    }}>
      {/* Faux-window title bar so it feels native */}
      <div style={{
        height: 36, display: "flex", alignItems: "center",
        padding: "0 0 0 14px",
        borderBottom: "1px solid var(--mv-line-2)",
        fontFamily: "var(--mv-font-mono)", fontSize: 11,
        color: "var(--mv-fg-3)", letterSpacing: "0.06em",
        flexShrink: 0,
        background: "rgba(7,16,28,0.6)",
        backdropFilter: "blur(6px)",
      }}>
        <img src="assets/metavisage-logo-mark.png" alt="" style={{ width: 18, height: 18, marginRight: 10, opacity: 0.9 }}/>
        <span style={{ color: "var(--mv-fg-1)", fontWeight: 600 }}>MetaVisage</span>
        <span style={{ color: "var(--mv-line-3)", margin: "0 10px" }}>·</span>
        <span style={{ color: "var(--mv-fg-2)" }}>Sign in</span>
        <div style={{ flex: 1 }}/>
        <div style={{ display: "flex", height: "100%" }}>
          {[
            <window.IconMin size={14} sw={1.6}/>,
            <window.IconMax size={14} sw={1.6}/>,
            <window.IconClose size={14} sw={1.6}/>,
          ].map((I, i) => (
            <button key={i} style={{
              width: 46, height: "100%", background: "transparent", border: 0,
              color: "var(--mv-fg-2)", cursor: "pointer",
              display: "inline-flex", alignItems: "center", justifyContent: "center",
            }}>{I}</button>
          ))}
        </div>
      </div>

      {/* Body */}
      <div style={{
        flex: 1, display: "flex", alignItems: "center", justifyContent: "center",
        padding: 24, position: "relative",
      }}>
        {/* Decorative blueprint marks */}
        <CornerBracket style={{ top: 60, left: 60 }}/>
        <CornerBracket style={{ top: 60, right: 60 }} flip/>
        <CornerBracket style={{ bottom: 60, left: 60 }} flipY/>
        <CornerBracket style={{ bottom: 60, right: 60 }} flip flipY/>

        {/* Auth card */}
        <div style={{
          width: 480,
          background: "var(--mv-bg-2)",
          border: "1px solid var(--mv-line-3)",
          borderRadius: 8,
          boxShadow: "var(--mv-shadow-3), 0 0 80px rgba(61,165,245,0.08)",
          overflow: "hidden",
          position: "relative",
        }}>
          {/* Top accent bar */}
          <div style={{
            height: 2,
            background: "linear-gradient(90deg, transparent, var(--mv-blue-500) 20%, var(--mv-blue-300) 50%, var(--mv-blue-500) 80%, transparent)",
            opacity: isWaiting ? 1 : 0.6,
            transition: "opacity .3s",
          }}/>

          {/* Decorative corner glow */}
          <div aria-hidden style={{
            position: "absolute", top: 0, right: 0, width: 220, height: 220,
            background: "radial-gradient(circle at 100% 0%, rgba(61,165,245,0.10), transparent 60%)",
            pointerEvents: "none",
          }}/>

          {/* Hero */}
          <div style={{ padding: "32px 36px 22px", display: "flex", alignItems: "center", gap: 18, position: "relative" }}>
            {/* Mark */}
            <div style={{
              width: 64, height: 64, borderRadius: 12, flexShrink: 0,
              background: "linear-gradient(135deg, var(--mv-blue-500), var(--mv-blue-700))",
              border: "1px solid var(--mv-blue-400)",
              display: "flex", alignItems: "center", justifyContent: "center",
              boxShadow: "0 0 24px rgba(61,165,245,0.30)",
              position: "relative",
            }}>
              <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="#E8F1FB" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round">
                <circle cx="12" cy="9" r="4"/>
                <path d="M5 21c0-4 3-7 7-7s7 3 7 7"/>
                <path d="M9 9c1-1 2-1 3 0M12 9c1-1 2-1 3 0" opacity="0.6"/>
              </svg>
              {isWaiting && (
                <div style={{
                  position: "absolute", inset: -6,
                  border: "1px solid rgba(61,165,245,0.4)",
                  borderRadius: 14,
                  borderTopColor: "transparent",
                  transform: `rotate(${tick * 2}deg)`,
                }}/>
              )}
            </div>
            <div style={{ minWidth: 0, flex: 1 }}>
              <div style={{
                fontFamily: "var(--mv-font-mono)", fontSize: 11,
                color: "var(--mv-blue-400)", letterSpacing: "0.16em",
                textTransform: "uppercase", marginBottom: 6,
              }}>v 0.9.4 · Sign in required</div>
              <div style={{
                fontFamily: "var(--mv-font-display)", fontStyle: "italic",
                fontWeight: 800, fontSize: 32, lineHeight: 1, color: "var(--mv-fg-1)",
                letterSpacing: "-0.005em", marginBottom: 6,
              }}>
                {isWaiting
                  ? <>Awaiting <span style={{ color: "var(--mv-blue-400)" }}>browser</span></>
                  : <>Sign in to <span style={{ color: "var(--mv-blue-400)" }}>MetaVisage</span></>}
              </div>
              <div style={{ fontSize: 14, color: "var(--mv-fg-2)", lineHeight: 1.45 }}>
                {isWaiting
                  ? "We've opened your default browser. Finish authenticating there and you'll be redirected back automatically."
                  : "Authenticate in your browser to unlock morphing, the asset library, and Live Link to Unreal."}
              </div>
            </div>
          </div>

          {/* Divider with mono label */}
          <div style={{
            display: "flex", alignItems: "center", gap: 12,
            padding: "0 36px",
          }}>
            <div style={{ height: 1, background: "var(--mv-line-2)", flex: 1 }}/>
            <span style={{
              fontFamily: "var(--mv-font-mono)", fontSize: 10,
              color: "var(--mv-fg-3)", letterSpacing: "0.18em",
              textTransform: "uppercase",
            }}>
              {isWaiting ? "Step 02 / 02 — Browser handoff" : "Step 01 / 02 — Authentication"}
            </span>
            <div style={{ height: 1, background: "var(--mv-line-2)", flex: 1 }}/>
          </div>

          {/* Body content */}
          <div style={{ padding: "20px 36px 8px" }}>
            {!isWaiting ? (
              <>
                {/* Idle: primary action */}
                <button
                  onClick={onSignIn}
                  style={{
                    width: "100%",
                    display: "flex", alignItems: "center", justifyContent: "center", gap: 10,
                    height: 48,
                    background: "var(--mv-blue-500)",
                    border: "1px solid var(--mv-blue-300)",
                    borderRadius: 6,
                    color: "#051322",
                    fontFamily: "var(--mv-font-body)", fontWeight: 600, fontSize: 15,
                    cursor: "pointer",
                    boxShadow: "var(--mv-glow-soft)",
                    transition: "background .15s, transform .1s",
                  }}
                  onMouseEnter={(e) => e.currentTarget.style.background = "var(--mv-blue-400)"}
                  onMouseLeave={(e) => e.currentTarget.style.background = "var(--mv-blue-500)"}
                  onMouseDown={(e) => e.currentTarget.style.transform = "translateY(1px)"}
                  onMouseUp={(e) => e.currentTarget.style.transform = "translateY(0)"}
                >
                  <ExternalIcon size={16}/>
                  Sign in with browser
                </button>

                {/* Sub: explainer rows */}
                <div style={{ marginTop: 18, display: "flex", flexDirection: "column", gap: 10 }}>
                  <Row n="01" title="Your browser opens a secure MetaVisage sign-in page."/>
                  <Row n="02" title="After you authenticate, the browser redirects back to this app."/>
                  <Row n="03" title="The desktop app picks up the session — no copy-paste required."/>
                </div>
              </>
            ) : (
              <>
                {/* Waiting: spinner block */}
                <div style={{
                  display: "flex", alignItems: "center", gap: 14,
                  padding: "14px 16px",
                  background: "var(--mv-bg-1)",
                  border: "1px solid var(--mv-line-2)",
                  borderRadius: 6,
                }}>
                  <Spinner tick={tick}/>
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{ fontSize: 13, color: "var(--mv-fg-1)", fontWeight: 500 }}>
                      Listening on <span style={{ fontFamily: "var(--mv-font-mono)", color: "var(--mv-blue-400)" }}>localhost:51730</span>
                    </div>
                    <div style={{
                      fontFamily: "var(--mv-font-mono)", fontSize: 11,
                      color: "var(--mv-fg-3)", letterSpacing: "0.06em", marginTop: 3,
                    }}>
                      Will time out in <span style={{ color: "var(--mv-fg-2)" }}>2:43</span>
                    </div>
                  </div>
                  <span style={{
                    display: "inline-flex", alignItems: "center", gap: 6,
                    padding: "4px 10px", borderRadius: 4,
                    border: "1px solid rgba(63,214,158,0.4)",
                    background: "rgba(63,214,158,0.08)",
                    color: "var(--mv-success)",
                    fontFamily: "var(--mv-font-mono)", fontSize: 10, letterSpacing: "0.12em",
                  }}>
                    <svg width="6" height="6"><circle cx="3" cy="3" r="3" fill="currentColor"/></svg>
                    LIVE
                  </span>
                </div>

                {/* Callback URI display */}
                <div style={{ marginTop: 14 }}>
                  <div style={{
                    fontFamily: "var(--mv-font-mono)", fontSize: 10,
                    letterSpacing: "0.16em", textTransform: "uppercase",
                    color: "var(--mv-fg-3)", marginBottom: 6,
                    display: "flex", alignItems: "center", gap: 8,
                  }}>
                    <span style={{ width: 6, height: 6, background: "var(--mv-blue-500)", borderRadius: 1 }}/>
                    Callback URI
                  </div>
                  <div style={{
                    display: "grid", gridTemplateColumns: "1fr auto", gap: 6,
                  }}>
                    <div style={{
                      background: "var(--mv-bg-1)",
                      border: "1px solid var(--mv-line-2)",
                      borderRadius: 4, padding: "9px 12px",
                      fontFamily: "var(--mv-font-mono)", fontSize: 12,
                      color: "var(--mv-fg-1)",
                      overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
                    }}>{callbackUri}</div>
                    <button style={{
                      ...window.btnStyle("ghost"),
                      padding: "0 12px",
                    }}>
                      <CopyIcon size={13}/> Copy
                    </button>
                  </div>
                </div>

                {/* Manual fallback */}
                <div style={{
                  marginTop: 14,
                  padding: "12px 14px",
                  background: "rgba(242,181,71,0.06)",
                  border: "1px solid rgba(242,181,71,0.25)",
                  borderRadius: 6,
                  display: "flex", alignItems: "flex-start", gap: 10,
                }}>
                  <div style={{
                    width: 18, height: 18, borderRadius: "50%", flexShrink: 0,
                    border: "1px solid var(--mv-warning)", color: "var(--mv-warning)",
                    display: "inline-flex", alignItems: "center", justifyContent: "center",
                    fontFamily: "var(--mv-font-mono)", fontSize: 11, fontWeight: 700,
                  }}>!</div>
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{ fontSize: 12, color: "var(--mv-fg-1)", lineHeight: 1.45 }}>
                      Browser didn't open? <a href="#" style={{ color: "var(--mv-blue-400)" }}>Open the link manually</a> or <a href="#" onClick={onRetry} style={{ color: "var(--mv-blue-400)" }}>retry</a>.
                    </div>
                  </div>
                </div>
              </>
            )}
          </div>

          {/* Footer */}
          <div style={{
            padding: "14px 36px 18px",
            marginTop: 12,
            borderTop: "1px solid var(--mv-line-2)",
            background: "var(--mv-bg-1)",
            display: "flex", alignItems: "center", justifyContent: "space-between", gap: 12,
          }}>
            <div style={{
              fontFamily: "var(--mv-font-mono)", fontSize: 10,
              color: "var(--mv-fg-3)", letterSpacing: "0.08em",
              display: "flex", alignItems: "center", gap: 8,
            }}>
              <window.IconLock size={12}/>
              SSO · OAuth 2.1 · PKCE
            </div>
            <div style={{ display: "flex", gap: 6 }}>
              {isWaiting ? (
                <button onClick={onCancel} style={window.btnStyle("ghost")}>Cancel</button>
              ) : (
                <button onClick={onOffline} style={window.btnStyle("ghost")}>Use offline</button>
              )}
            </div>
          </div>
        </div>

        {/* Bottom-fixed metadata strip */}
        <div style={{
          position: "absolute", bottom: 18, left: 0, right: 0,
          display: "flex", justifyContent: "center", gap: 18,
          fontFamily: "var(--mv-font-mono)", fontSize: 10,
          color: "var(--mv-fg-4)", letterSpacing: "0.12em",
        }}>
          <span>BUILD <span style={{ color: "var(--mv-fg-3)" }}>0.9.4-rc.2</span></span>
          <span style={{ color: "var(--mv-line-3)" }}>·</span>
          <span>NODE <span style={{ color: "var(--mv-fg-3)" }}>auth-eu-2</span></span>
          <span style={{ color: "var(--mv-line-3)" }}>·</span>
          <a href="#" style={{ color: "var(--mv-fg-3)" }}>HELP</a>
          <span style={{ color: "var(--mv-line-3)" }}>·</span>
          <a href="#" style={{ color: "var(--mv-fg-3)" }}>PRIVACY</a>
        </div>
      </div>
    </div>
  );
}

/* ---------- bits ---------- */
function Row({ n, title }) {
  return (
    <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
      <span style={{
        width: 24, height: 24, borderRadius: 4,
        background: "var(--mv-bg-1)",
        border: "1px solid var(--mv-line-2)",
        display: "inline-flex", alignItems: "center", justifyContent: "center",
        fontFamily: "var(--mv-font-mono)", fontSize: 10,
        color: "var(--mv-blue-400)", letterSpacing: "0.04em", flexShrink: 0,
      }}>{n}</span>
      <span style={{ fontSize: 13, color: "var(--mv-fg-2)", lineHeight: 1.4 }}>{title}</span>
    </div>
  );
}

function Spinner({ tick }) {
  const r = 14;
  const c = 2 * Math.PI * r;
  const offset = (tick * 4) % c;
  return (
    <svg width="34" height="34" viewBox="0 0 34 34" style={{ flexShrink: 0 }}>
      <circle cx="17" cy="17" r={r} stroke="var(--mv-line-2)" strokeWidth="2" fill="none"/>
      <circle cx="17" cy="17" r={r}
        stroke="var(--mv-blue-400)" strokeWidth="2" fill="none"
        strokeLinecap="round"
        strokeDasharray={`${c * 0.28} ${c}`}
        strokeDashoffset={-offset}
        transform="rotate(-90 17 17)"
        style={{ filter: "drop-shadow(0 0 4px rgba(61,165,245,0.6))" }}
      />
    </svg>
  );
}

function CornerBracket({ style, flip, flipY }) {
  return (
    <div style={{
      position: "absolute", width: 28, height: 28,
      transform: `scale(${flip ? -1 : 1}, ${flipY ? -1 : 1})`,
      ...style,
    }}>
      <div style={{ position: "absolute", top: 0, left: 0, width: 16, height: 1, background: "var(--mv-line-3)" }}/>
      <div style={{ position: "absolute", top: 0, left: 0, width: 1, height: 16, background: "var(--mv-line-3)" }}/>
    </div>
  );
}

const ExternalIcon = ({ size = 16 }) => (
  <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
    <path d="M14 4h6v6"/>
    <path d="M20 4l-9 9"/>
    <path d="M19 13v6a1 1 0 01-1 1H5a1 1 0 01-1-1V6a1 1 0 011-1h6"/>
  </svg>
);
const CopyIcon = ({ size = 14 }) => (
  <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.6" strokeLinecap="round" strokeLinejoin="round">
    <rect x="8" y="8" width="12" height="12" rx="1.5"/>
    <path d="M16 8V5a1 1 0 00-1-1H5a1 1 0 00-1 1v10a1 1 0 001 1h3"/>
  </svg>
);

window.LoginScreen = LoginScreen;
