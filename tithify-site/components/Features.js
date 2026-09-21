"use client";

import { motion } from "framer-motion";
import {
  SunMoon,
  RefreshCw,
  LayoutPanelLeft,
  Sparkles,
  EyeOff,
  ScanEye,
  PackageCheck,
  MousePointerClick,
} from "lucide-react";

const FEATURES = [
  {
    icon: SunMoon,
    title: "Theme auto-adapt",
    body: "Detects Windows Light or Dark mode and shifts its colors instantly — no restart, no flicker.",
  },
  {
    icon: RefreshCw,
    title: "Auto-updates",
    body: "A background thread checks GitHub Releases quietly and tells you when something new lands.",
  },
  {
    icon: LayoutPanelLeft,
    title: "Customizable layout",
    body: "Toggle the day-of-week badge beside the flag straight from the context menu.",
  },
  {
    icon: Sparkles,
    title: "Zero-distraction design",
    body: "Frameless, borderless, truly transparent — rendered with per-pixel alpha blending.",
  },
  {
    icon: EyeOff,
    title: "Smart auto-hide",
    body: "Steps aside the moment you go fullscreen for a game or a film, then returns on its own.",
  },
  {
    icon: ScanEye,
    title: "DPI aware",
    body: "Crisp text and glyphs on 4K displays — nothing soft, nothing blurred.",
  },
  {
    icon: PackageCheck,
    title: "Professional installer",
    body: "A standard Setup.exe with Start Menu shortcuts and a clean uninstall. No admin rights needed.",
  },
  {
    icon: MousePointerClick,
    title: "System tray access",
    body: "Right-click any time for quick settings — themes, startup, and update checks.",
  },
];

const container = {
  hidden: {},
  show: { transition: { staggerChildren: 0.08 } },
};
const item = {
  hidden: { opacity: 0, y: 26 },
  show: { opacity: 1, y: 0, transition: { duration: 0.6, ease: [0.16, 1, 0.3, 1] } },
};

export default function Features() {
  return (
    <section id="features" className="relative bg-gradient-to-b from-[#eaf4ff] via-[#f4f8ff] to-white pt-16 pb-20 md:pt-20 md:pb-28">
      <div className="max-w-7xl mx-auto px-6 md:px-10">
        <motion.div
          initial={{ opacity: 0, y: 16 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          transition={{ duration: 0.5 }}
          className="max-w-xl mb-10 md:mb-12"
        >
          <span className="text-sm font-semibold tracking-[0.18em] uppercase text-sapphire-600">
            Nepali Calendar &amp; Windows Widget Features
          </span>
          <h2 className="mt-3 md:mt-4 font-display text-4xl md:text-5xl font-extrabold text-slate-900 tracking-tight">
            Every detail of the Nepali widget, considered.
          </h2>
          <p className="mt-3 md:mt-4 text-lg text-slate-600 leading-relaxed">
            Designed as a native Windows desktop widget, Tithify shows your Bikram Sambat date and Nepali calendar quietly above your taskbar with zero distraction.
          </p>
        </motion.div>

        <motion.div
          variants={container}
          initial="hidden"
          whileInView="show"
          viewport={{ once: true }}
          className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-5"
        >
          {FEATURES.map((f) => (
            <motion.div
              key={f.title}
              variants={item}
              whileHover={{ y: -6 }}
              className="group rounded-2xl bg-white/95 backdrop-blur-sm border border-blue-100/80 p-6 shadow-[0_4px_24px_-4px_rgba(37,99,235,0.06)] hover:shadow-xl hover:border-sapphire-300 hover:bg-white transition-all"
            >
              <div className="w-11 h-11 rounded-xl bg-sapphire-50 flex items-center justify-center text-sapphire-600 group-hover:bg-crimson-50 group-hover:text-crimson-600 transition-all group-hover:scale-105">
                <f.icon className="w-5 h-5" />
              </div>
              <h3 className="mt-4 font-display font-bold text-slate-900">{f.title}</h3>
              <p className="mt-2 text-sm text-slate-600 leading-relaxed">{f.body}</p>
            </motion.div>
          ))}
        </motion.div>
      </div>
    </section>
  );
}
