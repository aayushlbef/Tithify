"use client";

import { useState } from "react";
import Image from "next/image";
import { motion } from "framer-motion";
import { Check, Copy, Terminal } from "lucide-react";
import { basePath } from "@/lib/basePath";

const OPTIONS = [
  {
    label: "PowerShell",
    cmd: "irm https://raw.githubusercontent.com/aayushlbef/Tithify/main/install.ps1 | iex",
  },
  {
    label: "Winget",
    cmd: "winget install Aayush.Tithify",
    soon: true,
  },
];

const easeOut = [0.16, 1, 0.3, 1];

/* Windows Terminal icon: >_ prompt */
function TerminalAppIcon() {
  return (
    <svg viewBox="0 0 16 16" className="w-4 h-4" fill="currentColor" aria-hidden="true">
      {/* > caret */}
      <path d="M2.5 4.5L6.5 8l-4 3.5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" fill="none" />
      {/* _ underscore cursor */}
      <line x1="7.5" y1="11.5" x2="13" y2="11.5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  );
}

/* PowerShell icon: filled right-chevron/arrow */
function PowerShellIcon() {
  return (
    <svg viewBox="0 0 16 16" className="w-4 h-4" fill="currentColor" aria-hidden="true">
      <path d="M5 3l6 5-6 5" stroke="currentColor" strokeWidth="2.2" strokeLinecap="round" strokeLinejoin="round" fill="none" />
    </svg>
  );
}

export default function Install() {
  const [tab, setTab] = useState(0);
  const [copied, setCopied] = useState(false);

  const copy = async () => {
    try {
      await navigator.clipboard.writeText(OPTIONS[tab].cmd);
      setCopied(true);
      setTimeout(() => setCopied(false), 1600);
    } catch {}
  };

  return (
    <section id="install" className="relative bg-gradient-to-b from-[#f4f8ff] to-white pt-16 pb-20 md:pt-20 md:pb-28 overflow-hidden">

      {/* Centered header + command block */}
      <div className="max-w-5xl mx-auto px-6 md:px-10 text-center">
        <motion.div
          initial={{ opacity: 0, y: 16 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          transition={{ duration: 0.5 }}
        >
          <span className="text-sm font-semibold tracking-[0.18em] uppercase text-sapphire-600">
            One command install
          </span>
          <h2 className="mt-3 md:mt-4 font-display text-4xl md:text-5xl font-extrabold text-slate-900 tracking-tight">
            Up and running in seconds.
          </h2>
          <p className="mt-4 text-lg text-slate-600">
            No installer wizard. No admin rights. Just paste one command and Tithify sets itself up silently.
          </p>
        </motion.div>

        <motion.div
          layout
          initial={{ opacity: 0, y: 24 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true, margin: "-80px" }}
          transition={{ duration: 0.4, ease: [0.16, 1, 0.3, 1] }}
          className="mt-12 mx-auto max-w-4xl rounded-2xl bg-slate-950 border border-slate-800 text-left shadow-2xl shadow-slate-300/40 overflow-hidden"
        >
          <div className="flex items-center gap-1 px-3 pt-3">
            {OPTIONS.map((o, i) => (
              <button
                key={o.label}
                onClick={() => setTab(i)}
                className={`focus-ring px-4 py-2 text-sm font-medium rounded-t-lg transition-colors ${
                  tab === i ? "bg-slate-800 text-white" : "text-slate-400 hover:text-slate-200"
                }`}
              >
                {o.label}
                {o.soon && (
                  <span className="ml-2 px-1.5 py-0.5 rounded-full text-[10px] font-semibold bg-amber-400/20 text-amber-300">
                    Coming soon
                  </span>
                )}
              </button>
            ))}
          </div>
          <div className="bg-slate-900 px-5 py-5 flex items-center gap-3 border-t border-slate-800">
            <Terminal className="w-4 h-4 text-emerald-400 shrink-0" />
            <code className="flex-1 text-xs sm:text-sm text-emerald-300 font-mono whitespace-nowrap overflow-x-auto [scrollbar-width:none] [-ms-overflow-style:none] [&::-webkit-scrollbar]:hidden">
              {OPTIONS[tab].cmd}
            </code>
            <button
              onClick={copy}
              aria-label="Copy command"
              className="focus-ring shrink-0 w-9 h-9 rounded-lg bg-white/5 hover:bg-white/10 flex items-center justify-center text-slate-300 transition-colors"
            >
              {copied ? <Check className="w-4 h-4 text-emerald-400" /> : <Copy className="w-4 h-4" />}
            </button>
          </div>
          {OPTIONS[tab].soon && (
            <p className="px-5 py-3 text-sm text-amber-500 font-medium border-t border-slate-800/80 bg-amber-500/5">
              Winget support is coming soon - this command will work once the package is published.
            </p>
          )}
        </motion.div>
      </div>

      {/* Two-column: steps left, laptop right */}
      <div className="relative mx-auto grid max-w-7xl items-center gap-12 px-6 md:px-10 mt-16 lg:grid-cols-[minmax(0,0.92fr)_minmax(0,1.08fr)] lg:gap-10">

        {/* LEFT: steps */}
        <motion.div
          initial={{ opacity: 0, x: -24 }}
          whileInView={{ opacity: 1, x: 0 }}
          viewport={{ once: true }}
          transition={{ duration: 0.6, ease: easeOut }}
          className="relative z-[1]"
        >
          <p className="text-xs font-semibold tracking-[0.15em] uppercase text-sapphire-600 mb-6">
            Step by step installation
          </p>
          <ol className="space-y-6">

            <li className="flex gap-4">
              <span className="mt-0.5 flex-shrink-0 w-8 h-8 rounded-full bg-sapphire-600 text-white text-sm font-bold flex items-center justify-center shadow-md">
                1
              </span>
              <div>
                <p className="font-display font-bold text-slate-900 text-sm md:text-base">
                  Open Terminal or PowerShell
                </p>
                <p className="mt-1 text-sm text-slate-500 leading-relaxed">
                  Search for{" "}
                  <span className="inline-flex items-center gap-1.5 mx-0.5 px-2 py-0.5 rounded-md bg-[#1c1c1c] text-white text-xs font-medium">
                    <TerminalAppIcon />
                    Terminal
                  </span>{" "}
                  or{" "}
                  <span className="inline-flex items-center gap-1.5 mx-0.5 px-2 py-0.5 rounded-md bg-[#012456] text-[#4ec9f0] text-xs font-medium">
                    <PowerShellIcon />
                    <span className="text-white">PowerShell</span>
                  </span>{" "}
                  on your PC and open it. No admin rights needed.
                </p>
              </div>
            </li>

            <li className="flex gap-4">
              <span className="mt-0.5 flex-shrink-0 w-8 h-8 rounded-full bg-sapphire-600 text-white text-sm font-bold flex items-center justify-center shadow-md">
                2
              </span>
              <div>
                <p className="font-display font-bold text-slate-900 text-sm md:text-base">
                  Copy the command and paste it
                </p>
                <p className="mt-1 text-sm text-slate-500 leading-relaxed">
                  Copy the one-liner above, paste it into your terminal, and hit{" "}
                  <kbd className="px-1.5 py-0.5 rounded bg-slate-100 border border-slate-300 text-slate-700 text-xs font-mono">
                    Enter
                  </kbd>
                  .
                </p>
              </div>
            </li>

            <li className="flex gap-4">
              <span className="mt-0.5 flex-shrink-0 w-8 h-8 rounded-full bg-sapphire-600 text-white text-sm font-bold flex items-center justify-center shadow-md">
                3
              </span>
              <div>
                <p className="font-display font-bold text-slate-900 text-sm md:text-base">
                  Wait a few seconds &mdash; you&apos;re done! 🎉
                </p>
                <p className="mt-1 text-sm text-slate-500 leading-relaxed">
                  Tithify downloads and installs silently &mdash; no wizard, no clicks.
                  The Nepali date widget appears on your taskbar automatically.
                </p>
              </div>
            </li>

          </ol>

          <p className="mt-8 text-sm text-slate-500">
            Prefer a manual download? Grab{" "}
            <a
              href="https://github.com/aayushlbef/Tithify/releases/latest"
              className="text-crimson font-semibold hover:underline focus-ring"
            >
              Tithify_Setup.exe
            </a>{" "}
            from the latest release. Unsigned builds may trigger a SmartScreen notice &mdash;
            <span className="font-medium text-slate-700"> More Info &rarr; Run Anyway</span> gets you through.
          </p>
        </motion.div>

        {/* RIGHT: laptop mockup */}
        <motion.div
          initial={{ opacity: 0, x: 36, scale: 0.97 }}
          whileInView={{ opacity: 1, x: 0, scale: 1 }}
          viewport={{ once: true, margin: "-100px" }}
          transition={{ duration: 0.85, delay: 0.08, ease: easeOut }}
          whileHover={{ y: -6, scale: 1.01 }}
          className="relative min-w-0 w-full flex items-center justify-center"
        >
          <div className="relative aspect-[3538/2208] w-full drop-shadow-2xl">
            <div
              className="absolute z-0 overflow-hidden bg-slate-950"
              style={{ left: "11.8%", top: "4.5%", width: "75.8%", height: "74.0%" }}
            >
              <video
                className="h-full w-full object-contain"
                src={`${basePath}/Tithify%20Downloading%20Process.mp4`}
                aria-label="Tithify download and install process"
                autoPlay
                loop
                muted
                playsInline
                preload="metadata"
              >
                Your browser does not support the product preview video.
              </video>
            </div>
            <Image
              src={`${basePath}/half%20cutout%20laptop.png`}
              alt="Tithify running on a Windows laptop"
              width={3538}
              height={2208}
              sizes="(min-width: 1024px) 50vw, 100vw"
              className="pointer-events-none absolute inset-0 z-[1] h-full w-full object-contain select-none"
            />
          </div>
        </motion.div>

      </div>
    </section>
  );
}
