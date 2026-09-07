"use client";

import { useRef } from "react";
import Image from "next/image";
import { motion, useScroll, useTransform } from "framer-motion";
import WidgetPill from "./WidgetPill";
import { Grid2x2, Search, Folder, Compass } from "lucide-react";
import { basePath } from "@/lib/basePath";

export default function Hero() {
  const ref = useRef(null);
  const { scrollYProgress } = useScroll({
    target: ref,
    offset: ["start start", "end start"],
  });

  // 0 = deep night, 1 = full day — this single value drives the whole scene.
  const progress = useTransform(scrollYProgress, [0, 1], [0, 1]);

  const skyTop = useTransform(progress, [0, 0.6, 1], ["#060912", "#16213f", "#bcdcff"]);
  const skyBottom = useTransform(progress, [0, 0.6, 1], ["#0b1120", "#25335c", "#eaf4ff"]);
  const mountainFar = useTransform(progress, [0, 1], ["#131c33", "#a9c3e6"]);
  const mountainMid = useTransform(progress, [0, 1], ["#0e1526", "#7fa3d6"]);
  const mountainNear = useTransform(progress, [0, 1], ["#080c17", "#4d6fa8"]);
  const snowGlow = useTransform(progress, [0, 1], ["#c9d6f2", "#ffffff"]);
  const starOpacity = useTransform(progress, [0, 0.5, 0.8], [1, 0.4, 0]);
  const celestialY = useTransform(progress, [0, 1], [40, 140]);
  const moonOpacity = useTransform(progress, [0, 0.55], [1, 0]);
  const sunOpacity = useTransform(progress, [0.45, 1], [0, 1]);
  const eyebrowBg = useTransform(progress, [0, 1], ["rgba(255,255,255,0.06)", "rgba(15,23,42,0.06)"]);
  const eyebrowBorder = useTransform(progress, [0, 1], ["rgba(255,255,255,0.12)", "rgba(15,23,42,0.12)"]);
  const eyebrowText = useTransform(progress, [0, 1], ["#e2e8f0", "#334155"]);
  const dotColor = useTransform(progress, [0, 1], ["#dc2626", "#2563eb"]);
  const headlineA = useTransform(progress, [0, 1], ["#ffffff", "#0f172a"]);
  const bodyText = useTransform(progress, [0, 1], ["#cbd5e1", "#475569"]);
  const secondaryText = useTransform(progress, [0, 1], ["#93c5fd", "#1d4ed8"]);
  const taskbarBg = useTransform(progress, [0, 1], ["rgba(10,14,26,0.85)", "rgba(255,255,255,0.75)"]);
  const taskbarIcon = useTransform(progress, [0, 1], ["#e2e8f0", "#1e293b"]);
  const taskbarBorder = useTransform(progress, [0, 1], ["rgba(255,255,255,0.08)", "rgba(15,23,42,0.1)"]);

  return (
    <section ref={ref} className="relative h-[190vh]">
      <div className="sticky top-0 h-screen w-full overflow-hidden">
        <motion.div
          className="absolute inset-0"
          style={{ background: useTransform([skyTop, skyBottom], ([t, b]) => `linear-gradient(180deg, ${t} 0%, ${b} 100%)`) }}
        />

        {/* stars */}
        <motion.div style={{ opacity: starOpacity }} className="absolute inset-0">
          {STARS.map((s, i) => (
            <span
              key={i}
              className="absolute rounded-full bg-white animate-twinkle"
              style={{
                top: s.top,
                left: s.left,
                width: s.size,
                height: s.size,
                animationDelay: `${s.delay}s`,
              }}
            />
          ))}
        </motion.div>

        {/* moon / sun */}
        <motion.div
          className="absolute right-[12%] md:right-[18%]"
          style={{ top: celestialY }}
        >
          <motion.div
            style={{ opacity: moonOpacity }}
            className="absolute w-16 h-16 md:w-24 md:h-24 rounded-full bg-[#f4f1ea] shadow-[0_0_70px_18px_rgba(244,241,234,0.35)]"
          >
            <div className="absolute top-2 left-3 w-4 h-4 rounded-full bg-black/10" />
            <div className="absolute bottom-3 right-4 w-3 h-3 rounded-full bg-black/10" />
          </motion.div>
          <motion.div
            style={{ opacity: sunOpacity }}
            className="absolute w-16 h-16 md:w-24 md:h-24 rounded-full bg-gradient-to-br from-amber-200 to-orange-300 shadow-[0_0_90px_28px_rgba(251,191,36,0.45)]"
          />
        </motion.div>

        {/* mountains */}
        <motion.svg
          viewBox="0 0 1440 500"
          preserveAspectRatio="none"
          className="absolute bottom-[9vh] left-0 w-full h-[46vh] md:h-[52vh]"
        >
          <motion.path
            d="M0 340 L180 220 L340 300 L520 160 L700 280 L880 190 L1060 320 L1240 210 L1440 300 L1440 500 L0 500 Z"
            style={{ fill: mountainFar }}
          />
          <motion.path
            d="M0 400 L220 300 L420 380 L620 260 L860 400 L1080 290 L1440 400 L1440 500 L0 500 Z"
            style={{ fill: mountainMid }}
          />
          <motion.path
            d="M0 460 L260 380 L520 450 L760 360 L1000 450 L1260 380 L1440 440 L1440 500 L0 500 Z"
            style={{ fill: mountainNear }}
          />
          {/* snow caps */}
          <motion.path d="M500 165 L520 160 L560 195 L520 190 Z" style={{ fill: snowGlow }} opacity={0.9} />
          <motion.path d="M860 195 L880 190 L915 220 L878 216 Z" style={{ fill: snowGlow }} opacity={0.9} />
          <motion.path d="M1220 216 L1240 210 L1278 240 L1238 236 Z" style={{ fill: snowGlow }} opacity={0.9} />
        </motion.svg>

        {/* floating leaves (day) */}
        <motion.div style={{ opacity: sunOpacity }} className="absolute inset-0 pointer-events-none">
          <span className="absolute top-[18%] left-[8%] text-2xl animate-drift">🍃</span>
          <span className="absolute top-[30%] left-[85%] text-xl animate-drift" style={{ animationDelay: "1.5s" }}>🍃</span>
        </motion.div>

        {/* content */}
        <div className="relative z-10 h-full max-w-7xl mx-auto px-6 md:px-10 flex flex-col justify-center pb-[22vh] pt-12">
          <motion.div
            style={{ backgroundColor: eyebrowBg, borderColor: eyebrowBorder, color: eyebrowText }}
            className="hidden [@media(min-height:700px)]:inline-flex w-fit items-center gap-2 rounded-full border px-4 py-1.5 text-sm font-medium mb-8"
          >
            <motion.span style={{ backgroundColor: dotColor }} className="w-2 h-2 rounded-full" />
            Windows 11 & 10 · Bikram Sambat (वि.सं.) · Native C++
          </motion.div>

          <div className="max-w-2xl">
            <motion.h1
              style={{ color: headlineA }}
              className="font-display text-5xl sm:text-6xl md:text-7xl font-extrabold leading-[1.02] tracking-tight"
            >
              Tithify —{" "}
              <br className="hidden sm:inline" />
              <span className="bg-gradient-to-r from-crimson via-rose-500 to-sapphire bg-clip-text text-transparent">
                Nepali Date Widget
              </span>
              <br />
              <span className="text-3xl sm:text-4xl md:text-5xl font-bold opacity-90 block mt-2 text-slate-400">
                Above Your Taskbar
              </span>
            </motion.h1>

            <motion.p style={{ color: bodyText }} className="mt-6 text-lg md:text-xl leading-relaxed max-w-lg">
              Tithify is a lightweight native desktop widget that displays the real-time Bikram Sambat (वि.सं.) Nepali date and calendar on Windows. Automatic light/dark theme adaptation, 100% offline, and zero CPU usage.
            </motion.p>

            <div className="mt-9 flex flex-wrap items-center gap-6"> 
              <a
                href="https://github.com/aayushlbef/Tithify"
                target="_blank"
                rel="noopener noreferrer"
                className="focus-ring group inline-flex items-center gap-2 rounded-xl bg-[#24292e] hover:bg-[#2d333b] border border-crimson hover:border-crimson-dark px-6 py-3.5 font-display font-semibold text-white shadow-[0_0_24px_rgba(36,41,46,0.6)] transition-all hover:scale-[1.03] active:scale-[0.98]"
              >
                {/* GitHub mark */}
                <svg className="w-5 h-5" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true">
                  <path d="M12 0C5.37 0 0 5.373 0 12c0 5.303 3.438 9.8 8.205 11.385.6.113.82-.258.82-.577 0-.285-.01-1.04-.015-2.04-3.338.724-4.042-1.61-4.042-1.61-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23A11.509 11.509 0 0 1 12 5.803c1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222 0 1.606-.015 2.898-.015 3.293 0 .322.216.694.825.576C20.565 21.796 24 17.3 24 12c0-6.627-5.373-12-12-12z" />
                </svg>
                GitHub
              </a>
              <a
                href="#preview"
                className="focus-ring inline-flex items-center gap-2 font-medium underline-offset-4 hover:underline"
              >
                <motion.span style={{ color: secondaryText }} className="inline-flex items-center gap-2">
                  <PlayDot /> See it in action
                </motion.span>
              </a>
            </div>
          </div>
        </div>

        {/* floating widget above the taskbar */}
        {/* <motion.div
          className="absolute bottom-[9.5vh] right-6 md:right-16 z-20"
          animate={{ y: [0, -8, 0] }}
          transition={{ duration: 4, repeat: Infinity, ease: "easeInOut" }}
        >
          <WidgetPill progress={progress} size="lg" />
        </motion.div> */}

        {/* taskbar mockup */}
        <motion.div
          style={{ backgroundColor: taskbarBg, borderColor: taskbarBorder }}
          className="absolute bottom-0 left-0 right-0 grid h-[9vh] grid-cols-[minmax(0,1fr)_auto_minmax(0,1fr)] items-center border-t px-6 backdrop-blur-xl md:px-10"
        >
          <div className="flex min-w-0 items-center">
            <div className="relative h-[4vh] w-full max-w-[11rem] overflow-hidden">
              <Image
                src={`${basePath}/widiget_transparent_shot.png`}
                alt="Tithify Nepali Date widget preview"
                fill
                sizes="176px"
                priority
                className="object-cover object-center"
              />
            </div>
          </div>
          <div className="flex items-center justify-center gap-5">
            <IconBox color={taskbarIcon}><Grid2x2 className="w-4 h-4" /></IconBox>
            <IconBox color={taskbarIcon}><Search className="w-4 h-4" /></IconBox>
            <IconBox color={taskbarIcon}><Folder className="w-4 h-4" /></IconBox>
            <IconBox color={taskbarIcon}><Compass className="w-4 h-4" /></IconBox>
          </div>
          <motion.div style={{ color: taskbarIcon }} className="justify-self-end text-xs font-medium tabular-nums md:text-sm">
            11:45 AM · May 7, 2026
          </motion.div>
        </motion.div>

        {/* scroll cue */}
        <div className="absolute bottom-[10.5vh] left-1/2 -translate-x-1/2 z-20 md:block hidden">
          <motion.div
            animate={{ y: [0, 8, 0] }}
            transition={{ duration: 1.8, repeat: Infinity }}
            style={{ color: eyebrowText }}
            className="text-[11px] tracking-[0.2em] uppercase opacity-60"
          >
            Scroll — watch the theme wake up
          </motion.div>
        </div>
      </div>
    </section>
  );
}

function IconBox({ children, color }) {
  return (
    <motion.div style={{ color }} className="w-4 h-4 flex items-center justify-center opacity-90">
      {children}
    </motion.div>
  );
}

function PlayDot() {
  return (
    <span className="inline-flex items-center justify-center w-6 h-6 rounded-full border border-current">
      <span className="w-0 h-0 border-y-[5px] border-y-transparent border-l-[7px] border-l-current ml-0.5" />
    </span>
  );
}

// Deterministic pseudo-random star field (avoids SSR/client hydration mismatch).
function seeded(i, mult) {
  const x = Math.sin(i * mult) * 10000;
  return x - Math.floor(x);
}

const STARS = Array.from({ length: 60 }).map((_, i) => ({
  top: `${seeded(i, 12.9898) * 55}%`,
  left: `${seeded(i, 78.233) * 100}%`,
  size: `${seeded(i, 45.164) * 2 + 1}px`,
  delay: seeded(i, 94.673) * 3,
}));
